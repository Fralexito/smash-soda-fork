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

	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
