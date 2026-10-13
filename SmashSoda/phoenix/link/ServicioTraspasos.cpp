#include "ServicioTraspasos.h"

#include <windows.h>
#include <shlobj.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>

#include "../PhoenixPrefs.h"
#include "BuzonJuego.h"
#include "Entrega.h"
#include "ModulosJuego.h"
#include "PhoenixLink.h"
#include "Traspasos.h"

namespace phoenix::traspasos {

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
	fs::path carpetaAjustes() {
		wchar_t b[1024];
		const DWORD n = GetEnvironmentVariableW(L"APPDATA", b, 1024);
		if (n == 0 || n >= 1024) return fs::path();
		return fs::path(std::wstring(b, n)) / L"Trybuchet" / L"Smash Soda" / L"phoenix-traspasos";
	}
	fs::path carpetaDocumentos() {
		PWSTR p = nullptr;
		fs::path r;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &p)) && p) r = fs::path(p);
		if (p) CoTaskMemFree(p);
		return r;
	}
	std::string usuarioWindows() {
		wchar_t b[256];
		const DWORD n = GetEnvironmentVariableW(L"USERNAME", b, 256);
		if (n == 0 || n >= 256) return "Anfitrion";
		return phoenix::entrega::aU8(fs::path(std::wstring(b, n)));
	}
	bool escribir(const fs::path& p, const std::string& t) {
		std::error_code ec;
		fs::create_directories(p.parent_path(), ec);
		fs::path tmp = p; tmp += ".tmp";
		{ std::ofstream f(tmp, std::ios::binary | std::ios::trunc); f << t; if (!f.good()) return false; }
		fs::rename(tmp, p, ec);
		if (ec) { std::error_code e2; fs::remove(tmp, e2); return false; }   // no deja un .tmp huerfano si no pudo reemplazar
		return true;
	}
	long long ahoraMs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
	json aJson(const CanalWeb& c) { return { {"codigo", c.codigo}, {"seguidores", c.seguidores} }; }
}

struct Servicio::Impl {
	struct Orden { std::string que, dato; };
	std::mutex m;                       // ajustes y cache
	std::condition_variable cv;
	bool publicar = false, recibir = false, confirmarAntes = true;
	std::string transporte = "web", carpeta, autor, optionFile;   // la web (Supabase) es el transporte por defecto; la carpeta es la alternativa
	std::shared_ptr<Motor> motor;
	std::shared_ptr<TransporteWeb> web;   // el mismo objeto que usa el motor cuando el transporte es la web
	std::thread hilo;
	std::atomic<bool> parar{ false }, corriendo{ false };
	bool iniciado = false;
	std::string cache;                  // ultimo estado del motor (JSON): la interfaz nunca espera al motor
	std::string mensaje;                // resultado de la ultima accion
	std::deque<Orden> acciones;         // las ejecuta el hilo del servicio
	fs::path juegoActual, optionFileEfectivo;
	long long proximaBusquedaOption = 0;
	// lo que se sabe de la web (canal propio, a quien sigo)
	CanalWeb canal;                       // mi codigo (si soy anfitrion)
	std::vector<SiguiendoWeb> siguiendo;  // a quien sigo
	std::vector<std::string> meSiguen;    // nombres de quienes me siguen
	bool pruebaOk = false;                // en modo Prueba ya me segui a mi mismo
	std::string errorWeb;
	bool webVisto = false;               // alguna respuesta buena de las rutas de traspasos
	long long proximoRefrescoWeb = 0;
	json diag;                           // autodiagnostico del juego (sider.ini, Phoenix-DB, phoenix.lua): lista «Antes de la prueba»
	long long proximoDiag = 0;

	fs::path ajustesArchivo() const { return carpetaAjustes() / "ajustes.json"; }

	void guardar() {
		json j = { {"publicar", publicar}, {"recibir", recibir}, {"confirmarAntes", confirmarAntes}, {"transporte", transporte}, {"carpeta", carpeta}, {"autor", autor}, {"optionFile", optionFile} };
		escribir(ajustesArchivo(), j.dump(1));
	}
	void cargar() {
		std::string t;
		{   // el archivo se CIERRA antes de guardar: con el flujo abierto Windows no deja reemplazarlo y la migracion no se guardaba
			std::ifstream f(ajustesArchivo(), std::ios::binary);
			if (!f) return;
			t.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		}
		const AjustesGuardados a = leerAjustes(t);   // logica pura (probada): la web es el transporte por defecto
		publicar = a.publicar; recibir = a.recibir; confirmarAntes = a.confirmarAntes;
		transporte = a.transporte; carpeta = a.carpeta; autor = a.autor; optionFile = a.optionFile;
		if (a.migrado) guardar();   // deja el archivo ya migrado a la web
	}

