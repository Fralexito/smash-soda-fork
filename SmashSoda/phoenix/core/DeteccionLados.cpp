#include "DeteccionLados.h"

#include <cmath>

namespace phoenix::lados {

	void Detector::seguir(double t, const std::vector<Ficha>& fichas) {
		// Une cada fichita con la más cercana del cuadro anterior (mismo equipo, a la velocidad máxima de un futbolista)
		const double dt = tPrev_ < 0 ? 0 : t - tPrev_;
		const bool dtValido = dt >= 0.02 && dt <= 0.3;
		std::vector<Pista> nuevas;
		std::vector<bool> usada(pistas_.size(), false);
		for (const Ficha& f : fichas) {
			if (f.eq != 0 && f.eq != 1) continue;
			Pista p{ 0, f.x, f.z, 0, 0, f.eq, false };
			int mejor = -1; float dm = dtValido ? static_cast<float>(10.0 * dt + 0.15) : 0.0f;
			for (size_t i = 0; i < pistas_.size(); i++) {
				if (usada[i] || pistas_[i].eq != f.eq) continue;
				const float d = std::hypot(f.x - pistas_[i].x, f.z - pistas_[i].z);
				if (d <= dm) { dm = d; mejor = static_cast<int>(i); }
			}
			if (mejor >= 0) {
				usada[static_cast<size_t>(mejor)] = true;
				const Pista& a = pistas_[static_cast<size_t>(mejor)];
				p.id = a.id; p.vx = static_cast<float>((f.x - a.x) / dt); p.vz = static_cast<float>((f.z - a.z) / dt); p.conVel = true;
			}
			else p.id = sigId_++;
			nuevas.push_back(p);
		}
		pistas_ = std::move(nuevas);
		tPrev_ = t;
	}

	void Detector::alimentar(double t, const std::vector<Ficha>& fichas, const std::vector<Stick>& sticks) {
		const double dtAnt = tPrev_ < 0 ? 0 : t - tPrev_;
		seguir(t, fichas);
		const double decae = dtAnt > 0 && dtAnt <= 0.3 ? std::exp(-dtAnt / kTau) : 0.0;
		const double peso = 1.0 - (decae > 0 ? decae : std::exp(-0.05 / kTau));   // normaliza: correlación sostenida 1 → S ≈ 1

		for (const Stick& s : sticks) {
			PorAsiento& pa = asientos_[s.asiento];
			pa.hist.push_back({ t, s.lx, s.ly });
			while (pa.hist.size() > 2 && pa.hist[1].t <= t - kRetraso) pa.hist.pop_front();
			// Stick de hace kRetraso (o el más viejo que haya)
			const Muestra& m = pa.hist.front();
			const float mag = std::hypot(m.lx, m.ly);
			const bool apretado = mag >= kStickMin && t - m.t >= kRetraso * 0.6;
			if (apretado) pa.apretado++;
			for (int o = 0; o < 4; o++) {
				const float dx = (o & 1 ? -m.lx : m.lx) / (mag > 0 ? mag : 1), dz = (o & 2 ? -m.ly : m.ly) / (mag > 0 ? mag : 1);
				std::map<int, double> S;
				double mejorS = -1; int mejorEq = -1;
				for (const Pista& p : pistas_) {
					double previo = 0;
					auto it = pa.S[o].find(p.id);
					if (it != pa.S[o].end()) previo = it->second;
					double c = 0;
					const float v = std::hypot(p.vx, p.vz);
					if (apretado && p.conVel && v >= kVelMin) c = (p.vx * dx + p.vz * dz) / v;
					const double nuevo = previo * decae + c * peso;
					S[p.id] = nuevo;
					if (nuevo > mejorS) { mejorS = nuevo; mejorEq = p.eq; }
				}
				pa.S[o] = std::move(S);
				if (!apretado) continue;
				if (mejorS >= kCorrMin && mejorEq >= 0) pa.votos[o][mejorEq]++;
			}
		}
	}

