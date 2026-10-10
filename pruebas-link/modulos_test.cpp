// Pruebas del instalador de módulos (phoenix/link/ModulosJuego.*).
// 1) Lógica pura de sider.ini.  2) Instalación completa sobre una COPIA de un juego (nunca el real):
//    argv[1] = carpeta de trabajo vacía, argv[2] = sider.ini real para copiar (opcional).
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "phoenix/link/ModulosJuego.h"

namespace fs = std::filesystem;
using namespace phoenix::modulos;

static int fallos = 0;
#define OK(c) do { if (!(c)) { std::cerr << "FALLA linea " << __LINE__ << ": " #c "\n"; fallos++; } } while (0)

static std::string leer(const fs::path& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void escribir(const fs::path& p, const std::string& t) { std::ofstream f(p, std::ios::binary); f << t; }
static int contar(const std::string& s, const std::string& sub) { int n = 0; for (size_t i = s.find(sub); i != std::string::npos; i = s.find(sub, i + 1)) n++; return n; }

int main(int argc, char** argv) {
	// ── 1) Lógica pura ──
	{
		const std::string ini = "\xEF\xBB\xBF[sider]\r\nlua.module = \"a.lua\"\r\n;lua.module = \"viejo.lua\"\r\nlua.module = \"phoenix.lua\"\r\n\r\noverlay.enabled = 1\r\n";
		bool cambio = false; std::string err;
		const std::string a = activarLinea(ini, "phoenix_estadio.lua", cambio, err);
		OK(cambio && err.empty());
		OK(a.find("lua.module = \"phoenix.lua\"\r\nlua.module = \"phoenix_estadio.lua\"\r\n\r\noverlay") != std::string::npos);  // justo debajo del último activo
		OK(a.rfind("\xEF\xBB\xBF", 0) == 0);                    // BOM intacto
		OK(a.size() == ini.size() + std::string("lua.module = \"phoenix_estadio.lua\"\r\n").size());   // solo una línea más
		const std::string b = activarLinea(a, "phoenix_estadio.lua", cambio, err);
		OK(!cambio && b == a);                                    // idempotente
		const std::string c = comentarLinea(a, "phoenix_estadio.lua", cambio);
		OK(cambio && c.find(";lua.module = \"phoenix_estadio.lua\"") != std::string::npos);
		bool act = true, com = false; buscarLinea(c, "phoenix_estadio.lua", act, com);
		OK(!act && com);
		const std::string d = activarLinea(c, "phoenix_estadio.lua", cambio, err);   // reactivar = quitar el ;
		OK(cambio && d == a);
		const std::string e = activarLinea("[sider]\nnada = 1\n", "x.lua", cambio, err);
		OK(e.empty() && !err.empty());                            // sin lista de módulos: no se toca
		const std::string f = activarLinea(ini, "viejo.lua", cambio, err);
		OK(cambio && f.find(";lua.module = \"viejo.lua\"") == std::string::npos && f.find("lua.module = \"viejo.lua\"") != std::string::npos);
		OK(versionDe("local m = { version = \"1.0\" }") == "1.0");
		std::cout << "logica pura: ok\n";
	}

	// ── 2) Instalación en una copia de juego ──
	if (argc >= 2) {
		const fs::path base(argv[1]);
		const fs::path juego = base / "Juego";
		const fs::path paquete = base / "paquete";
		fs::create_directories(juego / "SiderAddons" / "modules");
		fs::create_directories(juego / "ConmeGol Extras" / "ConmeGOL Patch 26" / "SiderAddons" / "modules");
		fs::create_directories(paquete);
		const std::string iniBase = argc >= 3 ? leer(argv[2]) : std::string("[sider]\r\nlua.module = \"phoenix.lua\"\r\n");
		escribir(juego / "SiderAddons" / "sider.ini", iniBase);
		escribir(juego / "ConmeGol Extras" / "ConmeGOL Patch 26" / "SiderAddons" / "sider.ini", iniBase);
		escribir(paquete / "phoenix_estadio.lua", "local m = { version = \"1.0\" }\nreturn m\n");

		OK(destinos(juego).size() == 2);
		auto est = estado(paquete, juego);
		OK(!est.empty() && est[0].archivo == "phoenix_estadio.lua" && est[0].estado == "no_instalado");

		Resultado r = instalar(paquete, juego, "phoenix_estadio.lua");
		std::cout << "instalar: " << r.mensaje << "\n";
		OK(r.ok && r.respaldos.size() == 2);
		for (const auto& d : destinos(juego)) {
			const std::string ini = leer(d.second / "sider.ini");
			OK(contar(ini, "lua.module = \"phoenix_estadio.lua\"") == 1);
			OK(ini.size() == iniBase.size() + std::string("lua.module = \"phoenix_estadio.lua\"").size() + (iniBase.find("\r\n") != std::string::npos ? 2 : 1));
			OK(leer(d.second / "modules" / "phoenix_estadio.lua") == leer(paquete / "phoenix_estadio.lua"));
			OK(!fs::exists(d.second / "sider.tmp"));
		}
		for (const auto& resp : r.respaldos) OK(leer(fs::u8path(resp)) == iniBase);
		est = estado(paquete, juego);
		OK(est[0].estado == "instalado");

		r = instalar(paquete, juego, "phoenix_estadio.lua");      // segunda vez: nada que cambiar
		OK(r.ok && r.respaldos.empty());

		r = quitar(paquete, juego, "phoenix_estadio.lua");
		std::cout << "quitar: " << r.mensaje << "\n";
		OK(r.ok);
		est = estado(paquete, juego);
		OK(est[0].estado == "apagado");

		escribir(paquete / "phoenix_estadio.lua", "local m = { version = \"1.1\" }\nreturn m\n");   // versión nueva
		r = instalar(paquete, juego, "phoenix_estadio.lua");
		OK(r.ok);
		est = estado(paquete, juego);
		OK(est[0].estado == "instalado" && est[0].version == "1.1");
		for (const auto& d : destinos(juego)) OK(contar(leer(d.second / "sider.ini"), "phoenix_estadio.lua") == 1);

		r = quitar(paquete, juego, "phoenix.lua");                // no es suyo: no lo toca
		OK(!r.ok);
		r = instalar(paquete, juego, "..\\malo.lua");
		OK(!r.ok);
		std::cout << "instalacion en copia: ok\n";
	}

	// ── 3) phoenix.lua en la PC de un amigo: sider.ini SIN las líneas de Phoenix (argv[3] = sider.ini real) ──
	if (argc >= 4) {
		const fs::path base = fs::path(argv[1]) / "amigo";
		const fs::path juego = base / "Juego";
		const fs::path paquete = base / "paquete";
		std::string limpio;
		{   // quitar las 2 líneas de Phoenix del sider.ini real = ConmeGOL recién instalado
			std::istringstream in(leer(argv[3])); std::string l;
			while (std::getline(in, l)) {
				if (!l.empty() && l.back() == '\r') l.pop_back();
				if (l.find("Phoenix-DB") != std::string::npos || l.find("\"phoenix.lua\"") != std::string::npos) continue;
				limpio += l + "\n";
			}
		}
		const fs::path sa[2] = { juego / "SiderAddons", juego / "ConmeGol Extras" / "ConmeGOL Patch 26" / "SiderAddons" };
		for (const auto& d : sa) {
			fs::create_directories(d / "modules");
			fs::create_directories(d / "olmosjr23" / "Database" / "common" / "etc" / "pesdb");
			escribir(d / "olmosjr23" / "Database" / "common" / "etc" / "pesdb" / "Player.bin", "base del parche");
			escribir(d / "sider.ini", limpio);
		}
		escribir(sa[1] / "modules" / "phoenix.lua", "local m = { version = \"0.17e-B\" }\n");   // una versión vieja en un destino
		fs::create_directories(paquete);
		escribir(paquete / "phoenix.lua", "local m = { version = \"0.18\" }\nreturn m\n");
		OK(!tieneRaizPhoenixDB(limpio));
		const auto estadoPhx = [&]() { for (const auto& m : estado(paquete, juego)) if (m.archivo == "phoenix.lua") return m.gestionable ? m.estado : std::string("NO-GESTIONABLE"); return std::string("NO-LISTADO"); };
		OK(estadoPhx() == "a_medias");   // una versión vieja en un destino
		Resultado r = instalar(paquete, juego, "phoenix.lua");
		std::cout << "phoenix.lua (amigo): " << r.mensaje << "\n";
		OK(r.ok && r.respaldos.size() == 2);
		OK(estadoPhx() == "instalado");
		for (const auto& d : sa) {
			const std::string ini = leer(d / "sider.ini");
			OK(contar(ini, "cpk.root = \".\\livecpk\\Phoenix-DB\"") == 1);
			OK(contar(ini, "lua.module = \"phoenix.lua\"") == 1);
			OK(ini.find("cpk.root = \".\\livecpk\\Phoenix-DB\"\ncpk.root = \".\\olmosjr23\\Database\"") != std::string::npos);   // justo antes de la base
			OK(contar(ini, "\n") == contar(limpio, "\n") + 2);                // solo 2 líneas más
			OK(fs::is_directory(d / "content" / "phoenix"));
			OK(fs::is_directory(d / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb"));
			OK(leer(d / "modules" / "phoenix.lua") == leer(paquete / "phoenix.lua"));
		}
		OK(leer(sa[1] / "modules" / "phoenix.lua.antes-v0.17e-B") == "local m = { version = \"0.17e-B\" }\n");   // la vieja se guardó
		{   // sin la raíz Phoenix-DB no cuenta como instalado
			const std::string ini0 = leer(sa[0] / "sider.ini");
			std::string sin; std::istringstream in(ini0); std::string l;
			while (std::getline(in, l)) if (l.find("Phoenix-DB") == std::string::npos) sin += l + "\n";
			escribir(sa[0] / "sider.ini", sin);
			OK(estadoPhx() == "a_medias");
			escribir(sa[0] / "sider.ini", ini0);
		}
		r = instalar(paquete, juego, "phoenix.lua");                          // segunda vez: nada cambia
		OK(r.ok && r.respaldos.empty());
		for (const auto& d : sa) OK(contar(leer(d / "sider.ini"), "Phoenix-DB") == 1);
		std::cout << "phoenix.lua en la PC de un amigo: ok\n";
	}

	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
