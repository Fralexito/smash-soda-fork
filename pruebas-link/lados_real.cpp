// Prueba manual con el juego: ¿en qué equipo juega cada mando? (docs/DETECCION-LADOS.md)
// Enciende el lector del Campo 2D (solo lectura de PES2021.exe, el mismo que usa Link) y lee los mandos XInput 0-3
// de este PC (los físicos y los virtuales de Link aparecen igual). Cada 2 s escribe el veredicto por mando y, si ve
// un gol, qué equipo marcó. Además graba todo en lados_AAAAMMDD_HHMMSS.csv para afinar después sin repetir el partido.
// No toca el juego ni Link: no escribe memoria, no manda nada a los mandos.
// Uso: lados_real.exe [segundos=240]
// Compilar con MSVC: cl /EHsc /std:c++20 /I SmashSoda pruebas-link\lados_real.cpp SmashSoda\phoenix\core\DeteccionLados.cpp
//   SmashSoda\phoenix\host\CampoVivo.cpp SmashSoda\phoenix\core\CampoVivo.cpp SmashSoda\phoenix\link\Entrega.cpp bcrypt.lib xinput.lib
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <thread>

#define NOMINMAX
#include <windows.h>
#include <Xinput.h>

#include "phoenix/core/DeteccionLados.h"
#include "phoenix/host/CampoVivo.h"

using namespace phoenix;

int main(int argc, char** argv) {
	const int segundos = argc > 1 ? std::atoi(argv[1]) : 240;
	char nombre[64]; const std::time_t ahora = std::time(nullptr); std::tm tmv{}; localtime_s(&tmv, &ahora);
	std::strftime(nombre, sizeof nombre, "lados_%Y%m%d_%H%M%S.csv", &tmv);
	FILE* csv = nullptr; fopen_s(&csv, nombre, "w");
	if (csv) std::fprintf(csv, "t,lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3,bola,bx,by,bz,n,fichas(x;z;eq)...\n");

	campo::iniciar();
	campo::pedirVivo(true);
	lados::Detector det;
	lados::LadoDefendido ld;
	const char* estados[] = { "apagado", "esperando", "buscando", "activo", "no_compatible" };
	const char* orient[] = { "x normal, z normal", "x invertida", "z invertida", "x y z invertidas" };
	const auto t0 = std::chrono::steady_clock::now();
	uint32_t ultimaSeq = 0; double proximoInforme = 2, golHasta = 0;

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
		if (t > segundos) break;
		const campo::FotoCampo f = campo::instantanea();
		if (f.estado != campo::EstadoCampo::Activo || f.seq == ultimaSeq) {
			if (t >= proximoInforme) { std::printf("[%5.0fs] campo: %s %s\n", t, estados[static_cast<int>(f.estado)], f.motivo.c_str()); proximoInforme = t + 2; }
			continue;
		}
		ultimaSeq = f.seq;

		std::vector<lados::Stick> sticks;
		float ejes[8] = {};
		for (DWORD i = 0; i < 4; i++) {
			XINPUT_STATE st{};
			if (XInputGetState(i, &st) != ERROR_SUCCESS) continue;
			const float lx = st.Gamepad.sThumbLX / 32767.0f, ly = st.Gamepad.sThumbLY / 32767.0f;
			ejes[i * 2] = lx; ejes[i * 2 + 1] = ly;
			sticks.push_back({ static_cast<int>(i) + 1, lx, ly });
		}
		std::vector<lados::Ficha> fichas;
		for (const campo::Pieza& p : f.j) fichas.push_back({ p.x, p.z, p.eq });
		if (f.equiposListos) { det.alimentar(t, fichas, sticks); ld.alimentar(t, fichas); }

		if (csv) {
			std::fprintf(csv, "%.3f", t);
			for (float e : ejes) std::fprintf(csv, ",%.3f", e);
			std::fprintf(csv, ",%d,%.2f,%.2f,%.2f,%zu", (int)f.hayBola, f.bx, f.by, f.bz, f.j.size());
			for (const campo::Pieza& p : f.j) std::fprintf(csv, ",%.2f;%.2f;%d", p.x, p.z, p.eq);
			std::fprintf(csv, "\n");
		}

		if (f.hayBola && t >= golHasta) {
			const int lado = lados::porteriaDelGol(f.bx, f.by, f.bz);
			if (lado != 0) {
				const int eq = lados::equipoQueMarco(lado, ld);
				std::printf("[%5.0fs] ** GOL en la porteria %s -> marco el equipo %s (A defiende %d, B defiende %d)\n", t, lado < 0 ? "izquierda" : "derecha",
					eq == 0 ? "A" : eq == 1 ? "B" : "?", ld.lado(0), ld.lado(1));
				golHasta = t + 15;
			}
		}

		if (t >= proximoInforme) {
			proximoInforme = t + 2;
			std::printf("[%5.0fs] equipos=%s camara: %s%s | A defiende %s\n", t, f.equiposListos ? "si" : "no (falta un saque de centro)",
				orient[det.orientacion()], det.orientacionClara() ? "" : " (dudosa)", ld.lado(0) < 0 ? "izquierda" : ld.lado(0) > 0 ? "derecha" : "?");
			for (const lados::Veredicto& v : det.veredictos())
				std::printf("         mando XInput %d: %s  (A %d / B %d de %d con stick, %.0f%%)%s\n", v.asiento, v.eq == 0 ? "equipo A" : v.eq == 1 ? "equipo B" : "?",
					v.votosA, v.votosB, v.apretado, v.proporcion * 100, v.decidido ? "  DECIDIDO" : "");
		}
	}
	campo::pedirVivo(false);
	campo::detener();
	if (csv) std::fclose(csv);
	std::printf("Grabado en %s\n", nombre);
	return 0;
}
