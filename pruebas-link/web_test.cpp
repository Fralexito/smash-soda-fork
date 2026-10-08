// Pruebas de la lógica pura de la interfaz web de Phoenix Link (Linux o Windows).
// Compilar y correr: pruebas-link/correr.sh
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "phoenix/web/Puente.h"
#include "phoenix/web/MonitorRed.h"
#include "phoenix/web/Partido.h"
#include "phoenix/web/PerfilesSala.h"

using namespace phoenix::web;

static int gFallos = 0, gPruebas = 0;
#define COMPROBAR(c) do { gPruebas++; if (!(c)) { gFallos++; std::printf("FALLO %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static void pruebaPuente() {
	std::vector<std::string> salida;
	Puente p([&](const std::string& s) { salida.push_back(s); });
	p.alSaludar([](const json& h) { return json{ {"app", "Phoenix Link"}, {"eco", h.value("version", "")} }; });
	p.registrar("suma", [](const json& d, uint64_t) -> std::optional<json> { return json{ {"r", d.at("a").get<int>() + d.at("b").get<int>()} }; });
	p.registrar("falla", [](const json&, uint64_t) -> std::optional<json> { throw ErrorAccion("SALA_CERRADA", "La sala está cerrada."); });
	uint64_t pendiente = 0;
	p.registrar("lento", [&](const json&, uint64_t id) -> std::optional<json> { pendiente = id; return std::nullopt; });

	// Antes de saludar no se publica estado
	COMPROBAR(!p.publicarEstado(json{ {"x", 1} }));
	p.recibir(R"({"t":"hola","version":"4.0.0"})");
	COMPROBAR(p.listo());
	COMPROBAR(salida.size() == 1);
	json b = json::parse(salida.back());
	COMPROBAR(b["t"] == "bienvenida" && b["datos"]["eco"] == "4.0.0");

	p.recibir(R"({"t":"pedir","id":5,"accion":"suma","datos":{"a":2,"b":3}})");
	json r = json::parse(salida.back());
	COMPROBAR(r["t"] == "resp" && r["id"] == 5 && r["ok"] == true && r["datos"]["r"] == 5);

	p.recibir(R"({"t":"pedir","id":6,"accion":"falla"})");
	r = json::parse(salida.back());
	COMPROBAR(r["ok"] == false && r["error"]["codigo"] == "SALA_CERRADA");

	p.recibir(R"({"t":"pedir","id":7,"accion":"no_existe"})");
	r = json::parse(salida.back());
	COMPROBAR(r["ok"] == false && r["error"]["codigo"] == "ACCION_DESCONOCIDA");

	// Tipo equivocado → DATOS_INVALIDOS, sin tumbar nada
	p.recibir(R"({"t":"pedir","id":8,"accion":"suma","datos":{"a":"dos","b":3}})");
	r = json::parse(salida.back());
	COMPROBAR(r["ok"] == false && r["error"]["codigo"] == "DATOS_INVALIDOS");

	// Respuesta diferida
	const size_t antes = salida.size();
	p.recibir(R"({"t":"pedir","id":9,"accion":"lento"})");
	COMPROBAR(salida.size() == antes && pendiente == 9);
	p.responder(9, json{ {"listo", true} });
	r = json::parse(salida.back());
	COMPROBAR(r["id"] == 9 && r["ok"] == true);

	// JSON roto y mensajes doblemente codificados
	p.recibir("{esto no es json");
	COMPROBAR(p.errores() >= 1);
	p.recibir(json(std::string(R"({"t":"pedir","id":10,"accion":"suma","datos":{"a":1,"b":1}})")).dump());
	r = json::parse(salida.back());
	COMPROBAR(r["id"] == 10 && r["datos"]["r"] == 2);

	// Estado: solo se envía si cambió
	COMPROBAR(p.publicarEstado(json{ {"x", 1} }));
	COMPROBAR(!p.publicarEstado(json{ {"x", 1} }));
	COMPROBAR(p.publicarEstado(json{ {"x", 2} }));
	COMPROBAR(p.publicarEstado(json{ {"x", 2} }, true));
	// Texto con UTF-8 inválido no rompe
	COMPROBAR(p.publicarEstado(json{ {"n", std::string("ab\xff\xfe")} }));
	// Al recargar la interfaz se vuelve a mandar todo
	p.reiniciar();
	COMPROBAR(!p.listo());
	p.recibir(R"({"t":"hola"})");
	COMPROBAR(p.publicarEstado(json{ {"x", 2} }));
}

