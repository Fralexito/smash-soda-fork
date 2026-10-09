#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Option file de PES 2021 (EDIT00000000)
// -----------------------------------------------------------------------------
//  1) Se descifra con libpesXcrypter (clave PES 2021).
//  2) Se leen: equipos (nombre), plantillas (40 IDs + 40 dorsales por equipo)
//     y los pocos jugadores que el parche guardó completos («editados»).
//  3) Un fichaje = quitar el ID de la plantilla A y ponerlo en la B.
//     El ID del jugador NUNCA cambia (rostro, cuerpo y botas viajan con él).
//     También se corrige la ALINEACIÓN de los dos equipos (bloque de tácticas,
//     628 B por equipo: orden de formación en +0x1E4 y roles en +0x20C), igual
//     que lo hace la Liga Máster: si no, al quitar a un jugador toda la
//     formación se corre un puesto (error visto con Marchesín en Boca).
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

	/// Alineación de un equipo en el option file (bloque de tácticas). Índices de plantilla.
	struct AlineacionLocal {
		std::vector<uint8_t> orden;          ///< las n entradas que lee el juego (0–10 titulares, luego banca y reservas)
		std::array<uint8_t, 6> roles{};      ///< capitán, penales, tiros libres… (índices de plantilla)
		bool colaIdentidad = true;           ///< el editor del juego deja las entradas n…39 como identidad (n, n+1, …)
	};

	class OptionFile {
	public:
		/// Lee y descifra. No modifica el archivo de origen.
		static Resultado<OptionFile> abrir(const std::string& ruta);

		const std::vector<EquipoLocal>& equipos() const { return _equipos; }
		const std::map<uint32_t, std::vector<PlazaPlantilla>>& plantillas() const { return _plantillas; }
		const std::vector<JugadorEditado>& editados() const { return _editados; }

		/// Alineación (orden de formación + roles) del equipo, leída del bloque de tácticas.
		Resultado<AlineacionLocal> alineacion(uint32_t equipo) const;

		/// Quién debería cubrir el puesto del jugador si se va (0 = no hace falta nadie: no es titular ni tiene rol).
		/// `posicionDe(idJugador)` = posición registrada (0 PT … 12 DC) o -1 si no se sabe (viene del catálogo del parche).
		Resultado<uint32_t> sugerirSustituto(uint32_t equipo, uint32_t jugador, const std::function<int(uint32_t)>& posicionDe) const;

		/// Mueve un jugador al equipo destino. dorsal 0 = el más alto libre (99 hacia abajo).
		/// Si el jugador está en varios equipos (club + selección), solo se quita del
		/// club `equipoOrigen` (0 = el primero que no sea `equipoDestino`).
		/// `sustituto` = quién ocupa su puesto en la alineación del origen (obligatorio si era titular o tenía un rol;
		/// ver sugerirSustituto). El que llega entra como última reserva en el destino. Todo o nada.
		Resultado<bool> mover(uint32_t jugador, uint32_t equipoDestino, uint32_t equipoOrigen = 0, uint16_t dorsal = 0, uint32_t sustituto = 0);

		/// Saca al jugador del equipo sin ponerlo en otro (queda libre). Mismas reglas de alineación que `mover`.
		Resultado<bool> quitar(uint32_t jugador, uint32_t equipo, uint32_t sustituto = 0);

		/// Cifra y escribe en `rutaNueva`. Falla si ya existe (nunca sobrescribe).
		Resultado<std::string> guardarComo(const std::string& rutaNueva) const;

	private:
		std::vector<uint8_t> _cifrado;   // archivo original (para reutilizar cabeceras)
		std::vector<uint8_t> _datos;     // bloque de DATOS descifrado
		std::vector<EquipoLocal> _equipos;
		std::map<uint32_t, std::vector<PlazaPlantilla>> _plantillas;
		std::map<uint32_t, size_t> _offsetPlantilla;  // equipo → posición en _datos
		std::map<uint32_t, size_t> _offsetTactica;    // equipo → posición del bloque de tácticas en _datos
		std::vector<JugadorEditado> _editados;

		void escribirPlantilla(uint32_t equipo);
		Resultado<AlineacionLocal> leerAlineacion(uint32_t equipo, size_t n) const;
		void escribirAlineacion(uint32_t equipo, const AlineacionLocal& a);
	};

	/// Lee `bits` bits (little-endian) desde la posición de bit `bitPos` de `base`.
	uint32_t leerBits(const uint8_t* base, size_t bitPos, int bits);
	std::string leerTexto(const uint8_t* p, size_t max);

}
