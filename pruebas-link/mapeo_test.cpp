// Prueba de la lógica pura del mapeo de mandos por persona (phoenix/core/MapeoMandos).
// MSVC: cl /nologo /std:c++17 /EHsc /O2 /I SmashSoda /I external pruebas-link\mapeo_test.cpp SmashSoda\phoenix\core\MapeoMandos.cpp
// g++:  g++ -std=c++17 -O2 -I SmashSoda -I external pruebas-link/mapeo_test.cpp SmashSoda/phoenix/core/MapeoMandos.cpp
#include "phoenix/core/MapeoMandos.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace phoenix::mapeo;

static int fallos = 0;
#define COMPROBAR(c) do { if (!(c)) { std::cerr << "FALLA linea " << __LINE__ << ": " #c "\n"; fallos++; } } while (0)

// Bits XInput
constexpr uint16_t A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000, START = 0x10, LB = 0x100;
static int bit(const char* n) { return indicePorNombre(n); }

int main() {
	// 1. Identidad: todo sale igual que entra
	{
		Perfil p;
		COMPROBAR(p.esIdentidad());
		Estado e; e.b = A | START; e.lt = 77; e.rt = 255; e.lx = -1234; e.ly = 32767; e.rx = -32768; e.ry = 5;
		COMPROBAR(aplicar(p, e) == e);
	}
	// 2. Botones cambiados: A <-> B
	{
		Perfil p;
		p.destino[bit("a")] = B; p.destino[bit("b")] = A;
		COMPROBAR(!p.esIdentidad());
		Estado e; e.b = A;
		COMPROBAR(aplicar(p, e).b == B);
		e.b = A | B | Y;
		COMPROBAR(aplicar(p, e).b == (A | B | Y));
		e.b = B;
		COMPROBAR(aplicar(p, e).b == A);
	}
	// 3. Botón a gatillo: X -> LT (a tope) y desactivar un botón
	{
		Perfil p;
		p.destino[bit("x")] = SALIDA_LT;
		p.destino[bit("start")] = 0;
		Estado e; e.b = X | START;
		const Estado s = aplicar(p, e);
		COMPROBAR(s.b == 0 && s.lt == 255 && s.rt == 0);
	}
	// 4. Gatillo a botón: LT -> A desde el umbral; LT ya no sale como gatillo
	{
		Perfil p;
		p.destino[FUENTE_LT] = A;
		Estado e; e.lt = 150;
		Estado s = aplicar(p, e);
		COMPROBAR(s.b == A && s.lt == 0);
		e.lt = UMBRAL_GATILLO - 1;
		s = aplicar(p, e);
		COMPROBAR(s.b == 0 && s.lt == 0);
	}
	// 5. Gatillos cambiados (LT <-> RT) conservan el valor analógico
	{
		Perfil p;
		p.destino[FUENTE_LT] = SALIDA_RT; p.destino[FUENTE_RT] = SALIDA_LT;
		Estado e; e.lt = 40; e.rt = 200;
		const Estado s = aplicar(p, e);
		COMPROBAR(s.lt == 200 && s.rt == 40 && s.b == 0);
	}
	// 6. Gatillo a gatillo + botón a la vez (LB y RT pulsados por el mismo gatillo)
	{
		Perfil p;
		p.destino[FUENTE_RT] = SALIDA_RT | LB;
		Estado e; e.rt = 255;
		const Estado s = aplicar(p, e);
		COMPROBAR(s.rt == 255 && s.b == LB);
	}
	// 7. Invertir ejes (con el extremo -32768 sin desbordar)
	{
		Perfil p;
		p.invertir[LY] = true; p.invertir[RX] = true;
		Estado e; e.lx = 1000; e.ly = 20000; e.rx = -32768; e.ry = 7;
		const Estado s = aplicar(p, e);
		COMPROBAR(s.lx == 1000 && s.ly == -20000 && s.rx == 32767 && s.ry == 7);
	}
	// 8. Cambiar stick izquierdo por derecho (y luego invertir la salida)
	{
		Perfil p;
		p.cambiarSticks = true;
		Estado e; e.lx = 1; e.ly = 2; e.rx = 3; e.ry = 4;
		Estado s = aplicar(p, e);
		COMPROBAR(s.lx == 3 && s.ly == 4 && s.rx == 1 && s.ry == 2);
		p.invertir[LY] = true;
		s = aplicar(p, e);
		COMPROBAR(s.lx == 3 && s.ly == -4);
	}
	// 9. Zona muerta: el stick que «camina solo» queda quieto; a tope sigue a tope; sin salto al salir
	{
		Perfil p;
		p.zona[0] = 20;   // 20 % ≈ 6553
		Estado e; e.lx = 5000; e.ly = -3000;   // magnitud ~5831 < 6553
		Estado s = aplicar(p, e);
		COMPROBAR(s.lx == 0 && s.ly == 0);
		e.lx = 32767; e.ly = 0;
		s = aplicar(p, e);
		COMPROBAR(s.lx == 32767 && s.ly == 0);
		e.lx = 6700; e.ly = 0;   // justo fuera: valor pequeño, no salto
		s = aplicar(p, e);
		COMPROBAR(s.lx > 0 && s.lx < 400);
		e.lx = 32767; e.ly = 32767;   // diagonal a tope: sin desbordar
		s = aplicar(p, e);
		COMPROBAR(s.lx == 32767 && s.ly == 32767);
		// El stick derecho no se toca
		e = Estado(); e.rx = 3000;
		COMPROBAR(aplicar(p, e).rx == 3000);
		// Zona por encima del máximo se limita
		Perfil q; q.zona[1] = 200;
		e = Estado(); e.rx = 32767;
		COMPROBAR(aplicar(q, e).rx == 32767);
	}
	// 10. JSON: ida y vuelta, y tolerante con basura
	{
		Perfil p;
		p.destino[bit("a")] = B; p.destino[bit("b")] = A;
		p.destino[bit("x")] = SALIDA_LT | Y;
		p.destino[bit("guia")] = 0;
		p.destino[FUENTE_RT] = A;
		p.invertir[LY] = true; p.cambiarSticks = true; p.zona[0] = 12; p.zona[1] = 60;
		const nlohmann::json j = aJson(p);
		const Perfil q = deJson(nlohmann::json::parse(j.dump()));
		for (int i = 0; i < FUENTES; i++) COMPROBAR(p.destino[i] == q.destino[i]);
		for (int k = 0; k < 4; k++) COMPROBAR(p.invertir[k] == q.invertir[k]);
		COMPROBAR(q.cambiarSticks && q.zona[0] == 12 && q.zona[1] == 60);
		COMPROBAR(j["botones"]["guia"].is_array() && j["botones"]["guia"].empty());
		COMPROBAR(!j["botones"].contains("y"));   // solo lo que cambia

		const Perfil r = deJson(nlohmann::json::parse(R"({"botones":{"zzz":["a"],"a":"b","b":[1,"nada","x"]},"invertir":[3,"lx"],"zona":[-5,"x",99],"cambiarSticks":"si"})"));
		COMPROBAR(r.destino[bit("a")] == A);          // "a":"b" no es lista: se ignora
		COMPROBAR(r.destino[bit("b")] == X);          // solo los nombres válidos
		COMPROBAR(r.invertir[LX] && !r.invertir[LY]);
		COMPROBAR(r.zona[0] == 0 && r.zona[1] == 0);  // -5 → 0 y "x" se ignora (solo 2 sticks)
		COMPROBAR(!r.cambiarSticks);
		COMPROBAR(deJson(nlohmann::json(42)).esIdentidad());
		COMPROBAR(deJson(aJson(Perfil())).esIdentidad());
	}
	// 11. Nombres
	{
		COMPROBAR(std::string(nombre(bit("a"))) == "a" && bit("lt") == FUENTE_LT && bit("rt") == FUENTE_RT);
		COMPROBAR(bit("nada") == -1 && std::string(nombre(99)).empty());
		COMPROBAR((1u << bit("a")) == A && (1u << bit("start")) == START);
	}
	// 12. Coste: se ejecuta en cada mensaje de mando
	{
		Perfil p;
		p.destino[bit("a")] = B; p.invertir[LY] = true; p.zona[0] = 15; p.zona[1] = 10; p.destino[FUENTE_LT] = A | SALIDA_LT;
		Estado e; e.b = A | X | START; e.lt = 200; e.lx = 12000; e.ly = -9000; e.rx = 300; e.ry = 30000;
		const int N = 5000000;
		volatile uint32_t sumidero = 0;
		const auto t0 = std::chrono::steady_clock::now();
		for (int i = 0; i < N; i++) { e.lx = static_cast<int16_t>(i & 0x7FFF); sumidero = sumidero + aplicar(p, e).b; }
		const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / N;
		std::cout << "aplicar(): " << ns << " ns por mensaje\n";
		COMPROBAR(ns < 1000.0);
	}
	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
