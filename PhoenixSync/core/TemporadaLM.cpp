#include "TemporadaLM.h"

#include <algorithm>
#include <unordered_set>

namespace mercado::lm {

	namespace {
		uint32_t le32(const std::vector<uint8_t>& d, size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (uint32_t(d[o + 3]) << 24); }
		uint16_t le16(const std::vector<uint8_t>& d, size_t o) { return uint16_t(d[o] | (d[o + 1] << 8)); }
		constexpr int kEquipos = 700;
		constexpr uint32_t kSinPuesto = 0xffffffffu;

		FilaTablaLM fila(const std::vector<uint8_t>& d, size_t o) {
			FilaTablaLM f;
			f.club = le32(d, o);
			const uint32_t puesto = le32(d, o + 4), C = le32(d, o + 8), A = le32(d, o + 12);
			f.puesto = puesto == kSinPuesto ? -1 : int(puesto);
			f.puntos = int(C & 0xff); f.ganados = int((C >> 8) & 0x3f); f.perdidos = int((C >> 14) & 0x3f);
			f.empatados = int((C >> 20) & 0x3f); f.ganadosFuera = int(C >> 26);
			f.golesFavor = int(A & 0xfff); f.golesContra = int((A >> 12) & 0xfff); f.jugados = int(A >> 24);
			f.golesFuera = int(le32(d, o + 16));
			return f;
		}
		/// Una fila de tabla es coherente si los números cuadran entre sí (así se descartan otras listas de clubes).
		bool coherente(const FilaTablaLM& f) {
			if (f.puesto < 0) return f.puntos == 0 && f.jugados == 0;
			return f.puesto >= 1 && f.puesto <= 64 && f.puntos == 3 * f.ganados + f.empatados
				&& f.jugados == f.ganados + f.empatados + f.perdidos && f.ganadosFuera <= f.ganados && f.golesFuera <= f.golesFavor;
		}
	}

	int RankingLM::total() const { int t = 0; for (auto& f : filas) t += f.valor; return t; }

	Resultado<uint32_t> TemporadaLM::idInterno(const GuardadoLM& g, int indice) {
		auto id = g.idOptionDe(indice);
		if (!id.ok()) return Resultado<uint32_t>::mal(id.error.codigo, id.error.detalle);
		if (*id.valor >= (1u << 18)) return Resultado<uint32_t>::mal("CLUB_INVALIDO", "ID option fuera de rango");
		return Resultado<uint32_t>::bien((*id.valor << 14) | uint32_t(indice));
	}

	Resultado<TemporadaLM> TemporadaLM::leer(const GuardadoLM& g, size_t limite) {
		using R = Resultado<TemporadaLM>;
		const auto& d = g.datos();
		if (limite == 0 || limite > d.size()) limite = d.size();
		if (limite < 0x100) return R::mal("TEMPORADA_CORTA", "Datos demasiado cortos");
		std::unordered_set<uint32_t> clubes;
		for (int k = 0; k < kEquipos; k++) { auto id = idInterno(g, k); if (id.ok()) clubes.insert(*id.valor); }
		if (clubes.size() < 20) return R::mal("TEMPORADA_SIN_CLUBES", "No se reconocen los clubes del guardado");
		auto esClub = [&](size_t o) { return o + 4 <= limite && clubes.count(le32(d, o)) > 0; };

		TemporadaLM t;
		// --- Partidos de liga: [local][visitante][nº u16][ffff][competición u16][código u16] ------------------
		std::unordered_set<uint32_t> vistos;
		for (size_t o = 0x50; o + 16 <= limite; o += 4) {
			if (le16(d, o + 10) != 0xffff || !esClub(o) || !esClub(o + 4)) continue;
			const uint32_t a = le32(d, o), b = le32(d, o + 4);
			const uint16_t comp = le16(d, o + 12), codigo = le16(d, o + 14);
			if (a == b || comp >= 512) continue;
			PartidoLM p; p.local = a; p.visitante = b; p.numero = le16(d, o + 8); p.competicion = comp;
			p.jornada = codigo & 0x3f; p.orden = codigo >> 6; p.ofs = o;
			if (vistos.insert(p.numero).second) t.partidos.push_back(p);
		}
		std::sort(t.partidos.begin(), t.partidos.end(), [](const PartidoLM& x, const PartidoLM& y) { return x.numero < y.numero; });

		// --- Tablas: filas de 20 B seguidas, coherentes y con el puesto que no baja -------------------------
		for (size_t o = 0x50 + 20; o + 20 <= limite; o += 4) {
			if (!esClub(o) || esClub(o - 20)) continue;
			TablaLM tb; tb.ofs = o;
			size_t k = o; int ultimo = 0; bool ok = true, vacia = true;
			while (k + 20 <= limite && esClub(k)) {
				FilaTablaLM f = fila(d, k);
				if (!coherente(f) || (f.puesto >= 0 && f.puesto < ultimo)) { ok = false; break; }
				if (f.puesto >= 0) { vacia = false; ultimo = f.puesto; }
				tb.filas.push_back(f); k += 20;
			}
			if (!ok || tb.filas.size() < 4) continue;
			if (!vacia && std::any_of(tb.filas.begin(), tb.filas.end(), [](const FilaTablaLM& f) { return f.puesto < 0; })) continue;
			int gf = 0, gc = 0;
			for (auto& f : tb.filas) { gf += f.golesFavor; gc += f.golesContra; }
			if (gf != gc) continue;   // en una liga, todo gol marcado lo recibe otro
			tb.vacia = vacia;
			t.tablas.push_back(std::move(tb));
			o = k - 4;
		}

		// --- Rankings: [reg][pid][club][puesto][valor], puesto que no baja y valor que no sube ----------------
		auto esFilaRanking = [&](size_t o) {
			if (o + 20 > limite || !esClub(o + 8)) return false;
			const uint32_t reg = le32(d, o), pid = le32(d, o + 4), puesto = le32(d, o + 12), valor = le32(d, o + 16);
			return (reg < 30000 || (reg >> 16) == 0xdb65) && pid > 0 && pid < 1000000 && puesto >= 1 && puesto <= 64 && valor >= 1 && valor < 200;
		};
		for (size_t o = 0x50 + 20; o + 20 <= limite; o += 4) {
			if (!esFilaRanking(o) || esFilaRanking(o - 20)) continue;
			RankingLM r; r.ofs = o;
			size_t k = o; bool ok = true;
			while (esFilaRanking(k)) {
				FilaRankingLM f{ le32(d, k), le32(d, k + 4), le32(d, k + 8), int(le32(d, k + 12)), int(le32(d, k + 16)) };
				if (!r.filas.empty() && (f.puesto < r.filas.back().puesto || f.valor > r.filas.back().valor)) { ok = false; break; }
				r.filas.push_back(f); k += 20;
			}
			if (!ok || r.filas.size() < 3 || r.filas.front().puesto != 1) continue;
			t.rankings.push_back(std::move(r));
			o = k - 4;
		}
		return R::bien(std::move(t));
	}

}
