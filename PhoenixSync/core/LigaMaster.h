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
//  Phoenix Sync · Liga Máster interna de PES 2021 (guardados ML0000000N)
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
		size_t inicioRel = 4;  ///< el registro empieza tantos bytes antes de `reg` (4 casi siempre; 28 en la de contratos)
	};

	/// Alineación del equipo del usuario (solo él la tiene). Tres piezas que el juego mantiene en espejo:
	///   · `orden`: índices de plantilla en orden de formación (0–10 XI; después la banca —12 en este guardado— y las reservas),
	///     40 bytes en el archivo, los libres a 0xff; vive en el bloque de alineación del usuario (el 627 en la referencia);
	///   · `roles`: 6 índices de plantilla (capitán, lanzadores…) justo después (+0x28);
	///   · lista K: un registro de 16 B `[flag, reg, pid, 0]` por posición de `orden` (K[i] = plantilla[orden[i]]),
	///     seguida de un registro libre (reg 0xffff) con flag 0xc0+ y luego 0xc7. Estrategia lee K.
	struct Alineacion {
		size_t ofsOrden = 0;
		size_t ofsRoles = 0;
		size_t ofsK = 0;                ///< campo `flag` del registro K0
		std::vector<uint8_t> orden;     ///< tantas entradas como jugadores
		std::array<uint8_t, 6> roles{};
		std::vector<uint32_t> flagsK;   ///< flag de cada K[i] usado (por jugador; si K estaba atrasada, 0xc0 para los nuevos)
		uint32_t flagLibreK = 0;        ///< flag del primer registro libre (0xc0 con 25 jugadores, 0xc1 con 26)
		bool kEspejo = true;            ///< false = la lista K del archivo no reflejaba el orden (el juego la rehace más tarde)
	};

	/// Dinero del club del usuario (en euros; en el archivo van en u32 ×100). ESTRUCTURA-ML.md §13 y §17.
	struct Finanzas {
		uint64_t presupuestoFichajes = 0;        ///< lo que queda para fichar (pantalla «Ppto. fichajes»)
		uint64_t presupuestoFichajesInicial = 0; ///< el de inicio de temporada
		uint64_t topeSalarial = 0;               ///< tope de masa salarial
		uint64_t sueldosActuales = 0;            ///< suma de los sueldos anuales de los contratos vigentes (tabla de contratos)
		uint64_t presupuestoSalarial() const { return topeSalarial > sueldosActuales ? topeSalarial - sueldosActuales : 0; } ///< lo que muestra la pantalla
		size_t ofs = 0;                          ///< dónde está el bloque (para el log)
	};

	/// Fecha tal como la guarda el juego: u16 año, u8 mes, u8 día.
	struct Fecha {
		uint16_t anio = 0; uint8_t mes = 0, dia = 0;
		bool valida() const { return anio >= 2000 && anio <= 2100 && mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31; }
		uint32_t empaquetada() const { return uint32_t(anio) | (uint32_t(mes) << 16) | (uint32_t(dia) << 24); }
		static Fecha desde(uint32_t v) { Fecha f; f.anio = uint16_t(v); f.mes = uint8_t(v >> 16); f.dia = uint8_t(v >> 24); return f; }
		bool operator<(const Fecha& o) const { return empaquetadaOrdenable() < o.empaquetadaOrdenable(); }
		uint32_t empaquetadaOrdenable() const { return (uint32_t(anio) << 16) | (uint32_t(mes) << 8) | dia; }
	};

	/// Condiciones de un fichaje PARA el equipo del usuario (IA → usuario). 0 / vacío = valor por defecto.
	struct OpcionesFichaje {
		uint16_t dorsal = 0;                 ///< 0 = el del jugador en su club / selección, o el más alto libre
		uint32_t pidSustituto = 0;           ///< quién ocupa su puesto en el equipo de la IA que lo pierde (si era titular o tenía rol)
		uint64_t montoEur = 0;               ///< lo que se paga al club de origen (se descuenta del presupuesto de fichajes)
		uint64_t sueldoEur = 0;              ///< sueldo anual del contrato nuevo (0 = el que tenía)
		uint64_t clausulaEur = 0;            ///< cláusula de rescisión
		Fecha finContrato;                   ///< vacía = 30 de junio del año siguiente al de la fecha del fichaje
		Fecha fecha;                         ///< fecha de llegada (vacía = la fecha actual de la partida)
		int edad = 0;                        ///< para su historial de medias (tabla E); 0 = la del compañero que sirve de molde
		int media = 0;                       ///< media actual (último valor del historial); 0 = la del molde
		std::function<int(uint32_t)> posicionDe;   ///< posición registrada por pid (catálogo) para elegir el molde y el sustituto
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

		/// ID del equipo en el option file. El bloque del equipo del usuario lleva -11 (0xfffffff5) en vez de su ID
		/// (marca de «lo lleva el usuario»); su ID real se recupera de la cabecera de sus contratos (club >> 14). §17.
		Resultado<uint32_t> idOptionDe(int indice) const;
		/// Índice del bloque de equipo (0–699) cuyo ID en el option file es `idOption` (también el del usuario).
		Resultado<int> indicePorIdOption(uint32_t idOption) const;

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

		/// Fecha actual de la partida (la que se ve arriba en la pantalla de la Liga Máster). ESTRUCTURA-ML §20.
		Resultado<Fecha> fechaActual() const;

		/// Ficha a `pid` del equipo de la IA `kOrigen` PARA el equipo del usuario, como lo hace el juego (Sommer, §19–§20):
		/// plantillas y alineaciones de los dos equipos, un registro nuevo en cada tabla del usuario (copiando el de un
		/// compañero del mismo puesto y poniendo los datos propios: fechas, club de origen, historial de medias), contrato,
		/// lista K y orden de formación, dinero, y en el blob su inscripción en las competiciones del club y el contrato.
		/// Todo o nada. Devuelve el dorsal asignado.
		Resultado<uint16_t> ficharParaUsuario(int kUsuario, int kOrigen, uint32_t pid, const OpcionesFichaje& opciones);

		/// Dinero del club del usuario. Se localiza por ancla (a distancia fija de la tabla A del usuario) y se valida contra
		/// los contratos (el tope salarial tiene que cubrir los sueldos). Probado en el juego (ranura 12: 500 M / 86.096.000 €).
		Resultado<Finanzas> finanzas(int kUsuario) const;
		/// Fija el presupuesto de fichajes y/o el tope salarial (en euros; 0 = no cambiar ese campo). Máximo 40.000 millones.
		/// El presupuesto salarial de pantalla sale solo: tope − sueldos. Todo o nada.
		Resultado<Finanzas> fijarFinanzas(int kUsuario, uint64_t presupuestoFichajesEur, uint64_t topeSalarialEur);

		const std::vector<uint8_t>& datos() const { return _datos; }

		/// Cifra y escribe en `rutaNueva` (falla si existe; verifica releyendo). `textoInfo` vacío = conserva el original.
		Resultado<std::string> guardarComo(const std::string& rutaNueva, const std::string& textoInfo = "") const;

	private:
		std::vector<uint8_t> _datos;
		std::optional<SobrePes> _sobre;   // ausente si vino de desdeDatos()
		std::string _ultimoInforme;
	};

}
