#pragma once

#include <string>

// =============================================================================
//  Phoenix Link · Preferencias de interfaz
// -----------------------------------------------------------------------------
//  Archivo propio `phoenix-ui.json` en la carpeta de configuración de Smash
//  Soda. Separado del config.json del autor para no mezclar formatos.
//  Si el archivo falta o está dañado se usan los valores por defecto.
// =============================================================================

namespace phoenix {

	struct PhoenixPrefs {
		bool interfazPhoenix = true;  ///< false = interfaz clásica del Smash Soda
		std::string idioma = "es";
		int seccion = 0;              ///< sección abierta al iniciar

		// Sala (se enviarán a la web con PhoenixLink)
		std::string visibilidad = "amigos"; ///< publica | amigos | privada
		bool espectadores = true;
		bool entradaParsec = false; ///< dejar entrar a quien no está en la lista (el host decide)
		int limiteEspectadores = 4;
		std::string juego = "eFootball PES 2021";
		std::string parche = "Conmegol";
		std::string region = "Lima";
		int mandosActivos = 2;    ///< 2–8 mandos visibles en el tablero
		int equipoLocal = 1;      ///< cuántos mandos son del equipo local

		static PhoenixPrefs& get();
		void cargar();
		void guardar() const;
	};

}
