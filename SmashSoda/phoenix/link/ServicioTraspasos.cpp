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
		return !ec;
	}
}

struct Servicio::Impl {
	std::mutex m;                       // ajustes y cache
	std::condition_variable cv;
	bool publicar = false, recibir = false, confirmarAntes = true;
	std::string transporte = "carpeta", carpeta, autor, optionFile;
	std::shared_ptr<Motor> motor;
	std::thread hilo;
	std::atomic<bool> parar{ false }, corriendo{ false };
	bool iniciado = false;
	std::string cache;                  // ultimo estado del motor (JSON): la interfaz nunca espera al motor
	std::string mensaje;                // resultado de la ultima accion
	std::deque<std::string> acciones;   // las ejecuta el hilo del servicio
	fs::path juegoActual, optionFileEfectivo;
	std::string errorOptionFile;
	long long proximaBusquedaOption = 0;

	fs::path ajustesArchivo() const { return carpetaAjustes() / "ajustes.json"; }

	void guardar() {
		json j = { {"publicar", publicar}, {"recibir", recibir}, {"confirmarAntes", confirmarAntes}, {"transporte", transporte}, {"carpeta", carpeta}, {"autor", autor}, {"optionFile", optionFile} };
		escribir(ajustesArchivo(), j.dump(1));
	}
	void cargar() {
		std::ifstream f(ajustesArchivo(), std::ios::binary);
		if (!f) return;
		const std::string t((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		const json j = json::parse(t, nullptr, false);
		if (!j.is_object()) return;
		publicar = j.value("publicar", false); recibir = j.value("recibir", false); confirmarAntes = j.value("confirmarAntes", true);
		transporte = j.value("transporte", std::string("carpeta")); carpeta = j.value("carpeta", std::string()); autor = j.value("autor", std::string()); optionFile = j.value("optionFile", std::string());
	}

	std::shared_ptr<Transporte> crearTransporte() const {
		if (transporte == "carpeta" && !carpeta.empty()) return std::make_shared<TransporteCarpeta>(phoenix::entrega::deU8(carpeta));
		return nullptr;   // "web": pendiente de que la web tenga el contrato (docs/PROMPT-WEB-traspasos-vivo.md)
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
		const auto ahora = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		if (!optionFileEfectivo.empty()) { option = optionFileEfectivo; return; }
		if (ahora < proximaBusquedaOption) return;
		proximaBusquedaOption = ahora + 30000;
		option = phoenix::entrega::buscarCarpetaOptionFile(carpetaDocumentos());
		optionFileEfectivo = option;
	}

	void paso() {
		std::shared_ptr<Motor> mo;
		bool pub, rec, conf;
		std::string aut, accion;
		{
			std::lock_guard<std::mutex> g(m);
			asegurarMotor();
			mo = motor; pub = publicar; rec = recibir; conf = confirmarAntes; aut = autor;
			if (!acciones.empty()) { accion = acciones.front(); acciones.pop_front(); }
		}
		fs::path juego, option;
		entorno(juego, option);
		{ std::lock_guard<std::mutex> g(m); juegoActual = juego; }
		mo->ajustar(juego, option, pub, rec, conf, aut);
		std::string msg;
		if (!accion.empty()) {
			bool ok = false;
			if (accion == "publicar") ok = mo->publicarPendientes(msg);
			else if (accion == "descartar") ok = mo->descartarPendientes(msg);
			else if (accion == "deshacer") ok = mo->deshacerUltimo(msg);
			else if (accion == "olvidarReferencia") { mo->reiniciarReferencia(); msg = "Listo: la proxima vez que PES guarde, Link solo tomara la foto de partida (no publica nada)."; ok = true; }
			std::lock_guard<std::mutex> g(m);
			mensaje = (ok ? "" : "No se pudo: ") + msg;
		}
		if (pub) mo->pasoPublicar();
		if (rec) mo->pasoRecibir();
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

Servicio::Servicio() : _i(new Impl()) { PhoenixPrefs::get(); }   // PhoenixPrefs se crea antes y se destruye despues que el servicio
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
	j["transporteListo"] = _i->transporte == "carpeta" && !_i->carpeta.empty();
	j["webDisponible"] = false;   // hasta que la web tenga el contrato
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
		interruptor("publicar", _i->publicar); interruptor("recibir", _i->recibir); interruptor("confirmarAntes", _i->confirmarAntes);
		texto("transporte", _i->transporte, 12); texto("carpeta", _i->carpeta, 500); texto("autor", _i->autor, 40); texto("optionFile", _i->optionFile, 500);
		if (_i->transporte != "carpeta" && _i->transporte != "web") { _i->transporte = transporteAntes; throw std::runtime_error("Transporte desconocido."); }
		if (_i->transporte == "web") { _i->transporte = transporteAntes; throw std::runtime_error("La web aun no tiene los traspasos en vivo (falta pegar docs/PROMPT-WEB-traspasos-vivo.md en el chat WEB). Usa una carpeta compartida por ahora."); }
		if (!_i->carpeta.empty()) {   // la carpeta debe existir o poder crearse; no se acepta una ruta rara
			std::error_code ec;
			const fs::path c = phoenix::entrega::deU8(_i->carpeta);
			if (!c.is_absolute()) { _i->carpeta = carpetaAntes; throw std::runtime_error("La carpeta del transporte debe ser una ruta completa."); }
		}
		if (!_i->optionFile.empty()) {
			std::error_code ec;
			const fs::path o = phoenix::entrega::deU8(_i->optionFile);
			if (!o.is_absolute() || !(fs::is_regular_file(o, ec) || fs::is_regular_file(o / "EDIT00000000", ec))) { _i->optionFile = optionAntes; throw std::runtime_error("No encuentro ese option file (EDIT00000000)."); }
		}
		_i->guardar();
		if (_i->carpeta != carpetaAntes || _i->transporte != transporteAntes) _i->motor.reset();   // otro transporte: motor nuevo (conserva su estado en disco)
		_i->optionFileEfectivo.clear(); _i->proximaBusquedaOption = 0;
	}
	bool algo;
	{ std::lock_guard<std::mutex> g(_i->m); algo = _i->publicar || _i->recibir; }
	if (algo) { _i->arrancar(); _i->cv.notify_all(); }
	else _i->frenar();
}

std::string Servicio::accion(const std::string& que) {
	if (que != "publicar" && que != "descartar" && que != "deshacer" && que != "olvidarReferencia") throw std::runtime_error("Accion desconocida.");
	{
		std::lock_guard<std::mutex> g(_i->m);
		if (_i->acciones.size() >= 2) throw std::runtime_error("Hay otra accion en curso; espera unos segundos.");
		_i->acciones.push_back(que);
		_i->mensaje = "En curso…";
	}
	_i->arrancar();   // si estaba apagado, el hilo hace la accion y sigue (solo mira lo que este encendido)
	_i->cv.notify_all();
	return "En curso…";
}

} // namespace phoenix::traspasos
