#include "Actualizacion.h"

#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shellapi.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

extern "C" {
#include "../terceros/ed25519/ed25519.h"
}

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace phoenix::actualizacion {

	namespace {
		std::wstring ancho(const std::string& s) {
			if (s.empty()) return {};
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(n, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
			return w;
		}
		std::vector<uint8_t> desdeBase64(const std::string& texto) {
			static const std::string abc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			std::vector<uint8_t> out;
			uint32_t acum = 0; int bits = 0;
			for (char c : texto) {
				if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;
				const size_t v = abc.find(c);
				if (v == std::string::npos) return {};
				acum = (acum << 6) | uint32_t(v); bits += 6;
				if (bits >= 8) { bits -= 8; out.push_back(uint8_t((acum >> bits) & 0xff)); }
			}
			return out;
		}
		std::wstring variable(const wchar_t* nombre) {
			wchar_t b[MAX_PATH * 2] = {};
			const DWORD n = GetEnvironmentVariableW(nombre, b, MAX_PATH * 2);
			return n && n < MAX_PATH * 2 ? std::wstring(b, n) : std::wstring();
		}
		/// Lanza un proceso sin ventana y espera (código de salida; -1 si no se pudo).
		int correr(const std::wstring& linea, DWORD esperaMs) {
			STARTUPINFOW si{}; si.cb = sizeof(si);
			PROCESS_INFORMATION pi{};
			std::wstring l = linea;
			if (!CreateProcessW(nullptr, &l[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) return -1;
			DWORD codigo = 1;
			if (WaitForSingleObject(pi.hProcess, esperaMs) == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, &codigo);
			else { TerminateProcess(pi.hProcess, 1); codigo = 1; }
			CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
			return static_cast<int>(codigo);
		}
		struct Manija { HINTERNET h = nullptr; ~Manija() { if (h) WinHttpCloseHandle(h); } };
	}

	const Clave& claveWeb() {
		static const Clave k = { "k67bd032d50", "emHq6k0YsiAANBET5YCgehooR0O4MvpdpOSAbnlxDAQ=" };
		return k;
	}

	bool versionValida(const std::string& v) {
		int partes = 0, digitos = 0;
		for (char c : v) {
			if (c >= '0' && c <= '9') { if (++digitos > 4) return false; }
			else if (c == '.') { if (digitos == 0) return false; partes++; digitos = 0; }
			else return false;
		}
		return partes == 2 && digitos > 0;
	}

	int compararVersion(const std::string& a, const std::string& b) {
		const bool va = versionValida(a), vb = versionValida(b);
		if (!va || !vb) return va ? 1 : vb ? -1 : 0;
		auto partes = [](const std::string& v) { int x[3] = {}; sscanf_s(v.c_str(), "%d.%d.%d", &x[0], &x[1], &x[2]); return std::vector<int>{ x[0], x[1], x[2] }; };
		const auto x = partes(a), y = partes(b);
		for (int i = 0; i < 3; i++) if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
		return 0;
	}

	bool abrirSobre(const std::string& sobreJson, const Clave& clave, Manifiesto& m, std::string& codigo) {
		const json s = json::parse(sobreJson, nullptr, false);
		if (s.is_discarded() || !s.is_object() || !s.contains("contenido") || !s["contenido"].is_string() || !s.contains("firma") || !s["firma"].is_string()) { codigo = "SOBRE_INVALIDO"; return false; }
		if (s.value("algoritmo", "") != "Ed25519") { codigo = "SOBRE_INVALIDO"; return false; }
		if (s.value("clave_id", "") != clave.id) { codigo = "CLAVE_DESCONOCIDA"; return false; }
		const std::string contenido = s["contenido"].get<std::string>();
		const auto pub = desdeBase64(clave.publicaB64), firma = desdeBase64(s["firma"].get<std::string>());
		if (pub.size() != 32 || firma.size() != 64
			|| ed25519_verify(firma.data(), reinterpret_cast<const unsigned char*>(contenido.data()), contenido.size(), pub.data()) != 1) { codigo = "FIRMA_INVALIDA"; return false; }
		const json c = json::parse(contenido, nullptr, false);
		if (c.is_discarded() || !c.is_object() || c.value("formato", "") != kFormato) { codigo = "FORMATO"; return false; }
		m.version = c.value("version", ""); m.etapa = c.value("etapa", ""); m.sha256 = c.value("sha256", "");
		m.notas = c.value("notas", ""); m.generado = c.value("generado", ""); m.obligatoria = c.value("obligatoria", false);
		m.tamano = c.contains("tamano") && c["tamano"].is_number_unsigned() ? c["tamano"].get<uint64_t>() : 0;
		bool shaOk = m.sha256.size() == 64;
		for (char ch : m.sha256) if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) shaOk = false;
		if (!versionValida(m.version) || !shaOk || m.tamano == 0 || m.tamano > 52428800ull) { codigo = "FORMATO"; return false; }
		return true;
	}

	std::string sha256Archivo(const std::wstring& ruta) {
		std::ifstream f(fs::path(ruta), std::ios::binary);
		if (!f) return {};
		BCRYPT_ALG_HANDLE alg = nullptr; BCRYPT_HASH_HANDLE h = nullptr;
		unsigned char resumen[32] = {};
		bool ok = false;
		if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0) {
			if (BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) == 0) {
				std::vector<char> buf(1 << 16);
				while (f) {
					f.read(buf.data(), buf.size());
					if (f.gcount() > 0) BCryptHashData(h, reinterpret_cast<PUCHAR>(buf.data()), static_cast<ULONG>(f.gcount()), 0);
				}
				ok = BCryptFinishHash(h, resumen, 32, 0) == 0;
				BCryptDestroyHash(h);
			}
			BCryptCloseAlgorithmProvider(alg, 0);
		}
		if (!ok) return {};
		static const char* hex = "0123456789abcdef";
		std::string s;
		for (unsigned char b : resumen) { s += hex[b >> 4]; s += hex[b & 15]; }
		return s;
	}

	bool descargar(const std::string& url, const std::wstring& destino, uint64_t tamano,
		const std::function<void(uint64_t)>& progreso, bool permitirLocal, std::string& codigo, int timeoutMs) {
		codigo.clear();
		const bool https = url.rfind("https://", 0) == 0;
		const bool local = permitirLocal && url.rfind("http://127.0.0.1:", 0) == 0;
		if (!https && !local) { codigo = "URL_INVALIDA"; return false; }
		const std::wstring wurl = ancho(url);
		URL_COMPONENTS partes{}; partes.dwStructSize = sizeof(partes);
		std::vector<wchar_t> host(256), ruta(4096), extra(8192);
		partes.lpszHostName = host.data(); partes.dwHostNameLength = 256;
		partes.lpszUrlPath = ruta.data(); partes.dwUrlPathLength = 4096;
		partes.lpszExtraInfo = extra.data(); partes.dwExtraInfoLength = 8192;
		if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &partes)) { codigo = "URL_INVALIDA"; return false; }
		const std::wstring camino = std::wstring(partes.lpszUrlPath, partes.dwUrlPathLength) + std::wstring(partes.lpszExtraInfo, partes.dwExtraInfoLength);

		Manija sesion, conexion, sol;
		sesion.h = WinHttpOpen(L"PhoenixLink-Actualizador/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!sesion.h) { codigo = "DESCARGA_FALLO"; return false; }
		WinHttpSetTimeouts(sesion.h, timeoutMs, timeoutMs, timeoutMs, timeoutMs);
		conexion.h = WinHttpConnect(sesion.h, std::wstring(partes.lpszHostName, partes.dwHostNameLength).c_str(), partes.nPort, 0);
		if (!conexion.h) { codigo = "DESCARGA_FALLO"; return false; }
		sol.h = WinHttpOpenRequest(conexion.h, L"GET", camino.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, https ? WINHTTP_FLAG_SECURE : 0);
		if (!sol.h || !WinHttpSendRequest(sol.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(sol.h, nullptr)) {
			codigo = "DESCARGA_FALLO"; return false;
		}
		DWORD estado = 0, tam = sizeof(estado);
		WinHttpQueryHeaders(sol.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &estado, &tam, WINHTTP_NO_HEADER_INDEX);
		if (estado != 200) { codigo = estado == 400 || estado == 403 ? "URL_VENCIDA" : "DESCARGA_FALLO"; return false; }

		std::ofstream f(fs::path(destino), std::ios::binary | std::ios::trunc);
		if (!f) { codigo = "SIN_ESPACIO"; return false; }
		uint64_t total = 0;
		bool cortada = false, grande = false;
		std::vector<char> buf(1 << 16);
		while (!cortada && !grande) {
			DWORD disponible = 0;
			if (!WinHttpQueryDataAvailable(sol.h, &disponible)) { cortada = true; break; }
			if (disponible == 0) break;
			while (disponible > 0) {
				DWORD leidos = 0;
				const DWORD pedir = (std::min<DWORD>)(disponible, static_cast<DWORD>(buf.size()));
				if (!WinHttpReadData(sol.h, buf.data(), pedir, &leidos) || leidos == 0) { cortada = true; break; }
				total += leidos; disponible -= leidos;
				if (total > tamano) { grande = true; break; }
				f.write(buf.data(), leidos);
				if (progreso) progreso(total);
			}
		}
		f.close();
		if (!f || grande || cortada || total != tamano) {
			codigo = grande ? "DESCARGA_GRANDE" : !f ? "SIN_ESPACIO" : "DESCARGA_CORTADA";
			std::error_code ec; fs::remove(fs::path(destino), ec);
			return false;
		}
		return true;
	}

	bool extraerZip(const std::wstring& zip, const std::wstring& carpeta, std::string& codigo) {
		std::error_code ec;
		fs::remove_all(fs::path(carpeta), ec);
		fs::create_directories(fs::path(carpeta), ec);
		const std::wstring sistema = variable(L"SystemRoot");
		const std::wstring tar = (sistema.empty() ? std::wstring(L"C:\\Windows") : sistema) + L"\\System32\\tar.exe";
		int r = -1;
		if (fs::exists(fs::path(tar), ec)) r = correr(L"\"" + tar + L"\" -xf \"" + zip + L"\" -C \"" + carpeta + L"\"", 180000);
		if (r != 0) {
			auto comillas = [](const std::wstring& s) { std::wstring o; for (wchar_t c : s) { o += c; if (c == L'\'') o += L'\''; } return L"'" + o + L"'"; };
			fs::remove_all(fs::path(carpeta), ec); fs::create_directories(fs::path(carpeta), ec);
			r = correr(L"powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -LiteralPath " + comillas(zip) + L" -DestinationPath " + comillas(carpeta) + L" -Force\"", 300000);
		}
		if (r != 0) { codigo = "ZIP_ROTO"; return false; }
		const fs::path raiz = fs::weakly_canonical(fs::path(carpeta), ec);
		const std::wstring prefijo = raiz.wstring() + L"\\";
		for (auto it = fs::recursive_directory_iterator(raiz, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
			if (it->is_symlink(ec)) { codigo = "ZIP_ROTO"; return false; }
			const std::wstring p = fs::weakly_canonical(it->path(), ec).wstring();
			if (p.rfind(prefijo, 0) != 0) { codigo = "ZIP_ROTO"; return false; }
		}
		if (ec) { codigo = "ZIP_ROTO"; return false; }
		if (!fs::is_regular_file(raiz / L"PhoenixLink.exe", ec)) { codigo = "ZIP_INCOMPLETO"; return false; }
		return true;
	}

	std::wstring carpetaTrabajo() {
		std::wstring base = variable(L"LOCALAPPDATA");
		if (base.empty()) base = variable(L"TEMP");
		return base + L"\\Phoenix Link\\actualizacion";
	}

	std::wstring carpetaExe() {
		wchar_t ruta[MAX_PATH * 2] = {};
		GetModuleFileNameW(nullptr, ruta, MAX_PATH * 2);
		return fs::path(ruta).parent_path().wstring();
	}

	bool preparar(const std::string& datosJson, const Opciones& o, Preparada& p, std::string& codigo) {
		const json d = json::parse(datosJson, nullptr, false);
		if (d.is_discarded() || !d.is_object() || !d.value("hay", false) || !d.contains("sobre") || !d["sobre"].is_object() || !d.contains("url") || !d["url"].is_string()) {
			codigo = "SIN_DATOS"; return false;
		}
		Manifiesto m;
		if (!abrirSobre(d["sobre"].dump(), o.clave, m, codigo)) return false;
		if (compararVersion(m.version, o.versionActual) <= 0) { codigo = "VERSION_VIEJA"; return false; }
		const std::wstring trabajo = o.trabajo.empty() ? carpetaTrabajo() : o.trabajo;
		std::error_code ec;
		fs::create_directories(fs::path(trabajo), ec);
		const std::wstring zip = trabajo + L"\\PhoenixLink-" + ancho(m.version) + L".zip";
		const uint64_t total = m.tamano;
		auto prog = [&](uint64_t n) { if (o.progreso) o.progreso(n, total); };
		if (!descargar(d["url"].get<std::string>(), zip, m.tamano, prog, o.permitirLocal, codigo)) return false;
		if (sha256Archivo(zip) != m.sha256) { fs::remove(fs::path(zip), ec); codigo = "SHA_DISTINTO"; return false; }
		const std::wstring nueva = trabajo + L"\\nueva-" + ancho(m.version);
		if (!extraerZip(zip, nueva, codigo)) { fs::remove_all(fs::path(nueva), ec); return false; }
		fs::remove(fs::path(zip), ec);
		p.m = m; p.carpetaNueva = nueva; p.trabajo = trabajo;
		codigo.clear();
		return true;
	}

	bool lanzarActualizador(const Preparada& p, const std::string& versionActual, std::string& codigo) {
		std::error_code ec;
		fs::path origen = fs::path(p.carpetaNueva) / L"PhoenixActualizador.exe";
		if (!fs::is_regular_file(origen, ec)) origen = fs::path(carpetaExe()) / L"PhoenixActualizador.exe";
		if (!fs::is_regular_file(origen, ec)) { codigo = "SIN_ACTUALIZADOR"; return false; }
		const fs::path copia = fs::path(p.trabajo) / L"PhoenixActualizador.exe";
		fs::copy_file(origen, copia, fs::copy_options::overwrite_existing, ec);
		if (ec) { codigo = "NO_SE_PUDO_LANZAR"; return false; }
		const std::wstring okArchivo = p.trabajo + L"\\arranco-" + ancho(p.m.version) + L".ok";
		fs::remove(fs::path(okArchivo), ec);
		std::wstring linea = L"\"" + copia.wstring() + L"\" supervisar --pid " + std::to_wstring(GetCurrentProcessId())
			+ L" --destino \"" + carpetaExe() + L"\" --nueva \"" + p.carpetaNueva + L"\" --respaldo \"" + p.trabajo + L"\\anterior\""
			+ L" --ok \"" + okArchivo + L"\" --resultado \"" + p.trabajo + L"\\resultado.json\""
			+ L" --version " + ancho(p.m.version) + L" --anterior " + ancho(versionActual);
		STARTUPINFOW si{}; si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		if (!CreateProcessW(nullptr, &linea[0], nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, p.trabajo.c_str(), &si, &pi)) { codigo = "NO_SE_PUDO_LANZAR"; return false; }
		CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
		return true;
	}

	void avisarArranqueOk() {
		int n = 0;
		LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &n);
		if (!args) return;
		for (int i = 1; i + 1 < n; i++) {
			if (std::wstring(args[i]) == L"--ok") {
				std::ofstream f(fs::path(args[i + 1]), std::ios::trunc);
				f << "ok";
			}
		}
		LocalFree(args);
	}

	bool tomarResultado(Resultado& r, const std::wstring& trabajo) {
		const fs::path ruta = fs::path(trabajo.empty() ? carpetaTrabajo() : trabajo) / L"resultado.json";
		std::ifstream f(ruta, std::ios::binary);
		if (!f) return false;
		const std::string texto((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		f.close();
		std::error_code ec; fs::remove(ruta, ec);
		const json j = json::parse(texto, nullptr, false);
		if (j.is_discarded() || !j.is_object()) return false;
		r.version = j.value("version", ""); r.anterior = j.value("anterior", ""); r.estado = j.value("estado", ""); r.detalle = j.value("detalle", "");
		return !r.estado.empty();
	}

	std::string mensajeDe(const std::string& c) {
		if (c == "FIRMA_INVALIDA" || c == "CLAVE_DESCONOCIDA" || c == "SOBRE_INVALIDO" || c == "FORMATO") return "La versión nueva no trae una firma válida de Phoenix. No se instaló nada.";
		if (c == "SHA_DISTINTO") return "El archivo bajado no es el que publicó Phoenix (está dañado o fue cambiado). No se instaló nada.";
		if (c == "DESCARGA_CORTADA") return "La descarga se cortó. Revisa tu internet y vuelve a pulsar ACTUALIZAR.";
		if (c == "DESCARGA_GRANDE") return "El archivo bajado es más grande de lo esperado. No se instaló nada.";
		if (c == "URL_VENCIDA") return "El enlace de descarga venció. Vuelve a pulsar ACTUALIZAR.";
		if (c == "DESCARGA_FALLO" || c == "URL_INVALIDA") return "No se pudo bajar la versión nueva. Revisa tu internet y vuelve a intentar.";
		if (c == "ZIP_ROTO" || c == "ZIP_INCOMPLETO") return "El paquete de la versión nueva está incompleto. No se instaló nada.";
		if (c == "SIN_ESPACIO") return "No hay espacio en el disco para bajar la versión nueva.";
		if (c == "VERSION_VIEJA") return "Ya tienes esta versión o una más nueva.";
		if (c == "SIN_ACTUALIZADOR" || c == "NO_SE_PUDO_LANZAR") return "No se pudo abrir el actualizador. Instala la versión nueva a mano.";
		if (c == "SIN_DATOS") return "Ya no hay versión nueva para bajar.";
		return "No se pudo actualizar (" + c + ").";
	}

}
