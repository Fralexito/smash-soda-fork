#include "MapeoMandos.h"

#include <cmath>

namespace phoenix::mapeo {

	using json = nlohmann::json;

	namespace {
		// Índice = bit XInput (0x1 arriba … 0x8000 Y), 11 no se usa; 16/17 = gatillos.
		const char* const NOMBRES[FUENTES] = {
			"arriba", "abajo", "izq", "der", "start", "back", "ls", "rs",
			"lb", "rb", "guia", "b11", "a", "b", "x", "y", "lt", "rt",
		};
		const char* const EJES[4] = { "lx", "ly", "rx", "ry" };

		inline int16_t limitar(int32_t v) {
			return static_cast<int16_t>(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
		}
		inline int16_t negar(int16_t v) { return limitar(-static_cast<int32_t>(v)); }

		void zonaMuerta(int16_t& x, int16_t& y, uint8_t porcentaje) {
			if (porcentaje == 0) return;
			const double zona = 32767.0 * (porcentaje > ZONA_MAX ? ZONA_MAX : porcentaje) / 100.0;
			const double fx = x, fy = y, mag = std::sqrt(fx * fx + fy * fy);
			if (mag <= zona) { x = 0; y = 0; return; }
			const double f = (mag - zona) / (32767.0 - zona) * 32767.0 / mag;   // reescala: sin salto al salir de la zona
			x = limitar(static_cast<int32_t>(std::lround(fx * f)));
			y = limitar(static_cast<int32_t>(std::lround(fy * f)));
		}
	}

	Perfil::Perfil() {
		for (int i = 0; i < FUENTES; i++) destino[i] = 1u << i;
	}

	bool Perfil::esIdentidad() const {
		for (int i = 0; i < FUENTES; i++) if (destino[i] != (1u << i)) return false;
		for (bool v : invertir) if (v) return false;
		return !cambiarSticks && zona[0] == 0 && zona[1] == 0;
	}

	Estado aplicar(const Perfil& p, const Estado& e) {
		Estado s;
		// Botones: cada bit pulsado manda su destino (botones y/o gatillo a tope)
		uint32_t salida = 0;
		for (uint32_t b = e.b, i = 0; b != 0; b >>= 1, i++)
			if (b & 1u) salida |= p.destino[i];
		s.b = static_cast<uint16_t>(salida & 0xFFFFu);
		s.lt = (salida & SALIDA_LT) ? 255 : 0;
		s.rt = (salida & SALIDA_RT) ? 255 : 0;
		// Gatillos: a gatillo pasan analógicos; a botón, desde el umbral
		const uint8_t gat[2] = { e.lt, e.rt };
		for (int g = 0; g < 2; g++) {
			const uint8_t v = gat[g];
			if (v == 0) continue;
			const uint32_t d = p.destino[FUENTE_LT + g];
			if ((d & SALIDA_LT) && v > s.lt) s.lt = v;
			if ((d & SALIDA_RT) && v > s.rt) s.rt = v;
			if (v >= UMBRAL_GATILLO) s.b |= static_cast<uint16_t>(d & 0xFFFFu);
		}
		// Sticks
		if (p.cambiarSticks) { s.lx = e.rx; s.ly = e.ry; s.rx = e.lx; s.ry = e.ly; }
		else { s.lx = e.lx; s.ly = e.ly; s.rx = e.rx; s.ry = e.ry; }
		if (p.invertir[LX]) s.lx = negar(s.lx);
		if (p.invertir[LY]) s.ly = negar(s.ly);
		if (p.invertir[RX]) s.rx = negar(s.rx);
		if (p.invertir[RY]) s.ry = negar(s.ry);
		zonaMuerta(s.lx, s.ly, p.zona[0]);
		zonaMuerta(s.rx, s.ry, p.zona[1]);
		return s;
	}

	const char* nombre(int i) { return (i >= 0 && i < FUENTES) ? NOMBRES[i] : ""; }

	int indicePorNombre(const std::string& n) {
		for (int i = 0; i < FUENTES; i++) if (n == NOMBRES[i]) return i;
		return -1;
	}

	json aJson(const Perfil& p) {
		json botones = json::object();
		for (int i = 0; i < FUENTES; i++) {
			if (p.destino[i] == (1u << i)) continue;
			json a = json::array();
			for (int k = 0; k < FUENTES; k++) if (p.destino[i] & (1u << k)) a.push_back(NOMBRES[k]);
			botones[NOMBRES[i]] = a;   // [] = desactivado
		}
		json inv = json::array();
		for (int k = 0; k < 4; k++) if (p.invertir[k]) inv.push_back(EJES[k]);
		return { {"botones", botones}, {"invertir", inv}, {"cambiarSticks", p.cambiarSticks},
			{"zona", json::array({ p.zona[0], p.zona[1] })} };
	}

	Perfil deJson(const json& j) {
		Perfil p;
		if (!j.is_object()) return p;
		if (auto it = j.find("botones"); it != j.end() && it->is_object()) {
			for (auto& [k, v] : it->items()) {
				const int f = indicePorNombre(k);
				if (f < 0 || f == 11 || !v.is_array()) continue;
				uint32_t d = 0;
				for (auto& n : v) {
					if (!n.is_string()) continue;
					const int s = indicePorNombre(n.get<std::string>());
					if (s >= 0 && s != 11) d |= 1u << s;
				}
				p.destino[f] = d;
			}
		}
		if (auto it = j.find("invertir"); it != j.end() && it->is_array()) {
			for (auto& n : *it) {
				if (!n.is_string()) continue;
				for (int k = 0; k < 4; k++) if (n.get<std::string>() == EJES[k]) p.invertir[k] = true;
			}
		}
		if (auto it = j.find("cambiarSticks"); it != j.end() && it->is_boolean()) p.cambiarSticks = it->get<bool>();
		if (auto it = j.find("zona"); it != j.end() && it->is_array()) {
			for (int k = 0; k < 2 && k < static_cast<int>(it->size()); k++) {
				const json& z = (*it)[k];
				if (!z.is_number()) continue;
				const int v = z.get<int>();
				p.zona[k] = static_cast<uint8_t>(v < 0 ? 0 : (v > ZONA_MAX ? ZONA_MAX : v));
			}
		}
		return p;
	}
}
