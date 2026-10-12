// Prueba de la lógica pura del equipo de cada puesto (core/EquiposPuestos.h): migración desde la configuración vieja
// (mandosActivos + equipoLocal), mapa válido, mover un puesto de equipo sin renumerar y el caso clave de Fralex:
// el puesto 01 en el Equipo B y el 02 en el A.
// Compilar: g++ -std=c++17 -O2 -Wall -Wextra -I SmashSoda pruebas-link/equipos_test.cpp
#include <cstdio>

#include "phoenix/core/EquiposPuestos.h"

using namespace phoenix::equipos;

static int fallos = 0;
#define CHEQUEA(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

int main() {
	// ---- Migración: los primeros equipoLocal = A, el resto hasta mandosActivos = B, el resto fuera ----
	CHEQUEA(migrar(2, 1) == "AB------");
	CHEQUEA(migrar(4, 2) == "AABB----");
	CHEQUEA(migrar(8, 4) == "AAAABBBB");
	CHEQUEA(migrar(5, 3) == "AAABB---");
	CHEQUEA(migrar(3, 1) == "ABB-----");
	// valores fuera de rango se acotan como en las preferencias de siempre
	CHEQUEA(migrar(0, 0) == "AB------");
	CHEQUEA(migrar(99, 99) == "AAAAAAAB");
	CHEQUEA(migrar(4, 9) == "AAAB----");

	// ---- Consultas ----
	const Mapa m = "BA--AB--";
	CHEQUEA(de(m, 0) == 'B' && de(m, 1) == 'A' && de(m, 2) == '-' && de(m, 4) == 'A' && de(m, 5) == 'B');
	CHEQUEA(de(m, -1) == '-' && de(m, 8) == '-' && de(m, 99) == '-');
	CHEQUEA(de("A", 3) == '-');   // mapa corto: lo que falta está fuera
	CHEQUEA(cuenta(m, 'A') == 2 && cuenta(m, 'B') == 2 && cuenta(m, '-') == 4);
	CHEQUEA(tramo(m) == 6);
	CHEQUEA(activos(m) == 6);
	CHEQUEA(tramo("--------") == 0 && activos("--------") == 2);
	CHEQUEA(activos("AB------") == 2);

	// ---- normalizar ----
	CHEQUEA(normalizar("BA------") == "BA------");
	CHEQUEA(normalizar("ba------") == "AB------");           // minúsculas no valen: queda sin A ni B → 1v1
	CHEQUEA(normalizar("AAAA----") == "AB------");           // sin Equipo B → 1v1
	CHEQUEA(normalizar("") == "AB------");
	CHEQUEA(normalizar("A?B") == "A-B-----");                // letras raras = fuera, y se completa a 8
	CHEQUEA(normalizar("ABABABABAB") == "ABABABAB");         // se corta a 8

	// ---- El caso clave: puesto 01 en el Equipo B y el 02 en el A, sin renumerar nada ----
	Mapa k = "AB------";
	CHEQUEA(pasarAlOtro(k, 0, 8) == false);                  // el 01 es el único del A: el A se quedaría vacío
	CHEQUEA(poner(k, 2, 'A', 8) && k == "ABA-----");         // añadir el 03 al A
	CHEQUEA(pasarAlOtro(k, 0, 8) && k == "BBA-----");        // ahora el 01 sí puede pasar al B
	CHEQUEA(pasarAlOtro(k, 1, 8) && k == "BAA-----");        // el 02 pasa al A
	CHEQUEA(de(k, 0) == 'B' && de(k, 1) == 'A');             // 01 = Equipo B, 02 = Equipo A
	CHEQUEA(poner(k, 2, '-', 8) && k == "BA------");         // quitar el 03: queda justo el caso de Fralex
	CHEQUEA(cuenta(k, 'A') == 1 && cuenta(k, 'B') == 1 && activos(k) == 2);

	// ---- Reglas de poner ----
	k = "BA------";
	CHEQUEA(poner(k, 0, '-', 8) == false && k == "BA------");   // 01 es el único del B
	CHEQUEA(poner(k, 1, 'B', 8) == false && k == "BA------");   // 02 es el único del A
	CHEQUEA(poner(k, 3, 'B', 8) && k == "BA-B----");            // un puesto salteado
	CHEQUEA(tramo(k) == 4 && activos(k) == 4);
	CHEQUEA(poner(k, 8, 'A', 8) == false);                      // el puesto 9 no existe
	CHEQUEA(poner(k, -1, 'A', 8) == false);
	CHEQUEA(poner(k, 5, 'A', 4) == false);                      // más allá de los mandos que hay
	CHEQUEA(poner(k, 4, 'X', 8) == false);                      // equipo inválido
	CHEQUEA(poner(k, 3, 'B', 8) && k == "BA-B----");            // ya estaba: no cambia
	CHEQUEA(pasarAlOtro(k, 2, 8) == false);                     // un puesto fuera no pasa de equipo

	// ---- primeroFuera ----
	CHEQUEA(primeroFuera("BA-B----", 8) == 2);
	CHEQUEA(primeroFuera("ABAB----", 4) == -1);                 // con 4 mandos no queda ninguno
	CHEQUEA(primeroFuera("AABBAABB", 8) == -1);

	// ---- ajustarCuentas: quita los últimos que sobran y añade los primeros libres ----
	k = "AABB----";
	CHEQUEA(ajustarCuentas(k, 3, 2, 8) && k == "AABBA---");
	k = "AABB----";
	CHEQUEA(ajustarCuentas(k, 1, 3, 8) && k == "ABBB----");
	k = "AABB----";
	CHEQUEA(ajustarCuentas(k, 4, 4, 8) && cuenta(k, 'A') == 4 && cuenta(k, 'B') == 4 && tramo(k) == 8);
	k = "AABB----";
	CHEQUEA(ajustarCuentas(k, 5, 4, 8) == false && k == "AABB----");   // 9 puestos no caben
	CHEQUEA(ajustarCuentas(k, 0, 2, 8) == false);                       // un equipo no puede quedar vacío
	CHEQUEA(ajustarCuentas(k, 2, 2, 3) == false);                       // caben 3 mandos, no 4
	k = "BA------";
	CHEQUEA(ajustarCuentas(k, 1, 1, 8) && k == "BA------");             // ya estaba: respeta el orden de Fralex

	// ---- recortar (menos mandos virtuales que puestos) ----
	CHEQUEA(recortar("AABBAABB", 4) == "AABB----");
	CHEQUEA(recortar("BA-BAB--", 4) == "BA-B----");
	CHEQUEA(recortar("AAAABBBB", 4) == "AB------");                     // el B quedó vacío → 1v1
	CHEQUEA(recortar("BA------", 2) == "BA------");

	// ---- desdeFormacion ----
	CHEQUEA(desdeFormacion(2, 2) == "AABB----");
	CHEQUEA(desdeFormacion(1, 3) == "ABBB----");
	CHEQUEA(desdeFormacion(4, 4) == "AAAABBBB");

	// ---- Ida y vuelta por texto (lo que se guarda en phoenix-ui.json) ----
	for (const Mapa& x : { Mapa("BA------"), Mapa("AABB----"), Mapa("BA-B----"), Mapa("ABABABAB") }) CHEQUEA(normalizar(x) == x);

	if (fallos == 0) std::printf("equipos_test: todo bien\n");
	return fallos == 0 ? 0 : 1;
}
