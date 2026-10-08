#include "PerfilesSala.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace phoenix::web {

	namespace detalle_perfiles {
		std::string minusculas(const std::string& s) {
			std::string r = s;
			std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return r;
		}
	}

	std::string PerfilesSala::normalizar(const std::string& nombre) {
		size_t a = 0, b = nombre.size();
		while (a < b && std::isspace(static_cast<unsigned char>(nombre[a]))) a++;
		while (b > a && std::isspace(static_cast<unsigned char>(nombre[b - 1]))) b--;
		std::string r = nombre.substr(a, b - a);
		// Sin caracteres de control
		r.erase(std::remove_if(r.begin(), r.end(), [](unsigned char c) { return c < 0x20; }), r.end());
		if (r.size() > kLargoNombre) {
			size_t corte = kLargoNombre;
			// no partir un carácter UTF-8 por la mitad
			while (corte > 0 && (static_cast<unsigned char>(r[corte]) & 0xC0) == 0x80) corte--;
			r = r.substr(0, corte);
		}
		return r;
	}

	std::vector<PerfilSala> PerfilesSala::lista() const {
		std::vector<PerfilSala> r;
		try {
			std::ifstream f(_ruta, std::ios::binary);
			if (!f) return r;
			std::stringstream ss;
			ss << f.rdbuf();
			const nlohmann::json j = nlohmann::json::parse(ss.str(), nullptr, false);
			if (j.is_discarded() || !j.is_object() || !j.contains("perfiles") || !j["perfiles"].is_array()) return r;
			for (const nlohmann::json& p : j["perfiles"]) {
				if (!p.is_object()) continue;
				PerfilSala ps;
				ps.nombre = normalizar(p.value("nombre", ""));
				if (ps.nombre.empty()) continue;
				if (p.contains("valores") && p["valores"].is_object()) ps.valores = p["valores"];
				r.push_back(ps);
				if (r.size() >= kMaximo) break;
			}
		}
		catch (...) {
			r.clear();
		}
		return r;
	}

	std::optional<PerfilSala> PerfilesSala::obtener(const std::string& nombre) const {
		const std::string clave = detalle_perfiles::minusculas(normalizar(nombre));
		for (const PerfilSala& p : lista()) {
			if (detalle_perfiles::minusculas(p.nombre) == clave) return p;
		}
		return std::nullopt;
	}

	bool PerfilesSala::escribir(const std::vector<PerfilSala>& todos) const {
		try {
			nlohmann::json arr = nlohmann::json::array();
			for (const PerfilSala& p : todos) arr.push_back({ {"nombre", p.nombre}, {"valores", p.valores} });
			const nlohmann::json archivo = { {"version", 1}, {"perfiles", arr} };
			const std::string temporal = _ruta + ".tmp";
			{
				std::ofstream f(temporal, std::ios::binary | std::ios::trunc);
				if (!f) return false;
				f << archivo.dump(1, '\t', false, nlohmann::json::error_handler_t::replace);
				if (!f.good()) return false;
			}
			std::error_code ec;
			std::filesystem::rename(temporal, _ruta, ec);
			if (ec) {
				std::filesystem::remove(temporal, ec);
				return false;
			}
			return true;
		}
		catch (...) {
			return false;
		}
	}

	bool PerfilesSala::guardar(const std::string& nombre, const nlohmann::json& valores) {
		const std::string limpio = normalizar(nombre);
		if (limpio.empty() || !valores.is_object()) return false;
		std::vector<PerfilSala> todos = lista();
		const std::string clave = detalle_perfiles::minusculas(limpio);
		bool reemplazado = false;
		for (PerfilSala& p : todos) {
			if (detalle_perfiles::minusculas(p.nombre) == clave) {
				p.nombre = limpio;
				p.valores = valores;
				reemplazado = true;
				break;
			}
		}
		if (!reemplazado) {
			if (todos.size() >= kMaximo) return false;
			todos.push_back({ limpio, valores });
		}
		return escribir(todos);
	}

	bool PerfilesSala::borrar(const std::string& nombre) {
		std::vector<PerfilSala> todos = lista();
		const std::string clave = detalle_perfiles::minusculas(normalizar(nombre));
		const size_t antes = todos.size();
		todos.erase(std::remove_if(todos.begin(), todos.end(),
			[&](const PerfilSala& p) { return detalle_perfiles::minusculas(p.nombre) == clave; }), todos.end());
		if (todos.size() == antes) return false;
		return escribir(todos);
	}

}
