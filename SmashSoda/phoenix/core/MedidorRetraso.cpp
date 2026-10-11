#include "MedidorRetraso.h"

#include <algorithm>
#include <cmath>

namespace phoenix::retraso {

	// ---- MedidorMando -------------------------------------------------------

	MedidorMando::Ranura* MedidorMando::ranura(uint32_t userId, double ahoraMs) {
		Ranura* libre = nullptr;
		Ranura* vieja = nullptr;
		for (Ranura& r : _r) {
			if (r.usada && r.userId == userId) return &r;
			if (!r.usada && libre == nullptr) libre = &r;
			if (r.usada && (vieja == nullptr || r.ultimo < vieja->ultimo)) vieja = &r;
		}
		Ranura* r = libre != nullptr ? libre : vieja;   // lleno: se recicla la más callada
		*r = Ranura();
		r->usada = true;
		r->userId = userId;
		r->segundo = ahoraMs;
		return r;
	}

	void MedidorMando::cerrarLlegada(Ranura& r) {
		if (!r.abierta) return;
		r.abierta = false;
		if (r.candidato) {
			// Hueco y luego varios mensajes de golpe: la red los retuvo y los soltó juntos.
			const double hace = (std::max)(3.0, 2.0 * r.tipicoMsgs);
			if (r.enLlegada >= hace) {
				r.saltos[static_cast<size_t>(r.saltosPos)] = r.llegada;
				r.saltosPos = (r.saltosPos + 1) % kSaltos;
				if (r.saltosN < kSaltos) r.saltosN++;
			}
			r.candidato = false;
		}
		else {
			r.tipicoMsgs += (r.enLlegada - r.tipicoMsgs) / 16.0;
		}
	}

	void MedidorMando::avanzarSegundo(Ranura& r, double ahoraMs) {
		if (r.segundo < 0.0) { r.segundo = ahoraMs; r.enSegundo = 0; return; }
		const double paso = ahoraMs - r.segundo;
		if (paso >= 2000.0) { r.ritmo = 0; r.enSegundo = 0; r.segundo = ahoraMs; }
		else if (paso >= 1000.0) { r.ritmo = r.enSegundo; r.enSegundo = 0; r.segundo += 1000.0; }
	}

	void MedidorMando::mensaje(uint32_t userId, double ahoraMs) {
		Ranura& r = *ranura(userId, ahoraMs);
		avanzarSegundo(r, ahoraMs);
		r.enSegundo++;
		if (r.abierta && ahoraMs - r.llegada <= kMismaLlegadaMs) {
			r.enLlegada++;
			r.ultimo = ahoraMs;
			return;
		}
		if (r.llegada >= 0.0) {
			cerrarLlegada(r);
			const double iv = ahoraMs - r.llegada;
			r.candidato = iv >= kSaltoMinMs && iv < kHuecoMaxMs && r.tipicoMs > 0.0 && iv >= 3.0 * r.tipicoMs;
			if (iv < kActivoMs) {
				r.iv[static_cast<size_t>(r.ivPos)] = static_cast<float>(iv);
				r.ivPos = (r.ivPos + 1) % kIntervalos;
				if (r.ivN < kIntervalos) r.ivN++;
				if (!r.candidato) r.tipicoMs = r.tipicoMs <= 0.0 ? iv : r.tipicoMs + (iv - r.tipicoMs) / 16.0;
			}
		}
		r.llegada = ahoraMs;
		r.enLlegada = 1;
		r.abierta = true;
		r.ultimo = ahoraMs;
	}

	void MedidorMando::tick(double ahoraMs) {
		for (Ranura& r : _r) {
			if (!r.usada) continue;
			avanzarSegundo(r, ahoraMs);
			if (r.abierta && ahoraMs - r.llegada > kMismaLlegadaMs) cerrarLlegada(r);
		}
	}

	std::vector<MedidaMando> MedidorMando::medir(double ahoraMs) const {
		std::vector<MedidaMando> out;
		for (const Ranura& r : _r) {
			if (!r.usada) continue;
			MedidaMando m;
			m.userId = r.userId;
			m.muestras = r.ivN;
			m.ultimoHaceMs = r.ultimo >= 0.0 ? ahoraMs - r.ultimo : -1.0;
			m.ritmo = ahoraMs - r.segundo >= 2000.0 ? 0 : r.ritmo;
			for (int i = 0; i < r.saltosN; i++) if (ahoraMs - r.saltos[static_cast<size_t>(i)] <= 60000.0) m.saltosMin++;
			if (r.ivN >= 8) {
				std::array<float, kIntervalos> orden{};
				const int inicio = (r.ivPos - r.ivN + kIntervalos) % kIntervalos;
				double suma = 0.0, var = 0.0;
				for (int k = 0; k < r.ivN; k++) {
					const float v = r.iv[static_cast<size_t>((inicio + k) % kIntervalos)];
					orden[static_cast<size_t>(k)] = v;
					suma += v;
					if (k > 0) var += std::fabs(v - r.iv[static_cast<size_t>((inicio + k - 1) % kIntervalos)]);
				}
				m.mediaMs = suma / r.ivN;
				m.jitterMs = var / (r.ivN - 1);
				std::sort(orden.begin(), orden.begin() + r.ivN);
				const int idx = (std::min)(r.ivN - 1, static_cast<int>(std::ceil(0.95 * r.ivN)) - 1);
				m.p95Ms = orden[static_cast<size_t>(idx)];
			}
			out.push_back(m);
		}
		return out;
	}

