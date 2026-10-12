// Prueba de la lógica pura de la detección del lado de cada mando (core/DeteccionLados) con un partido simulado:
// 10 + 10 jugadores de campo con IA aleatoria, dos porteros, un mando por equipo que maneja a un futbolista
// (cambia cada 4 s) con retraso de 0,25 s, un rival que lo persigue copiando su carrera 0,5 s después (el peor
// caso), un mando quieto y un mando que se mueve sin manejar a nadie (no debe decidir).
// Compilar: g++ -std=c++17 -O2 -I SmashSoda pruebas-link/lados_test.cpp SmashSoda/phoenix/core/DeteccionLados.cpp
#include <cmath>
#include <cstdio>
#include <deque>
#include <random>
#include <vector>

#include "phoenix/core/DeteccionLados.h"

using namespace phoenix::lados;

static int fallos = 0;
#define CHEQUEA(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

struct Jug { float x, z, vx = 0, vz = 0; int eq; bool portero = false; };
struct StickSim { float lx = 0, ly = 0; double cambio = 0; };

static void moverStick(StickSim& s, std::mt19937& rng, double t) {
	if (t < s.cambio) return;
	std::uniform_real_distribution<float> u(0, 1);
	if (u(rng) < 0.3f) { s.lx = s.ly = 0; }
	else { const float a = u(rng) * 6.2832f; s.lx = std::cos(a); s.ly = std::sin(a); }
	s.cambio = t + 0.8 + 1.2 * u(rng);
}

/// orient: bit 0 x invertida, bit 1 z invertida (la «cámara» del juego simulado)
static std::vector<Veredicto> simular(int orient, double segundos, uint32_t semilla, int& orientElegida, bool& clara, LadoDefendido& ld) {
	std::mt19937 rng(semilla);
	std::normal_distribution<float> n(0, 1);
	std::uniform_real_distribution<float> u(-1, 1);
	std::vector<Jug> j;
	for (int e = 0; e < 2; e++) for (int k = 0; k < 10; k++) j.push_back({ (e == 0 ? -1.f : 1.f) * (5 + 40 * (u(rng) + 1) / 2), 30 * u(rng), 0, 0, e });
	j.push_back({ -50, 0, 0, 0, 0, true }); j.push_back({ 50, 0, 0, 0, 1, true });
	StickSim s1, s2, s4;    // s1 maneja al A, s2 al B, s4 no maneja a nadie
	std::deque<std::pair<double, StickSim>> h1, h2;
	std::deque<std::pair<double, std::pair<float, float>>> hPerseguido;
	Detector d; ld = LadoDefendido();
	const double dt = 0.05;
	for (double t = 0; t < segundos; t += dt) {
		moverStick(s1, rng, t); moverStick(s2, rng, t); moverStick(s4, rng, t);
		h1.push_back({ t, s1 }); h2.push_back({ t, s2 });
		while (h1.size() > 1 && h1[1].first <= t - 0.25) h1.pop_front();
		while (h2.size() > 1 && h2[1].first <= t - 0.25) h2.pop_front();
		const int c1 = static_cast<int>(t / 4) % 10, c2 = 10 + static_cast<int>(t / 4 + 2) % 10;   // controlados
		const int perseguidor = 10 + (c1 + 3) % 10;                                                // rival que copia la carrera de c1
		for (size_t i = 0; i < j.size(); i++) {
			Jug& p = j[i];
			if (p.portero) { p.vx = 0.3f * n(rng); p.vz = 0.5f * n(rng); }
			else if (static_cast<int>(i) == c1 || static_cast<int>(i) == c2) {
				const StickSim& s = (static_cast<int>(i) == c1 ? h1 : h2).front().second;
				p.vx = 6.0f * (orient & 1 ? -s.lx : s.lx); p.vz = 6.0f * (orient & 2 ? -s.ly : s.ly);
			}
			else if (static_cast<int>(i) == perseguidor && !hPerseguido.empty()) { p.vx = hPerseguido.front().second.first; p.vz = hPerseguido.front().second.second; }
			else { p.vx = 0.9f * p.vx + 0.6f * n(rng); p.vz = 0.9f * p.vz + 0.6f * n(rng); }
			p.x += static_cast<float>(p.vx * dt); p.z += static_cast<float>(p.vz * dt);
			if (std::fabs(p.x) > 52) { p.x = std::copysign(52.f, p.x); p.vx = -p.vx; }
			if (std::fabs(p.z) > 33) { p.z = std::copysign(33.f, p.z); p.vz = -p.vz; }
		}
		hPerseguido.push_back({ t, { j[static_cast<size_t>(c1)].vx, j[static_cast<size_t>(c1)].vz } });
		while (hPerseguido.size() > 1 && hPerseguido[1].first <= t - 0.5) hPerseguido.pop_front();
		std::vector<Ficha> f;
		for (const Jug& p : j) f.push_back({ p.x, p.z, p.eq });
		d.alimentar(t, f, { { 1, s1.lx, s1.ly }, { 2, s2.lx, s2.ly }, { 3, 0, 0 }, { 4, s4.lx, s4.ly } });
		ld.alimentar(t, f);
	}
	orientElegida = d.orientacion(); clara = d.orientacionClara();
	return d.veredictos();
}

