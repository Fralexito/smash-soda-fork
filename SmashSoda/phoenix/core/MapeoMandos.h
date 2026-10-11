#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

// =============================================================================
//  MAPEO DE MANDOS POR PERSONA (lógica pura, sin Windows: se prueba en pruebas-link)
//  Corrige mandos genéricos o mal mapeados de un invitado antes de que su estado
//  llegue al mando virtual (ViGEm): botones cambiados, stick que camina solo,
//  sticks al revés, gatillos raros.
//
//  Fuentes (lo que llega por Parsec, ya en formato XInput):
//    0..15 = bit i de la máscara de botones · 16 = gatillo izq. (LT) · 17 = gatillo der. (RT)
//  Cada fuente tiene un DESTINO = máscara de salida con los mismos 18 bits:
//    bits 0..15 = botones XInput · bit 16 = LT · bit 17 = RT · 0 = desactivada.
//  Así «botón → gatillo» y «gatillo → botón» son lo mismo que «botón → botón».
//
//  Orden de los ejes: 1) cambiar stick izq. por der. · 2) invertir cada eje de salida
//  · 3) zona muerta radial de cada stick de salida (con reescalado, sin saltos).
// =============================================================================

namespace phoenix::mapeo {

	constexpr int FUENTES = 18;
	constexpr int FUENTE_LT = 16, FUENTE_RT = 17;
	constexpr uint32_t SALIDA_LT = 1u << 16, SALIDA_RT = 1u << 17;
	constexpr uint8_t UMBRAL_GATILLO = 100;   ///< gatillo → botón: pulsado desde aquí (0–255)
	constexpr int ZONA_MAX = 60;               ///< zona muerta máxima (%)

	/// Estado de un mando en formato XInput (Y del stick hacia arriba = positivo).
	struct Estado {
		uint16_t b = 0;
		uint8_t lt = 0, rt = 0;
		int16_t lx = 0, ly = 0, rx = 0, ry = 0;
		bool operator==(const Estado& o) const {
			return b == o.b && lt == o.lt && rt == o.rt && lx == o.lx && ly == o.ly && rx == o.rx && ry == o.ry;
		}
	};

	enum Eje { LX = 0, LY = 1, RX = 2, RY = 3 };

	struct Perfil {
		uint32_t destino[FUENTES]{};
		bool invertir[4]{};          ///< ejes de SALIDA: LX, LY, RX, RY
		bool cambiarSticks = false;
		uint8_t zona[2]{};           ///< % por stick de salida (0 = sin zona muerta), 0–ZONA_MAX

		Perfil();                    ///< identidad: todo sale igual que entra
		bool esIdentidad() const;
	};

	/// Aplica el perfil. Barato: se llama en cada mensaje de mando.
	Estado aplicar(const Perfil& p, const Estado& e);

	/// Nombre corto de una fuente/salida (a, b, x, y, lb, rb, lt, rt, start, back, guia, ls, rs, arriba, abajo, izq, der).
	const char* nombre(int indice);
	int indicePorNombre(const std::string& n);   ///< -1 si no existe

	/// JSON: {"botones": {"a":["b"], "lt":["a"], …} (solo lo que cambia), "invertir":["ly"], "cambiarSticks":bool, "zona":[izq,der]}
	nlohmann::json aJson(const Perfil& p);
	Perfil deJson(const nlohmann::json& j);       ///< tolerante: lo que no entiende se queda como identidad
}
