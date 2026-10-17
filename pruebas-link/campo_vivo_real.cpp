// Prueba manual con el juego abierto: enciende el lector y escribe lo que lee ~2 veces por segundo (20 s).
// Compilar con MSVC: cl /EHsc /std:c++20 /I SmashSoda pruebas-link\campo_vivo_real.cpp SmashSoda\phoenix\host\CampoVivoHost.cpp SmashSoda\phoenix\core\CampoVivo.cpp SmashSoda\phoenix\link\Entrega.cpp bcrypt.lib
#include <chrono>
#include <cstdio>
#include <thread>

#include "phoenix/host/CampoVivo.h"

int main() {
	using namespace phoenix::campo;
	iniciar();
	pedirVivo(true);
	const char* nombres[] = { "apagado", "esperando", "buscando", "activo", "no_compatible" };
	for (int i = 0; i < 120; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		FotoCampo f = instantanea();
		std::printf("[%2d] %-13s %zu piezas seq=%u equipos=%d balon=%s(%.1f,%.2f,%.1f) %s\n", i, nombres[static_cast<int>(f.estado)], f.j.size(), f.seq,
			(int)f.equiposListos, f.hayBola ? "si" : "no", f.bx, f.by, f.bz, f.motivo.c_str());
		if (f.estado == EstadoCampo::Activo && i % 4 == 0)
			for (size_t k = 0; k < f.j.size() && k < 6; k++) std::printf("      #%zu (%.1f, %.1f) eq=%d\n", k, f.j[k].x, f.j[k].z, f.j[k].eq);
	}
	pedirVivo(false);
	detener();
	return 0;
}
