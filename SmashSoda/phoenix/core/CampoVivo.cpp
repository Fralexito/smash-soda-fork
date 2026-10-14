#include "CampoVivo.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

namespace phoenix::campo {

	namespace {
		uint32_t u32(const uint8_t* p, uint32_t off) { uint32_t v; std::memcpy(&v, p + off, 4); return v; }
		bool vivo(const uint8_t* obj) { return u32(obj, kOffReg0) == 0x10u && u32(obj, kOffReg3) == 0xDEADu && (u32(obj, kOffCodigo) & 0xFFFFu) == 3u; }
		bool vec4(const uint8_t* p, float& x, float& y, float& z) {
			float v[4];
			std::memcpy(v, p, 16);
			if (!(v[3] == 1.0f)) return false;
			x = v[0]; y = v[1]; z = v[2];
			return true;
		}
	}

	bool posicionValida(float x, float y, float z) {
		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
		return std::fabs(x) <= 57.0f && std::fabs(z) <= 50.0f && y > 0.0f && y < 3.0f; // en la tanda de penales los que esperan se colocan fuera de la banda (z ≈ 40-45)
	}

	bool posicionBalonValida(float x, float y, float z) {
		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
		if (y < 0.0f || y >= 25.0f || std::fabs(x) > 60.0f) return false;
		return std::fabs(z) <= 40.0f || (std::fabs(z) >= 95.0f && std::fabs(z) <= 105.0f); // aparcado durante repeticiones
	}

	bool leerActor(const uint8_t* obj, Actor& out) {
		if (u32(obj, kOffTipo) != 0x18) return false;
		const uint32_t ladoC = u32(obj, kOffLado);
		if (ladoC != 2 && ladoC != 0) return false;   // 2 durante el partido, 0 en la tanda de penales / tras el pitido
		if (u32(obj, kOffMarca) != 0x80002u) return false;
		if (!vivo(obj)) return false;
		float x, y, z;
		if (!vec4(obj + kOffPosicion, x, y, z) || !posicionValida(x, y, z)) return false;
		out.codigo = u32(obj, kOffCodigo) >> 16; out.clase = Clase::Campo;
		out.x = x; out.y = y; out.z = z;
		return true;
	}

	bool leerPortero(const uint8_t* obj, Actor& out) {
		if (u32(obj, kOffTipo) != 0x18) return false;
		const uint32_t ladoP = u32(obj, kOffLado);
		if (ladoP != 1 && ladoP != 0) return false;   // 1 durante el partido, 0 en la tanda de penales
		const uint32_t marca = u32(obj, kOffMarca);
		if (marca != 0x80001u && marca != 0x10001u && marca != 0x10002u) return false;   // 0x10002: visto en un partido real (14-oct)
		if (!vivo(obj)) return false;
		float x, y, z;
		if (!vec4(obj + kOffPosicion, x, y, z) || !posicionValida(x, y, z)) return false;
		out.codigo = u32(obj, kOffCodigo) >> 16; out.clase = Clase::Portero;
		out.x = x; out.y = y; out.z = z;
		return true;
	}

	bool leerBalon(const uint8_t* obj, Actor& out) {
		if (u32(obj, kOffTipo) != 0) return false;
		if (u32(obj, kOffLado) != 1) return false;
		if (u32(obj, kOffMarca) != 0x10001u) return false;
		if (!vivo(obj)) return false;
		float x, y, z;
		if (!vec4(obj + kOffPosicion, x, y, z) || !posicionBalonValida(x, y, z)) return false;
		out.codigo = u32(obj, kOffCodigo) >> 16; out.clase = Clase::Balon;
		out.x = x; out.y = y; out.z = z;
		return true;
	}

	bool leerPosicion(const uint8_t* obj16, float& x, float& y, float& z) {
		return vec4(obj16, x, y, z) && posicionValida(x, y, z);
	}

	bool leerPosicionBalon(const uint8_t* obj16, float& x, float& y, float& z) {
		return vec4(obj16, x, y, z) && posicionBalonValida(x, y, z);
	}

