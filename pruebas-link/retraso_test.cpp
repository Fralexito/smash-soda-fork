// Prueba de la lógica pura del medidor de retraso del mando (phoenix/core/MedidorRetraso).
// MSVC: cl /nologo /std:c++17 /EHsc /O2 /utf-8 /I SmashSoda pruebas-link\retraso_test.cpp SmashSoda\phoenix\core\MedidorRetraso.cpp
// g++:  g++ -std=c++17 -O2 -I SmashSoda pruebas-link/retraso_test.cpp SmashSoda/phoenix/core/MedidorRetraso.cpp
#include "phoenix/core/MedidorRetraso.h"

#include <iostream>
#include <string>

using namespace phoenix::retraso;

static int fallos = 0;
#define COMPROBAR(c) do { if (!(c)) { std::cerr << "FALLA linea " << __LINE__ << ": " #c "\n"; fallos++; } } while (0)

static const MedidaMando* buscar(const std::vector<MedidaMando>& v, uint32_t id) {
	for (const MedidaMando& m : v) if (m.userId == id) return &m;
	return nullptr;
}

int main() {
	// 1. Mando suave: estado cada 8 ms (125 Hz) durante 3 s → media ≈ 8, jitter ≈ 0, sin saltos, ritmo 125
	{
		MedidorMando md;
		double t = 0.0;
		for (int i = 0; i < 375; i++) { md.mensaje(7, t); t += 8.0; }
		const auto v = md.medir(t);
		const MedidaMando* m = buscar(v, 7);
		COMPROBAR(m != nullptr);
		COMPROBAR(m->muestras == MedidorMando::kIntervalos);
		COMPROBAR(m->mediaMs > 7.9 && m->mediaMs < 8.1);
		COMPROBAR(m->p95Ms > 7.9 && m->p95Ms < 8.1);
		COMPROBAR(m->jitterMs < 0.01);
		COMPROBAR(m->saltosMin == 0);
		COMPROBAR(m->ritmo >= 124 && m->ritmo <= 126);
	}

	// 2. Ejes X e Y por separado (2 mensajes en el mismo instante) cuentan como UNA llegada
	{
		MedidorMando md;
		double t = 0.0;
		for (int i = 0; i < 200; i++) { md.mensaje(3, t); md.mensaje(3, t + 0.3); t += 16.0; }
		const MedidaMando* m = buscar(md.medir(t), 3);
		COMPROBAR(m && m->mediaMs > 15.9 && m->mediaMs < 16.1);
		COMPROBAR(m && m->saltosMin == 0);
	}

	// 3. Red a saltos: cada 0,5 s la red retiene 60 ms y suelta 6 mensajes de golpe
	{
		MedidorMando md;
		double t = 0.0;
		for (int ciclo = 0; ciclo < 20; ciclo++) {
			for (int i = 0; i < 50; i++) { md.mensaje(9, t); t += 10.0; }
			t += 60.0;                                          // hueco
			for (int k = 0; k < 6; k++) md.mensaje(9, t + k * 0.1);   // ráfaga
			t += 10.0;
		}
		const MedidaMando* m = buscar(md.medir(t), 9);
		COMPROBAR(m != nullptr);
		COMPROBAR(m->saltosMin >= 18);
		COMPROBAR(m->mediaMs > 10.5);                      // los huecos sí suben la media
		COMPROBAR(m->jitterMs > 1.0);
	}

	// 4. Pausas del jugador (suelta el mando 150 ms y vuelve con UN mensaje): no son saltos
	{
		MedidorMando md;
		double t = 0.0;
		for (int ciclo = 0; ciclo < 20; ciclo++) {
			for (int i = 0; i < 30; i++) { md.mensaje(5, t); t += 8.0; }
			t += 150.0;
		}
		const MedidaMando* m = buscar(md.medir(t), 5);
		COMPROBAR(m && m->saltosMin == 0);
		COMPROBAR(m && m->mediaMs < 9.0);   // las pausas largas no entran en la media
	}

	// 5. Los saltos caducan al minuto; el ritmo cae a 0 sin mensajes; tick cierra la llegada
	{
		MedidorMando md;
		double t = 0.0;
		for (int i = 0; i < 50; i++) { md.mensaje(2, t); t += 10.0; }
		t += 60.0;
		for (int k = 0; k < 5; k++) md.mensaje(2, t + k * 0.1);
		md.tick(t + 5.0);
		COMPROBAR(buscar(md.medir(t + 5.0), 2)->saltosMin == 1);
		md.tick(t + 61000.0);
		const MedidaMando* m = buscar(md.medir(t + 61000.0), 2);
		COMPROBAR(m->saltosMin == 0);
		COMPROBAR(m->ritmo == 0);
		COMPROBAR(m->ultimoHaceMs > 60000.0);
	}

	// 6. Más de 16 jugadores: se recicla la ranura más callada, sin romper nada
	{
		MedidorMando md;
		for (uint32_t id = 1; id <= 20; id++) md.mensaje(id, id * 10.0);
		const auto v = md.medir(300.0);
		COMPROBAR(v.size() == MedidorMando::kJugadores);
		COMPROBAR(buscar(v, 20) != nullptr && buscar(v, 1) == nullptr);
		md.olvidar(20);
		COMPROBAR(buscar(md.medir(300.0), 20) == nullptr);
	}

	// 7. Evaluación y semáforo
	{
		MedidaMando bueno; bueno.muestras = 100; bueno.jitterMs = 2.0;
		DatosParsec p; p.pingMs = 30; p.codificarMs = 5.0; p.decodificarMs = 4.0;
		Evaluacion e = evaluar(&bueno, p);
		COMPROBAR(e.totalMs == 41);
		COMPROBAR(e.semaforo == "verde" && e.motivo.empty());

		p.pingMs = 95;
		e = evaluar(&bueno, p);
		COMPROBAR(e.semaforo == "ambar" && e.motivo == "ping alto");

		p.pingMs = 160;
		COMPROBAR(evaluar(&bueno, p).semaforo == "rojo");

		p.pingMs = 30; p.perdidaPct = 6.0;
		e = evaluar(&bueno, p);
		COMPROBAR(e.semaforo == "rojo" && e.motivo == "pierde paquetes");

		p.perdidaPct = 0.0;
		MedidaMando saltos = bueno; saltos.saltosMin = 8;
		e = evaluar(&saltos, p);
		COMPROBAR(e.semaforo == "ambar" && e.motivo == "mando a saltos");

		MedidaMando irregular = bueno; irregular.jitterMs = 60.0;
		e = evaluar(&irregular, p);
		COMPROBAR(e.semaforo == "ambar" && e.motivo == "mando irregular");

		// Sin ping y sin problemas: sin datos. Sin mando: solo ping + vídeo.
		DatosParsec sinPing;
		COMPROBAR(evaluar(&bueno, sinPing).semaforo == "sin_datos");
		COMPROBAR(evaluar(nullptr, sinPing).totalMs == -1);
		COMPROBAR(evaluar(nullptr, p).totalMs == 39);
	}

	// 8. Pérdida de paquetes en ventana
	{
		PerdidaPaquetes pp;
		COMPROBAR(pp.registrar(1, 1000, 0) == 0.0);
		COMPROBAR(pp.registrar(1, 1000, 0) == 0.0);           // repetido: no cuenta
		const double x = pp.registrar(1, 2000, 50);
		COMPROBAR(x > 4.9 && x < 5.1);
		COMPROBAR(pp.registrar(1, 100, 0) == 0.0);            // reconexión
		COMPROBAR(pp.registrar(2, 10, 0) == 0.0 && pp.registrar(2, 30, 5) == 0.0);   // pocos paquetes
	}

	// 9. Antispam del aviso
	{
		AvisosRetraso av;
		double t = 100.0;
		bool aviso = false;
		for (int i = 0; i < 9; i++) aviso |= av.actualizar(1, "rojo", true, t + i);
		COMPROBAR(!aviso);                                    // aún no 10 s seguidos
		COMPROBAR(av.actualizar(1, "rojo", true, t + 10));
		COMPROBAR(!av.actualizar(1, "rojo", true, t + 11));  // ya avisado
		COMPROBAR(!av.actualizar(1, "rojo", false, t + 12)); // fuera de partido: se reinicia
		for (int i = 0; i <= 10; i++) av.actualizar(2, "rojo", true, t + 13 + i);
		COMPROBAR(!av.actualizar(2, "rojo", true, t + 24));  // global: < 30 s del anterior
		COMPROBAR(av.actualizar(2, "rojo", true, t + 41));
		for (int i = 0; i <= 11; i++) av.actualizar(1, "rojo", true, t + 100 + i);
		COMPROBAR(!av.actualizar(1, "rojo", true, t + 112)); // < 3 min del suyo
		COMPROBAR(!av.actualizar(3, "ambar", true, t + 500));
	}

	if (fallos == 0) std::cout << "retraso_test: TODO OK\n";
	else std::cout << "retraso_test: " << fallos << " FALLOS\n";
	return fallos == 0 ? 0 : 1;
}
