#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Mercado · Base de datos del parche (dentro de un .cpk)
// -----------------------------------------------------------------------------
//  .cpk (formato CRI)  →  common/etc/pesdb/Player.bin
//  Player.bin: cabecera WESYS + zlib  →  filas de 312 bytes, una por jugador.
//  Las posiciones de cada campo se dedujeron comparando con el option file
//  (ConmeGOL Patch 26). Las habilidades se guardan como (valor − 40) en 6 bits.
//  Pendiente: habilidades de portero y fecha de nacimiento (solo hay edad).
// =============================================================================

namespace mercado {

	struct FichaJugador {
		uint32_t id = 0;
		std::string nombre, nombreCamiseta;
		int nacionalidad = 0, altura = 0, peso = 0, edad = 0, posicion = 0;
		std::map<std::string, int> habilidades;   ///< ataque, velocidad, defensa…
	};

	namespace cpk {
		/// Lista los archivos internos del .cpk.
		Resultado<std::vector<std::string>> listar(const std::string& rutaCpk);
		/// Extrae un archivo (descomprime CRILAYLA si hace falta).
		Resultado<std::vector<uint8_t>> extraer(const std::string& rutaCpk, const std::string& rutaInterna);
	}

	/// Quita la cabecera WESYS y descomprime zlib (archivos de pesdb).
	Resultado<std::vector<uint8_t>> descomprimirWesys(const std::vector<uint8_t>& datos);

	/// Lee Player.bin (ya descomprimido).
	Resultado<std::map<uint32_t, FichaJugador>> leerPlayerBin(const std::vector<uint8_t>& raw);

	/// Atajo: abre el .cpk, busca Player.bin y lo lee.
	Resultado<std::map<uint32_t, FichaJugador>> leerBaseDatos(const std::string& rutaCpk);

	extern const char* const kPosiciones[13];   // GK, CB, LB…

}