	int Detector::orientacion() const {
		int mejor = 0; double q[4] = {};
		for (const auto& par : asientos_) for (int o = 0; o < 4; o++) q[o] += par.second.votos[o][0] + par.second.votos[o][1];   // la orientación buena es la que más votos da
		for (int o = 1; o < 4; o++) if (q[o] > q[mejor]) mejor = o;
		return mejor;
	}

	bool Detector::orientacionClara() const {
		double q[4] = {};
		for (const auto& par : asientos_) for (int o = 0; o < 4; o++) q[o] += par.second.votos[o][0] + par.second.votos[o][1];   // la orientación buena es la que más votos da
		const int m = orientacion();
		double segunda = 0;
		for (int o = 0; o < 4; o++) if (o != m && q[o] > segunda) segunda = q[o];
		return q[m] > 0 && q[m] >= 1.5 * segunda;
	}

	std::vector<Veredicto> Detector::veredictos() const {
		std::vector<Veredicto> r;
		const int o = orientacion();
		const bool clara = orientacionClara();
		for (const auto& par : asientos_) {
			Veredicto v; v.asiento = par.first;
			v.votosA = par.second.votos[o][0]; v.votosB = par.second.votos[o][1];
			const int total = v.votosA + v.votosB;
			if (total > 0) {
				v.eq = v.votosA >= v.votosB ? 0 : 1;
				v.proporcion = static_cast<double>(v.eq == 0 ? v.votosA : v.votosB) / total;
			}
			v.apretado = par.second.apretado;
			v.decidido = clara && total >= kVotosMin && v.proporcion >= kProporcionMin && total >= kTasaMin * v.apretado;
			if (total == 0) v.eq = -1;
			r.push_back(v);
		}
		return r;
	}

	// ---- Ancla: qué portería defiende cada equipo y quién marcó ------------------------------------

	void LadoDefendido::alimentar(double t, const std::vector<Ficha>& fichas) {
		const double dt = tPrev_ < 0 ? 0.05 : t - tPrev_;
		tPrev_ = t;
		if (dt <= 0 || dt > 1.0) return;
		const double decae = std::exp(-dt / 20.0);
		bool hay[2][2] = {};
		for (const Ficha& f : fichas) {
			if (f.eq != 0 && f.eq != 1) continue;
			if (std::fabs(f.x) >= 46.0f && std::fabs(f.z) <= 10.0f) hay[f.eq][f.x > 0 ? 1 : 0] = true;   // dentro del área chica ampliada
		}
		for (int e = 0; e < 2; e++) for (int l = 0; l < 2; l++) p_[e][l] = p_[e][l] * decae + (hay[e][l] ? 1.0 - decae : 0.0);
	}

	int LadoDefendido::lado(int eq) const {
		if (eq != 0 && eq != 1) return 0;
		const double izq = p_[eq][0], der = p_[eq][1];
		// el portero está casi siempre en su área: presencia alta en un lado y claramente menor en el otro
		if (izq >= 0.5 && izq >= 2.0 * der) return -1;
		if (der >= 0.5 && der >= 2.0 * izq) return +1;
		return 0;
	}

	int equipoQueMarco(int ladoPorteria, const LadoDefendido& ld) {
		if (ladoPorteria != -1 && ladoPorteria != 1) return -1;
		const int a = ld.lado(0), b = ld.lado(1);
		if (a == 0 || b == 0 || a == b) return -1;
		return a == ladoPorteria ? 1 : 0;
	}

	int porteriaDelGol(float bx, float by, float bz) {
		if (!std::isfinite(bx) || !std::isfinite(by) || !std::isfinite(bz)) return 0;
		if (std::fabs(bx) < 52.5f || std::fabs(bx) > 56.0f || std::fabs(bz) >= 3.66f || by >= 2.44f) return 0;
		return bx > 0 ? 1 : -1;
	}

}
