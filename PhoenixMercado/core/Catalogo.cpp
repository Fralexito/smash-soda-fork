#include "Catalogo.h"

#include <nlohmann/json.hpp>
#include <tuple>

using json = nlohmann::json;

namespace mercado {

	ResumenCatalogo construirCatalogo(const OptionFile& option, const std::map<uint32_t, FichaJugador>& base,
		const std::string& nombreParche) {
		ResumenCatalogo r;
		auto pos = [](int p) { return p >= 0 && p < 13 ? std::string(kPosiciones[p]) : std::string(); };
		std::map<uint32_t, const JugadorEditado*> editados;
		for (const auto& e : option.editados()) editados[e.id] = &e;

		json equipos = json::array();
		std::map<std::string, int> vecesNombre;
		for (const auto& t : option.equipos()) {
			auto it = option.plantillas().find(t.id);
			if (it != option.plantillas().end() && !it->second.empty()) vecesNombre[t.nombre]++;
		}
		for (const auto& t : option.equipos()) {
			auto it = option.plantillas().find(t.id);
			if (it == option.plantillas().end() || it->second.empty()) continue;
			std::string nombre = t.nombre.empty() ? std::string("Equipo") : t.nombre;
			if (vecesNombre[t.nombre] > 1 || t.nombre.empty() || t.nombre == "-") nombre += " (#" + std::to_string(t.id) + ")";
			equipos.push_back({ {"pes_team_id", t.id}, {"nombre", nombre.substr(0, 60)}, {"abreviatura", t.abreviatura} });
		}

		// Un jugador puede estar en club + selección: el primero es su equipo principal.
		std::map<uint32_t, std::vector<std::pair<uint32_t, uint16_t>>> equiposDe;
		for (const auto& [eq, pl] : option.plantillas())
			for (const auto& p : pl) equiposDe[p.jugador].push_back({ eq, p.dorsal });

		json jugadores = json::array();
		for (const auto& [id, eqs] : equiposDe) {
			json j = { {"pes_id", id}, {"pes_team_id", eqs[0].first}, {"dorsal", eqs[0].second} };
			if (auto e = editados.find(id); e != editados.end()) {
				const auto& x = *e->second;
				j.update({ {"nombre", x.nombre}, {"posicion", pos(x.posicion)}, {"edad", x.edad},
					{"nacionalidad", x.nacionalidad}, {"altura", x.altura}, {"peso", x.peso}, {"fuente", "option"} });
			}
			else if (auto b = base.find(id); b != base.end()) {
				const auto& x = b->second;
				j.update({ {"nombre", x.nombre}, {"posicion", pos(x.posicion)}, {"edad", x.edad},
					{"nacionalidad", x.nacionalidad}, {"altura", x.altura}, {"peso", x.peso},
					{"habilidades", x.habilidades}, {"fuente", "parche"} });
			}
			else { r.sinDatos++; j["fuente"] = "desconocida"; }
			if (eqs.size() > 1) {
				json otros = json::array();
				for (size_t i = 1; i < eqs.size(); i++) otros.push_back({ {"pes_team_id", eqs[i].first}, {"dorsal", eqs[i].second} });
				j["otros_equipos"] = otros;
			}
			jugadores.push_back(std::move(j));
		}
		r.equipos = int(equipos.size());
		r.jugadores = int(jugadores.size());
		r.json = json({ {"formato", "phoenix-mercado/catalogo@0.2"}, {"parche", nombreParche},
			{"equipos", equipos}, {"jugadores", jugadores} }).dump();
		return r;
	}

}

namespace mercado {

	namespace {
		// Reglas de la web (CHECK de lm_jugadores / lm_clubes). Un dato fuera de rango
		// se QUITA (la web conserva el que tenía); un jugador sin nombre no se envía.
		bool enRango(const json& j, const char* k, int lo, int hi) {
			return !j.contains(k) || (j[k].is_number_integer() && j[k].get<int>() >= lo && j[k].get<int>() <= hi);
		}
		json limpiarJugador(json j) {
			if (!j.contains("nombre") || !j["nombre"].is_string() || j["nombre"].get<std::string>().empty()) return nullptr;
			std::string n = j["nombre"]; if (n.size() > 80) j["nombre"] = n.substr(0, 80);
			for (const auto& [k, lo, hi] : std::initializer_list<std::tuple<const char*, int, int>>{ {"dorsal", 1, 99}, {"edad", 10, 60}, {"altura", 120, 230}, {"peso", 30, 150}, {"media", 1, 99} })
				if (!enRango(j, k, lo, hi)) j.erase(k);
			if (j.contains("fuente") && j["fuente"] != "option" && j["fuente"] != "parche") j.erase("fuente");
			if (j.contains("habilidades") && j["habilidades"].dump().size() > 3500) j.erase("habilidades");
			if (j.contains("otros_equipos") && j["otros_equipos"].dump().size() > 1800) j.erase("otros_equipos");
			if (j.contains("nacionalidad") && !j["nacionalidad"].is_string()) j["nacionalidad"] = j["nacionalidad"].dump();
			return j;
		}
	}

	std::vector<std::string> lotesCatalogo(const std::string& catalogoJson, size_t porLote) {
		const json c = json::parse(catalogoJson);
		json jug = json::array();
		for (const auto& j : c.at("jugadores")) { json l = limpiarJugador(j); if (!l.is_null()) jug.push_back(l); }
		json equipos = json::array();
		for (auto e : c.at("equipos")) {
			std::string n = e.value("nombre", "");
			if (n.size() < 2) n = "Equipo #" + std::to_string(e.value("pes_team_id", 0));
			e["nombre"] = n.substr(0, 60);
			equipos.push_back(e);
		}
		std::vector<std::string> lotes;
		for (size_t i = 0; i == 0 || i < jug.size(); i += porLote) {
			json lote = { {"formato", c.value("formato", "")}, {"parche", c.value("parche", "")},
				{"equipos", i == 0 ? equipos : json::array()}, {"jugadores", json::array()} };
			for (size_t k = i; k < jug.size() && k < i + porLote; k++) lote["jugadores"].push_back(jug[k]);
			lotes.push_back(lote.dump());
		}
		return lotes;
	}

}