	std::vector<Actor> elegirLista(const std::vector<Actor>& todos) {
		std::map<uint32_t, std::vector<Actor>> porCodigo;
		for (const Actor& a : todos) if (a.clase == Clase::Campo) porCodigo[a.codigo].push_back(a);
		const std::vector<Actor>* mejor = nullptr;
		for (const auto& par : porCodigo) {
			const size_t n = par.second.size();
			if (n < 18 || n > 24) continue;
			if (!mejor || n > mejor->size()) mejor = &par.second;
		}
		if (!mejor) return {};
		std::vector<Actor> r = *mejor;
		std::sort(r.begin(), r.end(), [](const Actor& a, const Actor& b) { return a.dir < b.dir; }); // orden estable entre lecturas
		return r;
	}

	std::vector<Actor> elegirPorteros(const std::vector<Actor>& todos) {
		std::vector<Actor> r;
		bool izq = false, der = false;
		std::vector<Actor> orden = todos;
		std::sort(orden.begin(), orden.end(), [](const Actor& a, const Actor& b) { return a.dir < b.dir; });
		for (const Actor& a : orden) {
			if (a.clase != Clase::Portero) continue;
			if (a.x < 0 && !izq) { izq = true; r.push_back(a); }
			else if (a.x > 0 && !der) { der = true; r.push_back(a); }
		}
		return r;
	}

	bool pareceAsistente(const Actor& a) {
		const float z = std::fabs(a.z);
		return z >= 34.8f && z <= 36.5f;
	}

	bool esSaqueDeCentro(const std::vector<Actor>& lista, float maxDesplazamiento) {
		if (maxDesplazamiento >= 0.05f) return false;
		int izq = 0, der = 0, centro = 0; double sz = 0, sz2 = 0; int n = 0;
		for (const Actor& a : lista) {
			if (a.clase != Clase::Campo || pareceAsistente(a)) continue;
			// Los rivales se pegan a la línea de medio campo (x ≈ 1-2 m): «centro» es solo el círculo del balón
			if (std::sqrt(a.x * a.x + a.z * a.z) < 2.5f) centro++; else if (a.x < 0) izq++; else der++;
			sz += a.z; sz2 += (double)a.z * a.z; n++;
		}
		if (n < 16 || izq < 7 || izq > 11 || der < 7 || der > 11 || centro > 3) return false;
		const double var = sz2 / n - (sz / n) * (sz / n);
		return var >= 81.0; // desviación en z ≥ 9 m: formación abierta, no un corro
	}

	std::vector<int> repartirEquipos(const std::vector<Actor>& lista) {
		std::vector<int> eq(lista.size(), kEqDesconocido);
		// El árbitro: en el saque se coloca clavado en (±4.8, ±10.0) (dos partidos, misma décima). Tolerancia corta
		// porque un rival puede quedar cerca (se vio uno en (4.0, -9.3)); si hay varios, el más exacto.
		int iArb = -1; float mejorArb = 1e9f;
		for (size_t i = 0; i < lista.size(); i++) {
			const Actor& a = lista[i];
			if (a.clase != Clase::Campo || pareceAsistente(a)) continue;
			const float d = std::fabs(std::fabs(a.x) - 4.8f) + std::fabs(std::fabs(a.z) - 10.0f);
			if (d <= 0.7f && d < mejorArb) { mejorArb = d; iArb = (int)i; }
		}
		int nIzq = 0, nDer = 0;
		for (size_t i = 0; i < lista.size(); i++) {
			const Actor& a = lista[i];
			if (a.clase == Clase::Balon) continue;
			if (pareceAsistente(a) || (int)i == iArb) { eq[i] = kEqArbitro; continue; }
			const float d = std::sqrt(a.x * a.x + a.z * a.z);
			if (d < 2.5f) continue;           // los del círculo central se deciden al final
			eq[i] = a.x < 0 ? kEqA : kEqB;
			if (a.clase == Clase::Campo) { if (a.x < 0) nIzq++; else nDer++; }
		}
		const int saca = nIzq <= nDer ? kEqA : kEqB; // el que saca tiene a uno (o dos) en el centro y, por tanto, menos en su mitad
		for (size_t i = 0; i < lista.size(); i++) if (eq[i] == kEqDesconocido && lista[i].clase != Clase::Balon) eq[i] = saca;
		return eq;
	}

