#include "Emparejamiento.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace mercado {

	const char* nombreEstado(EstadoEmparejamiento e) {
		switch (e) {
		case EstadoEmparejamiento::Automatico: return "automatico";
		case EstadoEmparejamiento::Revisar: return "revisar";
		default: return "sin_candidato";
		}
	}

	namespace {
		// Equivalencia ASCII para U+00C0–U+017F (latín con acentos).
		constexpr char kLatin1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
		constexpr char kLatinA[] =
			"AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiJjJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";
		static_assert(sizeof(kLatin1) - 1 == 64, "tabla Latin-1");
		static_assert(sizeof(kLatinA) - 1 == 128, "tabla Latin Extended-A");

		std::vector<std::string> palabras(const std::string& n) {
			std::vector<std::string> r; std::string w;
			for (char c : n) { if (c == ' ') { if (!w.empty()) r.push_back(w); w.clear(); } else w += c; }
			if (!w.empty()) r.push_back(w);
			return r;
		}

		double levenshteinRelativo(const std::string& a, const std::string& b) {
			if (a.empty() && b.empty()) return 1.0;
			std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
			for (size_t j = 0; j <= b.size(); j++) prev[j] = int(j);
			for (size_t i = 1; i <= a.size(); i++) {
				cur[0] = int(i);
				for (size_t j = 1; j <= b.size(); j++)
					cur[j] = std::min({ prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (a[i - 1] != b[j - 1]) });
				std::swap(prev, cur);
			}
			return 1.0 - double(prev[b.size()]) / double(std::max(a.size(), b.size()));
		}

		// Línea del campo: 0 portero, 1 defensa, 2 medio, 3 ataque
		int linea(int p) {
			if (p == 0) return 0; if (p >= 1 && p <= 3) return 1; if (p >= 4 && p <= 8) return 2; return 3;
		}
	}

	std::string normalizarNombre(const std::string& s) {
		std::string r;
		for (size_t i = 0; i < s.size();) {
			unsigned char c = static_cast<unsigned char>(s[i]);
			uint32_t cp = c; size_t n = 1;
			if (c >= 0xC0 && c < 0xE0 && i + 1 < s.size()) { cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F); n = 2; }
			else if (c >= 0xE0 && c < 0xF0 && i + 2 < s.size()) { cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); n = 3; }
			else if (c >= 0xF0) { n = 4; cp = 0; }
			i += n;
			char a = 0;
			if (cp < 0x80) a = char(cp);
			else if (cp >= 0xC0 && cp <= 0xFF) a = kLatin1[cp - 0xC0];
			else if (cp >= 0x100 && cp <= 0x17F) a = kLatinA[cp - 0x100];
			if (a >= 'A' && a <= 'Z') a = char(a - 'A' + 'a');
			if ((a >= 'a' && a <= 'z') || (a >= '0' && a <= '9')) r += a;
			else if (!r.empty() && r.back() != ' ') r += ' ';   // espacios, guiones, puntos…
		}
		while (!r.empty() && r.back() == ' ') r.pop_back();
		return r;
	}

	double parecidoNombre(const std::string& a0, const std::string& b0) {
		const std::string a = normalizarNombre(a0), b = normalizarNombre(b0);
		if (a.empty() || b.empty()) return 0.0;
		if (a == b) return 1.0;
		const auto pa = palabras(a), pb = palabras(b);
		std::set<std::string> sa(pa.begin(), pa.end()), sb(pb.begin(), pb.end());
		int comunes = 0;
		for (const auto& w : sa) comunes += int(sb.count(w));
		const double jaccard = double(comunes) / double(sa.size() + sb.size() - comunes);
		double r = std::max(jaccard, levenshteinRelativo(a, b));
		// Mismo apellido Y nombre que empieza igual: «Gaby Torres» ≈ «Gabriel Torres».
		// (Si el nombre es distinto —«Richard Ortiz» vs «Celso Ortiz»— son personas distintas.)
		if (pa.size() >= 2 && pb.size() >= 2 && pa.back() == pb.back() && pa.back().size() >= 3
			&& pa.front()[0] == pb.front()[0])
			r = std::max(r, 0.7 + 0.3 * jaccard);
		// Uno contiene al otro: «Dante» ≈ «Dante Bonfim»
		if (sa.size() != sb.size() && comunes == int(std::min(sa.size(), sb.size())))
			r = std::max(r, 0.8);
		return r;
	}

	int puntuar(const JugadorReferencia& r, const FichaJugador& f) {
		double p = 50.0 * parecidoNombre(r.nombre, f.nombre);
		if (r.nacionalidad && f.nacionalidad) p += r.nacionalidad == f.nacionalidad ? 20 : -15;
		if (r.edad && f.edad) {
			const int d = std::abs(r.edad - f.edad);
			p += d == 0 ? 10 : d == 1 ? 8 : d == 2 ? 3 : -10;
		}
		if (r.altura && f.altura) {
			const int d = std::abs(r.altura - f.altura);
			p += d <= 1 ? 10 : d <= 3 ? 7 : d <= 5 ? 2 : -10;
		}
		if (r.posicion >= 0) p += r.posicion == f.posicion ? 5 : linea(r.posicion) == linea(f.posicion) ? 2 : -3;
		return std::clamp(int(p + 0.5), 0, 100);
	}

	Emparejador::Emparejador(const std::map<uint32_t, FichaJugador>& base) : _base(base) {
		for (const auto& [id, f] : base)
			for (const auto& w : palabras(normalizarNombre(f.nombre)))
				if (w.size() >= 3) _indice[w].push_back(id);
	}

	ResultadoJugador Emparejador::emparejar(const JugadorReferencia& r) const {
		ResultadoJugador res;
		res.phoenixId = r.phoenixId;

		// --- Regla 1/2: mismo ID (se decide después de ver también los candidatos por datos)
		int puntajePorId = -1;
		if (r.pesIdReferencia) {
			auto it = _base.find(r.pesIdReferencia);
			if (it != _base.end()) puntajePorId = puntuar(r, it->second);
		}

		// --- Regla 3: búsqueda por datos ------------------------------------
		std::set<uint32_t> ids;
		for (const auto& w : palabras(normalizarNombre(r.nombre))) {
			if (w.size() < 3) continue;
			auto it = _indice.find(w);
			if (it != _indice.end()) ids.insert(it->second.begin(), it->second.end());
		}
		if (r.pesIdReferencia && _base.count(r.pesIdReferencia)) ids.insert(r.pesIdReferencia);
		std::vector<Candidato> cands;
		for (uint32_t id : ids) {
			const auto& f = _base.at(id);
			cands.push_back({ id, puntuar(r, f), f.nombre });
		}
		std::sort(cands.begin(), cands.end(), [](const Candidato& a, const Candidato& b) { return a.puntaje > b.puntaje; });
		if (cands.size() > 3) cands.resize(3);
		res.candidatos = cands;

		// Mejor candidato que NO sea el mismo ID
		const Candidato* alt = nullptr;
		for (const auto& c : cands) if (c.idLocal != r.pesIdReferencia) { alt = &c; break; }
		const int puntajeAlt = alt ? alt->puntaje : 0;

		if (puntajePorId >= umbralPorId && puntajeAlt < puntajePorId + 10) {
			res.estado = EstadoEmparejamiento::Automatico;
			res.idLocal = r.pesIdReferencia;
			res.metodo = "por_id";
			res.motivo = "Mismo ID y datos compatibles (" + std::to_string(puntajePorId) + "/100)";
			return res;
		}
		const int mejor = cands.empty() ? 0 : cands[0].puntaje;
		const int segundo = cands.size() > 1 ? cands[1].puntaje : 0;
		if (puntajePorId >= umbralPorId) {
			// El mismo ID existe y es plausible, pero hay otro jugador igual o más parecido
			// (posible homónimo). Nunca se decide solo: va a revisión con la sugerencia.
			res.estado = EstadoEmparejamiento::Revisar;
			res.metodo = "por_id";
			res.motivo = "Posible homónimo: mismo ID (" + std::to_string(puntajePorId) + "/100) y otro candidato ("
				+ (alt ? alt->nombre : std::string()) + ", " + std::to_string(puntajeAlt) + "/100)";
			return res;
		}
		if (mejor >= umbralAutomatico && mejor - segundo >= margenMinimo) {
			res.estado = EstadoEmparejamiento::Automatico;
			res.idLocal = cands[0].idLocal;
			res.metodo = "por_datos";
			res.motivo = "Datos coinciden (" + std::to_string(mejor) + "/100, 2.º " + std::to_string(segundo) + ")";
		}
		else if (mejor >= umbralRevisar) {
			res.estado = EstadoEmparejamiento::Revisar;
			res.metodo = "por_datos";
			res.motivo = mejor - segundo < margenMinimo ? "Varios candidatos parecidos" : "Parecido, pero no seguro";
			if (puntajePorId >= 0) res.motivo += " · el mismo ID existe con datos distintos (" + std::to_string(puntajePorId) + "/100)";
		}
		else {
			res.estado = EstadoEmparejamiento::SinCandidato;
			res.motivo = puntajePorId >= 0 ? "El mismo ID es otro jugador y no hay candidatos parecidos" : "No está en este parche";
		}
		return res;
	}

	std::vector<ResultadoJugador> Emparejador::emparejarTodos(const std::vector<JugadorReferencia>& refs) const {
		std::vector<ResultadoJugador> r;
		r.reserve(refs.size());
		for (const auto& x : refs) r.push_back(emparejar(x));

		// Seguridad: un mismo ID local no puede quedar asignado a dos Phoenix ID.
		std::map<uint32_t, std::vector<size_t>> usados;
		for (size_t i = 0; i < r.size(); i++)
			if (r[i].estado == EstadoEmparejamiento::Automatico) usados[r[i].idLocal].push_back(i);
		for (auto& [id, lista] : usados) {
			if (lista.size() < 2) continue;
			for (size_t i : lista) {
				r[i].estado = EstadoEmparejamiento::Revisar;
				r[i].motivo = "Conflicto: el ID local " + std::to_string(id) + " coincide con varios jugadores de la web";
				r[i].idLocal = 0;
			}
		}
		return r;
	}

	std::vector<ResultadoClub> Emparejador::emparejarClubes(const std::vector<ClubReferencia>& clubes,
		const std::vector<ResultadoJugador>& jugadores, const OptionFile& option) const {
		std::map<int64_t, uint32_t> local;
		for (const auto& j : jugadores) if (j.estado == EstadoEmparejamiento::Automatico) local[j.phoenixId] = j.idLocal;
		std::map<uint32_t, std::vector<uint32_t>> clubesDe;   // jugador local → clubes locales
		for (const auto& [eq, pl] : option.plantillas()) for (const auto& p : pl) clubesDe[p.jugador].push_back(eq);
		std::map<uint32_t, std::string> nombres;
		for (const auto& t : option.equipos()) nombres[t.id] = t.nombre;

		std::vector<ResultadoClub> out;
		std::map<uint32_t, int> usados;
		for (const auto& c : clubes) {
			ResultadoClub r; r.phoenixId = c.phoenixId;
			std::map<uint32_t, int> votos; int conocidos = 0;
			for (int64_t pj : c.jugadores) {
				auto it = local.find(pj);
				if (it == local.end()) continue;
				conocidos++;
				for (uint32_t eq : clubesDe[it->second]) votos[eq]++;
			}
			auto pct = [&](uint32_t eq) { return conocidos ? double(votos[eq]) / conocidos : 0.0; };
			uint32_t mejor = 0, segundo = 0;
			for (const auto& [eq, v] : votos) {
				if (!mejor || v > votos[mejor]) { segundo = mejor; mejor = eq; }
				else if (!segundo || v > votos[segundo]) segundo = eq;
			}
			const bool mismoIdExiste = c.pesTeamIdReferencia && nombres.count(c.pesTeamIdReferencia);
			if (mismoIdExiste && (conocidos < 5 || pct(c.pesTeamIdReferencia) >= 0.4)) {
				r.estado = EstadoEmparejamiento::Automatico; r.idLocal = c.pesTeamIdReferencia; r.metodo = "por_id";
				r.coincidencia = pct(c.pesTeamIdReferencia);
				r.motivo = conocidos < 5 ? "Mismo ID (pocos jugadores para comprobar plantilla)" : "Mismo ID y plantilla parecida";
			}
			else if (mejor && conocidos >= 5 && pct(mejor) >= 0.65 && pct(mejor) - pct(segundo) >= 0.35) {
				r.estado = EstadoEmparejamiento::Automatico; r.idLocal = mejor; r.metodo = "por_plantilla";
				r.coincidencia = pct(mejor);
				r.motivo = "La mayoría de su plantilla está en este club";
			}
			else if (mejor) {
				r.estado = EstadoEmparejamiento::Revisar; r.idLocal = 0; r.metodo = "por_plantilla";
				r.coincidencia = pct(mejor);
				r.motivo = "Club más probable: " + nombres[mejor] + " (" + std::to_string(int(pct(mejor) * 100)) + "% de la plantilla)";
			}
			else r.motivo = "Ningún jugador conocido de este club está en el parche";
			if (r.idLocal) { r.nombreLocal = nombres[r.idLocal]; usados[r.idLocal]++; }
			out.push_back(r);
		}
		for (auto& r : out)   // dos clubes de la web no pueden ser el mismo club del juego
			if (r.idLocal && usados[r.idLocal] > 1) {
				r.estado = EstadoEmparejamiento::Revisar;
				r.motivo = "Conflicto: varios clubes de la web apuntan a " + r.nombreLocal;
				r.idLocal = 0;
			}
		return out;
	}

	std::string informeJson(const std::vector<ResultadoJugador>& js, const std::vector<ResultadoClub>& cs,
		const std::string& perfil) {
		json j = { {"formato", "phoenix-mercado/emparejamiento@0.1"}, {"perfil_parche", perfil} };
		std::map<std::string, int> cuenta;
		json jj = json::array();
		for (const auto& r : js) {
			cuenta[nombreEstado(r.estado)]++;
			json c = json::array();
			for (const auto& x : r.candidatos) c.push_back({ {"pes_id_local", x.idLocal}, {"puntaje", x.puntaje}, {"nombre", x.nombre} });
			json fila = { {"phoenix_id", r.phoenixId}, {"estado", nombreEstado(r.estado)},
				{"metodo", r.metodo}, {"motivo", r.motivo}, {"candidatos", c} };
			if (r.idLocal) fila["pes_id_local"] = r.idLocal;
			if (!r.candidatos.empty()) fila["puntaje"] = r.candidatos[0].puntaje;
			jj.push_back(fila);
		}
		json jc = json::array();
		for (const auto& r : cs)
			jc.push_back({ {"phoenix_id", r.phoenixId}, {"estado", nombreEstado(r.estado)}, {"pes_team_id_local", r.idLocal},
				{"nombre_local", r.nombreLocal}, {"metodo", r.metodo}, {"motivo", r.motivo}, {"coincidencia", r.coincidencia} });
		j["resumen"] = cuenta;
		j["jugadores"] = jj;
		j["clubes"] = jc;
		return j.dump();
	}

}
