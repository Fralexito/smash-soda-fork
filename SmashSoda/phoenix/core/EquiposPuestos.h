#pragma once

#include <string>

// =============================================================================
//  Phoenix Link · Equipo de cada puesto (lógica pura, sin dependencias)
// -----------------------------------------------------------------------------
//  Los puestos son los mandos virtuales de Link (con «Orden de mandos en PES»,
//  el puesto N = jugador N de PES). Hasta 8 de serie; con «Puestos máximos» hasta 16
//  (los 9+ son EXPERIMENTALES: necesitan el módulo de Sider de mandos extra). Cada puesto pertenece al EQUIPO A, al EQUIPO B
//  o está fuera. El número del puesto nunca cambia: lo que se mueve es la etiqueta.
//
//  El mapa es un texto de 8 letras (más si hay puestos 9+ con equipo), una por puesto:
//  'A', 'B' o '-' (fuera). Los '-' del final más allá del 8 no se guardan.
//    "AB------"  puesto 01 = Equipo A, puesto 02 = Equipo B
//    "BA------"  puesto 01 = Equipo B, puesto 02 = Equipo A (el anfitrión en el 01 siendo del B)
//
//  Equipo A / B son equipos de Link: NO tienen por qué coincidir con local/visita del
//  juego (A = el que estaba a la izquierda en el saque, según el Campo 2D).
//  Probado en pruebas-link/equipos_test.cpp.
// =============================================================================

namespace phoenix::equipos {

	constexpr int kPuestos = 16;      ///< tope de puestos (ViGEmBus medido: 16 seguros; ver docs/MANDOS.md («MAS-DE-8-MANDOS»))
	constexpr int kPuestosPes = 8;    ///< casillas de settings.dat; el mapa siempre tiene al menos estas letras
	using Mapa = std::string;

	/// Equipo del puesto i (0-based): 'A', 'B' o '-' (fuera, o índice inválido).
	inline char de(const Mapa& m, int i) {
		if (i < 0 || i >= kPuestos || i >= static_cast<int>(m.size())) return '-';
		return (m[static_cast<size_t>(i)] == 'A' || m[static_cast<size_t>(i)] == 'B') ? m[static_cast<size_t>(i)] : '-';
	}

	inline int cuenta(const Mapa& m, char equipo) {
		int n = 0;
		const int largo = static_cast<int>(m.size()) > kPuestosPes ? (static_cast<int>(m.size()) < kPuestos ? static_cast<int>(m.size()) : kPuestos) : kPuestosPes;
		for (int i = 0; i < largo; i++) if (de(m, i) == equipo) n++;
		return n;
	}

	/// Tramo de asientos que hay que mirar: último puesto con equipo + 1 (0 si no hay ninguno).
	inline int tramo(const Mapa& m) {
		int t = 0;
		for (int i = 0; i < kPuestos; i++) if (de(m, i) != '-') t = i + 1;
		return t;
	}

	/// Forma guardada: 8 letras, o hasta el último puesto con equipo si pasa del 8.
	inline Mapa corto(Mapa m) {
		m.resize(static_cast<size_t>(kPuestos), '-');
		const int t = tramo(m);
		m.resize(static_cast<size_t>(t > kPuestosPes ? t : kPuestosPes));
		return m;
	}

	/// Formación de siempre: los primeros nA son del A, los siguientes nB del B, el resto fuera.
	inline Mapa desdeFormacion(int nA, int nB) {
		Mapa m(kPuestos, '-');
		for (int i = 0; i < kPuestos; i++) {
			if (i < nA) m[static_cast<size_t>(i)] = 'A';
			else if (i < nA + nB) m[static_cast<size_t>(i)] = 'B';
		}
		return corto(m);
	}

	/// Migración desde las preferencias viejas (mandosActivos y equipoLocal): los primeros equipoLocal = A,
	/// el resto hasta mandosActivos = B, el resto fuera.
	inline Mapa migrar(int mandosActivos, int equipoLocal) {
		// Las preferencias viejas nunca pasaron de 8 puestos
		const int activos = mandosActivos < 2 ? 2 : (mandosActivos > kPuestosPes ? kPuestosPes : mandosActivos);
		const int a = equipoLocal < 1 ? 1 : (equipoLocal > activos - 1 ? activos - 1 : equipoLocal);
		return desdeFormacion(a, activos - a);
	}

	/// Mapa válido: 8 letras A/B/- (o hasta 16), con al menos un puesto en cada equipo. Si no, la formación 1v1 de siempre.
	inline Mapa normalizar(const Mapa& entrada) {
		Mapa m(kPuestos, '-');
		for (int i = 0; i < kPuestos && i < static_cast<int>(entrada.size()); i++) {
			const char c = entrada[static_cast<size_t>(i)];
			if (c == 'A' || c == 'B') m[static_cast<size_t>(i)] = c;
		}
		if (cuenta(m, 'A') < 1 || cuenta(m, 'B') < 1) return desdeFormacion(1, 1);
		return corto(m);
	}