	bool exeConocido(uint64_t tamano, const std::string& sha256) {
		// PES2021.exe del parche ConmeGOL (y del SP2026): el mismo archivo.
		return tamano == 458806784ull && sha256 == "5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A";
	}

}

// ---- Tanda de penales --------------------------------------------------------------------------------
namespace phoenix::campo {

	namespace {
		constexpr float kPuntoX = 41.5f, kGolX = 52.5f, kMedioPalo = 3.66f, kLarguero = 2.44f;
		bool enElPunto(float bx, float by, float bz, float& ladoX) {
			if (by >= 0.25f || std::fabs(bz) > 0.7f) return false;
			if (std::fabs(bx + kPuntoX) <= 0.8f) { ladoX = -1; return true; }
			if (std::fabs(bx - kPuntoX) <= 0.8f) { ladoX = +1; return true; }
			return false;
		}
		/// Los demás esperan (círculo central o fuera de la banda): así se distingue de un penal en pleno partido.
		bool esperan(const std::vector<Actor>& lista) {
			int n = 0;
			for (const Actor& a : lista) if (a.clase == Clase::Campo && (std::sqrt(a.x * a.x + a.z * a.z) < 12.0f || std::fabs(a.z) > 34.0f)) n++;
			return n >= 12;
		}
		int equipoDelPateador(float bx, float bz, const std::vector<Actor>& lista, const std::vector<int>& eq) {
			int mejor = kEqDesconocido; float dm = 4.0f;
			for (size_t i = 0; i < lista.size() && i < eq.size(); i++) {
				const Actor& a = lista[i];
				if (a.clase != Clase::Campo || a.y < 0.3f || eq[i] == kEqArbitro) continue;
				const float d = std::sqrt((a.x - bx) * (a.x - bx) + (a.z - bz) * (a.z - bz));
				if (d < dm) { dm = d; mejor = eq[i]; }
			}
			return mejor;
		}
	}

	void Tanda::terminar(bool gol, double t) {
		Lanzamiento l; l.eq = pateadorEq_; l.gol = gol;
		e_.lanzamientos.push_back(l);
		e_.enVuelo = false; e_.listo = false; e_.turnoEq = kEqDesconocido;
		fase_ = Fase::Nada; tUltimo_ = t; tListo_ = 0;
	}

	void Tanda::alimentar(bool hayBola, float bx, float by, float bz, const std::vector<Actor>& lista, const std::vector<int>& eq, double t) {
		if (e_.activa && fase_ == Fase::Nada && t - tUltimo_ > 150.0) { e_ = {}; }   // 2,5 min sin lanzamientos: se acabó
		float lado = 0;
		const bool punto = hayBola && enElPunto(bx, by, bz, lado);
		if (fase_ == Fase::Nada) {
			if (punto && esperan(lista)) {
				if (tListo_ == 0) tListo_ = t;
				if (t - tListo_ >= 0.5) { fase_ = Fase::Listo; ladoX_ = lado; e_.activa = true; e_.listo = true; tUltimo_ = t; }
			}
			else tListo_ = 0;
			if (fase_ == Fase::Nada) return;
		}
		if (fase_ == Fase::Listo) {
			if (punto) { e_.turnoEq = equipoDelPateador(bx, bz, lista, eq); e_.listo = true; return; }
			if (!hayBola) return;                 // el balón se aparcó sin golpeo (repetición): sigue listo
			// se movió: en vuelo
			pateadorEq_ = e_.turnoEq; fase_ = Fase::EnVuelo; e_.enVuelo = true; e_.listo = false; tVuelo_ = t; cruzo_ = false; golCruce_ = false;
		}
		if (fase_ == Fase::EnVuelo) {
			if (hayBola && !cruzo_ && bx * ladoX_ >= kGolX) { cruzo_ = true; golCruce_ = std::fabs(bz) < kMedioPalo && by < kLarguero; }
			if (hayBola && std::fabs(bz) > 60.0f) { terminar(cruzo_ && golCruce_, t); return; }                   // aparcado: fin
			if (punto && t - tVuelo_ > 1.0) { terminar(cruzo_ && golCruce_, t); tListo_ = t; return; }            // volvió al punto: siguiente
			if (hayBola && bx * ladoX_ < kPuntoX - 6.0f) { terminar(cruzo_ && golCruce_, t); return; }           // se fue hacia el centro: no hubo gol
			if (t - tVuelo_ > 10.0) { terminar(cruzo_ && golCruce_, t); return; }
		}
	}

