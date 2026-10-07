#pragma once

#include <string>

// =============================================================================
//  Phoenix Soda · Preferencias de interfaz
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

		static PhoenixPrefs& get();
		void cargar();
		void guardar() const;
	};

}