	std::shared_ptr<Transporte> crearTransporte() {   // m tomado
		if (transporte == "carpeta" && !carpeta.empty()) { web.reset(); return std::make_shared<TransporteCarpeta>(phoenix::entrega::deU8(carpeta)); }
		if (!web) {
			web = std::make_shared<TransporteWeb>([](const std::string& metodo, const std::string& ruta, const std::string& cuerpo, int& estado, std::string& resp) {
				return phoenix::PhoenixLink::instancia().peticionTraspasos(metodo, ruta, cuerpo, estado, resp);
			});
		}
		return web;
	}

	void asegurarMotor() {   // m tomado
		if (motor) return;
		Config c;
		c.dirEstado = carpetaAjustes() / "estado";
		c.autor = autor.empty() ? usuarioWindows() : autor;
		c.pesAbierto = []() {
			return !phoenix::modulos::procesosAbiertos().empty() || phoenix::buzon::detectarJuego().puente != phoenix::buzon::Puente::JuegoCerrado;
		};
		c.modoRecarga = []() { return PhoenixPrefs::get().modoRecarga; };
		c.listaManual = []() {
			phoenix::fichajes::Lista l; std::string e;
			if (!phoenix::fichajes::cargarLista(phoenix::fichajes::carpetaListaUsuario(), phoenix::fichajes::carpetaListaPaquete(), l, e)) return phoenix::fichajes::Lista();
			return l;
		};
		auto* yo = this;
		c.enriquecer = [yo](std::vector<CambioLote>& cs, std::string& parche, std::string& huella) {
			fs::path j;
			{ std::lock_guard<std::mutex> g(yo->m); j = yo->juegoActual; }
			enriquecerConBaseDelJuego(j, cs, parche, huella);
		};
		motor = std::make_shared<Motor>(std::move(c), crearTransporte());
	}

	/// Carpeta del juego (la de PES abierto o la ultima conocida) y option file (a mano o el de Documentos).
	void entorno(fs::path& juego, fs::path& option) {
		const phoenix::buzon::EstadoJuego g = phoenix::buzon::detectarJuego();
		if (g.puente != phoenix::buzon::Puente::JuegoCerrado && !g.carpetaJuego.empty()) juego = g.carpetaJuego;
		else if (!PhoenixPrefs::get().carpetaJuego.empty()) juego = phoenix::entrega::deU8(PhoenixPrefs::get().carpetaJuego);
		std::string manual;
		{ std::lock_guard<std::mutex> g2(m); manual = optionFile; }
		if (!manual.empty()) { option = phoenix::entrega::deU8(manual); return; }
		const long long ahora = ahoraMs();
		if (!optionFileEfectivo.empty()) { option = optionFileEfectivo; return; }
		if (ahora < proximaBusquedaOption) return;
		proximaBusquedaOption = ahora + 30000;
		option = phoenix::entrega::buscarCarpetaOptionFile(carpetaDocumentos());
		optionFileEfectivo = option;
	}

	/// Refresca lo que sabe la web (mi código si publico, a quién sigo y quién me sigue) y le dice al transporte si tiene sentido leer.
	/// Respeta las esperas del transporte (404 = 5 min, 429…): no insiste.
	void refrescarWeb(bool forzar, bool pub, bool rec) {
		std::shared_ptr<TransporteWeb> w;
		{ std::lock_guard<std::mutex> g(m); w = web; if (transporte != "web" || !w) return; }
		if (!forzar && ahoraMs() < proximoRefrescoWeb) return;
		if (w->esperaMs() > 0) return;
		ClienteTraspasosWeb cli(*w);
		std::string e;
		bool ok = true;
		CanalWeb c;
		std::vector<SiguiendoWeb> sig;
		std::vector<std::string> meSig;
		if (pub) ok = cli.codigo(false, c, e);
		if (ok && (pub || rec)) ok = cli.siguiendo(sig, meSig, e);
		bool prueba;
		{ std::lock_guard<std::mutex> g(m); prueba = pruebaOk; }
		if (ok && pub && rec && !prueba && c.hay) {   // modo Prueba: me sigo a mi mismo con mi propio codigo (la web lo exige con prueba:true)
			SiguiendoWeb s;
			std::string e2;
			if (cli.seguir(c.codigo, true, s, e2)) { std::lock_guard<std::mutex> g(m); pruebaOk = true; }
			else { std::lock_guard<std::mutex> g(m); errorWeb = e2; }
			cli.siguiendo(sig, meSig, e);
		}
		std::lock_guard<std::mutex> g(m);
		proximoRefrescoWeb = ahoraMs() + 20000;
		if (!ok) { errorWeb = e; return; }
		if (errorWeb == e || !pub || ok) errorWeb.clear();
		webVisto = true;
		if (pub) canal = c;
		siguiendo = sig; meSiguen = meSig;
		w->ponerPuedeLeer(!rec || pub || !sig.empty());   // un amigo que aun no sigue a nadie no llama a la web
	}

