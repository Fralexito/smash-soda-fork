// Prueba de la lógica pura del lado de cada mando en la pantalla de mandos de PES (core/LadoPantalla).
// Compilar: g++ -std=c++17 -O2 -I SmashSoda pruebas-link/ladopantalla_test.cpp SmashSoda/phoenix/core/LadoPantalla.cpp
#include <cstdio>

#include "phoenix/core/LadoPantalla.h"

using namespace phoenix::ladopantalla;

static int fallos = 0;
#define CHEQUEA(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

static Lectura lec(int8_t a, int8_t b = kDesconocido) { Lectura l; l.valida = true; l.lado[0] = a; l.lado[1] = b; return l; }

int main() {
	Estabilizador e;
	// Sin lecturas: todo desconocido
	CHEQUEA(e.lado(0, 0) == '?' && !e.vivo(0));

	// Mando 1 a la izquierda y mando 2 a la derecha, estables 2 s (lecturas a 2/s)
	double t = 0;
	for (; t <= 2.0; t += 0.5) e.alimentar(t, lec(kIzquierda, kDerecha));
	CHEQUEA(e.lado(0, t) == 'A' && e.lado(1, t) == 'B' && e.lado(2, t) == '?');

	// Una lectura suelta distinta (parpadeo) no cambia nada
	e.alimentar(t, lec(kDerecha, kDerecha)); t += 0.5;
	e.alimentar(t, lec(kIzquierda, kDerecha)); t += 0.5;
	CHEQUEA(e.lado(0, t) == 'A');

	// Pasa al centro: hasta 1,5 s seguidos sigue en A; luego Mirando
	const double t0 = t;
	for (; t < t0 + 1.0; t += 0.5) e.alimentar(t, lec(kCentro, kDerecha));
	CHEQUEA(e.lado(0, t) == 'A');
	for (; t <= t0 + 2.0; t += 0.5) e.alimentar(t, lec(kCentro, kDerecha));
	CHEQUEA(e.lado(0, t) == '-');

	// Lecturas con valores raros no cuentan y, si no llega nada válido en 3 s, todo vuelve a desconocido
	const double tUltima = t - 0.5;
	Lectura rara = lec(5, kDerecha);
	for (int k = 0; k < 8; k++) { e.alimentar(t, rara); t += 0.5; }
	CHEQUEA(e.lado(0, tUltima + 1.0) == '-');
	CHEQUEA(e.lado(0, t) == '?' && !e.vivo(t));

	// Tras el corte no se arrastra lo anterior: vuelve a hacer falta 1,5 s
	e.alimentar(t, lec(kDerecha));
	CHEQUEA(e.lado(0, t) == '?');
	double t1 = t;
	for (; t1 <= t + 2.0; t1 += 0.4) e.alimentar(t1, lec(kDerecha));
	CHEQUEA(e.lado(0, t1) == 'B');

	// Sin proveedor no hay lectura
	Lectura l;
	CHEQUEA(!leer(l));
	fijarProveedor([](Lectura& o) { o = Lectura(); o.valida = true; o.lado[2] = kIzquierda; return true; });
	CHEQUEA(leer(l) && l.lado[2] == kIzquierda);
	fijarProveedor(nullptr);
	CHEQUEA(!leer(l));

	std::printf(fallos ? "HAY FALLOS: %d\n" : "ladopantalla_test OK\n", fallos);
	return fallos ? 1 : 0;
}
