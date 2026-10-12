// Pruebas de ESCENARIOS del instalador de módulos (Sync › Módulos del juego) en carpetas temporales con juegos FALSOS
// (estructura de ConmeGOL: SiderAddons, ConmeGol Extras\<modo>\SiderAddons, olmosjr23\Database...). No toca ningún juego real.
// Compila en Windows (MSVC): ver pruebas-link/correr_escenarios.bat.  Uso: escenarios_sync_test <carpeta temporal vacía> <carpeta sider del paquete>
// Cada escenario se describe en docs/SYNC-ESCENARIOS.md.
#include "phoenix/link/ModulosJuego.h"
#include "phoenix/link/Entrega.h"

#include <windows.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace phoenix::modulos;
using phoenix::entrega::deU8;

static int total = 0, fallos = 0;
#define CHECK(cond, msg) do { total++; if (!(cond)) { fallos++; std::cout << "  FALLO: " << msg << "\n"; } else std::cout << "  ok: " << msg << "\n"; } while (0)

static fs::path base, paquete;
static int contador = 0;

static void escribir(const fs::path& p, const std::string& s) {
	fs::create_directories(p.parent_path());
	std::ofstream f(p, std::ios::binary | std::ios::trunc);
	f.write(s.data(), static_cast<std::streamsize>(s.size()));
}
static std::string leer(const fs::path& p) {
	std::ifstream f(p, std::ios::binary);
	std::stringstream ss; ss << f.rdbuf();
	return ss.str();
}
static int contarRespaldos(const fs::path& sider) {
	int n = 0;
	for (const auto& e : fs::directory_iterator(sider)) if (e.path().filename().string().rfind("sider.ini.phoenix-", 0) == 0) n++;
	return n;
}
static int contarLineas(const std::string& ini, const std::string& frag) {
	int n = 0; size_t i = 0;
	while ((i = ini.find(frag, i)) != std::string::npos) { n++; i += frag.size(); }
	return n;
}

// sider.ini típicos
static const std::string FACES = "cpk.root = \".\\livecpk\\Faces\"\r\n";
static const std::string OLMOS = "cpk.root = \".\\olmosjr23\\Database\"\r\n";
static const std::string MODS = "\r\nlua.module = \"CGP_data.lua\"\r\nlua.module = \"kserv.lua\"\r\nlua.module = \"DR_KitMixer.lua\"\r\n";
static const std::string INI_26 = FACES + OLMOS + MODS;   // parche 26: tiene base
static const std::string INI_27 = FACES + MODS;           // parche 27: la base va dentro de un .cpk, sin base reconocida

static fs::path nuevoJuego(const std::string& nombre) {
	const fs::path j = base / ("t" + std::to_string(++contador)) / deU8(nombre);
	fs::create_directories(j);
	escribir(j / "PES2021.exe", "MZ");
	return j;
}
/// Crea <carpeta>\SiderAddons con sider.ini y modules\ (y, si pides base, olmosjr23\Database con su Player.bin).
static fs::path sider(const fs::path& carpeta, const std::string& ini, bool conBase) {
	const fs::path s = carpeta / "SiderAddons";
	fs::create_directories(s / "modules");
	escribir(s / "sider.ini", ini);
	if (conBase) escribir(s / "olmosjr23" / "Database" / "common" / "etc" / "pesdb" / "Player.bin", "x");
	return s;
}
static void instalarAmbos(const fs::path& j, Resultado& a, Resultado& b) {
	a = instalar(paquete, j, "phoenix_referee.lua");
	b = instalar(paquete, j, "phoenix.lua");
}
static const Modulo* modulo(const std::vector<Modulo>& v, const std::string& n) { for (const auto& m : v) if (m.archivo == n) return &m; return nullptr; }