static void pruebaRed() {
	MonitorRed m;
	UmbralesRed u;
	COMPROBAR(MonitorRed::semaforo(-1, -1.0, u) == "sin_datos");
	COMPROBAR(MonitorRed::semaforo(40, 5.0, u) == "verde");
	COMPROBAR(MonitorRed::semaforo(80, 5.0, u) == "ambar");
	COMPROBAR(MonitorRed::semaforo(40, 30.0, u) == "rojo");
	COMPROBAR(MonitorRed::semaforo(150, -1.0, u) == "rojo");
	double t = 0;
	for (int i = 0; i < 10; i++) m.registrar({ {111, {"Ana", 30 + (i % 2) * 10}}, {222, {"Beto", 140}} }, t += 1.0);
	auto r = m.resumen(u);
	COMPROBAR(r.size() == 2);
	const ResumenRed* ana = nullptr; const ResumenRed* beto = nullptr;
	for (const auto& x : r) { if (x.parsecId == 111) ana = &x; if (x.parsecId == 222) beto = &x; }
	COMPROBAR(ana && beto);
	COMPROBAR(ana->media == 35 && ana->maximo == 40 && ana->jitter == 10.0 && ana->semaforo == "verde");
	COMPROBAR(beto->semaforo == "rojo" && beto->alerta && beto->picos == 10);
	// Beto se va: sigue 2 min y luego se olvida
	for (int i = 0; i < 5; i++) m.registrar({ {111, {"Ana", 35}} }, t += 1.0);
	r = m.resumen(u);
	COMPROBAR(r.size() == 2 && r[0].presente && !r[1].presente && !r[1].alerta);
	m.registrar({ {111, {"Ana", 35}} }, t += 130.0);
	COMPROBAR(m.resumen(u).size() == 1);
	// La serie se limita a 120 muestras
	for (int i = 0; i < 300; i++) m.registrar({ {111, {"Ana", -1}} }, t += 1.0);
	r = m.resumen(u);
	COMPROBAR(r[0].serie.size() == MonitorRed::kMuestras && r[0].media == -1 && r[0].semaforo == "sin_datos");
}

