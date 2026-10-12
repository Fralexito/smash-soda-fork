#include "LadoPantalla.h"

#include <atomic>

namespace phoenix::ladopantalla {

	namespace {
		std::atomic<Proveedor> g_proveedor{ nullptr };
		bool valorValido(int8_t v) { return v == kIzquierda || v == kCentro || v == kDerecha || v == kDesconocido; }
	}

	void Estabilizador::alimentar(double t, const Lectura& l) {
		if (!l.valida) return;
		for (int i = 0; i < kCasillas; i++) if (!valorValido(l.lado[i])) return;   // lectura rara: no cuenta nada
		if (!vivo(t)) {   // tras un corte se empieza de cero (no se arrastra lo de antes)
			for (int i = 0; i < kCasillas; i++) { estable_[i] = kDesconocido; candidato_[i] = kDesconocido; desde_[i] = t; }
		}
		tValida_ = t;
		for (int i = 0; i < kCasillas; i++) {
			if (l.lado[i] != candidato_[i]) { candidato_[i] = l.lado[i]; desde_[i] = t; }
			if (candidato_[i] != estable_[i] && t - desde_[i] >= kEstableSeg) estable_[i] = candidato_[i];
		}
	}

	char Estabilizador::lado(int i, double t) const {
		if (i < 0 || i >= kCasillas || !vivo(t)) return '?';
		switch (estable_[i]) {
		case kIzquierda: return 'A';
		case kDerecha: return 'B';
		case kCentro: return '-';
		default: return '?';
		}
	}

	void fijarProveedor(Proveedor p) { g_proveedor = p; }

	bool leer(Lectura& out) {
		const Proveedor p = g_proveedor.load();
		if (!p) return false;
		try { return p(out) && out.valida; }
		catch (...) { return false; }
	}

}
