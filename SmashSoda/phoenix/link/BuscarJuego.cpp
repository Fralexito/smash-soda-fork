#include "BuscarJuego.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#endif
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <deque>
#include <fstream>
#include <mutex>
#include <set>
#include <thread>

namespace phoenix::buscar {

namespace {
	std::string minusculas(std::string s) { for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }

	/// Carpetas que nunca tienen un PES y que son lentas o enormes.
	bool saltar(const std::string& nombreMinusculas) {
		static const std::set<std::string> fuera = { "windows", "$recycle.bin", "system volume information", "programdata", "appdata", "node_modules", ".git", "$windows.~bt", "recovery", "perflogs", "msocache", "winsxs", "temp", "tmp", "cache", "backup", "respaldo" };
		if (fuera.count(nombreMinusculas)) return true;
		return !nombreMinusculas.empty() && nombreMinusculas[0] == '$';
	}
	bool esJuego(const fs::path& d, std::error_code& ec) { return fs::is_regular_file(d / "PES2021.exe", ec); }
}

std::vector<std::string> rutasDeBibliotecasSteam(const std::string& vdf) {
	// Lineas como:   "path"		"D:\\SteamLibrary"     (las barras vienen dobladas)
	std::vector<std::string> r;
	size_t i = 0;
	while ((i = vdf.find("\"path\"", i)) != std::string::npos) {
		i += 6;
		const size_t a = vdf.find('"', i);
		if (a == std::string::npos) break;
		std::string v;
		size_t k = a + 1;
		for (; k < vdf.size() && vdf[k] != '"' && vdf[k] != '\n'; k++) {
			if (vdf[k] == '\\' && k + 1 < vdf.size() && (vdf[k + 1] == '\\' || vdf[k + 1] == '"')) { v += vdf[k + 1]; k++; }
			else v += vdf[k];
		}
		if (k < vdf.size() && vdf[k] == '"' && !v.empty() && v.size() < 400) r.push_back(v);
		i = k;
	}
	return r;
}

std::vector<Candidato> buscarJuegos(const std::vector<fs::path>& raices, int profundidad, int limiteMs, const std::function<bool()>& cancelar) {
	std::vector<Candidato> r;
	std::set<std::string> vistos;
	const auto inicio = std::chrono::steady_clock::now();
	auto agotado = [&]() {
		if (cancelar && cancelar()) return true;
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - inicio).count() >= limiteMs;
	};
	std::error_code ec;
	for (const fs::path& raiz : raices) {
		if (agotado()) break;
		if (!fs::is_directory(raiz, ec)) continue;
		std::deque<std::pair<fs::path, int>> cola;
		cola.push_back({ raiz, 0 });
		while (!cola.empty() && !agotado()) {
			const auto [dir, nivel] = cola.front();
			cola.pop_front();
			if (esJuego(dir, ec)) {
				Candidato c;
				c.carpeta = dir;
				c.conSider = fs::is_regular_file(dir / "SiderAddons" / "sider.ini", ec);
				const std::string clave = minusculas(dir.string());
				if (vistos.insert(clave).second) r.push_back(c);
				continue;   // dentro de un juego no se busca otro
			}
			if (nivel >= profundidad) continue;
			for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec), fin; !ec && it != fin; it.increment(ec)) {
				if (agotado()) break;
				std::error_code e2;
				if (!it->is_directory(e2) || e2) continue;
				if (saltar(minusculas(it->path().filename().string()))) continue;
				cola.push_back({ it->path(), nivel + 1 });
			}
			ec.clear();
		}
	}
	std::stable_sort(r.begin(), r.end(), [](const Candidato& a, const Candidato& b) { return a.conSider > b.conSider; });
	return r;
}

std::vector<fs::path> raicesDeBusqueda() {
	std::vector<fs::path> r;
#ifdef _WIN32
	// Steam: carpeta de Steam (registro) + bibliotecas extra (libraryfolders.vdf)
	std::vector<fs::path> steam;
	HKEY k = nullptr;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_READ, &k) == ERROR_SUCCESS) {
		wchar_t b[1024];
		DWORD tam = sizeof(b);
		if (RegQueryValueExW(k, L"SteamPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(b), &tam) == ERROR_SUCCESS) steam.push_back(fs::path(b));
		RegCloseKey(k);
	}
	std::vector<fs::path> extra;
	for (const fs::path& s : steam) {
		std::ifstream f(s / "steamapps" / "libraryfolders.vdf", std::ios::binary);
		if (!f) continue;
		const std::string t((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		for (const std::string& p : rutasDeBibliotecasSteam(t)) extra.push_back(fs::u8path(p));
	}
	for (const fs::path& s : steam) r.push_back(s / "steamapps" / "common");
	for (const fs::path& s : extra) r.push_back(s / "steamapps" / "common");
	// Unidades fijas (C:\, D:\…)
	DWORD mascara = GetLogicalDrives();
	for (int i = 0; i < 26; i++) {
		if (!(mascara & (1u << i))) continue;
		const wchar_t letra[] = { static_cast<wchar_t>(L'A' + i), L':', L'\\', 0 };
		if (GetDriveTypeW(letra) == DRIVE_FIXED) r.push_back(fs::path(letra));
	}
#endif
	return r;
}

namespace {
	std::mutex mtx;
	EstadoBusqueda estado;
	std::thread hilo;
	std::atomic<bool> parar{ false };
	struct Guardia { ~Guardia() { parar = true; if (hilo.joinable()) hilo.join(); } } guardia;   // al cerrar Link no deja el hilo suelto
}

void iniciarBusqueda() {
	{
		std::lock_guard<std::mutex> g(mtx);
		if (estado.buscando) return;
		estado = EstadoBusqueda();
		estado.buscando = true;
	}
	if (hilo.joinable()) hilo.join();
	parar = false;
	hilo = std::thread([]() {
		std::vector<Candidato> c;
		try { c = buscarJuegos(raicesDeBusqueda(), 4, 12000, []() { return parar.load(); }); } catch (...) {}
		std::lock_guard<std::mutex> g(mtx);
		estado.candidatos = std::move(c);
		estado.buscando = false;
		estado.terminada = true;
	});
}

EstadoBusqueda estadoBusqueda() { std::lock_guard<std::mutex> g(mtx); return estado; }

} // namespace phoenix::buscar
