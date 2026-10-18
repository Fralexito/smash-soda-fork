#include "Instalador.h"

#include <shellapi.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace phoenix::instalador {

	namespace {
		std::string utf8(const std::wstring& w) {
			if (w.empty()) return {};
			const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
			std::string s(n, '\0');
			WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
			return s;
		}
		std::wstring ancho(const std::string& s) {
			if (s.empty()) return {};
			const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(n, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
			return w;
		}
		std::wstring minus(std::wstring s) { for (auto& c : s) c = static_cast<wchar_t>(std::towlower(c)); return s; }

		/// Rutas que el actualizador nunca escribe (datos de la persona y su propia carpeta).
		bool protegida(const fs::path& rel) {
			const std::wstring primero = minus(rel.begin()->wstring());
			const std::wstring todo = minus(rel.wstring());
			return primero == L"trybuchet" || primero == L"_actualizacion" || todo == L"portable.txt" || todo == L"imgui.ini";
		}
		bool copiar(const fs::path& de, const fs::path& a) {
			std::error_code ec;
			fs::create_directories(a.parent_path(), ec);
			for (int i = 0; i < 20; i++) {   // un antivirus o el Link que se cierra pueden tener el archivo un momento
				ec.clear();
				fs::copy_file(de, a, fs::copy_options::overwrite_existing, ec);
				if (!ec) return true;
				Sleep(500);
			}
			return false;
		}
		void anotar(const std::wstring& resultado, const std::string& linea) {
			if (resultado.empty()) return;
			std::ofstream f(fs::path(resultado).parent_path() / L"actualizador.log", std::ios::app);
			SYSTEMTIME t{}; GetLocalTime(&t);
			char h[32]; snprintf(h, sizeof(h), "%04d-%02d-%02d %02d:%02d:%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
			f << h << linea << "\n";
		}
		std::string jsonTexto(const std::string& s) {
			std::string o = "\"";
			for (unsigned char c : s) {
				if (c == '"' || c == '\\') { o += '\\'; o += static_cast<char>(c); }
				else if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
				else o += static_cast<char>(c);
			}
			return o + "\"";
		}
		void escribirResultado(const Supervision& s, const std::string& estado, const std::string& detalle) {
			anotar(s.resultado, s.version + ": " + estado + (detalle.empty() ? "" : " - " + detalle));
			if (s.resultado.empty()) return;
			std::ofstream f(fs::path(s.resultado), std::ios::trunc);
			f << "{\"version\":" << jsonTexto(s.version) << ",\"anterior\":" << jsonTexto(s.anterior)
				<< ",\"estado\":" << jsonTexto(estado) << ",\"detalle\":" << jsonTexto(detalle) << "}";
		}
		/// aplicar / restaurar: en este proceso si se puede escribir; si no, este mismo exe con permiso de administrador.
		bool paso(const Supervision& s, bool esAplicar, std::string& error) {
			if (sePuedeEscribir(s.destino)) return esAplicar ? aplicar(s.destino, s.nueva, s.respaldo, error) : restaurar(s.destino, s.respaldo, error);
			std::wstring params = esAplicar
				? L"aplicar --destino \"" + s.destino + L"\" --nueva \"" + s.nueva + L"\" --respaldo \"" + s.respaldo + L"\""
				: L"restaurar --destino \"" + s.destino + L"\" --respaldo \"" + s.respaldo + L"\"";
			SHELLEXECUTEINFOW ei{}; ei.cbSize = sizeof(ei);
			ei.fMask = SEE_MASK_NOCLOSEPROCESS; ei.lpVerb = L"runas"; ei.lpFile = s.yoMismo.c_str(); ei.lpParameters = params.c_str(); ei.nShow = SW_HIDE;
			if (!ShellExecuteExW(&ei) || !ei.hProcess) { error = "Windows no dio permiso para copiar los archivos"; return false; }
			WaitForSingleObject(ei.hProcess, 10 * 60 * 1000);
			DWORD codigo = 1; GetExitCodeProcess(ei.hProcess, &codigo); CloseHandle(ei.hProcess);
			if (codigo != 0) error = "La copia con permiso de administrador falló";
			return codigo == 0;
		}
		HANDLE abrirLink(const Supervision& s, const std::wstring& args) {
			std::wstring linea = L"\"" + (fs::path(s.destino) / s.exe).wstring() + L"\" " + args;
			STARTUPINFOW si{}; si.cb = sizeof(si);
			PROCESS_INFORMATION pi{};
			if (!CreateProcessW(nullptr, &linea[0], nullptr, nullptr, FALSE, 0, nullptr, s.destino.c_str(), &si, &pi)) return nullptr;
			CloseHandle(pi.hThread);
			return pi.hProcess;
		}
	}

	bool sePuedeEscribir(const std::wstring& carpeta) {
		const fs::path prueba = fs::path(carpeta) / L".phx-prueba-escritura";
		HANDLE h = CreateFileW(prueba.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
		if (h == INVALID_HANDLE_VALUE) return false;
		CloseHandle(h);
		return true;
	}

	bool aplicar(const std::wstring& destino, const std::wstring& nueva, const std::wstring& respaldo, std::string& error) {
		std::error_code ec;
		const fs::path D(destino), N(nueva), R(respaldo);
		if (!fs::is_regular_file(N / L"PhoenixLink.exe", ec)) { error = "La carpeta nueva no tiene PhoenixLink.exe"; return false; }
		fs::remove_all(R, ec);
		fs::create_directories(R / L"archivos", ec);
		// 1) Lista completa y respaldo ANTES de pisar nada
		std::vector<fs::path> archivos;
		for (auto it = fs::recursive_directory_iterator(N, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
			if (it->is_regular_file(ec)) { const fs::path rel = fs::relative(it->path(), N, ec); if (!protegida(rel)) archivos.push_back(rel); }
		if (ec) { error = "No se pudo leer la carpeta nueva"; return false; }
		std::ofstream nuevos(R / L"nuevos.txt", std::ios::trunc);
		for (const fs::path& rel : archivos) {
			if (fs::exists(D / rel, ec)) {
				if (!copiar(D / rel, R / L"archivos" / rel)) { error = "No se pudo guardar " + utf8(rel.wstring()); return false; }
			}
			else nuevos << utf8(rel.wstring()) << "\n";
		}
		nuevos.close();
		if (!nuevos) { error = "No se pudo escribir el respaldo"; return false; }
		{ std::ofstream listo(R / L"completo.txt"); listo << "ok"; }
		// 2) Copiar lo nuevo; si algo falla, se deshace todo
		for (const fs::path& rel : archivos) {
			if (!copiar(N / rel, D / rel)) {
				error = "No se pudo copiar " + utf8(rel.wstring()) + " (¿Link sigue abierto?)";
				std::string e2; restaurar(destino, respaldo, e2);
				return false;
			}
		}
		return true;
	}

	bool restaurar(const std::wstring& destino, const std::wstring& respaldo, std::string& error) {
		std::error_code ec;
		const fs::path D(destino), R(respaldo);
		if (!fs::exists(R / L"completo.txt", ec)) { error = "No hay respaldo completo"; return false; }
		bool ok = true;
		const fs::path A = R / L"archivos";
		for (auto it = fs::recursive_directory_iterator(A, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
			if (!it->is_regular_file(ec)) continue;
			const fs::path rel = fs::relative(it->path(), A, ec);
			if (!copiar(it->path(), D / rel)) { ok = false; error = "No se pudo devolver " + utf8(rel.wstring()); }
		}
		std::ifstream nuevos(R / L"nuevos.txt");
		std::string linea;
		while (std::getline(nuevos, linea)) {
			if (linea.empty()) continue;
			const fs::path rel(ancho(linea));
			if (rel.is_absolute() || protegida(rel) || linea.find("..") != std::string::npos) continue;
			fs::remove(D / rel, ec);
		}
		return ok;
	}

	int supervisar(const Supervision& s) {
		anotar(s.resultado, "supervisar " + s.anterior + " -> " + s.version);
		if (s.pid) {
			HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, s.pid);
			if (h) {
				const DWORD r = WaitForSingleObject(h, s.esperaCierreMs);
				CloseHandle(h);
				if (r != WAIT_OBJECT_0) { escribirResultado(s, "fallo", "Link no se cerró a tiempo; no se cambió nada"); return 2; }
			}
		}
		Sleep(300);
		std::string error;
		std::error_code ec;
		fs::remove(fs::path(s.okArchivo), ec);
		if (!paso(s, true, error)) {
			escribirResultado(s, "fallo", error + "; se dejó la versión anterior");
			HANDLE h = abrirLink(s, L"--actualizacion-fallida " + ancho(s.version));
			if (h) CloseHandle(h);
			return 2;
		}
		HANDLE nuevo = abrirLink(s, L"--tras-actualizar " + ancho(s.version) + L" --ok \"" + s.okArchivo + L"\"");
		std::string porQue = nuevo ? "" : "no se pudo abrir";
		const ULONGLONG fin = GetTickCount64() + s.esperaArranqueMs;
		bool arranco = false;
		while (nuevo && GetTickCount64() < fin) {
			if (fs::exists(fs::path(s.okArchivo), ec)) { arranco = true; break; }
			if (WaitForSingleObject(nuevo, 250) == WAIT_OBJECT_0) {
				DWORD codigo = 0; GetExitCodeProcess(nuevo, &codigo);
				porQue = "se cerró al arrancar (código " + std::to_string(codigo) + ")";
				break;
			}
		}
		if (!arranco && porQue.empty()) porQue = "no respondió en " + std::to_string(s.esperaArranqueMs / 1000) + " s";
		if (arranco) {
			CloseHandle(nuevo);
			fs::remove(fs::path(s.okArchivo), ec);
			fs::remove_all(fs::path(s.nueva), ec);
			escribirResultado(s, "instalada", "");
			return 0;
		}
		if (nuevo) { TerminateProcess(nuevo, 1); WaitForSingleObject(nuevo, 5000); CloseHandle(nuevo); }
		Sleep(300);
		std::string e2;
		const bool restaurada = paso(s, false, e2);
		escribirResultado(s, restaurada ? "restaurada" : "fallo", "La versión nueva " + porQue + (restaurada ? "; volvió la anterior" : "; NO se pudo volver a la anterior: " + e2));
		HANDLE viejo = abrirLink(s, L"--actualizacion-fallida " + ancho(s.version));
		if (viejo) CloseHandle(viejo);
		return 1;
	}

}
