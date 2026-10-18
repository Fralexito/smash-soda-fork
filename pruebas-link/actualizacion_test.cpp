// Pruebas de la auto-actualización de Phoenix Link (phoenix/link/Actualizacion.cpp + PhoenixActualizador/Instalador.cpp).
// Sin internet: levanta un servidor web FALSO en 127.0.0.1, firma con una clave de PRUEBA y arma zips de prueba con tar.exe.
// Casos: zip bueno, firma mala, sha malo, descarga cortada, archivo más grande, zip incompleto, versión vieja, URL no https,
// instalar con respaldo (sin tocar Trybuchet ni portable.txt) y versión que no arranca → vuelve sola la anterior.
// Uso: pruebas-link\correr_actualizacion.bat   (compila con MSVC y corre en %TEMP%\phx_actualizacion)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>

#include "phoenix/link/Actualizacion.h"
#include "../PhoenixActualizador/Instalador.h"
extern "C" {
#include "phoenix/terceros/ed25519/ed25519.h"
}
#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace phoenix;

static int fallos = 0, total = 0;
#define CHECK(c, msg) do { total++; if (c) std::printf("  OK   %s\n", msg); else { fallos++; std::printf("  FALLA %s  (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

// ---------- servidor falso ----------
struct Servidor {
	SOCKET s = INVALID_SOCKET; int puerto = 0; std::thread hilo; std::atomic<bool> vivo{ true };
	std::map<std::string, std::string> archivos;   // ruta → contenido
	std::map<std::string, bool> cortar;            // ruta → manda solo la mitad y cierra
	void iniciar() {
		WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = 0;
		bind(s, (sockaddr*)&a, sizeof(a)); listen(s, 8);
		int l = sizeof(a); getsockname(s, (sockaddr*)&a, &l); puerto = ntohs(a.sin_port);
		hilo = std::thread([this] {
			while (vivo) {
				SOCKET c = accept(s, nullptr, nullptr);
				if (c == INVALID_SOCKET) break;
				char buf[4096] = {}; recv(c, buf, sizeof(buf) - 1, 0);
				std::string pet(buf), ruta = pet.substr(4, pet.find(' ', 4) - 4);
				ruta = ruta.substr(0, ruta.find('?'));
				auto it = archivos.find(ruta);
				if (it == archivos.end()) { const char* r = "HTTP/1.1 404 No\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"; send(c, r, (int)strlen(r), 0); }
				else {
					const std::string& d = it->second;
					std::string cab = "HTTP/1.1 200 OK\r\nContent-Type: application/zip\r\nContent-Length: " + std::to_string(d.size()) + "\r\nConnection: close\r\n\r\n";
					send(c, cab.data(), (int)cab.size(), 0);
					const size_t n = cortar[ruta] ? d.size() / 2 : d.size();
					for (size_t i = 0; i < n; i += 8192) send(c, d.data() + i, (int)(std::min)((size_t)8192, n - i), 0);
				}
				shutdown(c, SD_BOTH); closesocket(c);
			}
		});
	}
	void parar() { vivo = false; closesocket(s); if (hilo.joinable()) hilo.join(); }
	std::string url(const std::string& ruta) const { return "http://127.0.0.1:" + std::to_string(puerto) + ruta + "?token=prueba"; }
};

