#pragma once

#include <string>
#include <vector>

// =============================================================================
//  Phoenix Soda · Cliente HTTPS mínimo sobre WinHTTP (viene con Windows:
//  cero dependencias nuevas). Bloqueante: usar SOLO desde el hilo de PhoenixLink.
// =============================================================================

namespace phoenix::http {

	struct Respuesta {
		int estado = 0;          ///< código HTTP; 0 = error de red / timeout
		std::string cuerpo;
		std::string error;       ///< descripción si estado == 0
	};

	/// `url` completa https://host/ruta. `cabeceras` en formato "Nombre: valor".
	Respuesta peticion(const std::string& metodo, const std::string& url,
		const std::string& cuerpo, const std::vector<std::string>& cabeceras, int timeoutMs = 5000);

}