	// ---- Poseedor del balón (histéresis) ----
	namespace {
		constexpr float kAdquirir = 1.4f, kSoltar = 2.8f, kDisputa = 1.0f, kDisputaActual = 1.8f;
		constexpr float kAlturaMax = 2.4f;     // por encima el balón va por el aire: nadie lo «lleva»
		constexpr float kVelLanzado = 13.0f;   // m/s: más rápido es un pase largo o un tiro
		constexpr double kSosten = 0.35, kContactoMin = 0.08;
	}

	int Poseedor::actualizar(const std::vector<PuntoCampo>& j, bool hayBola, float bx, float by, float bz, double t) {
		const double dt = (tUltimo_ > 0 && t > tUltimo_) ? (t - tUltimo_) : 0.0;
		tUltimo_ = t;
		if (poseedor_ >= static_cast<int>(j.size())) poseedor_ = -1;
		if (!hayBola) {
			previo_ = false; candidato_ = -1;
			if (poseedor_ >= 0) { sinContacto_ += dt; if (sinContacto_ > kSosten) poseedor_ = -1; }
			return poseedor_;
		}
		// velocidad horizontal del balón (la lectura anterior debe ser reciente)
		float vb = 0;
		if (previo_ && dt > 0 && dt < 0.5) vb = std::sqrt((bx - px_) * (bx - px_) + (bz - pz_) * (bz - pz_)) / static_cast<float>(dt);
		previo_ = true; px_ = bx; pz_ = bz;

		auto dist = [&](int i) { const float dx = j[static_cast<size_t>(i)].x - bx, dz = j[static_cast<size_t>(i)].z - bz; return std::sqrt(dx * dx + dz * dz); };
		int mejor = -1; float dMejor = 1e9f;
		for (size_t i = 0; i < j.size(); i++) {
			if (!j[i].elegible) continue;
			const float d = dist(static_cast<int>(i));
			if (d < dMejor) { dMejor = d; mejor = static_cast<int>(i); }
		}
		const bool aire = by > kAlturaMax;
		const bool lanzado = vb > kVelLanzado;

		if (poseedor_ >= 0) {
			const float dh = dist(poseedor_);
			if (aire || (lanzado && dh > 1.6f) || dh > kSoltar) sinContacto_ += dt; else sinContacto_ = 0;
			if (mejor >= 0 && mejor != poseedor_ && dMejor <= kDisputa && dh > kDisputaActual && !aire) { poseedor_ = mejor; sinContacto_ = 0; candidato_ = -1; }
			else if (sinContacto_ > kSosten) { poseedor_ = -1; candidato_ = -1; }
			return poseedor_;
		}
		// balón libre: ¿alguien lo controla de forma sostenida?
		if (mejor >= 0 && dMejor <= kAdquirir && !aire && !lanzado) {
			if (candidato_ != mejor) { candidato_ = mejor; tCandidato_ = t; }
			if (t - tCandidato_ >= kContactoMin) { poseedor_ = mejor; sinContacto_ = 0; candidato_ = -1; }
		}
		else candidato_ = -1;
		return poseedor_;
	}

