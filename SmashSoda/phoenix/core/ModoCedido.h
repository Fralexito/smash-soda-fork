#pragma once

// =============================================================================
//  Phoenix Link · Modo cedido (lógica pura, sin Windows: se prueba aparte en
//  pruebas-link/cedido_test.cpp).
// -----------------------------------------------------------------------------
//  El anfitrión cede su PC a UN invitado de Parsec: ese invitado maneja con
//  teclado y ratón solo Phoenix Link y PES 2021. Link inyecta él mismo la
//  entrada de los invitados (InputControlService → SendInput), así que el
//  filtro trabaja ANTES de inyectar: lo bloqueado nunca llega a Windows.
//  El teclado y el ratón físicos del anfitrión no pasan por aquí.
//  La parte de Windows (ventanas, foco, abrir PES) vive en phoenix/host/Cedido.
// =============================================================================

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace phoenix::cedido {

	/// Dónde cae una tecla (ventana con el foco) o un clic (ventana bajo el cursor).
	enum class Ventana { Pes, Link, Otra, Nada };

	// ---- Acciones del puente ------------------------------------------------
	/// ¿Se puede hacer esta acción de la interfaz mientras la PC está cedida?
	/// Lista blanca: lo nuevo que no esté aquí queda bloqueado.
	bool accionPermitida(const std::string& accion, const nlohmann::json& datos);

	// ---- Ventanas -----------------------------------------------------------
	/// Nombre de archivo en minúsculas ("C:\\X\\PES2021.exe" → "pes2021.exe").
	std::string nombreExe(const std::string& ruta);
	/// Clasifica un proceso por su ruta.
	///  - esLink: el propio Link o un proceso suyo de WebView2.
	///  - carpetaJuego: carpeta de PES2021.exe (todo lo de dentro cuenta como PES: sider, Settings…).
	///  - extras: más ejecutables que el anfitrión quiera permitir (nombres, sin ruta).
	Ventana clasificar(const std::string& rutaExe, bool esLink, const std::string& carpetaJuego,
		const std::string& exeJuego, const std::vector<std::string>& extras);
	/// ¿Hay que devolver el foco a PES/Link? Solo si el foco está en otra cosa
	/// y el anfitrión no está usando su PC (teclado/ratón físicos) desde hace `quietoMs`.
	bool debeRecuperarFoco(Ventana foco, uint64_t msDesdeFisico, uint64_t quietoMs = 15000);

	// ---- Filtro de entrada del invitado ------------------------------------------
	/// Códigos de tecla virtual de Windows que usa el filtro (sin windows.h).
	namespace vk {
		constexpr uint16_t TAB = 0x09, ESC = 0x1B, ESPACIO = 0x20, IMPR = 0x2C,
			LWIN = 0x5B, RWIN = 0x5C, APPS = 0x5D, F4 = 0x73, F7 = 0x76, F12 = 0x7B,
			SHIFT = 0x10, CTRL = 0x11, ALT = 0x12, LSHIFT = 0xA0, RSHIFT = 0xA1,
			LCTRL = 0xA2, RCTRL = 0xA3, LALT = 0xA4, RALT = 0xA5;
	}

	/// Una sola instancia, en el hilo de entrada. Recuerda qué dejó pasar para
	/// que cada «soltar» siga a su «pulsar» (nada queda pegado ni a medias).
	class Filtro {
	public:
		/// true = inyectar. `foco` = ventana con el foco ahora.
		bool tecla(uint16_t codigo, bool pulsada, Ventana foco);
		/// Botón del ratón (0 izq, 1 der, 2 medio, 3 X1, 4 X2). `bajo` = ventana bajo el cursor.
		bool boton(int b, bool pulsado, Ventana bajo);
		bool rueda(Ventana bajo) const { return bajo == Ventana::Pes || bajo == Ventana::Link; }
		void reiniciar();

		bool ctrl() const;
		bool alt() const;
		bool shift() const;

	private:
		bool decidir(uint16_t codigo, Ventana foco) const;
		std::array<bool, 256> _abajo{};    ///< pulsada y dejada pasar: su «soltar» pasa
		std::array<bool, 256> _tragada{};  ///< pulsada y bloqueada: su «soltar» también se traga
		std::array<bool, 5> _botonAbajo{};
		std::array<bool, 5> _botonTragado{};
	};

}