	std::string accionWeb(const Orden& o, bool& ok) {   // sin m tomado
		std::shared_ptr<TransporteWeb> w;
		{ std::lock_guard<std::mutex> g(m); w = web; }
		if (!w) { ok = false; return "Esto solo funciona con la web de Phoenix como transporte."; }
		ClienteTraspasosWeb cli(*w);
		std::string e;
		ok = false;
		if (o.que == "rotarCodigo") {
			CanalWeb c;
			if (!cli.codigo(true, c, e)) return e;
			std::lock_guard<std::mutex> g(m); canal = c; webVisto = true; pruebaOk = false; ok = true;
			return "Código nuevo: el anterior ya no sirve para unirse (quienes ya te siguen siguen).";
		}
		if (o.que == "seguirCodigo") {
			SiguiendoWeb s;
			if (!cli.seguir(o.dato, false, s, e)) return e;
			std::vector<SiguiendoWeb> sig; std::vector<std::string> meSig;
			cli.siguiendo(sig, meSig, e);
			std::lock_guard<std::mutex> g(m);
			siguiendo = sig.empty() ? std::vector<SiguiendoWeb>{ s } : sig; webVisto = true; ok = true;
			w->ponerPuedeLeer(true);
			return "Listo: sigues a " + (s.anfitrionNombre.empty() ? std::string("ese anfitrión") : s.anfitrionNombre) + ".";
		}
		if (o.que == "dejarDeSeguir") {
			if (!cli.dejar(o.dato, e)) return e;
			std::vector<SiguiendoWeb> sig; std::vector<std::string> meSig;
			cli.siguiendo(sig, meSig, e);
			std::lock_guard<std::mutex> g(m);
			siguiendo = sig; meSiguen = meSig; webVisto = true; ok = true; pruebaOk = false;
			w->ponerPuedeLeer(!recibir || publicar || !sig.empty());
			return "Dejaste de seguir a ese anfitrión.";
		}
		return "Acción desconocida.";
	}

	void paso() {
		std::shared_ptr<Motor> mo;
		bool pub, rec, conf;
		std::string aut;
		Orden orden;
		{
			std::lock_guard<std::mutex> g(m);
			asegurarMotor();
			mo = motor; pub = publicar; rec = recibir; conf = confirmarAntes; aut = autor;
			if (!acciones.empty()) { orden = acciones.front(); acciones.pop_front(); }
		}
		fs::path juego, option;
		entorno(juego, option);
		{ std::lock_guard<std::mutex> g(m); juegoActual = juego; }
		mo->ajustar(juego, option, pub, rec, conf, aut);
		bool refrescar = false;
		if (!orden.que.empty()) {
			bool ok = false;
			std::string msg;
			if (orden.que == "publicar") ok = mo->publicarPendientes(msg);
			else if (orden.que == "descartar") ok = mo->descartarPendientes(msg);
			else if (orden.que == "deshacer") ok = mo->deshacerUltimo(msg);
			else if (orden.que == "olvidarReferencia") { mo->reiniciarReferencia(); msg = "Listo: la proxima vez que PES guarde, Link solo tomara la foto de partida (no publica nada)."; ok = true; }
			else if (orden.que == "reintentar") { mo->reintentarYa(); msg = "Volviendo a comprobar la web…"; ok = true; refrescar = true; }
			else { msg = accionWeb(orden, ok); refrescar = ok; }
			std::lock_guard<std::mutex> g(m);
			mensaje = (ok ? "" : "No se pudo: ") + msg;
		}
		refrescarWeb(refrescar, pub, rec);
		if (ahoraMs() >= proximoDiag || !orden.que.empty()) {   // el diagnostico solo LEE archivos del juego: sirve con PES abierto
			const json dj = diagAJson(diagnosticarJuego(juego, phoenix::modulos::carpetaPaquete()));
			std::lock_guard<std::mutex> g(m);
			diag = dj; proximoDiag = ahoraMs() + 10000;
		}
		if (pub) mo->pasoPublicar();
		if (rec) mo->pasoRecibir();
		const std::string aviso = mo->tomarAviso();   // «Fichaje aplicado: X pasa de A a B. …» → overlay del juego (solo si PES esta abierto; si no, lo ve en Link)
		if (!aviso.empty() && (!phoenix::modulos::procesosAbiertos().empty() || phoenix::buzon::detectarJuego().puente != phoenix::buzon::Puente::JuegoCerrado))
			phoenix::PhoenixLink::instancia().avisoLocalDiferido(phoenix::buzon::cortarUtf8(aviso, 230));
		const std::string est = mo->estadoJson();
		std::lock_guard<std::mutex> g(m);
		cache = est;
	}

