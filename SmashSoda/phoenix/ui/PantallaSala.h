#pragma once

#include <functional>
#include "imgui.h"

namespace phoenix {

	class ProveedorSala;

	// =========================================================================
	//  Pantalla «Sala» (rediseñada desde cero, R2)
	//  - Tarjeta principal: estado, nombre, Abrir/Cerrar sala, enlace con copiar
	//  - Visibilidad (pública/amigos/privada) y espectadores
	//  - Asientos (un mando = una tarjeta) y espectadores
	//  - Actividad a la derecha; «Configuración avanzada» abre el panel original
	// =========================================================================
	class PantallaSala {
	public:
		/// `acoplar(dibujar, pos, tam)` lo provee el shell para paneles del autor.
		using Acoplador = std::function<void(const std::function<void()>&, ImVec2, ImVec2)>;

		static void render(ProveedorSala* sala,
			const std::function<void()>& panelActividad,
			const std::function<void()>& panelAvanzado,
			const Acoplador& acoplar,
			ImVec2 pos, ImVec2 tam, float alfa);
	};

}
