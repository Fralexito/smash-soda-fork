#include "ServicioFichas.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "BuzonJuego.h"
#include "Entrega.h"
#include "Fichas.h"
#include "FichasWeb.h"
#include "ModulosJuego.h"
#include "PhoenixLink.h"
#include "ServicioTraspasos.h"
#include "../PhoenixBuild.h"
#include "../PhoenixPrefs.h"

namespace phoenix::fichas {

using json = nlohmann::json;
namespace fsys = std::filesystem;

namespace {
	constexpr int kPagina = 100;
	constexpr long long kCadaMs = 60 * 1000;               // mirar la web cada minuto
	constexpr long long kTodoCadaMs = 6LL * 3600 * 1000;   // releer todo desde 0 cada 6 h (los anulados desaparecen)
	constexpr long long kInformeCadaMs = 10 * 60 * 1000;   // latido del informe (contrato §4)

	long long ahoraMs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
	std::string ahoraIso() {
		std::time_t t = std::time(nullptr); std::tm g{}; gmtime_s(&g, &t);
		char b[32]; std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%SZ", &g); return b;
	}
	bool leerTexto(const fsys::path& p, std::string& t) {
		std::ifstream f(p, std::ios::binary);
		if (!f) return false;
		t.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		return true;
	}
	bool pesAbierto() {
		return !phoenix::modulos::procesosAbiertos().empty() || phoenix::buzon::detectarJuego().puente != phoenix::buzon::Puente::JuegoCerrado;
	}
	fsys::path carpetaJuego() {
		const phoenix::buzon::EstadoJuego g = phoenix::buzon::detectarJuego();
		if (g.puente != phoenix::buzon::Puente::JuegoCerrado && !g.carpetaJuego.empty()) return g.carpetaJuego;
		if (!PhoenixPrefs::get().carpetaJuego.empty()) return phoenix::entrega::deU8(PhoenixPrefs::get().carpetaJuego);
		return {};
	}
}

struct Servicio::Impl {
	std::mutex m;
	std::condition_variable cv;
	std::thread hilo;
	bool corriendo = false, parar = false, forzar = false, quitar = false;
	web::Almacen almacen;
	bool cargado = false;
	std::string mensaje = "Aún no se ha mirado la web.", errorWeb;
	bool webSinRuta = false;
	long long proximoTodo = 0, proximoInforme = 0, esperaHasta = 0;
	std::string ultimoInformeHuella;
	json porModo = json::array();
	json ultimosResultados = json::array();

	fsys::path archivoAlmacen() { const fsys::path c = carpetaListaUsuario(); return c.empty() ? fsys::path() : c / "cambios-web.json"; }
	void cargarAlmacen() {
		std::string t;
		if (!cargado && leerTexto(archivoAlmacen(), t)) web::leer(t, almacen);
		cargado = true;
	}
	void guardarAlmacen(const web::Almacen& a) {
		const fsys::path f = archivoAlmacen();
		if (f.empty()) return;
		std::error_code ec; fsys::create_directories(f.parent_path(), ec);
		std::string err;
		phoenix::buzon::escribirAtomicoComo(f.parent_path(), "cambios-web", ".json", web::guardar(a), &err);
	}

	/// Baja de la web lo nuevo (o todo, si toca). false = no se pudo (sin vínculo, sin ruta, error): el almacén no cambia.
	bool bajar(web::Almacen& a, bool todo) {
		web::Almacen nuevo = todo ? web::Almacen() : a;
		for (int vuelta = 0; vuelta < 200; vuelta++) {
			int estado = 0; std::string resp;
			const std::string ruta = "/cambios?desde=" + std::to_string(nuevo.ultimoSeq) + "&destino=base&limite=" + std::to_string(kPagina);
			if (!phoenix::PhoenixLink::instancia().peticionTraspasos("GET", ruta, "", estado, resp)) { errorWeb = "sin conexión con la web"; return false; }
			if (estado == -1) { errorWeb = "este PC no está vinculado con la web de Phoenix"; return false; }
			if (estado == 404) { webSinRuta = true; errorWeb = "la web aún no tiene la ruta de cambios oficiales (/v1/cambios)"; esperaHasta = ahoraMs() + 10 * 60 * 1000; return false; }
			if (estado == 429) { errorWeb = "la web pide esperar"; esperaHasta = ahoraMs() + 2 * 60 * 1000; return false; }
			if (estado != 200) { errorWeb = "la web respondió " + std::to_string(estado); return false; }
			std::string e;
			const int n = web::leerPagina(resp, nuevo, e);
			if (n < 0) { errorWeb = e; return false; }
			if (n < kPagina) break;
		}
		webSinRuta = false; errorWeb.clear();
		a = std::move(nuevo);
		return true;
	}

	void informar(const std::vector<web::ResultadoCambio>& rs, const std::vector<Analisis>& an, bool abierto, const fsys::path& juego) {
		json res = json::array();
		for (const auto& r : rs) {
			if (res.size() >= 50) break;
			res.push_back({ {"cambio_id", r.cambioId}, {"destino", "base"}, {"resultado", r.resultado}, {"motivo", r.motivo}, {"jugadores", r.jugadores}, {"cuando", ahoraIso()} });
		}
		ultimosResultados = res;
		const std::string huella = res.dump();
		if (huella == ultimoInformeHuella && ahoraMs() < proximoInforme) return;   // nada nuevo y aún no toca el latido
		std::string parche, base;
		for (const Analisis& a : an) if (a.baseOk) { parche = a.parche; base = a.baseSha.substr(0, 12); break; }
		const json aj = phoenix::traspasos::Servicio::get().estado().value("ajustes", json::object());
		const json cuerpo = { {"link_version", phoenix::kVersion}, {"parche", parche.substr(0, 60)}, {"base_huella", base},
			{"juego", !juego.empty()}, {"pes_abierto", abierto}, {"oficial", true},
			{"recibir", aj.value("recibir", false)}, {"publicar", aj.value("publicar", false)}, {"resultados", res} };
		int estado = 0; std::string resp;
		phoenix::PhoenixLink::instancia().peticionTraspasos("POST", "/pc/informe", cuerpo.dump(), estado, resp);
		if (estado == 200 || estado == 201) { ultimoInformeHuella = huella; proximoInforme = ahoraMs() + kInformeCadaMs; }
		else proximoInforme = ahoraMs() + (estado == 404 ? kInformeCadaMs : 60 * 1000);
	}

