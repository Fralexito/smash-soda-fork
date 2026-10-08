#include "Catalogo.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace mercado {

	ResumenCatalogo construirCatalogo(const OptionFile& option, const std::map<uint32_t, FichaJugador>& base,
		const std::string& nombreParche) {
		ResumenCatalogo r;
		auto pos = [](int p) { return p >= 0 && p < 13 ? std::string(kPosiciones[p]) : std::string(); };
		std::map<uint32_t, const JugadorEditado*> editados;
		for (const auto& e : option.editados()) editados[e.id] = &e;

		json equipos = json::array();
		for (const auto& t : option.equipos()) {
			auto it = option.plantillas().find(t.id);
			if (it == option.plantillas().end() || it->second.empty()) continue;
			equipos.push_back({ {"pes_team_id", t.id}, {"nombre", t.nombre}, {"abreviatura", t.abreviatura} });
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
