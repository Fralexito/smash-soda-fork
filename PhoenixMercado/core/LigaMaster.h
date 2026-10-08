#pragma once

#include <array>
#include <cstdint>
#include <functional>
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
//  Qué hace hoy este módulo:
//   · Lee los 700 equipos (nombre, plantilla, dorsales).                         [probado]
//   · Mueve un jugador entre dos equipos de la IA: plantilla, dorsales, contador
//     y la ALINEACIÓN de cada uno (bloques de 600 B, §10).                       [pendiente de verlo en el juego]
//   · Vende un jugador del equipo del USUARIO a la IA: 12+ tablas por jugador,
//     orden de formación, roles, lista K, plantillas y alineación del destino.  [probado en el juego salvo §10]
//   · Propone sustitutos por posición (reservas primero, como el juego).
//   · Guarda siempre en un archivo NUEVO (nunca sobrescribe) y lo verifica.
//  Qué NO hace todavía: fichar PARA el usuario (IA → usuario).
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

	/// Alineación del equipo del usuario (solo él la tiene). Tres piezas que el juego mantiene en espejo:
	///   · `orden`: índices de plantilla en orden de formación (0–10 XI; después la banca —12 en este guardado— y las reservas),
	///     40 bytes en el archivo, los libres a 0xff;
	///   · `roles`: 6 índices de plantilla (capitán, lanzadores…) justo después (+0x28);
	///   · lista K: un registro de 16 B `[flag, reg, pid, 0]` por posición de `orden` (K[i] = plantilla[orden[i]]),
	///     seguida de un registro libre (reg 0xffff) con flag 0xc0+ y luego 0xc7. Estrategia lee K.
	struct Alineacion {
		size_t ofsOrden = 0;
		size_t ofsRoles = 0;
		size_t ofsK = 0;                ///< campo `flag` del registro K0
		std::vector<uint8_t> orden;     ///< tantas entradas como jugadores
		std::array<uint8_t, 6> roles{};
		std::vector<uint32_t> flagsK;   ///< flag de cada K[i] usado
		uint32_t flagLibreK = 0;        ///< flag del primer registro libre (0xc0 con 25 jugadores, 0xc1 con 26)
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

		/// Tablas alineadas halladas para ese equipo (vacío = equipo de la IA). Son CANDIDATAS: tras jugar un partido
		/// también aparece la ficha de ese partido (lista a los que jugaron); moverUsuarioAIA la descarta.
		std::vector<TablaAlineada> tablasDe(int indice) const;
		bool esEquipoUsuario(int indice) const { return tablasDe(indice).size() >= 3; }

		/// Mueve `pid` de `origen` a `destino` (los dos de la IA). dorsal 0 o ya ocupado = el más alto libre.
		/// Además de las plantillas, actualiza la ALINEACIÓN de los dos equipos (bloques de 600 B, ESTRUCTURA-ML.md §10):
		/// en el origen, si era titular o tenía un rol, `pidSustituto` (del mismo equipo) ocupa su puesto; en el
		/// destino, entra como última reserva. Devuelve el dorsal que quedó. Todo o nada: si falla, no cambia nada.
		Resultado<uint16_t> moverEntreIA(int origen, int destino, uint32_t pid, uint16_t dorsal = 0, uint32_t pidSustituto = 0);

		/// Propone quién cubre el puesto de `pid` en el equipo `k` si se va (0 = no hace falta nadie).
		/// Hace falta sustituto solo si es titular (puestos 0–10) o tiene un rol.
		/// `posicionDe(pid)` da la posición registrada (0 PT, 1 DC, 2 LI, 3 LD, 4 MCD, 5 MC, 6 II, 7 ID, 8 MP,
		/// 9 EI, 10 ED, 11 SD, 12 DC) o -1 si no se sabe. Busca desde el final de la lista (reservas) hacia la banca:
		/// misma posición > mismo grupo (p. ej. LI-LD) > misma línea > cualquiera; un portero solo lo cubre otro portero.
		Resultado<uint32_t> sugerirSustituto(int k, uint32_t pid, const std::function<int(uint32_t)>& posicionDe) const;

		/// Alineación del equipo del USUARIO, localizada por anclas (no por direcciones fijas).
		Resultado<Alineacion> alineacionDe(int indice) const;

		/// Mueve `pid` del equipo del USUARIO `kUsuario` a un equipo de la IA `kDestino` (venta / cesión).
		/// Reproduce exactamente lo probado en el juego (prototipo v6, 8 oct 2026):
		///   · compacta las 12 tablas alineadas del usuario (A…M y L),
		///   · en la alineación, `pidSustituto` ocupa el puesto del que se va y desaparece de su sitio anterior;
		///     si el que se va no es titular ni tiene rol puede ir sin sustituto (0),
		///   · reescribe la lista K como espejo de la alineación, la plantilla y los dorsales del usuario,
		///   · añade al jugador al final de la plantilla del destino con `dorsal` (0 u ocupado = el más alto libre)
		///     y como última reserva en la alineación del destino.
		/// Todo o nada: si algo no cuadra, no cambia un solo byte. Devuelve el dorsal que quedó.
		Resultado<uint16_t> moverUsuarioAIA(int kUsuario, int kDestino, uint32_t pid, uint16_t dorsal, uint32_t pidSustituto);
		/// Qué tablas tocó la última operación sobre el equipo del usuario (para el log / la bitácora).
		const std::string& ultimoInforme() const { return _ultimoInforme; }

		const std::vector<uint8_t>& datos() const { return _datos; }

		/// Cifra y escribe en `rutaNueva` (falla si existe; verifica releyendo). `textoInfo` vacío = conserva el original.
		Resultado<std::string> guardarComo(const std::string& rutaNueva, const std::string& textoInfo = "") const;

	private:
		std::vector<uint8_t> _datos;
		std::optional<SobrePes> _sobre;   // ausente si vino de desdeDatos()
		std::string _ultimoInforme;
	};

}