	void pasada() {
		web::Almacen a;
		bool todo, quitarYa;
		{
			std::lock_guard<std::mutex> g(m);
			cargarAlmacen();
			a = almacen;
			todo = forzar || ahoraMs() >= proximoTodo;
			quitarYa = quitar;
			forzar = false; quitar = false;
		}
		const fsys::path juego = carpetaJuego();
		const bool abierto = pesAbierto();
		if (quitarYa) {
			const Informe inf = abierto ? Informe{ false, "Cierra PES para quitar las estadísticas." } : quitarDelJuego(juego);
			std::lock_guard<std::mutex> g(m); mensaje = inf.mensaje; return;
		}
		if (ahoraMs() >= esperaHasta || todo) {
			const bool ok = bajar(a, todo);
			std::lock_guard<std::mutex> g(m);
			if (ok) { almacen = a; if (todo) proximoTodo = ahoraMs() + kTodoCadaMs; guardarAlmacen(a); }
		}
		const Lista lista = web::listaDesde(a);
		if (juego.empty()) { std::lock_guard<std::mutex> g(m); mensaje = "No sé dónde está tu juego: abre PES una vez con Link abierto."; return; }
		std::vector<Analisis> an = analizar(juego, lista);
		bool pendiente = false; std::string porQue;
		bool hayQueEscribir = false;
		for (const Analisis& x : an) if (x.baseOk && ((x.cambiados > 0 && x.estado != "aplicado") || (x.cambiados == 0 && x.estado == "desfasado"))) hayQueEscribir = true;
		if (hayQueEscribir && abierto) {
			// Con PES abierto solo se escribe si Phoenix-DB ya está en el sider.ini (no se toca sider.ini con el juego abierto).
			for (const Analisis& x : an) {
				std::string ini;
				if (x.baseOk && (!leerTexto(x.objetivo.ini, ini) || !phoenix::modulos::tieneRaizPhoenixDB(ini))) { pendiente = true; porQue = "PES está abierto: se pone al cerrar PES"; }
			}
		}
		std::string msg;
		if (hayQueEscribir && !pendiente) {
			const Informe inf = aplicarAlJuego(juego, lista);
			msg = inf.mensaje;
			if (!inf.ok) { pendiente = true; porQue = inf.mensaje; }
			else an = inf.objetivos;
		}
		else msg = pendiente ? porQue + "." : lista.cambios.empty() ? "No hay estadísticas oficiales de la web." : "Las estadísticas oficiales ya están puestas en tu juego.";
		const std::vector<web::ResultadoCambio> rs = web::resultados(a, an, pendiente, porQue);
		json modos = json::array();
		for (const Analisis& x : an) modos.push_back({ {"modo", x.objetivo.nombre}, {"parche", x.parche}, {"estado", x.estado}, {"cambiados", x.cambiados}, {"saltados", x.saltados.size()}, {"error", x.error} });
		{ std::lock_guard<std::mutex> g(m); mensaje = msg; porModo = modos; }
		informar(rs, an, abierto, juego);
	}

	void bucle() {
		while (true) {
			try { pasada(); } catch (...) { std::lock_guard<std::mutex> g(m); mensaje = "Error inesperado al aplicar las estadísticas oficiales: se reintenta."; }
			std::unique_lock<std::mutex> l(m);
			cv.wait_for(l, std::chrono::milliseconds(kCadaMs), [this] { return parar || forzar || quitar; });
			if (parar) break;
		}
	}
	void arrancar() {
		std::lock_guard<std::mutex> g(m);
		if (corriendo) return;
		corriendo = true; parar = false;
		hilo = std::thread([this] { bucle(); });
	}
	void frenar() {
		{ std::lock_guard<std::mutex> g(m); if (!corriendo) return; parar = true; corriendo = false; }
		cv.notify_all();
		if (hilo.joinable()) hilo.join();
	}
};

Servicio::Servicio() : _i(new Impl()) { PhoenixPrefs::get(); phoenix::PhoenixLink::instancia(); phoenix::traspasos::Servicio::get(); }
Servicio::~Servicio() { _i->frenar(); delete _i; }
Servicio& Servicio::get() { static Servicio s; return s; }
void Servicio::iniciar() { _i->arrancar(); }
void Servicio::detener() { _i->frenar(); }

json Servicio::estado() {
	std::lock_guard<std::mutex> g(_i->m);
	return { {"mensaje", _i->mensaje}, {"errorWeb", _i->errorWeb}, {"webSinRuta", _i->webSinRuta}, {"cambios", _i->almacen.cambios.size()},
		{"ultimoSeq", _i->almacen.ultimoSeq}, {"modos", _i->porModo}, {"resultados", _i->ultimosResultados} };
}

void Servicio::accion(const std::string& que) {
	{
		std::lock_guard<std::mutex> g(_i->m);
		if (que == "reintentar") _i->forzar = true;
		else if (que == "quitar") _i->quitar = true;
		else throw std::runtime_error("Acción desconocida: " + que);
	}
	_i->arrancar();
	_i->cv.notify_all();
}

} // namespace phoenix::fichas