	void bucle() {
		while (!parar) {
			try { paso(); } catch (...) {}   // un fallo no tumba Link: se vuelve a intentar en el siguiente paso
			{ std::lock_guard<std::mutex> g(m); if (!publicar && !recibir && acciones.empty()) break; }   // nada encendido: el hilo termina
			std::unique_lock<std::mutex> l(m);
			cv.wait_for(l, std::chrono::seconds(3), [this]() { return parar.load() || !acciones.empty(); });
		}
	}

	void arrancar() {   // m NO tomado
		if (corriendo) return;
		if (hilo.joinable()) hilo.join();   // uno que ya termino solo
		parar = false;
		corriendo = true;
		hilo = std::thread([this]() { bucle(); corriendo = false; });
	}
	void frenar() {
		parar = true;
		cv.notify_all();
		if (hilo.joinable()) hilo.join();
	}
};

Servicio::Servicio() : _i(new Impl()) { PhoenixPrefs::get(); phoenix::PhoenixLink::instancia(); }   // se crean antes y se destruyen despues que el servicio
Servicio::~Servicio() { _i->frenar(); delete _i; }
Servicio& Servicio::get() { static Servicio s; return s; }

void Servicio::iniciar() {
	bool algo;
	{
		std::lock_guard<std::mutex> g(_i->m);
		if (!_i->iniciado) { _i->iniciado = true; _i->cargar(); }
		algo = _i->publicar || _i->recibir;
	}
	if (algo) _i->arrancar();
}

void Servicio::detener() { _i->frenar(); }

json Servicio::estado() {
	const bool vinculada = phoenix::PhoenixLink::instancia().pcVinculada();
	std::lock_guard<std::mutex> g(_i->m);
	if (!_i->iniciado) { _i->iniciado = true; _i->cargar(); }
	json j = json::object();
	if (_i->cache.empty() && !_i->corriendo) { _i->asegurarMotor(); _i->cache = _i->motor->estadoJson(); }   // apagado: solo se enseña el historial guardado
	if (!_i->cache.empty()) j = json::parse(_i->cache, nullptr, false);
	if (!j.is_object()) j = json::object();
	j["ajustes"] = { {"publicar", _i->publicar}, {"recibir", _i->recibir}, {"confirmarAntes", _i->confirmarAntes}, {"transporte", _i->transporte}, {"carpeta", _i->carpeta}, {"autor", _i->autor}, {"optionFile", _i->optionFile} };
	j["activo"] = _i->publicar || _i->recibir;
	j["mensaje"] = _i->mensaje;
	j["juego"] = phoenix::entrega::aU8(_i->juegoActual);
	const bool esWeb = _i->transporte == "web";
	j["transporteListo"] = esWeb ? vinculada : !_i->carpeta.empty();
	// ¿la web ya tiene los traspasos? true = ya contestó bien; false = 404 (falta pegar el prompt); null = aún no se sabe
	const std::string clave = _i->web ? _i->web->estadoClave() : std::string("ok");
	j["webDisponible"] = !esWeb ? json(nullptr) : clave == "web_sin_endpoints" ? json(false) : _i->webVisto ? json(true) : json(nullptr);
	json w = { {"transporte", _i->transporte}, {"vinculada", vinculada}, {"estado", esWeb ? clave : std::string("ok")}, {"texto", esWeb && _i->web ? _i->web->estadoTexto() : std::string()},
		{"esperaSeg", esWeb && _i->web ? _i->web->esperaMs() / 1000 : 0LL}, {"error", _i->errorWeb}, {"siguiendo", json::array()}, {"meSiguen", _i->meSiguen}, {"canal", nullptr} };
	for (const SiguiendoWeb& s : _i->siguiendo) w["siguiendo"].push_back({ {"anfitrionId", s.anfitrionId}, {"anfitrionNombre", s.anfitrionNombre} });
	if (_i->canal.hay) w["canal"] = aJson(_i->canal);
	j["web"] = w;
	j["diag"] = _i->diag.is_object() ? _i->diag : json(nullptr);
	j["carpetaPrueba"] = phoenix::entrega::aU8(carpetaAjustes() / "carpeta-de-prueba");   // para probar en una sola PC sin elegir nada
	return j;
}