	void MedidorMando::olvidar(uint32_t userId) {
		for (Ranura& r : _r) if (r.usada && r.userId == userId) r = Ranura();
	}

	void MedidorMando::limpiar() {
		for (Ranura& r : _r) r = Ranura();
	}

	// ---- Evaluación ---------------------------------------------------------

	Evaluacion evaluar(const MedidaMando* m, const DatosParsec& p, const UmbralesRetraso& u) {
		Evaluacion e;
		const bool hayMando = m != nullptr && m->muestras >= 8;
		int nivel = -1;
		auto subir = [&](int n, const char* motivo) {
			if (n > nivel) { nivel = n; e.motivo = n > 0 ? motivo : ""; }
		};
		if (p.pingMs >= 0) {
			const double jitter = hayMando ? m->jitterMs : 0.0;
			const double video = p.codificarMs + p.decodificarMs;
			e.totalMs = static_cast<int>(std::lround(p.pingMs + video + jitter));
			const int n = e.totalMs <= u.totalVerde ? 0 : e.totalMs <= u.totalAmbar ? 1 : 2;
			// La causa: lo que más pesa del total
			const char* motivo = "ping alto";
			if (jitter >= p.pingMs && jitter >= video) motivo = "mando irregular";
			else if (video > p.pingMs) motivo = "vídeo lento (codificar/decodificar)";
			subir(n, motivo);
		}
		if (p.perdidaPct >= u.perdidaRojo) subir(2, "pierde paquetes");
		else if (p.perdidaPct >= u.perdidaAmbar) subir(1, "pierde paquetes");
		if (m != nullptr) {
			if (m->saltosMin >= u.saltosRojo) subir(2, "mando a saltos");
			else if (m->saltosMin >= u.saltosAmbar) subir(1, "mando a saltos");
		}
		if (nivel < 0 || (nivel == 0 && p.pingMs < 0)) { e.semaforo = "sin_datos"; e.motivo.clear(); return e; }
		static const char* nombres[] = { "verde", "ambar", "rojo" };
		e.semaforo = nombres[nivel];
		return e;
	}

	// ---- Pérdida de paquetes ------------------------------------------------

	double PerdidaPaquetes::registrar(uint32_t id, uint32_t enviados, uint32_t reenviados) {
		Muestras* s = nullptr;
		for (Muestras& x : _m) if (x.id == id) s = &x;
		if (s == nullptr) { _m.push_back(Muestras()); s = &_m.back(); s->id = id; }
		constexpr int K = 10;
		bool nueva = true;
		if (s->n > 0) {
			const int ult = (s->pos - 1 + K) % K;
			if (enviados < s->e[static_cast<size_t>(ult)]) s->n = 0;                 // reconexión: contadores a cero
			else if (enviados == s->e[static_cast<size_t>(ult)]) nueva = false;    // Parsec aún no actualizó
		}
		if (nueva) {
			s->e[static_cast<size_t>(s->pos)] = enviados;
			s->r[static_cast<size_t>(s->pos)] = reenviados;
			s->pos = (s->pos + 1) % K;
			if (s->n < K) s->n++;
		}
		if (s->n < 2) return 0.0;
		const size_t viejo = static_cast<size_t>((s->pos - s->n + K) % K);
		const size_t nuevo = static_cast<size_t>((s->pos - 1 + K) % K);
		const uint32_t dE = s->e[nuevo] - s->e[viejo];
		if (dE < 50) return 0.0;
		const uint32_t dR = s->r[nuevo] >= s->r[viejo] ? s->r[nuevo] - s->r[viejo] : 0;
		return 100.0 * dR / dE;
	}

	void PerdidaPaquetes::olvidar(uint32_t id) {
		_m.erase(std::remove_if(_m.begin(), _m.end(), [id](const Muestras& x) { return x.id == id; }), _m.end());
	}

	// ---- Avisos en el chat --------------------------------------------------

	bool AvisosRetraso::actualizar(uint32_t id, const std::string& semaforo, bool enPartido, double ahoraSeg) {
		Jugador* j = nullptr;
		for (Jugador& x : _j) if (x.id == id) j = &x;
		if (j == nullptr) { _j.push_back(Jugador()); j = &_j.back(); j->id = id; }
		if (!enPartido || semaforo != "rojo") { j->rojoDesde = -1.0; return false; }
		if (j->rojoDesde < 0.0) j->rojoDesde = ahoraSeg;
		if (ahoraSeg - j->rojoDesde < kRojoSeguido) return false;
		if (ahoraSeg - j->avisado < kPorJugador || ahoraSeg - _ultimoGlobal < kGlobal) return false;
		j->avisado = ahoraSeg;
		_ultimoGlobal = ahoraSeg;
		return true;
	}

}
