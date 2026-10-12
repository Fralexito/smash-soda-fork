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
		if (marca != 0x80001u && marca != 0x10001u) return false;
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

}