void Servicio::poner(const json& d) {
	{
		std::lock_guard<std::mutex> g(_i->m);
		if (!_i->iniciado) { _i->iniciado = true; _i->cargar(); }
		auto texto = [&](const char* k, std::string& dest, size_t max) {
			if (!d.contains(k)) return;
			if (!d[k].is_string()) throw std::runtime_error(std::string("Dato invalido: ") + k);
			std::string t = d[k].get<std::string>();
			if (t.size() > max) throw std::runtime_error(std::string("Dato demasiado largo: ") + k);
			dest = t;
		};
		auto interruptor = [&](const char* k, bool& dest) {
			if (!d.contains(k)) return;
			if (!d[k].is_boolean()) throw std::runtime_error(std::string("Dato invalido: ") + k);
			dest = d[k].get<bool>();
		};
		const std::string transporteAntes = _i->transporte, carpetaAntes = _i->carpeta, optionAntes = _i->optionFile;
		const bool pubAntes = _i->publicar, recAntes = _i->recibir;
		interruptor("publicar", _i->publicar); interruptor("recibir", _i->recibir); interruptor("confirmarAntes", _i->confirmarAntes);
		texto("transporte", _i->transporte, 12); texto("carpeta", _i->carpeta, 500); texto("autor", _i->autor, 40); texto("optionFile", _i->optionFile, 500);
		auto deshacerCambios = [&]() { _i->transporte = transporteAntes; _i->carpeta = carpetaAntes; _i->optionFile = optionAntes; _i->publicar = pubAntes; _i->recibir = recAntes; };
		if (_i->transporte != "carpeta" && _i->transporte != "web") { deshacerCambios(); throw std::runtime_error("Transporte desconocido."); }
		if (_i->transporte == "carpeta") {
			const fs::path c = phoenix::entrega::deU8(_i->carpeta);
			if (_i->carpeta.empty() || !c.is_absolute()) { deshacerCambios(); throw std::runtime_error("Para usar una carpeta compartida pon su ruta completa."); }
		}
		else if (!_i->carpeta.empty()) {   // la carpeta se guarda aunque se use la web (por si luego se prueba), pero tiene que ser una ruta completa
			if (!phoenix::entrega::deU8(_i->carpeta).is_absolute()) { deshacerCambios(); throw std::runtime_error("La carpeta debe ser una ruta completa."); }
		}
		if (!_i->optionFile.empty()) {
			std::error_code ec;
			const fs::path o = phoenix::entrega::deU8(_i->optionFile);
			if (!o.is_absolute() || !(fs::is_regular_file(o, ec) || fs::is_regular_file(o / "EDIT00000000", ec))) { deshacerCambios(); throw std::runtime_error("No encuentro ese option file (EDIT00000000)."); }
		}
		_i->guardar();
		if (_i->motor && (_i->carpeta != carpetaAntes || _i->transporte != transporteAntes)) {
			_i->motor->cambiarTransporte(_i->crearTransporte());   // otro buzon: el motor recuerda hasta donde leyo en cada uno
			_i->proximoRefrescoWeb = 0;
		}
		if (_i->publicar != pubAntes || _i->recibir != recAntes) _i->proximoRefrescoWeb = 0;
		_i->optionFileEfectivo.clear(); _i->proximaBusquedaOption = 0;
	}
	bool algo;
	{ std::lock_guard<std::mutex> g(_i->m); algo = _i->publicar || _i->recibir; }
	if (algo) { _i->arrancar(); _i->cv.notify_all(); }
	else _i->frenar();
}

std::string Servicio::accion(const std::string& que, const std::string& dato) {
	static const char* validas[] = { "publicar", "descartar", "deshacer", "olvidarReferencia", "reintentar", "rotarCodigo", "seguirCodigo", "dejarDeSeguir" };
	bool ok = false;
	for (const char* v : validas) if (que == v) ok = true;
	if (!ok) throw std::runtime_error("Accion desconocida.");
	if (dato.size() > 64) throw std::runtime_error("Dato demasiado largo.");
	{
		std::lock_guard<std::mutex> g(_i->m);
		if (_i->acciones.size() >= 3) throw std::runtime_error("Hay otras acciones en curso; espera unos segundos.");
		_i->acciones.push_back({ que, dato });
		_i->mensaje = "En curso…";
	}
	_i->arrancar();   // si estaba apagado, el hilo hace la accion y sigue (solo mira lo que este encendido)
	_i->cv.notify_all();
	return "En curso…";
}

} // namespace phoenix::traspasos
