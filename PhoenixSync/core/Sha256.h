#pragma once

#include <string>

// Phoenix Sync · SHA-256 portable (sin Windows), para el antitrampa:
// la huella de un archivo cambia por completo si cambia un solo byte.
namespace mercado::sha256 {

	std::string deTexto(const std::string& datos);          ///< 64 caracteres hex
	/// Huella de un archivo leído por bloques. Vacío si no se pudo abrir.
	std::string deArchivo(const std::string& ruta, std::string* error = nullptr);

}