static const Veredicto* de(const std::vector<Veredicto>& v, int a) { for (const Veredicto& x : v) if (x.asiento == a) return &x; return nullptr; }

int main() {
	int aciertos = 0, decididos = 0, falsos = 0;
	for (uint32_t semilla = 1; semilla <= 20; semilla++) {
		const int orient = static_cast<int>(semilla % 4);
		int oe = -1; bool clara = false; LadoDefendido ld;
		const auto v = simular(orient, 60.0, semilla, oe, clara, ld);
		const Veredicto *a1 = de(v, 1), *a2 = de(v, 2), *a3 = de(v, 3), *a4 = de(v, 4);
		CHEQUEA(a1 && a2 && a3 && a4);
		if (!a1 || !a2 || !a3 || !a4) continue;
		CHEQUEA(oe == orient && clara);
		CHEQUEA(!a3->decidido && a3->eq == -1);                 // mando quieto: nada
		if (a4->decidido) falsos++;                              // mando suelto: no debe decidir
		for (const Veredicto* x : { a1, a2 }) {
			const int esperado = x->asiento == 1 ? 0 : 1;
			if (x->decidido) { decididos++; if (x->eq == esperado) aciertos++; else falsos++; }
		}
		std::printf("semilla %2u orient %d->%d clara=%d | m1 eq=%d %d/%d de %d | m2 eq=%d %d/%d | m4 %d/%d de %d dec=%d\n", semilla, orient, oe, (int)clara,
			a1->eq, a1->votosA, a1->votosB, a1->apretado, a2->eq, a2->votosA, a2->votosB, a4->votosA, a4->votosB, a4->apretado, (int)a4->decidido);
		// Ancla: el A defiende x < 0
		CHEQUEA(ld.lado(0) == -1 && ld.lado(1) == 1);
		CHEQUEA(equipoQueMarco(+1, ld) == 0 && equipoQueMarco(-1, ld) == 1);
	}
	std::printf("decididos %d/40, aciertos %d, falsos %d\n", decididos, aciertos, falsos);
	CHEQUEA(decididos >= 36 && aciertos == decididos && falsos == 0);

	// Portería del gol
	CHEQUEA(porteriaDelGol(53.0f, 0.5f, 1.0f) == 1 && porteriaDelGol(-53.0f, 0.5f, -2.0f) == -1);
	CHEQUEA(porteriaDelGol(53.0f, 3.0f, 0.0f) == 0 && porteriaDelGol(53.0f, 0.5f, 5.0f) == 0 && porteriaDelGol(50.0f, 0.5f, 0.0f) == 0);
	// Sin datos de portero no se adivina
	LadoDefendido vacio; CHEQUEA(equipoQueMarco(1, vacio) == -1);

	// Anclas: el gol manda; si el saque dice lo contrario, no se sabe
	CHEQUEA(equipoLocal(0, -1) == 0 && equipoLocal(-1, 1) == 1 && equipoLocal(1, 1) == 1 && equipoLocal(0, 1) == -1 && equipoLocal(-1, -1) == -1);
	// Decisión: el local del Campo es el equipo 0
	CHEQUEA(decidir({ { 0, true } }, 0) == Decision::Coinciden);                 // 1v1 de local y Link también lo tiene de local
	CHEQUEA(decidir({ { 1, true } }, 0) == Decision::Corregir);                  // juega de visita y Link lo tiene de local
	CHEQUEA(decidir({ { 1, true }, { 0, false } }, 0) == Decision::Corregir);    // los dos al revés
	CHEQUEA(decidir({ { 1, true }, { 1, false } }, 0) == Decision::Nada);        // mezcla: no se toca
	CHEQUEA(decidir({ { -1, true } }, 0) == Decision::Nada);                     // sin veredicto
	CHEQUEA(decidir({ { 1, true } }, -1) == Decision::Nada);                     // sin ancla

	std::printf(fallos ? "HAY FALLOS: %d\n" : "lados_test OK\n", fallos);
	return fallos ? 1 : 0;
}
