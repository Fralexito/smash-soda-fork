#pragma once

#include "imgui.h"

// =============================================================================
//  Phoenix Link · Acople de paneles
// -----------------------------------------------------------------------------
//  Los paneles del autor abren su propia ventana flotante en Widget::startWidget.
//  El shell Phoenix reserva un rectángulo y llama a colocar(); el siguiente
//  startWidget lo consume (una sola vez) y dibuja el panel fijo, sin barra de
//  título, sin mover ni redimensionar. Si nadie llama a colocar(), el panel se
//  comporta exactamente como el original (interfaz clásica).
// =============================================================================

namespace phoenix {

	class UiDock {
	public:
		static void colocar(const ImVec2& pos, const ImVec2& tam) {
			_pendiente = true;
			_pos = pos;
			_tam = tam;
		}

		/// Devuelve true (y el destino) si hay un acople pendiente; lo consume.
		static bool tomar(ImVec2& pos, ImVec2& tam) {
			if (!_pendiente) return false;
			_pendiente = false;
			pos = _pos;
			tam = _tam;
			return true;
		}

		static void cancelar() { _pendiente = false; }

	private:
		static inline bool _pendiente = false;
		static inline ImVec2 _pos = ImVec2(0, 0);
		static inline ImVec2 _tam = ImVec2(0, 0);
	};

}
