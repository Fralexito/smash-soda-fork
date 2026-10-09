#include "Integridad.h"
#include "Emparejamiento.h"
#include "Sha256.h"

#include <algorithm>
#include <set>
#include <nlohmann/json.hpp>

namespace mercado {

	PlantillasPhoenix traducirPlantillas(const OptionFile& option, const std::map<uint32_t, int64_t>& clubes,
		const std::map<uint32_t, int64_t>& jugadores, int* sinTraducir) {
		PlantillasPhoenix r;
		int ignorados = 0;
		for (const auto& [eqLocal, pl] : option.plantillas()) {
			auto c = clubes.find(eqLocal);
			if (c == clubes.end()) continue;
			auto& lista = r[c->second];
			for (const auto& p : pl) {
				auto j = jugadores.find(p.jugador);
				if (j != jugadores.end()) lista.push_back(j->second); else ignorados++;
			}
			std::sort(lista.begin(), lista.end());
		}
		if (sinTraducir) *sinTraducir = ignorados;
		return r;
	}

	std::string textoCanonico(const PlantillasPhoenix& p) {
		std::string t;
		for (const auto& [club, lista] : p) {   // std::map ya ordena por club
			std::vector<int64_t> l = lista;
			std::sort(l.begin(), l.end());
			t += std::to_string(club) + ":";
			for (size_t i = 0; i < l.size(); i++) t += (i ? "," : "") + std::to_string(l[i]);
			t += "\n";
		}
		return t;
	}

	std::string huellaPlantillas(const PlantillasPhoenix& p) { return sha256::deTexto(textoCanonico(p)); }

	std::vector<Diferencia> diferencias(const PlantillasPhoenix& esperado, const PlantillasPhoenix& actual) {
		std::map<int64_t, int64_t> e, a;
		for (const auto& [c, l] : esperado) for (int64_t j : l) e[j] = c;
		for (const auto& [c, l] : actual) for (int64_t j : l) a[j] = c;
		std::set<int64_t> todos;
		for (auto& x : e) todos.insert(x.first);
		for (auto& x : a) todos.insert(x.first);
		std::vector<Diferencia> r;
		for (int64_t j : todos) {
			const int64_t ce = e.count(j) ? e[j] : 0, ca = a.count(j) ? a[j] : 0;
			if (ce != ca) r.push_back({ j, ce, ca });
		}
		return r;
	}

	std::vector<Anomalia> revisarEstructura(const OptionFile& option, const std::map<uint32_t, FichaJugador>& base) {
		std::vector<Anomalia> r;
		const size_t ne = option.equipos().size();
		if (ne < 100 || ne > 2000) r.push_back({ "bloquea", "EQUIPOS_FUERA_DE_RANGO", std::to_string(ne) + " equipos" });
		size_t total = 0, faltan = 0, llenos = 0;
		std::set<uint32_t> idsEquipo;
		for (const auto& t : option.equipos()) {
			if (!idsEquipo.insert(t.id).second) r.push_back({ "bloquea", "EQUIPO_DUPLICADO", std::to_string(t.id) });
		}
		for (const auto& [eq, pl] : option.plantillas()) {
			if (pl.size() > 40) r.push_back({ "bloquea", "PLANTILLA_MAYOR_A_40", std::to_string(eq) });
			if (pl.size() == 40) llenos++;
			std::set<uint32_t> vistos;
			for (const auto& p : pl) {
				total++;
				if (!base.count(p.jugador)) faltan++;
				if (!vistos.insert(p.jugador).second)
					r.push_back({ "aviso", "JUGADOR_REPETIDO_EN_PLANTILLA", std::to_string(eq) + ":" + std::to_string(p.jugador) });
			}
		}
		// Si muchos jugadores de las plantillas no existen en la base, el option file
		// y la base son de parches distintos (o la base cambió de formato).
		if (total && faltan * 100 > total * 5)
			r.push_back({ "bloquea", "OPTION_Y_BASE_NO_COINCIDEN",
				std::to_string(faltan) + " de " + std::to_string(total) + " jugadores de las plantillas no están en la base" });
		else if (faltan) r.push_back({ "aviso", "JUGADORES_SIN_FICHA", std::to_string(faltan) });

		// Rangos físicos: si se rompen en masa, cambió el formato de Player.bin.
		size_t raros = 0;
		for (const auto& [id, f] : base)
			if (f.edad < 14 || f.edad > 50 || f.altura < 140 || f.altura > 215 || f.posicion > 12) raros++;
		if (!base.empty() && raros * 100 > base.size() * 2)
			r.push_back({ "bloquea", "BASE_FORMATO_DESCONOCIDO", std::to_string(raros) + " fichas con datos imposibles" });
		if (llenos) r.push_back({ "aviso", "PLANTILLAS_LLENAS", std::to_string(llenos) + " equipos con 40 jugadores (no admiten fichajes)" });
		return r;
	}

	bool hayBloqueo(const std::vector<Anomalia>& a) {
		return std::any_of(a.begin(), a.end(), [](const Anomalia& x) { return x.gravedad == "bloquea"; });
	}

	InformeCambios compararBases(const std::map<uint32_t, FichaJugador>& antes,
		const std::map<uint32_t, FichaJugador>& despues, const std::vector<uint32_t>& idsLiga) {
		InformeCambios r;
		for (const auto& [id, f] : despues) if (!antes.count(id)) r.nuevos++;
		for (const auto& [id, f] : antes) {
			auto it = despues.find(id);
			if (it == despues.end()) { r.eliminados++; continue; }
			const auto& g = it->second;
			if (f.nombre != g.nombre || f.altura != g.altura || f.edad != g.edad || f.nacionalidad != g.nacionalidad
				|| f.posicion != g.posicion || f.habilidades != g.habilidades) r.modificados++;
		}
		for (uint32_t id : idsLiga) {
			auto a = antes.find(id), d = despues.find(id);
			if (a == antes.end()) continue;
			if (d == despues.end()) { r.ligaEliminados.push_back(id); continue; }
			JugadorReferencia ref{ 0, 0, a->second.nombre, a->second.nacionalidad, a->second.altura, a->second.edad, a->second.posicion };
			// +1 de edad es normal entre temporadas; un puntaje bajo indica otra persona.
			if (puntuar(ref, d->second) < 60) r.ligaCambiaronIdentidad.push_back(id);
		}
		return r;
	}

	std::string InformeCambios::json() const {
		return nlohmann::json({ {"nuevos", nuevos}, {"eliminados", eliminados}, {"modificados", modificados},
			{"liga_eliminados", ligaEliminados}, {"liga_cambiaron_identidad", ligaCambiaronIdentidad},
			{"requiere_revision", !ligaEliminados.empty() || !ligaCambiaronIdentidad.empty()} }).dump();
	}

}
