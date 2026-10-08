#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "SobrePes.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Mercado · Liga Máster interna de PES 2021 (guardados ML0000000N)
// -----------------------------------------------------------------------------
//  Estructura y pruebas en el juego: liga-master/ESTRUCTURA-ML.md.
//
//  Qué hace hoy este módulo (y qué NO):
//   · Lee los 700 equipos (nombre, plantilla, dorsales).                         [probado]
//   · Mueve un jugador entre dos equipos de la IA: solo cambian la lista de      [probado en el
//     plantilla, los dorsales y el contador de cada bloque de equipo.             juego: Mbappé → Santos]
//   · Detecta el equipo del USUARIO (tiene tablas alineadas con su plantilla) y
//     se NIEGA a tocarlo por este camino: ese equipo guarda datos extra por
//     jugador (tablas, alineación, listas ordenadas) que aún no se editan aquí.
//   · Guarda siempre en un archivo NUEVO (nunca sobrescribe) y lo verifica.
// =============================================================================

namespace mercado::lm {

	constexpr int kNumEquipos = 700;
	constexpr int kMaxPlantilla = 40;

	/// Una plaza de la plantilla de un equipo.
	struct Plaza {
		uint32_t reg = 0;      ///< ranura global permanente del jugador (NO cambia al traspasar)
		uint32_t pid = 0;      ///< ID del jugador (el mismo del option file y del catálogo)
		uint16_t dorsal = 0;
	};

	struct EquipoLM {
		int indice = 0;        ///< posición del bloque (City = 154 en el guardado de referencia)
		std::string nombre;
		uint32_t idOption = 0; ///< ID del equipo en el option file
		std::vector<Plaza> plantilla;
	};

	/// Tabla que el juego guarda SOLO para el equipo del usuario, con un registro por jugador y
	/// el mismo orden que su plantilla (dir +1) o el inverso (dir -1). Cada registro: [x][reg][pid]…
	struct TablaAlineada {
		size_t ofsReg0 = 0;    ///< posición del campo `reg` del registro 0
		size_t stride = 0;     ///< bytes entre registros
		int dir = +1;
	};

	class GuardadoLM {
	public:
		/// Lee y descifra un guardado ML. No modifica el archivo de origen.
		static Resultado<GuardadoLM> abrir(const std::string& ruta);
		/// Para pruebas y herramientas: DATOS ya descifrados (sin sobre, no se puede guardar).
		static Resultado<GuardadoLM> desdeDatos(std::vector<uint8_t> datos);

		Resultado<EquipoLM> equipo(int indice) const;
		/// Equipos (de club y selección) en cuya plantilla está el jugador.
		std::vector<int> equiposDe(uint32_t pid) const;

		/// Tablas alineadas halladas para ese equipo (vacío = equipo de la IA).
		std::vector<TablaAlineada> tablasDe(int indice) const;
		bool esEquipoUsuario(int indice) const { return tablasDe(indice).size() >= 3; }

		/// Mueve `pid` de `origen` a `destino` (los dos de la IA). dorsal 0 o ya ocupado = el más alto libre.
		/// Devuelve el dorsal que quedó. Todo o nada: si falla, no cambia nada.
		Resultado<uint16_t> moverEntreIA(int origen, int destino, uint32_t pid, uint16_t dorsal = 0);

		const std::vector<uint8_t>& datos() const { return _datos; }

		/// Cifra y escribe en `rutaNueva` (falla si existe; verifica releyendo). `textoInfo` vacío = conserva el original.
		Resultado<std::string> guardarComo(const std::string& rutaNueva, const std::string& textoInfo = "") const;

	private:
		std::vector<uint8_t> _datos;
		std::optional<SobrePes> _sobre;   // ausente si vino de desdeDatos()
	};

}