int main(int argc, char** argv) {
	if (argc < 3) { std::cerr << "uso: escenarios_sync_test <carpeta temporal> <carpeta sider del paquete>\n"; return 2; }
	base = deU8(argv[1]); paquete = deU8(argv[2]);
	fs::create_directories(base);
	Resultado a, b;

	std::cout << "E01 ruta con espacios, acentos, ñ y OneDrive\n";
	{
		const fs::path j = nuevoJuego("OneDrive - Fralex ñandú\\Juegos é\\PES 2021 ConmeGOL");
		const fs::path s = sider(j, INI_26, true);
		instalarAmbos(j, a, b);
		CHECK(a.ok && b.ok, "instala los dos módulos: " + a.mensaje.substr(0, 40));
		const auto est = estado(paquete, j);
		CHECK(modulo(est, "phoenix.lua") && modulo(est, "phoenix.lua")->estado == "instalado", "phoenix.lua queda INSTALADO");
		CHECK(contarLineas(leer(s / "sider.ini"), "Phoenix-DB") == 1, "una sola raíz Phoenix-DB");
		CHECK(contarRespaldos(s) >= 1, "hay copia de sider.ini con sello");
	}

	std::cout << "E02 sin Sider (no hay SiderAddons\\sider.ini): se niega con mensaje claro y no crea nada\n";
	{
		const fs::path j = nuevoJuego("PES vanilla");
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(!a.ok && a.mensaje.find("SiderAddons") != std::string::npos, "mensaje: " + a.mensaje);
		CHECK(!fs::exists(j / "SiderAddons"), "no se creó SiderAddons");
	}

	std::cout << "E03 sider.ini vacío / sin lista de módulos: se niega y no cambia nada\n";
	for (const std::string& ini : { std::string(), FACES + OLMOS }) {
		const fs::path j = nuevoJuego("vacio");
		const fs::path s = sider(j, ini, true);
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(!a.ok && a.mensaje.find("lua.module") != std::string::npos, "mensaje: " + a.mensaje);
		CHECK(leer(s / "sider.ini") == ini, "sider.ini intacto");
		CHECK(contarRespaldos(s) == 0, "sin copias inútiles");
	}

	std::cout << "E04 sider.ini con BOM y saltos LF: se respetan\n";
	{
		const fs::path j = nuevoJuego("bom");
		std::string ini = "\xEF\xBB\xBF" "cpk.root = \".\\olmosjr23\\Database\"\nlua.module = \"CGP_data.lua\"\nlua.module = \"kserv.lua\"\n";
		const fs::path s = sider(j, ini, true);
		a = instalar(paquete, j, "phoenix.lua");
		const std::string r = leer(s / "sider.ini");
		CHECK(a.ok, "instala: " + a.mensaje.substr(0, 50));
		CHECK(r.rfind("\xEF\xBB\xBF", 0) == 0, "conserva el BOM");
		CHECK(r.find('\r') == std::string::npos, "sigue con LF (sin CR)");
		CHECK(r.find("Phoenix-DB") < r.find("olmosjr23"), "Phoenix-DB antes de la base aunque la primera línea tenga BOM");
		const fs::path j2 = nuevoJuego("bom2");
		const fs::path s2 = sider(j2, "\xEF\xBB\xBF" "cpk.root = \".\\olmosjr23\\Database\"\r\nlua.module = \"a.lua\"\r\n", true);
		instalar(paquete, j2, "phoenix.lua");
		CHECK(leer(s2 / "sider.ini").find("Phoenix-DB") != std::string::npos, "reconoce la base aunque su línea lleve BOM");
	}

	std::cout << "E05 líneas de Phoenix duplicadas / en otra posición / comentadas / modificadas a mano\n";
	{
		const fs::path j = nuevoJuego("dup");
		const std::string ini = FACES + "cpk.root = \".\\livecpk\\Phoenix-DB\"\r\n" + OLMOS + "\r\nlua.module = \"phoenix.lua\"\r\nlua.module = \"CGP_data.lua\"\r\nlua.module = \"PHOENIX.LUA\"\r\nlua.module = \"kserv.lua\"\r\n";
		const fs::path s = sider(j, ini, true);
		a = instalar(paquete, j, "phoenix.lua");
		const std::string r = leer(s / "sider.ini");
		CHECK(a.ok, "instala: " + a.mensaje.substr(0, 40));
		CHECK(contarLineas(r, "phoenix.lua") + contarLineas(r, "PHOENIX.LUA") == 1, "queda UNA sola línea phoenix.lua");
		CHECK(contarLineas(r, "kserv.lua") == 1 && contarLineas(r, "CGP_data.lua") == 1, "los demás módulos intactos");
		const std::string antes = r;
		const int respaldos = contarRespaldos(s);
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(leer(s / "sider.ini") == antes && contarRespaldos(s) == respaldos, "segunda instalación: sin cambios ni copias nuevas");
		const fs::path j2 = nuevoJuego("comentada");
		const fs::path s2 = sider(j2, FACES + OLMOS + "\r\nlua.module = \"CGP_data.lua\"\r\n;lua.module = \"phoenix.lua\"\r\nlua.module = \"kserv.lua\"\r\n", true);
		instalar(paquete, j2, "phoenix.lua");
		const std::string r2 = leer(s2 / "sider.ini");
		CHECK(r2.find(";lua.module = \"phoenix.lua\"") == std::string::npos && contarLineas(r2, "lua.module = \"phoenix.lua\"") == 1, "la comentada se enciende sin duplicar");
	}

	std::cout << "E06 varios modos: 26 con base y 27 sin base; Phoenix-DB solo donde hay base reconocida\n";
	{
		const fs::path j = nuevoJuego("ConmeGOL multi");
		const fs::path raiz = sider(j, INI_27, false);
		const fs::path m26 = sider(j / "ConmeGol Extras" / "ConmeGOL Patch 26", INI_26, true);
		sider(j / "ConmeGol Extras" / "Clasicos (No LM)", FACES + "cpk.root = \".\\livecpk\\Clasicos\"\r\n" + MODS, false);
		escribir(j / "version_actual.txt", "ConmeGOL Patch 27");
		escribir(j / "ConmeGol Extras" / "ConmeGOL Patch 27" / "SiderAddons" / "cosa.txt", "x");
		instalarAmbos(j, a, b);
		CHECK(a.ok && b.ok, "instala: " + b.mensaje);
		CHECK(b.mensaje.find("Phoenix-DB no se puso") != std::string::npos, "el mensaje explica por qué no se puso Phoenix-DB (no es un error)");
		CHECK(leer(raiz / "sider.ini").find("Phoenix-DB") == std::string::npos, "raíz sin base: sin Phoenix-DB");
		CHECK(!fs::exists(raiz / "livecpk" / "Phoenix-DB"), "no se crea la carpeta Phoenix-DB sin base");
		CHECK(leer(m26 / "sider.ini").find("Phoenix-DB") != std::string::npos, "modo 26 con base: Phoenix-DB puesta");
		CHECK(fs::exists(m26 / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb"), "carpeta Phoenix-DB vacía creada solo en el 26");
		const auto est = estado(paquete, j);
		CHECK(modulo(est, "phoenix.lua")->estado == "instalado", "estado INSTALADO (la falta de base no cuenta como incompleto)");
		const Parche p = infoParche(j);
		CHECK(p.modoActual == "ConmeGOL Patch 27" && !p.modosRaiz.empty() && !p.modosPropios.empty(), "reconoce modos del 27 (raíz) y del 26 (propios)");
	}

	std::cout << "E07 PES vanilla / otro parche sin cambiador, sin base reconocida\n";
	{
		const fs::path j = nuevoJuego("vanilla con sider");
		const fs::path s = sider(j, FACES + MODS, false);
		instalarAmbos(j, a, b);
		CHECK(a.ok && b.ok, "instala los módulos (solo avisos y referee)");
		CHECK(leer(s / "sider.ini").find("Phoenix-DB") == std::string::npos && !fs::exists(s / "livecpk"), "no pone ni crea Phoenix-DB");
	}

	std::cout << "E08 PES o Sider abiertos: se detectan (la acción se niega antes de escribir)\n";
	{
		const fs::path j = nuevoJuego("en uso");
		const fs::path exe = j / "PES2021.exe";
		fs::copy_file("C:\\Windows\\System32\\ping.exe", exe, fs::copy_options::overwrite_existing);
		STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
		std::wstring cmd = L"\"" + exe.wstring() + L"\" -n 20 127.0.0.1";
		const BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
		CHECK(ok, "arranca un PES2021.exe de mentira");
		if (ok) {
			Sleep(400);
			const auto ab = procesosAbiertos();
			CHECK(!ab.empty() && ab.front() == "PES2021.exe", "procesosAbiertos() lo ve");
			TerminateProcess(pi.hProcess, 0); WaitForSingleObject(pi.hProcess, 3000);
			CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
			Sleep(200);
			CHECK(procesosAbiertos().empty(), "cerrado: ya no aparece");
		}
	}

	std::cout << "E09 transacción: un sider.ini de solo lectura o en uso en OTRO modo → se deja TODO como estaba\n";
	{
		const fs::path j = nuevoJuego("transaccion");
		const fs::path raiz = sider(j, INI_26, true);
		const fs::path m = sider(j / "ConmeGol Extras" / "Modo B", INI_26, true);
		const std::string r0 = leer(raiz / "sider.ini");
		SetFileAttributesW((m / "sider.ini").c_str(), FILE_ATTRIBUTE_READONLY);
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(!a.ok && a.mensaje.find("sider.ini") != std::string::npos, "solo lectura: falla con mensaje: " + a.mensaje.substr(0, 90));
		CHECK(a.mensaje.find("administrador") != std::string::npos, "el mensaje da una pista (permisos/administrador)");
		CHECK(leer(raiz / "sider.ini") == r0, "la raíz quedó EXACTAMENTE como estaba (vuelta atrás)");
		SetFileAttributesW((m / "sider.ini").c_str(), FILE_ATTRIBUTE_NORMAL);
		HANDLE h = CreateFileW((m / "sider.ini").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(!a.ok, "archivo en uso por otro programa: falla sin romper: " + a.mensaje.substr(0, 70));
		CHECK(leer(raiz / "sider.ini") == r0, "la raíz sigue intacta");
		if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(a.ok, "liberado el archivo, instala bien");
	}

	std::cout << "E10 falta la carpeta modules en un modo: se niega sin tocar nada\n";
	{
		const fs::path j = nuevoJuego("sin modules");
		const fs::path raiz = sider(j, INI_26, true);
		const fs::path m = sider(j / "ConmeGol Extras" / "Modo B", INI_26, true);
		fs::remove_all(m / "modules");
		const std::string r0 = leer(raiz / "sider.ini");
		a = instalar(paquete, j, "phoenix.lua");
		CHECK(!a.ok && a.mensaje.find("modules") != std::string::npos, "mensaje: " + a.mensaje);
		CHECK(leer(raiz / "sider.ini") == r0 && contarRespaldos(raiz) == 0, "nada cambiado");
	}

	std::cout << "E11 instalar dos veces, desinstalar y reinstalar; vuelta a una versión anterior con las copias\n";
	{
		const fs::path j = nuevoJuego("ciclo");
		const fs::path s = sider(j, INI_26, true);
		const std::string original = leer(s / "sider.ini");
		instalarAmbos(j, a, b);
		const std::string instalado = leer(s / "sider.ini");
		a = desinstalar(paquete, j, "phoenix.lua");
		b = desinstalar(paquete, j, "phoenix_referee.lua");
		CHECK(a.ok && b.ok, "desinstala los dos");
		CHECK(leer(s / "sider.ini") == original, "sider.ini idéntico al original tras desinstalar");
		CHECK(!fs::exists(s / "modules" / "phoenix.lua"), "el .lua se apartó (no se borró)");
		instalarAmbos(j, a, b);
		CHECK(a.ok && b.ok && leer(s / "sider.ini") == instalado, "reinstalar deja lo mismo que la primera vez");
		std::string copiaOriginal;
		for (const auto& e : fs::directory_iterator(s)) {
			const std::string n = e.path().filename().string();
			if (n.rfind("sider.ini.phoenix-", 0) == 0 && leer(e.path()) == original) copiaOriginal = n;
		}
		CHECK(!copiaOriginal.empty(), "existe una copia con sello idéntica al sider.ini original");
		escribir(s / "modules" / "phoenix_referee.lua", "local m = { version = \"1.2\" }\n");
		const auto est = estado(paquete, j);
		CHECK(modulo(est, "phoenix_referee.lua")->estado == "desactualizado", "referee 1.2 → HAY VERSIÓN NUEVA");
		instalar(paquete, j, "phoenix_referee.lua");
		CHECK(fs::exists(s / "modules" / "phoenix_referee.lua.antes-v1.2"), "la versión vieja queda guardada como .antes-v1.2");
	}

	std::cout << "E12 actualización del parche que borra las líneas de Phoenix\n";
	{
		const fs::path j = nuevoJuego("parche borra");
		const fs::path s = sider(j, INI_26, true);
		instalarAmbos(j, a, b);
		escribir(s / "sider.ini", INI_26);
		const auto est = estado(paquete, j);
		CHECK(modulo(est, "phoenix.lua")->estado == "perdido" && modulo(est, "phoenix_referee.lua")->estado == "perdido", "se detecta PERDIDO (REINSTALAR)");
		instalarAmbos(j, a, b);
		CHECK(modulo(estado(paquete, j), "phoenix.lua")->estado == "instalado", "REINSTALAR lo arregla");
	}

	std::cout << "E13 Phoenix-DB ya puesta sobre una base 27 (instalación vieja): Link lo advierte\n";
	{
		const fs::path j = nuevoJuego("db sobre 27");
		sider(j, FACES + "cpk.root = \".\\livecpk\\Phoenix-DB\"\r\n" + MODS, false);
		escribir(j / "version_actual.txt", "ConmeGOL Patch 27");
		escribir(j / "ConmeGol Extras" / "ConmeGOL Patch 27" / "SiderAddons" / "x.txt", "x");
		const Parche p = infoParche(j);
		CHECK(!p.advertencias.empty(), "advertencia: " + (p.advertencias.empty() ? std::string("(ninguna)") : p.advertencias.front().substr(0, 70)));
	}

	std::cout << "E14 nombre de módulo peligroso\n";
	{
		const fs::path j = nuevoJuego("seguro");
		sider(j, INI_26, true);
		a = instalar(paquete, j, "..\\..\\evil.lua");
		CHECK(!a.ok, "se niega: " + a.mensaje);
	}

	std::cout << "\nTotal " << total << " comprobaciones, " << fallos << " fallos\n";
	return fallos ? 1 : 0;
}