// ---------- ayudas ----------
static std::string leer(const fs::path& p) { std::ifstream f(p, std::ios::binary); return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); }
static void escribir(const fs::path& p, const std::string& t) { fs::create_directories(p.parent_path()); std::ofstream f(p, std::ios::binary | std::ios::trunc); f << t; }
static std::string b64(const unsigned char* d, size_t n) {
	static const char* abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string o; size_t i = 0;
	for (; i + 2 < n; i += 3) { uint32_t v = (d[i] << 16) | (d[i + 1] << 8) | d[i + 2]; o += abc[(v >> 18) & 63]; o += abc[(v >> 12) & 63]; o += abc[(v >> 6) & 63]; o += abc[v & 63]; }
	if (i + 1 == n) { uint32_t v = d[i] << 16; o += abc[(v >> 18) & 63]; o += abc[(v >> 12) & 63]; o += "=="; }
	else if (i + 2 == n) { uint32_t v = (d[i] << 16) | (d[i + 1] << 8); o += abc[(v >> 18) & 63]; o += abc[(v >> 12) & 63]; o += abc[(v >> 6) & 63]; o += '='; }
	return o;
}
static int correr(const std::wstring& linea) {
	STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{}; std::wstring l = linea;
	if (!CreateProcessW(nullptr, &l[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) return -1;
	WaitForSingleObject(pi.hProcess, 60000); DWORD c = 1; GetExitCodeProcess(pi.hProcess, &c); CloseHandle(pi.hThread); CloseHandle(pi.hProcess); return (int)c;
}
/// zip con tar.exe (bsdtar): -a elige zip por la extensión
static std::string hacerZip(const fs::path& carpeta, const fs::path& zip) {
	fs::remove(zip);
	correr(L"tar.exe -a -cf \"" + zip.wstring() + L"\" -C \"" + carpeta.wstring() + L"\" *");
	return leer(zip);
}

struct Firmante {
	unsigned char pub[32], priv[64];
	actualizacion::Clave clave;
	Firmante() { unsigned char semilla[32]; for (int i = 0; i < 32; i++) semilla[i] = (unsigned char)(i * 7 + 1); ed25519_create_keypair(pub, priv, semilla); clave = { "kprueba", b64(pub, 32) }; }
	json sobre(const json& contenido, const std::string& claveId = "kprueba") const {
		const std::string t = contenido.dump(); unsigned char f[64];
		ed25519_sign(f, (const unsigned char*)t.data(), t.size(), pub, priv);
		return { {"contenido", t}, {"firma", b64(f, 64)}, {"clave_id", claveId}, {"algoritmo", "Ed25519"} };
	}
};
static json manifiesto(const std::string& v, const std::string& sha, uint64_t tam) {
	return { {"formato", actualizacion::kFormato}, {"version", v}, {"etapa", "beta 2"}, {"sha256", sha}, {"tamano", tam}, {"notas", "Prueba"}, {"obligatoria", false}, {"generado", "2026-10-17T00:00:00Z"} };
}

int wmain(int argc, wchar_t** argv) {
	if (argc < 3) { std::printf("uso: actualizacion_test <carpeta-temporal> <falso_link.exe>\n"); return 2; }
	const fs::path T = argv[1], falso = argv[2];
	fs::remove_all(T); fs::create_directories(T);
	SetEnvironmentVariableW(L"LOCALAPPDATA", (T / L"localappdata").c_str());   // nada fuera de la carpeta de prueba

	std::printf("1) Versiones\n");
	CHECK(actualizacion::compararVersion("0.3.9", "0.3.16") < 0, "0.3.9 < 0.3.16");
	CHECK(actualizacion::compararVersion("1.0.0", "0.99.99") > 0, "1.0.0 > 0.99.99");
	CHECK(!actualizacion::versionValida("0.3") && !actualizacion::versionValida("0.3.16-beta") && actualizacion::versionValida("0.3.16"), "formato de versión");

	// Paquetes de prueba: v2 bueno (con PhoenixLink.exe falso) y uno sin PhoenixLink.exe
	const fs::path pqBueno = T / L"paquete-bueno", pqIncompleto = T / L"paquete-incompleto";
	fs::create_directories(pqBueno / L"ui"); fs::copy_file(falso, pqBueno / L"PhoenixLink.exe");
	escribir(pqBueno / L"version.txt", "v2"); escribir(pqBueno / L"ui" / L"index.html", "<p>nueva</p>"); escribir(pqBueno / L"solo-nueva.dll", "nueva");
	escribir(pqIncompleto / L"version.txt", "v2");
	const std::string zipBueno = hacerZip(pqBueno, T / L"bueno.zip"), zipIncompleto = hacerZip(pqIncompleto, T / L"incompleto.zip");
	CHECK(zipBueno.size() > 100 && zipIncompleto.size() > 20, "tar.exe arma los zips de prueba");
	const std::string shaBueno = actualizacion::sha256Archivo((T / L"bueno.zip").wstring());
	const std::string shaIncompleto = actualizacion::sha256Archivo((T / L"incompleto.zip").wstring());
	CHECK(shaBueno.size() == 64, "sha256 del zip");

	Servidor srv;
	srv.archivos["/bueno.zip"] = zipBueno; srv.archivos["/cortado.zip"] = zipBueno; srv.cortar["/cortado.zip"] = true;
	srv.archivos["/incompleto.zip"] = zipIncompleto;
	srv.iniciar();
	Firmante k;

	std::printf("2) Sobre firmado\n");
	actualizacion::Manifiesto m; std::string cod;
	CHECK(actualizacion::abrirSobre(k.sobre(manifiesto("0.3.17", shaBueno, zipBueno.size())).dump(), k.clave, m, cod) && m.version == "0.3.17", "firma buena se acepta");
	json malo = k.sobre(manifiesto("0.3.17", shaBueno, zipBueno.size()));
	malo["contenido"] = std::string(malo["contenido"]).replace(malo["contenido"].get<std::string>().find("0.3.17"), 6, "9.9.99");
	CHECK(!actualizacion::abrirSobre(malo.dump(), k.clave, m, cod) && cod == "FIRMA_INVALIDA", "contenido cambiado → FIRMA_INVALIDA");
	CHECK(!actualizacion::abrirSobre(k.sobre(manifiesto("0.3.17", shaBueno, 1), "otra").dump(), k.clave, m, cod) && cod == "CLAVE_DESCONOCIDA", "otra clave → CLAVE_DESCONOCIDA");
	json sinFormato = manifiesto("0.3.17", shaBueno, 1); sinFormato["formato"] = "otra-cosa";
	CHECK(!actualizacion::abrirSobre(k.sobre(sinFormato).dump(), k.clave, m, cod) && cod == "FORMATO", "formato desconocido → FORMATO");
	CHECK(!actualizacion::abrirSobre(k.sobre(manifiesto("0.3.17", shaBueno, 1)).dump(), actualizacion::claveWeb(), m, cod), "la clave REAL de la web no acepta lo firmado con la de prueba");

	auto datos = [&](const json& sobre, const std::string& ruta) { return json{ {"hay", true}, {"sobre", sobre}, {"url", srv.url(ruta)} }.dump(); };
	auto opciones = [&](const std::wstring& sub) {
		actualizacion::Opciones o; o.trabajo = (T / sub).wstring(); o.clave = k.clave; o.versionActual = "0.3.16"; o.permitirLocal = true; return o;
	};
	actualizacion::Preparada p;

	std::printf("3) Preparar (bajar + comprobar + descomprimir)\n");
	uint64_t ultimoProgreso = 0;
	auto oBueno = opciones(L"trabajo-bueno"); oBueno.progreso = [&](uint64_t n, uint64_t) { ultimoProgreso = n; };
	CHECK(actualizacion::preparar(datos(k.sobre(manifiesto("0.3.17", shaBueno, zipBueno.size())), "/bueno.zip"), oBueno, p, cod), ("zip bueno → listo " + cod).c_str());
	CHECK(fs::exists(fs::path(p.carpetaNueva) / L"PhoenixLink.exe") && leer(fs::path(p.carpetaNueva) / L"ui" / L"index.html") == "<p>nueva</p>", "descomprimido con PhoenixLink.exe y subcarpetas");
	CHECK(ultimoProgreso == zipBueno.size(), "el progreso llega al 100 %");
	CHECK(!fs::exists(T / L"trabajo-bueno" / L"PhoenixLink-0.3.17.zip"), "el zip se borra después de descomprimir");

	std::string shaMalo = shaBueno; shaMalo[0] = shaMalo[0] == 'a' ? 'b' : 'a';
	CHECK(!actualizacion::preparar(datos(k.sobre(manifiesto("0.3.17", shaMalo, zipBueno.size())), "/bueno.zip"), opciones(L"t-sha"), p, cod) && cod == "SHA_DISTINTO", "sha malo → SHA_DISTINTO");
	CHECK(!fs::exists(T / L"t-sha" / L"nueva-0.3.17"), "con sha malo no se descomprime nada");
	CHECK(!actualizacion::preparar(datos(malo, "/bueno.zip"), opciones(L"t-firma"), p, cod) && cod == "FIRMA_INVALIDA", "firma mala → no baja nada");
	CHECK(!fs::exists(T / L"t-firma" / L"PhoenixLink-9.9.99.zip"), "con firma mala no hay descarga");
	CHECK(!actualizacion::preparar(datos(k.sobre(manifiesto("0.3.17", shaBueno, zipBueno.size())), "/cortado.zip"), opciones(L"t-cortado"), p, cod) && cod == "DESCARGA_CORTADA", ("descarga cortada → DESCARGA_CORTADA (" + cod + ")").c_str());
	CHECK(!fs::exists(T / L"t-cortado" / L"PhoenixLink-0.3.17.zip"), "la descarga cortada se borra");
	CHECK(!actualizacion::preparar(datos(k.sobre(manifiesto("0.3.17", shaBueno, zipBueno.size() - 10)), "/bueno.zip"), opciones(L"t-grande"), p, cod) && cod == "DESCARGA_GRANDE", "llega más de lo firmado → DESCARGA_GRANDE");
	CHECK(!actualizacion::preparar(datos(k.sobre(manifiesto("0.3.17", shaIncompleto, zipIncompleto.size())), "/incompleto.zip"), opciones(L"t-incompleto"), p, cod) && cod == "ZIP_INCOMPLETO", "zip sin PhoenixLink.exe → ZIP_INCOMPLETO");
	CHECK(!actualizacion::preparar(datos(k.sobre(manifiesto("0.3.16", shaBueno, zipBueno.size())), "/bueno.zip"), opciones(L"t-vieja"), p, cod) && cod == "VERSION_VIEJA", "misma versión → VERSION_VIEJA");
	auto sinLocal = opciones(L"t-http"); sinLocal.permitirLocal = false;
	CHECK(!actualizacion::preparar(datos(k.sobre(manifiesto("0.3.17", shaBueno, zipBueno.size())), "/bueno.zip"), sinLocal, p, cod) && cod == "URL_INVALIDA", "sin https → URL_INVALIDA");
	CHECK(!actualizacion::preparar(json{ {"hay", false} }.dump(), opciones(L"t-nada"), p, cod) && cod == "SIN_DATOS", "hay:false → SIN_DATOS");
	srv.parar();

	// Instalación: carpeta de Link «vieja» (v1) con datos de la persona
	auto armarViejo = [&](const fs::path& D) {
		fs::remove_all(D); fs::create_directories(D / L"ui");
		fs::copy_file(falso, D / L"PhoenixLink.exe");
		escribir(D / L"version.txt", "v1"); escribir(D / L"ui" / L"index.html", "<p>vieja</p>"); escribir(D / L"solo-vieja.dll", "vieja");
		escribir(D / L"portable.txt", "1"); escribir(D / L"Trybuchet" / L"Smash Soda" / L"config.json", "{\"mio\":1}");
	};
	std::printf("4) aplicar / restaurar\n");
	{
		const fs::path D = T / L"link-a", R = T / L"resp-a";
		armarViejo(D);
		fs::path N = T / L"trabajo-bueno" / L"nueva-0.3.17";
		escribir(N / L"Trybuchet" / L"Smash Soda" / L"config.json", "PISADO");   // un zip mal hecho no debe pisar los datos
		std::string e;
		CHECK(instalador::aplicar(D.wstring(), N.wstring(), R.wstring(), e), ("aplicar " + e).c_str());
		CHECK(leer(D / L"version.txt") == "v2" && leer(D / L"ui" / L"index.html") == "<p>nueva</p>" && fs::exists(D / L"solo-nueva.dll"), "lo nuevo quedó copiado");
		CHECK(leer(D / L"Trybuchet" / L"Smash Soda" / L"config.json") == "{\"mio\":1}" && leer(D / L"portable.txt") == "1", "Trybuchet y portable.txt intactos");
		CHECK(fs::exists(D / L"solo-vieja.dll"), "lo viejo que no viene en la nueva se queda");
		CHECK(instalador::restaurar(D.wstring(), R.wstring(), e), ("restaurar " + e).c_str());
		CHECK(leer(D / L"version.txt") == "v1" && leer(D / L"ui" / L"index.html") == "<p>vieja</p>" && !fs::exists(D / L"solo-nueva.dll"), "restaurar deja todo como antes");
		fs::remove_all(N / L"Trybuchet");
	}

	std::printf("5) Supervisar: arranca bien\n");
	{
		const fs::path D = T / L"link-b", N = T / L"nueva-b", W = T / L"trabajo-b";
		armarViejo(D); fs::copy(pqBueno, N, fs::copy_options::recursive); fs::create_directories(W);
		instalador::Supervision s;
		s.destino = D.wstring(); s.nueva = N.wstring(); s.respaldo = (W / L"anterior").wstring(); s.okArchivo = (W / L"arranco.ok").wstring();
		s.resultado = (W / L"resultado.json").wstring(); s.version = "0.3.17"; s.anterior = "0.3.16"; s.esperaArranqueMs = 15000;
		const int r = instalador::supervisar(s);
		CHECK(r == 0, "resultado 0 (instalada)");
		CHECK(leer(D / L"version.txt") == "v2", "quedó la versión nueva");
		CHECK(leer(D / L"Trybuchet" / L"Smash Soda" / L"config.json") == "{\"mio\":1}", "ajustes intactos");
		actualizacion::Resultado res;
		CHECK(actualizacion::tomarResultado(res, W.wstring()) && res.estado == "instalada" && res.version == "0.3.17" && res.anterior == "0.3.16", "resultado.json = instalada");
		CHECK(!fs::exists(W / L"resultado.json"), "tomarResultado lo borra");
		CHECK(!fs::exists(N), "la carpeta nueva se limpia");
	}

	std::printf("6) Supervisar: la nueva NO arranca → vuelve la anterior\n");
	{
		const fs::path D = T / L"link-c", N = T / L"nueva-c", W = T / L"trabajo-c";
		armarViejo(D); fs::copy(pqBueno, N, fs::copy_options::recursive); escribir(N / L"romper.txt", "1"); fs::create_directories(W);
		instalador::Supervision s;
		s.destino = D.wstring(); s.nueva = N.wstring(); s.respaldo = (W / L"anterior").wstring(); s.okArchivo = (W / L"arranco.ok").wstring();
		s.resultado = (W / L"resultado.json").wstring(); s.version = "0.3.17"; s.anterior = "0.3.16"; s.esperaArranqueMs = 15000;
		const int r = instalador::supervisar(s);
		CHECK(r == 1, "resultado 1 (restaurada)");
		CHECK(leer(D / L"version.txt") == "v1" && !fs::exists(D / L"romper.txt") && !fs::exists(D / L"solo-nueva.dll"), "volvió exactamente la anterior");
		Sleep(2500);
		CHECK(leer(D / L"ultima-orden.txt").find("--actualizacion-fallida") != std::string::npos, "se volvió a abrir el Link anterior");
		actualizacion::Resultado res;
		CHECK(actualizacion::tomarResultado(res, W.wstring()) && res.estado == "restaurada" && res.detalle.find("código 7") != std::string::npos, ("resultado.json = restaurada: " + res.detalle).c_str());
	}

	std::printf("7) Supervisar: Link viejo no se cierra → no se toca nada\n");
	{
		const fs::path D = T / L"link-d", N = T / L"nueva-d", W = T / L"trabajo-d";
		armarViejo(D); fs::copy(pqBueno, N, fs::copy_options::recursive); fs::create_directories(W);
		instalador::Supervision s;
		s.pid = GetCurrentProcessId(); s.esperaCierreMs = 500;   // «Link» = esta prueba, que no se cierra
		s.destino = D.wstring(); s.nueva = N.wstring(); s.respaldo = (W / L"anterior").wstring(); s.okArchivo = (W / L"arranco.ok").wstring();
		s.resultado = (W / L"resultado.json").wstring(); s.version = "0.3.17"; s.anterior = "0.3.16";
		CHECK(instalador::supervisar(s) == 2 && leer(D / L"version.txt") == "v1", "sigue la anterior, sin cambios");
	}

	std::printf("\n%d de %d bien\n", total - fallos, total);
	return fallos ? 1 : 0;
}
