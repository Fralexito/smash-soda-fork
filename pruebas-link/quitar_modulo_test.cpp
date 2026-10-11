// Prueba de phoenix::modulos::desinstalar sobre una COPIA de juego (nunca el juego real).
// OJO: el .exe NO puede llevar «instal» en el nombre: Windows lo toma por un instalador y pide permisos.
// Uso: quitar_modulo_test <carpeta vacía de trabajo> <sider.ini real (solo se copia)>
#include "phoenix/link/ModulosJuego.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace phoenix::modulos;

static int fallos = 0;
#define OK(x) do { if (!(x)) { std::cerr << "FALLA linea " << __LINE__ << ": " #x "\n"; fallos++; } } while (0)

static std::string leer(const fs::path& p) { std::ifstream in(p, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), {}); }
static void escribir(const fs::path& p, const std::string& s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
static int contar(const std::string& s, const std::string& sub) { int n = 0; for (size_t i = s.find(sub); i != std::string::npos; i = s.find(sub, i + 1)) n++; return n; }

static int principal(int argc, char** argv);
int main(int argc, char** argv) {
	try { return principal(argc, argv); }
	catch (const std::exception& e) { std::cerr << "EXCEPCION: " << e.what() << std::endl; return 3; }
}

static int principal(int argc, char** argv) {
	if (argc < 3) { std::cerr << "uso: quitar_modulo_test <trabajo> <sider.ini>\n"; return 2; }
	const fs::path base = argv[1];
	const fs::path juego = base / "Juego", paquete = base / "paquete";
	std::string limpio;
	{
		std::istringstream in(leer(argv[2])); std::string l;
		while (std::getline(in, l)) {
			if (!l.empty() && l.back() == '\r') l.pop_back();
			if (l.find("Phoenix-DB") != std::string::npos || l.find("\"phoenix") != std::string::npos) continue;
			limpio += l + "\n";
		}
	}
	const fs::path sa[2] = { juego / "SiderAddons", juego / "ConmeGol Extras" / "ConmeGOL Patch 26" / "SiderAddons" };
	for (const auto& d : sa) {
		escribir(d / "olmosjr23" / "Database" / "common" / "etc" / "pesdb" / "Player.bin", "base");
		fs::create_directories(d / "modules");
		escribir(d / "sider.ini", limpio);
	}
	escribir(paquete / "phoenix.lua", "local m = { version = \"0.18b\" }\nreturn m\n");
	escribir(paquete / "phoenix_estadio.lua", "local m = { version = \"1.0\" }\nreturn m\n");

	OK(instalar(paquete, juego, "phoenix_estadio.lua").ok);
	OK(instalar(paquete, juego, "phoenix.lua").ok);
	for (const auto& d : sa) OK(contar(leer(d / "sider.ini"), "\"phoenix_estadio.lua\"") == 1 && contar(leer(d / "sider.ini"), "Phoenix-DB") == 1);

	// Apagar y luego desinstalar: también quita la línea comentada
	OK(quitar(paquete, juego, "phoenix_estadio.lua").ok);
	Resultado r = desinstalar(paquete, juego, "phoenix_estadio.lua");
	std::cout << "estadio: " << r.mensaje << "\n";
	OK(r.ok);
	for (const auto& d : sa) {
		const std::string ini = leer(d / "sider.ini");
		OK(contar(ini, "phoenix_estadio.lua") == 0);
		OK(contar(ini, "\"phoenix.lua\"") == 1);          // el otro módulo no se toca
		OK(!fs::exists(d / "modules" / "phoenix_estadio.lua"));
		bool apartado = false;
		for (const auto& e : fs::directory_iterator(d / "modules")) if (e.path().filename().string().rfind("phoenix_estadio.lua.desinstalado-", 0) == 0) apartado = true;
		OK(apartado);
	}

	// phoenix.lua: también se va la raíz Phoenix-DB; el sider.ini vuelve a ser el de partida
	r = desinstalar(paquete, juego, "phoenix.lua");
	std::cout << "phoenix: " << r.mensaje << "\n";
	OK(r.ok);
	for (const auto& d : sa) {
		const std::string ini = leer(d / "sider.ini");
		OK(contar(ini, "Phoenix-DB") == 0);
		OK(contar(ini, "\"phoenix.lua\"") == 0);
		OK(ini == limpio);
	}

	// Otra vez: no hay nada que quitar
	r = desinstalar(paquete, juego, "phoenix.lua");
	OK(r.ok);
	// Solo los módulos que lleva Link
	OK(!desinstalar(paquete, juego, "otro.lua").ok);

	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
