#include "Http.h"

#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace phoenix::http {

	namespace {
		std::wstring aAncho(const std::string& s) {
			if (s.empty()) return std::wstring();
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(n, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
			return w;
		}

		struct Manija {
			HINTERNET h = nullptr;
			~Manija() { if (h) WinHttpCloseHandle(h); }
		};
	}

	Respuesta peticion(const std::string& metodo, const std::string& url,
		const std::string& cuerpo, const std::vector<std::string>& cabeceras, int timeoutMs) {

		Respuesta r;
		try {
			const std::wstring wurl = aAncho(url);
			URL_COMPONENTS partes{};
			partes.dwStructSize = sizeof(partes);
			wchar_t host[256] = {}, ruta[2048] = {};
			partes.lpszHostName = host; partes.dwHostNameLength = 256;
			partes.lpszUrlPath = ruta; partes.dwUrlPathLength = 2048;
			if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &partes)) { r.error = "URL inválida"; return r; }

			Manija sesion, conexion, solicitud;
			sesion.h = WinHttpOpen(L"PhoenixSoda/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
				WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (!sesion.h) { r.error = "WinHttpOpen"; return r; }
			WinHttpSetTimeouts(sesion.h, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

			conexion.h = WinHttpConnect(sesion.h, host, partes.nPort, 0);
			if (!conexion.h) { r.error = "Sin conexión"; return r; }

			solicitud.h = WinHttpOpenRequest(conexion.h, aAncho(metodo).c_str(), ruta, nullptr,
				WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
				partes.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
			if (!solicitud.h) { r.error = "WinHttpOpenRequest"; return r; }

			std::wstring todas = L"Content-Type: application/json; charset=utf-8\r\n";
			for (const std::string& c : cabeceras) todas += aAncho(c) + L"\r\n";

			if (!WinHttpSendRequest(solicitud.h, todas.c_str(), static_cast<DWORD>(-1L),
				cuerpo.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(cuerpo.data()),
				static_cast<DWORD>(cuerpo.size()), static_cast<DWORD>(cuerpo.size()), 0)
				|| !WinHttpReceiveResponse(solicitud.h, nullptr)) {
				r.error = "Error de red (" + std::to_string(GetLastError()) + ")";
				return r;
			}

			DWORD codigo = 0, tam = sizeof(codigo);
			WinHttpQueryHeaders(solicitud.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX, &codigo, &tam, WINHTTP_NO_HEADER_INDEX);

			DWORD disponible = 0;
			while (WinHttpQueryDataAvailable(solicitud.h, &disponible) && disponible > 0) {
				std::string trozo(disponible, '\0');
				DWORD leidos = 0;
				if (!WinHttpReadData(solicitud.h, &trozo[0], disponible, &leidos)) break;
				r.cuerpo.append(trozo.data(), leidos);
				if (r.cuerpo.size() > 1024 * 1024) break; // nunca más de 1 MB
			}
			r.estado = static_cast<int>(codigo);
		}
		catch (...) {
			r.estado = 0;
			r.error = "Excepción HTTP";
		}
		return r;
	}

}
