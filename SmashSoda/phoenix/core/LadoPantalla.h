#pragma once

// =============================================================================
//  Phoenix Link · Lado de cada mando en la pantalla de mandos de PES (antes del pitazo) — lógica pura
// -----------------------------------------------------------------------------
//  En la pantalla de selección de mandos de PES cada mando (casilla PES 1-8 = puesto N de Link) está a la
//  izquierda (equipo local = Equipo A), en el centro (no juega = Mirando) o a la derecha (visita = Equipo B).
//  Quien lee ese dato del juego (solo lectura de memoria) se engancha con fijarProveedor(); mientras no haya
//  proveedor, o no esté confirmado, todo queda «desconocido» y Link se comporta como siempre.
//  El Estabilizador evita parpadeos: un lado solo cuenta si se lee igual kEstableSeg seguidos, y si las
//  lecturas dejan de llegar (PES cerrado, otra pantalla) todo vuelve a «desconocido».
//  Ver docs/DETECCION-LADOS.md §7. Pruebas: pruebas-link/ladopantalla_test.cpp.
// =============================================================================

#include <cstdint>

namespace phoenix::ladopantalla {

	constexpr int kCasillas = 8;
	constexpr int8_t kIzquierda = -1, kCentro = 0, kDerecha = 1, kDesconocido = 2;

	/// Una lectura del juego: lado por casilla (kIzquierda/kCentro/kDerecha, o kDesconocido si esa casilla no tiene mando).
	struct Lectura {
		bool valida = false;
		int8_t lado[kCasillas] = { kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido };
	};

	constexpr double kEstableSeg = 1.5;   ///< un cambio de lado cuenta tras leerse igual este tiempo
	constexpr double kCaducaSeg = 3.0;    ///< sin lecturas válidas este tiempo: todo vuelve a desconocido

	class Estabilizador {
	public:
		/// Cada lectura (cualquier ritmo; t en segundos, creciente). Las no válidas o con valores raros no cuentan.
		void alimentar(double t, const Lectura& l);
		/// Lado estable de la casilla i (0-7) en el instante t: 'A' (izquierda), 'B' (derecha), '-' (centro, Mirando) o '?'.
		char lado(int i, double t) const;
		/// ¿Hay lecturas válidas recientes?
		bool vivo(double t) const { return tValida_ >= 0 && t - tValida_ <= kCaducaSeg; }
	private:
		int8_t estable_[kCasillas] = { kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido };
		int8_t candidato_[kCasillas] = { kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido, kDesconocido };
		double desde_[kCasillas] = {};
		double tValida_ = -1;
	};

	/// Enganche del lector del juego (lo pone quien lee la memoria de PES). nullptr = no hay lector.
	using Proveedor = bool (*)(Lectura& out);
	void fijarProveedor(Proveedor p);
	/// Lee con el proveedor (si lo hay). false si no hay proveedor o la lectura no vale.
	bool leer(Lectura& out);

}