static void pruebaPartido(const std::string& carpeta) {
	Partido p;
	LadoPartido a{ "", { {111, "Ana"} }, 0 }, b{ "", { {222, "Beto"} }, 0 };
	COMPROBAR(!p.iniciar(1000, 0.0));                  // sin preparar
	COMPROBAR(!p.preparar(a, LadoPartido{ "", {{111, "Ana"}}, 0 })); // misma persona en los dos lados
	COMPROBAR(!p.preparar(a, LadoPartido{}));           // lado vacío
	COMPROBAR(p.preparar(a, b));
	COMPROBAR(p.ladoA().nombre == "Ana" && p.fase() == FasePartido::Listo);
	COMPROBAR(!p.gol(0, 1));                           // aún no empieza
	COMPROBAR(p.iniciar(1700000000000LL, 10.0));
	COMPROBAR(p.gol(0, 1) && p.gol(0, 1) && p.gol(1, 1) && p.gol(1, -5));
	COMPROBAR(p.marcador() == "2-0");
	COMPROBAR(p.pausar(true, 70.0) && !p.pausar(true, 71.0));
	COMPROBAR(p.segundosJugados(100.0) == 60);
	COMPROBAR(p.pausar(false, 130.0));
	COMPROBAR(p.segundosJugados(140.0) == 70);
	p.invertirMandos();
	COMPROBAR(p.comoJson(140.0)["invertido"] == true);
	auto reg = p.finalizar(200.0);
	COMPROBAR(reg && reg->ganador == "a" && reg->duracionSeg == 130 && p.fase() == FasePartido::Libre && !p.mandosInvertidos());
	COMPROBAR(!p.finalizar(201.0));
	// Durante un partido no se puede volver a preparar ni iniciar; el marcador tope es 99
	for (int i = 0; i < 120; i++) { p.preparar(a, b); p.iniciar(1, 0); p.gol(1, 1); }
	COMPROBAR(p.fase() == FasePartido::EnJuego && p.ladoB().goles == 99);
	p.cancelar();
	COMPROBAR(p.fase() == FasePartido::Libre && p.ladoA().jugadores.empty());

	const std::string ruta = carpeta + "/partidos.json";
	std::filesystem::remove(ruta);
	HistorialPartidos h(ruta);
	COMPROBAR(h.lista().empty());
	COMPROBAR(h.agregar(*reg));
	RegistroPartido otro = *reg; otro.id = otro.inicioMs = reg->inicioMs + 5000; otro.ganador = "empate";
	COMPROBAR(h.agregar(otro));
	auto l = h.lista();
	COMPROBAR(l.size() == 2 && l[0].ganador == "empate" && l[1].a.goles == 2 && l[1].b.jugadores[0].nombre == "Beto");
	for (int i = 0; i < 250; i++) { RegistroPartido x = otro; x.inicioMs += i + 10; h.agregar(x); }
	COMPROBAR(h.lista().size() == HistorialPartidos::kMaximo);
	// Archivo dañado → lista vacía, y se puede volver a escribir
	{ std::FILE* f = std::fopen(ruta.c_str(), "wb"); std::fputs("{roto", f); std::fclose(f); }
	COMPROBAR(h.lista().empty());
	COMPROBAR(h.agregar(otro) && h.lista().size() == 1);
}

static void pruebaPerfiles(const std::string& carpeta) {
	const std::string ruta = carpeta + "/perfiles.json";
	std::filesystem::remove(ruta);
	PerfilesSala p(ruta);
	COMPROBAR(p.lista().empty());
	COMPROBAR(!p.guardar("   ", json::object()));
	COMPROBAR(p.guardar("  Liga oficial ", json{ {"plazas", 4} }));
	COMPROBAR(p.guardar("Amistosos", json{ {"plazas", 8} }));
	COMPROBAR(p.guardar("LIGA OFICIAL", json{ {"plazas", 6} }));   // reemplaza
	auto l = p.lista();
	COMPROBAR(l.size() == 2 && l[0].nombre == "LIGA OFICIAL" && l[0].valores["plazas"] == 6);
	COMPROBAR(p.obtener("liga oficial").has_value() && !p.obtener("otra").has_value());
	COMPROBAR(p.borrar("amistosos") && !p.borrar("amistosos") && p.lista().size() == 1);
	for (int i = 0; i < 30; i++) p.guardar("P" + std::to_string(i), json::object());
	COMPROBAR(p.lista().size() == PerfilesSala::kMaximo);
	const std::string largo(60, 'x');
	COMPROBAR(PerfilesSala::normalizar(largo).size() == PerfilesSala::kLargoNombre);
	// No parte una letra con tilde (2 bytes) en el corte
	std::string tildes; for (int i = 0; i < 30; i++) tildes += "\xC3\xA1"; // «á» x30 = 60 bytes
	const std::string corto = PerfilesSala::normalizar(tildes);
	COMPROBAR(corto.size() == 40 && (static_cast<unsigned char>(corto.back()) & 0xC0) == 0x80);
}

int main() {
	const std::string carpeta = std::filesystem::temp_directory_path().string() + "/phx_web_test";
	std::filesystem::create_directories(carpeta);
	pruebaPuente();
	pruebaRed();
	pruebaPartido(carpeta);
	pruebaPerfiles(carpeta);
	std::printf("%d pruebas, %d fallos\n", gPruebas, gFallos);
	return gFallos == 0 ? 0 : 1;
}
