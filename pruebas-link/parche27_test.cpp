// Prueba del instalador de módulos con la estructura del parche ConmeGOL 27 (2026-10-12), sobre una COPIA (nunca el juego real).
// Estructura que imita:  raíz con SiderAddons\sider.ini (el del parche nuevo, SIN líneas de Phoenix ni base) + ConmeGol Extras con
// 3 modos 27 (SiderAddons sin sider.ini) y 1 modo 26 (con su sider.ini y las líneas de Phoenix).
// OJO: el .exe NO puede llevar «instal» en el nombre (Windows pide elevación).
// Uso: parche27_test <carpeta de trabajo (se vacía)> [sider.ini real de la raíz 27] [sider.ini real de un modo 26]
#include "phoenix/link/ModulosJuego.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using namespace phoenix::modulos;

static int fallos = 0;
#define OK(x) do { if (!(x)) { std::cerr << "FALLA linea " << __LINE__ << ": " #x "\n"; fallos++; } } while (0)

static std::string leer(const fs::path& p) { std::ifstream in(p, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), {}); }
static void escribir(const fs::path& p, const std::string& s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
static int contar(const std::string& s, const std::string& sub) { int n = 0; for (size_t i = s.find(sub); i != std::string::npos; i = s.find(sub, i + 1)) n++; return n; }
static int lineas(const std::string& s) { return contar(s, "\n"); }

static const Modulo* buscar(const std::vector<Modulo>& v, const std::string& a) { for (const auto& m : v) if (m.archivo == a) return &m; return nullptr; }

static int principal(int argc, char** argv) {
	if (argc < 2) { std::cerr << "uso: parche27_test <trabajo> [sider.ini 27] [sider.ini 26]\n"; return 2; }
	const fs::path base(argv[1]);
	const fs::path juego = base / "Juego", paquete = base / "paquete";
	std::error_code ec;
	fs::remove_all(base, ec);
	const std::string phx = "local m = { version = \"0.18b\" }\nreturn m\n", ref = "local m = { version = \"1.2\" }\nreturn m\n";
	escribir(paquete / "phoenix.lua", phx);
	escribir(paquete / "phoenix_referee.lua", ref);

	// sider.ini del parche 27: sin Phoenix, sin Phoenix-DB y sin base de olmosjr23
	const std::string ini27 = argc >= 3 ? leer(argv[2]) : std::string("[sider]\r\nmatch-stats.enabled = 1\r\ncpk.root = \".\\livecpk\\Faces\"\r\nlua.enabled = 1\r\nlua.module = \"CGP_data.lua\"\r\nlua.module = \"StartingYearChanger.lua\"\r\n\r\noverlay.enabled = 1\r\n");
	OK(!ini27.empty());
	OK(contar(ini27, "phoenix") == 0 && contar(ini27, "olmosjr23") == 0);
	// sider.ini de un modo 26: con Phoenix, Phoenix-DB y la base de olmosjr23
	const std::string ini26 = argc >= 4 ? leer(argv[3]) : std::string("[sider]\r\ncpk.root = \".\\livecpk\\Phoenix-DB\"\r\ncpk.root = \".\\olmosjr23\\Database\"\r\nlua.module = \"CGP_data.lua\"\r\nlua.module = \"phoenix.lua\"\r\nlua.module = \"phoenix_referee.lua\"\r\n");
	OK(contar(ini26, "\"phoenix.lua\"") == 1 && contar(ini26, "\"phoenix_referee.lua\"") == 1);

	const fs::path raiz = juego / "SiderAddons";
	escribir(raiz / "sider.ini", ini27);
	escribir(raiz / "modules" / "phoenix.lua", phx);
	escribir(raiz / "modules" / "phoenix_referee.lua", ref);
	escribir(raiz / "sider.ini.phoenix-20261011-155118", ini26);   // copia que dejó una instalación anterior de Link en la raíz
	fs::create_directories(raiz / "content" / "phoenix");
	const fs::path extras = juego / "ConmeGol Extras";
	for (const char* m : { "ConmeGOL Patch 27", "UEFAGOL 27", "ConmeGOL Clasicos" }) {
		fs::create_directories(extras / m / "SiderAddons" / "modules");
		fs::create_directories(extras / m / "SiderAddons" / "content");
		fs::create_directories(extras / m / "download");
		escribir(extras / m / "CPY.ini", "[Settings]\r\nSteamID=239200\r\n");
	}
	const fs::path m26 = extras / "ConmeGOL Patch 26" / "SiderAddons";
	escribir(m26 / "sider.ini", ini26);
	escribir(m26 / "modules" / "phoenix.lua", phx);
	escribir(m26 / "modules" / "phoenix_referee.lua", ref);
	escribir(juego / "version_actual.txt", "ConmeGOL Patch 27\r\n");

	// ── 1) Lectura de la estructura ──
	{
		const auto ds = destinos(juego);
		OK(ds.size() == 2);                                  // la raíz y el modo 26; los modos 27 NO tienen sider.ini propio
		const Parche p = infoParche(juego);
		OK(p.modoActual == "ConmeGOL Patch 27");
		OK(p.modosRaiz.size() == 3 && p.modosPropios.size() == 1 && p.modosPropios[0] == "ConmeGOL Patch 26");
		OK(p.advertencias.empty());
	}

	// ── 2) Estado tras la actualización del parche: la línea se perdió en la raíz ──
	{
		const auto est = estado(paquete, juego);
		const Modulo* a = buscar(est, "phoenix.lua");
		const Modulo* b = buscar(est, "phoenix_referee.lua");
		OK(a && a->estado == "perdido");
		OK(b && b->estado == "perdido");
		OK(a && a->destinos.size() == 2 && a->destinos[0].perdida && !a->destinos[0].linea && a->destinos[0].sinBase);
		OK(a && !a->destinos[1].perdida && a->destinos[1].linea && !a->destinos[1].sinBase);
	}

	// ── 3) Reinstalar: solo cambia el sider.ini de la raíz, con UNA línea por módulo y sin Phoenix-DB ──
	{
		const std::string ini26Antes = leer(m26 / "sider.ini");
		Resultado r = instalar(paquete, juego, "phoenix.lua");
		std::cout << "phoenix.lua: " << r.mensaje << "\n";
		OK(r.ok && r.respaldos.size() == 1);                // solo la raíz (el modo 26 ya la tenía)
		r = instalar(paquete, juego, "phoenix_referee.lua");
		std::cout << "phoenix_referee.lua: " << r.mensaje << "\n";
		OK(r.ok && r.respaldos.size() == 1);
		const std::string nuevo = leer(raiz / "sider.ini");
		OK(contar(nuevo, "lua.module = \"phoenix.lua\"") == 1);
		OK(contar(nuevo, "lua.module = \"phoenix_referee.lua\"") == 1);
		OK(lineas(nuevo) == lineas(ini27) + 2);              // nada más del ini cambia
		OK(nuevo.find("Phoenix-DB") == std::string::npos);   // sin base reconocida NO se pone Phoenix-DB
		OK(!fs::exists(raiz / "livecpk" / "Phoenix-DB"));    // ni se crea su carpeta
		OK(fs::is_directory(raiz / "content" / "phoenix"));
		OK(leer(m26 / "sider.ini") == ini26Antes);           // el modo 26 queda intacto
		int copias = 0;
		bool hayIgual = false;                               // una copia nueva es exactamente el sider.ini de antes
		for (const auto& e : fs::directory_iterator(raiz)) {
			if (e.path().filename().string().rfind("sider.ini.phoenix-", 0) != 0) continue;
			copias++;
			if (leer(e.path()) == ini27) hayIgual = true;
		}
		OK(copias >= 2 && hayIgual);
		const auto est = estado(paquete, juego);
		OK(buscar(est, "phoenix.lua") && buscar(est, "phoenix.lua")->estado == "instalado");   // sin base no se exige Phoenix-DB
		OK(buscar(est, "phoenix_referee.lua") && buscar(est, "phoenix_referee.lua")->estado == "instalado");
		r = instalar(paquete, juego, "phoenix.lua");          // segunda vez: nada que cambiar
		OK(r.ok && r.respaldos.empty());
	}

	// ── 4) Advertencias: el modo actual es del 27 pero la raíz trae Phoenix-DB / la base de un modo 26 (la dejó el lanzador al iniciar un 26) ──
	{
		const std::string ini = leer(raiz / "sider.ini");
		escribir(raiz / "sider.ini", ini26);
		OK(!infoParche(juego).advertencias.empty());          // Phoenix-DB activa en un modo 27
		bool c = false;
		escribir(raiz / "sider.ini", quitarRaizPhoenixDB(ini26, c));
		OK(infoParche(juego).advertencias.size() == 1);       // solo la base de olmosjr23
		escribir(juego / "version_actual.txt", "ConmeGOL Patch 26\r\n");
		OK(infoParche(juego).advertencias.empty());           // con un modo 26 es lo normal
		escribir(juego / "version_actual.txt", "ConmeGOL Patch 27\r\n");
		escribir(raiz / "sider.ini", ini);
	}

	// ── 5) Sin pruebas de que Phoenix ya estuvo ahí no se afirma «perdido» ──
	{
		const fs::path j2 = base / "Juego2";
		escribir(j2 / "SiderAddons" / "sider.ini", ini27);
		escribir(j2 / "SiderAddons" / "modules" / "phoenix_referee.lua", ref);
		const Modulo* b = buscar(estado(paquete, j2), "phoenix_referee.lua");
		OK(b && b->estado != "perdido");
		Resultado r = instalar(paquete, j2, "phoenix_referee.lua");
		OK(r.ok && contar(leer(j2 / "SiderAddons" / "sider.ini"), "lua.module = \"phoenix_referee.lua\"") == 1);
	}

	// ── 6) Apagar y desinstalar siguen funcionando en la raíz del 27 ──
	{
		Resultado r = quitar(paquete, juego, "phoenix_referee.lua");
		OK(r.ok);
		OK(buscar(estado(paquete, juego), "phoenix_referee.lua")->estado == "apagado");
		r = desinstalar(paquete, juego, "phoenix_referee.lua");
		OK(r.ok);
		OK(contar(leer(raiz / "sider.ini"), "phoenix_referee.lua") == 0);
		OK(contar(leer(m26 / "sider.ini"), "phoenix_referee.lua") == 0);
		OK(lineas(leer(raiz / "sider.ini")) >= lineas(ini27));
	}

	// ── 7) Lógica pura de la base ──
	{
		const auto sinPlayer = [](const std::string&) { return false; };
		OK(!hayBaseParche(ini27, sinPlayer));
		OK(hayBaseParche("[sider]\r\ncpk.root = \".\\olmosjr23\\Database\"\r\n", sinPlayer));
		OK(!hayBaseParche("[sider]\r\n;cpk.root = \".\\olmosjr23\\Database\"\r\n", sinPlayer));          // comentada: no cuenta
		OK(!hayBaseParche("[sider]\r\ncpk.root = \".\\livecpk\\Phoenix-DB\"\r\n", [](const std::string&) { return true; }));   // Phoenix-DB no es una base
		bool cambio = false; std::string err;
		const std::string r = activarRaizPhoenixDB(ini27, sinPlayer, cambio, err);
		OK(!cambio && r == ini27);                              // con el ini del 27 (sin base) no se pone
	}

	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}

int main(int argc, char** argv) {
	try { return principal(argc, argv); }
	catch (const std::exception& e) { std::cerr << "EXCEPCION: " << e.what() << std::endl; return 3; }
}
