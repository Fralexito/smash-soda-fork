#include "RutasJuego.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>

namespace fs = std::filesystem;

namespace mercado::rutas {

	namespace {
		std::string minus(std::string s) {
			for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return s;
		}
		long long segundos(const fs::file_time_type& t) {
			return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
		}
		/// Hijo con ese nombre sin distinguir mayúsculas (Windows no distingue; Linux sí, para las pruebas).
		std::optional<fs::path> hijo(const fs::path& padre, const std::string& nombre) {
			std::error_code ec;
			const fs::path directo = padre / aRuta(nombre);
			if (fs::exists(directo, ec)) return directo;
			if (!fs::is_directory(padre, ec)) return std::nullopt;
			for (fs::directory_iterator it(padre, fs::directory_options::skip_permission_denied, ec), fin; !ec && it != fin; it.increment(ec))
				if (minus(deRuta(it->path().filename())) == minus(nombre)) return it->path();
			return std::nullopt;
		}
		std::vector<fs::path> subcarpetas(const fs::path& p) {
			std::vector<fs::path> r;
			std::error_code ec;
			if (!fs::is_directory(p, ec)) return r;
			for (fs::directory_iterator it(p, fs::directory_options::skip_permission_denied, ec), fin; !ec && it != fin; it.increment(ec)) {
				std::error_code e2;
				if (it->is_directory(e2) && !it->is_symlink(e2)) r.push_back(it->path());
			}
			return r;
		}
	}

	std::vector<CarpetaSave> buscarSaves(const std::vector<std::string>& carpetasDocumentos) {
		std::vector<CarpetaSave> r;
		std::set<std::string> vistas;
		for (const auto& doc : carpetasDocumentos) {
			if (doc.empty()) continue;
			const auto konami = hijo(aRuta(doc), "KONAMI");
			if (!konami) continue;
			for (const auto& edicion : subcarpetas(*konami))
				for (const auto& steam : subcarpetas(edicion)) {
					const auto save = hijo(steam, "save");
					if (!save) continue;
					const auto edit = hijo(*save, "EDIT00000000");
					std::error_code ec;
					if (!edit || !fs::is_regular_file(*edit, ec)) continue;
					std::error_code e1; const fs::path canon = fs::weakly_canonical(*save, e1);
					const std::string clave = minus(deRuta(e1 ? *save : canon));
					if (!vistas.insert(clave).second) continue;   // la misma carpeta vista por dos caminos (OneDrive)
					const auto t = fs::last_write_time(*edit, ec);
					r.push_back({ deRuta(*save), deRuta(edicion.filename()), deRuta(steam.filename()), ec ? 0 : segundos(t) });
				}
		}
		std::stable_sort(r.begin(), r.end(), [](const CarpetaSave& a, const CarpetaSave& b) { return a.modificado > b.modificado; });
		return r;
	}

	std::optional<CarpetaSave> saveActivo(const std::vector<std::string>& carpetasDocumentos) {
		auto todas = buscarSaves(carpetasDocumentos);
		if (todas.empty()) return std::nullopt;
		return todas.front();
	}

	std::vector<BaseParche> buscarPlayerBin(const std::string& carpetaJuego) {
		std::vector<BaseParche> r;
		const fs::path raiz = aRuta(carpetaJuego);
		std::error_code ec;
		if (carpetaJuego.empty() || !fs::is_directory(raiz, ec)) return r;

		// Recorrido por niveles (hasta 4) buscando carpetas «SiderAddons».
		std::vector<std::pair<fs::path, int>> cola{ { raiz, 0 } };
		for (size_t i = 0; i < cola.size(); i++) {
			const auto [dir, nivel] = cola[i];
			for (const auto& sub : subcarpetas(dir)) {
				const std::string nombre = minus(deRuta(sub.filename()));
				if (nombre == "sideraddons") {
					fs::path p = sub;
					bool ok = true;
					for (const char* parte : { "livecpk", "Phoenix-DB", "common", "etc", "pesdb", "Player.bin" }) {
						auto h = hijo(p, parte);
						if (!h) { ok = false; break; }
						p = *h;
					}
					if (ok && fs::is_regular_file(p, ec)) {
						std::error_code er;
						const fs::path rel = fs::relative(dir, raiz, er);
						std::string modo = er ? std::string() : deRuta(rel);
						std::replace(modo.begin(), modo.end(), '\\', '/');
						if (modo.empty() || modo == ".") modo = "principal";
						r.push_back({ modo, deRuta(p) });
					}
					continue;   // no se entra dentro de SiderAddons
				}
				if (nivel + 1 < 4 && nombre != "download" && nombre != "dt") cola.push_back({ sub, nivel + 1 });
			}
		}
		std::sort(r.begin(), r.end(), [](const BaseParche& a, const BaseParche& b) {
			if ((a.modo == "principal") != (b.modo == "principal")) return a.modo == "principal";
			return a.modo < b.modo;
		});
		return r;
	}

	std::string nombreParche(const std::string& carpetaJuego) {
		const fs::path raiz = aRuta(carpetaJuego);
		if (auto v = hijo(raiz, "version_actual.txt")) {
			std::ifstream f(*v, std::ios::binary);
			std::string linea;
			std::getline(f, linea);
			if (linea.size() >= 3 && static_cast<unsigned char>(linea[0]) == 0xEF) linea = linea.substr(3);   // BOM
			while (!linea.empty() && std::isspace(static_cast<unsigned char>(linea.back()))) linea.pop_back();
			while (!linea.empty() && std::isspace(static_cast<unsigned char>(linea.front()))) linea.erase(linea.begin());
			if (!linea.empty() && linea.size() <= 80) return linea;
		}
		std::string n = deRuta(raiz.filename());
		if (n.empty()) n = deRuta(raiz.parent_path().filename());
		return n.empty() ? "desconocido" : n;
	}

	std::string etiquetaSegura(const std::string& texto) {
		std::string r;
		for (char c : texto) r += (std::string("\\/:*?\"<>|").find(c) != std::string::npos || static_cast<unsigned char>(c) < 32) ? '_' : c;
		while (!r.empty() && (r.back() == '.' || r.back() == ' ')) r.pop_back();
		return r.empty() ? "_" : r;
	}

}
