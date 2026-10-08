#pragma once

#include <functional>

// =============================================================================
//  Phoenix Link · Shell (interfaz principal)
// -----------------------------------------------------------------------------
//  Reemplaza las 16 ventanas flotantes por una app de distribución fija:
//
//   ┌ barra superior: marca · estado de la sala · idioma · interfaz clásica ┐
//   │ menú │ contenido de la sección (paneles acoplados)      │   chat      │
//   └ barra inferior: versión · cuenta Parsec · invitados                   ┘
//
//  No conoce a los paneles: recibe funciones que los dibujan (Paneles). Así el
//  shell no depende de las clases del autor y Main.cpp solo hace el cableado.
//  Reversible: PhoenixPrefs::interfazPhoenix = false vuelve a la interfaz clásica.
// =============================================================================

namespace phoenix {

	struct Paneles {
		std::function<void()> configSala;   // HostSettings
		std::function<void()> chat;
		std::function<void()> actividad;    // Log
		std::function<void()> invitados;    // GuestList
		std::function<void()> mandos;       // Gamepads
		std::function<void()> puppets;      // MasterOfPuppets
		std::function<void()> hotseat;
		std::function<void()> bloqueo;      // ButtonLock
		std::function<void()> teclado;      // KeyboardMap
		std::function<void()> general;      // Settings
		std::function<void()> video;
		std::function<void()> audio;
		std::function<void()> streaming;
		std::function<void()> overlay;      // Overlay
		std::function<void()> biblioteca;   // Library
		std::function<void()> avanzado;     // Advanced
	};

	class ProveedorSala;

	class Shell {
	public:
		/// Dibuja la interfaz Phoenix completa (un frame).
		static void render(const Paneles& paneles);

		/// En la interfaz clásica: botón flotante para volver a Phoenix.
		static void renderBotonVolver();

		/// Conecta el motor de host (nullptr = app sin modo host).
		static void establecerProveedor(ProveedorSala* proveedor);

		/// Lógica que debe correr cada frame aunque el shell no se dibuje
		/// (web y asientos, Ctrl+Alt+N). render() ya la llama.
		static void tickLogica();

		/// Abre una sección y pestaña (0 = Sala, 1 = Mandos, 2 = Gente, 3 = Ajustes).
		/// Lo usa la interfaz web para «Abrir en el panel clásico».
		static void irA(int seccion, int pestana);
	};

}