	/// Cuántos puestos ve la sala: el tramo, y como mínimo 2.
	inline int activos(const Mapa& m) {
		const int t = tramo(m);
		return t < 2 ? 2 : t;
	}

	/// Pone el puesto i (0-based) en 'A', 'B' o '-'. maxPuestos = mandos virtuales que existen (2–16).
	/// Falso (y no cambia nada) si el puesto no existe o dejaría a un equipo sin puestos.
	inline bool poner(Mapa& m, int i, char equipo, int maxPuestos) {
		if (equipo != 'A' && equipo != 'B' && equipo != '-') return false;
		if (maxPuestos > kPuestos) maxPuestos = kPuestos;
		if (i < 0 || i >= maxPuestos) return false;
		m = normalizar(m);
		const char antes = de(m, i);
		if (antes == equipo) return true;
		if (antes != '-' && cuenta(m, antes) <= 1) return false;   // era el último de su equipo
		m.resize(static_cast<size_t>(kPuestos), '-');
		m[static_cast<size_t>(i)] = equipo;
		m = corto(m);
		return true;
	}

	/// Cambia el puesto i al otro equipo (A↔B). Falso si está fuera o es el último de su equipo.
	inline bool pasarAlOtro(Mapa& m, int i, int maxPuestos) {
		const char e = de(m, i);
		if (e == '-') return false;
		return poner(m, i, e == 'A' ? 'B' : 'A', maxPuestos);
	}

	/// Intercambia los equipos de dos puestos (uno del A y otro del B): cada puesto conserva su número y su mando.
	/// Falso si alguno está fuera, no existen o son del mismo equipo. Nunca deja a un equipo vacío.
	inline bool intercambiarEquipos(Mapa& m, int i, int j, int maxPuestos) {
		if (maxPuestos > kPuestos) maxPuestos = kPuestos;
		if (i < 0 || j < 0 || i >= maxPuestos || j >= maxPuestos || i == j) return false;
		m = normalizar(m);
		const char a = de(m, i), b = de(m, j);
		if (a == '-' || b == '-' || a == b) return false;
		m.resize(static_cast<size_t>(kPuestos), '-');
		m[static_cast<size_t>(i)] = b;
		m[static_cast<size_t>(j)] = a;
		m = corto(m);
		return true;
	}

	/// Primer puesto fuera (0-based) dentro de los mandos que existen; -1 si no queda ninguno.
	inline int primeroFuera(const Mapa& m, int maxPuestos) {
		if (maxPuestos > kPuestos) maxPuestos = kPuestos;
		for (int i = 0; i < maxPuestos; i++) if (de(m, i) == '-') return i;
		return -1;
	}

	/// Deja exactamente nA puestos en A y nB en B, tocando lo mínimo: primero quita los últimos de los que sobran,
	/// luego añade los primeros puestos libres. Falso si no caben (nA + nB > maxPuestos) o algún equipo queda vacío.
	inline bool ajustarCuentas(Mapa& m, int nA, int nB, int maxPuestos) {
		if (maxPuestos > kPuestos) maxPuestos = kPuestos;
		if (nA < 1 || nB < 1 || nA + nB > maxPuestos) return false;
		Mapa t = normalizar(m);
		t.resize(static_cast<size_t>(kPuestos), '-');
		for (char e : { 'A', 'B' }) {
			const int quiere = e == 'A' ? nA : nB;
			for (int i = kPuestos - 1; i >= 0 && cuenta(t, e) > quiere; i--) if (de(t, i) == e) t[static_cast<size_t>(i)] = '-';
		}
		for (char e : { 'A', 'B' }) {
			const int quiere = e == 'A' ? nA : nB;
			while (cuenta(t, e) < quiere) {
				const int i = primeroFuera(t, maxPuestos);
				if (i < 0) return false;
				t[static_cast<size_t>(i)] = e;
			}
		}
		m = corto(t);
		return true;
	}

	/// Si hay menos mandos virtuales que puestos usados, los de más allá quedan fuera; si un equipo se queda vacío,
	/// se rehace con la formación de siempre.
	inline Mapa recortar(const Mapa& m, int maxPuestos) {
		Mapa t = normalizar(m);
		t.resize(static_cast<size_t>(kPuestos), '-');
		if (maxPuestos > kPuestos) maxPuestos = kPuestos;
		for (int i = maxPuestos < 0 ? 0 : maxPuestos; i < kPuestos; i++) t[static_cast<size_t>(i)] = '-';
		if (cuenta(t, 'A') < 1 || cuenta(t, 'B') < 1) return desdeFormacion(1, 1);
		return corto(t);
	}

}
