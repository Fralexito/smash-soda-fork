#include "RetrasoMando.h"

#include <atomic>
#include <mutex>

#include <windows.h>

namespace phoenix::retraso {

	namespace {
		MedidorMando g_medidor;                 // solo el hilo de entrada
		double g_ultimaPublicacion = 0.0;       // solo el hilo de entrada
		std::atomic<bool> g_limpiar{ false };
		std::mutex g_mutex;                     // solo para la copia publicada (1 vez por segundo)
		std::vector<MedidaMando> g_publicada;

		double ahoraMs() {
			static const double porMs = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return static_cast<double>(f.QuadPart) / 1000.0; }();
			LARGE_INTEGER c;
			QueryPerformanceCounter(&c);
			return static_cast<double>(c.QuadPart) / porMs;
		}
	}

	void alMensajeMando(uint32_t userId) {
		g_medidor.mensaje(userId, ahoraMs());
	}

	void alGirar() {
		try {
			const double t = ahoraMs();
			if (g_limpiar.exchange(false)) g_medidor.limpiar();
			g_medidor.tick(t);
			if (t - g_ultimaPublicacion < 1000.0) return;
			g_ultimaPublicacion = t;
			std::vector<MedidaMando> v = g_medidor.medir(t);
			std::lock_guard<std::mutex> l(g_mutex);
			g_publicada.swap(v);
		}
		catch (...) {
			// El medidor nunca debe tumbar la entrada de los mandos.
		}
	}

	std::vector<MedidaMando> instantanea() {
		std::lock_guard<std::mutex> l(g_mutex);
		return g_publicada;
	}

	void pedirLimpieza() {
		g_limpiar = true;
	}

}
