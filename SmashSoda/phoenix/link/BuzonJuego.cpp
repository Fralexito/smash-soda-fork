#include "BuzonJuego.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace phoenix::buzon {

std::string horaLima(long long unixSeg) {
	long long l = unixSeg - 5 * 3600;
	long long d = ((l % 86400) + 86400) % 86400;
	char b[8];
	std::snprintf(b, sizeof b, "%02d:%02d", (int)(d / 3600), (int)((d % 3600) / 60));
	return b;
}

long long segundosDeIso(const std::string& iso) {
	int a = 0, m = 0, d = 0, h = 0, mi = 0, se = 0;
	if (std::sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &a, &m, &d, &h, &mi, &se) < 6) return 0;
	if (m < 1 || m > 12 || d < 1 || d > 31) return 0;
	// dias desde 1970 (algoritmo de Howard Hinnant)
	a -= m <= 2;
	const long long era = (a >= 0 ? a : a - 399) / 400;
	const long long yoe = a - era * 400;
	const long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	const long long dias = era * 146097 + doe - 719468;
	return dias * 86400 + h * 3600 + mi * 60 + se;
}

std::string cortarUtf8(const std::string& s, size_t max) {
	if (s.size() <= max) return s;
	size_t n = max;
	while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) --n;
	return s.substr(0, n);
}

std::string textoSinAvisos() { return "Sin avisos nuevos de Phoenix Evolution."; }

std::string formatear(const std::vector<Aviso>& avisos, std::vector<long long>* incluidos) {
	if (incluidos) incluidos->clear();
	std::string out;
	int lineas = 0;
	for (const auto& a : avisos) {
		// el texto puede traer saltos: se aplana a una sola linea
		std::string t = a.texto;
		for (auto& c : t) if (c == '\n' || c == '\r' || c == '\t') c = ' ';
		std::string linea = "[" + horaLima(a.creadoEn) + "] " + t;
		linea = cortarUtf8(linea, 110);
		size_t extra = linea.size() + (out.empty() ? 0 : 2); // "\n\n" de separacion
		int lineasExtra = out.empty() ? 1 : 2;
		if (lineas + lineasExtra > MAX_LINEAS) break;
		if (out.size() + extra > (size_t)MAX_BYTES) break;
		if (!out.empty()) out += "\n\n";
		out += linea;
		lineas += lineasExtra;
		if (incluidos) incluidos->push_back(a.id);
	}
	if (out.empty()) return textoSinAvisos();
	return out; // sin salto final: evitaria una linea vacia de mas
}

EstadoJuego estadoDeCarpeta(const std::filesystem::path& carpetaJuego) {
	EstadoJuego e;
	e.carpetaJuego = carpetaJuego;
	e.carpetaBuzon = carpetaJuego / "SiderAddons" / "content" / "phoenix";
	std::error_code ec;
	e.puente = std::filesystem::is_directory(e.carpetaBuzon, ec) ? Puente::Listo : Puente::NoInstalado;
	return e;
}

EstadoJuego detectarJuego() {
#ifdef _WIN32
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return {};
	PROCESSENTRY32W pe{};
	pe.dwSize = sizeof pe;
	DWORD pid = 0;
	if (Process32FirstW(snap, &pe)) {
		do {
			if (_wcsicmp(pe.szExeFile, L"PES2021.exe") == 0) { pid = pe.th32ProcessID; break; }
		} while (Process32NextW(snap, &pe));
	}
	CloseHandle(snap);
	if (!pid) return {};
	HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!h) return {};
	wchar_t ruta[32768];
	DWORD n = 32768;
	BOOL ok = QueryFullProcessImageNameW(h, 0, ruta, &n);
	CloseHandle(h);
	if (!ok) return {};
	return estadoDeCarpeta(std::filesystem::path(std::wstring(ruta, n)).parent_path());
#else
	return {};
#endif
}

bool escribirAtomico(const std::filesystem::path& carpetaBuzon, const std::string& contenido, std::string* error) {
	namespace fs = std::filesystem;
	std::error_code ec;
	if (!fs::is_directory(carpetaBuzon, ec)) {
		if (error) *error = "la carpeta del buzon no existe";
		return false;
	}
	fs::path tmp = carpetaBuzon / "avisos.tmp";
	fs::path dst = carpetaBuzon / "avisos.txt";
	{
		std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
		if (!f) { if (error) *error = "no se pudo crear avisos.tmp"; return false; }
		f.write(contenido.data(), (std::streamsize)contenido.size());
		f.flush();
		if (!f) { if (error) *error = "no se pudo escribir avisos.tmp"; return false; }
	}
	for (int i = 0; i < 10; ++i) {
		bool ok = false;
#ifdef _WIN32
		if (fs::exists(dst, ec))
			ok = ReplaceFileW(dst.c_str(), tmp.c_str(), nullptr, 0, nullptr, nullptr) != 0;
		else
			ok = MoveFileExW(tmp.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
		fs::rename(tmp, dst, ec);
		ok = !ec;
#endif
		if (ok) return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	fs::remove(tmp, ec);
	if (error) *error = "el juego tenia el archivo ocupado";
	return false;
}

} // namespace phoenix::buzon