	// ---- Estadísticas derivadas ----
	void Estadisticas::alimentar(double t, bool hayBola, float bx, float by, float bz, int poseedor, const std::vector<int>& eq, const std::vector<PuntoCampo>& j, int ladoDefensaA) {
		const double dt = (tUltimo_ > 0 && t > tUltimo_) ? (t - tUltimo_) : 0.0;
		tUltimo_ = t;
		if (!hayBola) { previo_ = false; return; }
		float vx = 0, vz = 0;
		if (previo_ && dt > 0 && dt < 0.5) { vx = (bx - px_) / static_cast<float>(dt); vz = (bz - pz_) / static_cast<float>(dt); }
		previo_ = true; px_ = bx; pz_ = bz;
		const float vel = std::sqrt(vx * vx + vz * vz);

		const int eqPos = (poseedor >= 0 && poseedor < static_cast<int>(eq.size())) ? eq[static_cast<size_t>(poseedor)] : -1;
		const int eqP = (eqPos == 0 || eqPos == 1) ? eqPos : -1;
		// posesión, pases, pérdidas y recuperaciones
		if (eqP >= 0) {
			e_.tPosesion[eqP] += dt;
			if (poseedor != piezaAnterior_) {
				if (eqAnterior_ >= 0 && t - tSoltado_ <= 3.0) {
					if (eqAnterior_ == eqP) e_.pases[eqP]++;
					else { e_.perdidas[eqAnterior_]++; e_.recuperaciones[eqP]++; }
				}
				piezaAnterior_ = poseedor; eqAnterior_ = eqP;
			}
			ultimoEq_ = eqP;
			tSoltado_ = t;   // mientras lo lleva, «el momento en que lo soltó» es ahora
		}
		// tiro: balón lanzado hacia la portería rival, poco después de que lo llevara alguien
		if (eqP < 0 && ultimoEq_ >= 0 && ladoDefensaA != 0 && vel > 14.0f && t - tSoltado_ < 0.6 && t - tTiro_ > 3.0 && std::fabs(vx) > 1e-3f) {
			const int ataca = ultimoEq_ == 0 ? -ladoDefensaA : ladoDefensaA;   // +1 = ataca hacia x > 0
			const float golX = 52.5f * static_cast<float>(ataca);
			if (vx * static_cast<float>(ataca) > 0 && std::fabs(golX - bx) <= 32.0f) {
				const float zLinea = bz + vz * ((golX - bx) / vx);
				if (std::fabs(zLinea) < 7.0f) {
					e_.tiros[ultimoEq_]++;
					if (std::fabs(zLinea) < 4.2f) e_.tirosPuerta[ultimoEq_]++;
					tTiro_ = t;
				}
			}
		}
		// balón fuera: córner, saque de puerta, saque de banda (un gol no cuenta)
		const bool golPosible = std::fabs(bz) < 3.7f && by < 2.6f && std::fabs(bx) > 52.4f;
		const bool saleFondo = std::fabs(bx) > 52.8f, saleBanda = std::fabs(bz) > 34.3f;
		if (!fuera_ && (saleFondo || saleBanda) && !golPosible && ultimoEq_ >= 0 && ladoDefensaA != 0) {
			fuera_ = true;
			const int otro = 1 - ultimoEq_;
			if (saleFondo) {
				// ¿de quién es la portería por la que sale? A defiende x = 52,5 * ladoDefensaA
				const int equipoDefensor = (bx * static_cast<float>(ladoDefensaA) > 0) ? 0 : 1;
				if (ultimoEq_ == equipoDefensor) e_.corners[otro]++;       // lo tocó el que defiende: córner para el rival
				else e_.saquesMeta[equipoDefensor]++;                      // lo tocó el atacante: saque de puerta
			}
			else e_.saquesBanda[otro]++;
		}
		else if (fuera_ && std::fabs(bx) < 52.0f && std::fabs(bz) < 33.8f) {
			tDentro_ += dt;
			if (tDentro_ > 0.5) { fuera_ = false; tDentro_ = 0; }
		}
		else if (fuera_) tDentro_ = 0;
		(void)j;
	}

}
