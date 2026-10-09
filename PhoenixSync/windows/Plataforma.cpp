#include "Plataforma.h"

#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

namespace mercado::windows {

	namespace {
		std::wstring ancho(const std::string& s) {
			if (s.empty()) return {};
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
			std::wstring w(n, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
			return w;
		}
		std::string estrecho(const std::wstring& w) {
			if (w.empty()) return {};
			const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
			std::string s(n, '\0');
			WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
			return s;
		}
		struct Manija { HINTERNET h = nullptr; ~Manija() { if (h) WinHttpCloseHandle(h); } };

		std::string appData() {
			PWSTR ruta = nullptr;
			std::string s;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &ruta))) s = estrecho(ruta);
			if (ruta) CoTaskMemFree(ruta);
			return s;
		}

		/// Abre la petición. Devuelve false con `error` si falla antes de recibir respuesta.
		bool abrir(const std::string& metodo, const std::string& url, const std::string& cuerpo,
			const std::vector<std::string>& cabeceras, int timeoutMs,
			Manija& sesion, Manija& conexion, Manija& solicitud, DWORD& estado, std::string& error) {
			const std::wstring wurl = ancho(url);
			URL_COMPONENTS p{}; p.dwStructSize = sizeof(p);
			wchar_t host[256] = {}, ruta[4096] = {}, extra[4096] = {};
			p.lpszHostName = host; p.dwHostNameLength = 256;
			p.lpszUrlPath = ruta; p.dwUrlPathLength = 4096;
			p.lpszExtraInfo = extra; p.dwExtraInfoLength = 4096;   // ← el «?token=…» va aquí
			if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &p)) { error = "URL inválida"; return false; }
			const std::wstring rutaCompleta = std::wstring(ruta) + extra;

			sesion.h = WinHttpOpen(L"PhoenixSync/0.1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
				WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (!sesion.h) { error = "WinHttpOpen"; return false; }
			WinHttpSetTimeouts(sesion.h, timeoutMs, timeoutMs, timeoutMs, timeoutMs);
			conexion.h = WinHttpConnect(sesion.h, host, p.nPort, 0);
			if (!conexion.h) { error = "Sin conexión"; return false; }
			solicitud.h = WinHttpOpenRequest(conexion.h, ancho(metodo).c_str(), rutaCompleta.c_str(), nullptr,
				WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, p.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
			if (!solicitud.h) { error = "WinHttpOpenRequest"; return false; }

			std::wstring todas;
			if (!cuerpo.empty()) todas = L"Content-Type: application/json; charset=utf-8\r\n";
			for (const auto& c : cabeceras) todas += ancho(c) + L"\r\n";
			if (!WinHttpSendRequest(solicitud.h, todas.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : todas.c_str(), (DWORD)-1L,
				cuerpo.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)cuerpo.data(), (DWORD)cuerpo.size(), (DWORD)cuerpo.size(), 0)
				|| !WinHttpReceiveResponse(solicitud.h, nullptr)) {
				error = "Error de red (" + std::to_string(GetLastError()) + ")";
				return false;
			}
			DWORD tam = sizeof(estado);
			WinHttpQueryHeaders(solicitud.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX, &estado, &tam, WINHTTP_NO_HEADER_INDEX);
			return true;
		}

		std::string descifrar(const std::string& ruta) {
			std::ifstream f(aRuta(ruta), std::ios::binary);
			if (!f) return {};
			std::string datos((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			DATA_BLOB in{ (DWORD)datos.size(), (BYTE*)datos.data() }, out{};
			if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) return {};
			std::string t((char*)out.pbData, out.cbData);
			LocalFree(out.pbData);
			return t;
		}
	}

	RespuestaHttp HttpWinHttp::peticion(const std::string& metodo, const std::string& url,
		const std::string& cuerpo, const std::vector<std::string>& cabeceras) {
		RespuestaHttp r;
		try {
			Manija s, c, q; DWORD estado = 0;
			if (!abrir(metodo, url, cuerpo, cabeceras, _timeoutMs, s, c, q, estado, r.errorRed)) return r;
			DWORD disp = 0;
			while (WinHttpQueryDataAvailable(q.h, &disp) && disp > 0) {
				std::string trozo(disp, '\0'); DWORD leidos = 0;
				if (!WinHttpReadData(q.h, trozo.data(), disp, &leidos)) break;
				r.cuerpo.append(trozo.data(), leidos);
				if (r.cuerpo.size() > 4 * 1024 * 1024) break; // respuestas de la API: tope 4 MB
			}
			r.estado = (int)estado;
		}
		catch (...) { r.estado = 0; r.errorRed = "Excepción HTTP"; }
		return r;
	}

	bool HttpWinHttp::descargar(const std::string& url, const std::string& rutaDestino, long long maxBytes, std::string& error) {
		try {
			Manija s, c, q; DWORD estado = 0;
			if (!abrir("GET", url, "", {}, (std::max)(_timeoutMs, 30000), s, c, q, estado, error)) return false;
			if (estado != 200) { error = "HTTP " + std::to_string(estado); return false; }
			std::ofstream f(aRuta(rutaDestino), std::ios::binary | std::ios::trunc);
			if (!f) { error = "No se pudo crear " + rutaDestino; return false; }
			long long total = 0; DWORD disp = 0;
			std::string trozo;
			while (WinHttpQueryDataAvailable(q.h, &disp) && disp > 0) {
				trozo.resize(disp); DWORD leidos = 0;
				if (!WinHttpReadData(q.h, trozo.data(), disp, &leidos)) { error = "Lectura cortada"; return false; }
				total += leidos;
				if (total > maxBytes) { error = "El archivo es más grande de lo anunciado"; return false; }
				f.write(trozo.data(), leidos);
			}
			return f.good();
		}
		catch (...) { error = "Excepción en descarga"; return false; }
	}

	std::string TokenCompartido::token() {
		// Misma carpeta que PathHelper::GetConfigPath() de Phoenix Soda (modo no portable).
		return descifrar(appData() + "\\Trybuchet\\Smash Soda\\phoenix-token.dat");
	}

	std::string carpetaDatos() {
		const std::string c = appData() + "\\Phoenix Mercado\\";
		std::error_code ec; fs::create_directories(aRuta(c), ec);
		return c;
	}

	std::string TokenManager::token() { return descifrar(carpetaDatos() + "mercado-token.dat"); }

	bool TokenManager::guardar(const std::string& token) {
		DATA_BLOB in{ (DWORD)token.size(), (BYTE*)token.data() }, out{};
		if (!CryptProtectData(&in, L"PhoenixMercado", nullptr, nullptr, nullptr, 0, &out)) return false;
		std::ofstream f(aRuta(carpetaDatos() + "mercado-token.dat"), std::ios::binary | std::ios::trunc);
		f.write((char*)out.pbData, out.cbData);
		LocalFree(out.pbData);
		return f.good();
	}

	void TokenManager::borrar() {
		std::error_code ec; fs::remove(aRuta(carpetaDatos() + "mercado-token.dat"), ec);
	}

	std::string nombrePc() {
		wchar_t n[64] = {}; DWORD t = 64;
		GetComputerNameW(n, &t);
		const std::string s = estrecho(n);
		return s.empty() ? "Mi PC" : s.substr(0, 40);
	}

}
