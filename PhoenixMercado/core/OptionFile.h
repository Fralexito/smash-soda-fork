#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Mercado · Option file de PES 2021 (EDIT00000000)
// -----------------------------------------------------------------------------
//  1) Se descifra con libpesXcrypter (clave PES 2021).
//  2) Se leen: equipos (nombre), plantillas (40 IDs + 40 dorsales por equipo)
//     y los pocos jugadores que el parche guardó completos («editados»).
//  3) Un fichaje = quitar el ID de la plantilla A y ponerlo en la B.
//     El ID del jugador NUNCA cambia (rostro, cuerpo y botas viajan con él).
//  4) Se vuelve a cifrar y se guarda SIEMPRE en un archivo nuevo.
//  Posiciones tomadas de 4ccEditor (pes20.cpp, versión 21) y verificadas con
//  un option file real del ConmeGOL Patch 26.
// =============================================================================

namespace mercado {

	struct EquipoLocal {
		uint32_t id = 0;
		std::string nombre, abreviatura;
	};

	struct PlazaPlantilla {
		uint32_t jugador = 0;
		uint16_t dorsal = 0;
	};

	struct JugadorEditado {
		uint32_t id = 0;
		std::string nombre, nombreCamiseta;
		int nacionalidad = 0, altura = 0, peso = 0, edad = 0, posicion = 0;
	};

	class OptionFile {
	public:
		/// Lee y descifra. No modifica el archivo de origen.
		static Resultado<OptionFile> abrir(const std::string& ruta);

		const std::vector<EquipoLocal>& equipos() const { return _equipos; }
		const std::map<uint32_t, std::vector<PlazaPlantilla>>& plantillas() const { return _plantillas; }
		const std::vector<JugadorEditado>& editados() const { return _editados; }

		/// Mueve un jugador al equipo destino. dorsal 0 = el más alto libre (99 hacia abajo).
		/// Si el jugador está en varios equipos (club + selección), solo se quita del
		/// club `equipoOrigen` (0 = el primero que no sea `equipoDestino`).
		Resultado<bool> mover(uint32_t jugador, uint32_t equipoDestino, uint32_t equipoOrigen = 0, uint16_t dorsal = 0);

		/// Cifra y escribe en `rutaNueva`. Falla si ya existe (nunca sobrescribe).
		Resultado<std::string> guardarComo(const std::string& rutaNueva) const;

	private:
		std::vector<uint8_t> _cifrado;   // archivo original (para reutilizar cabeceras)
		std::vector<uint8_t> _datos;     // bloque de DATOS descifrado
		std::vector<EquipoLocal> _equipos;
		std::map<uint32_t, std::vector<PlazaPlantilla>> _plantillas;
		std::map<uint32_t, size_t> _offsetPlantilla;  // equipo → posición en _datos
		std::vector<JugadorEditado> _editados;

		void escribirPlantilla(uint32_t equipo);
	};

	/// Lee `bits` bits (little-endian) desde la posición de bit `bitPos` de `base`.
	uint32_t leerBits(const uint8_t* base, size_t bitPos, int bits);
	std::string leerTexto(const uint8_t* p, size_t max);

}
