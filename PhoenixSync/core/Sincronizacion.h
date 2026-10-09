#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "Firma.h"
#include "LigaMaster.h"
#include "OptionFile.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Sincronización con la web (docs/mercado-api.md, «Versión de liga»)
// -----------------------------------------------------------------------------
//  La web lleva la verdad de la liga. Cada traspaso cerrado sube `version_liga`
//  y queda en la lista de cambios. El programa:
//    1. pide GET /liga/cambios?desde=<última versión aplicada>  (viene FIRMADO),
//    2. verifica la firma (Firma.h) y recién entonces lee el contenido,
//    3. aplica cada cambio, en orden de versión, a lo que el usuario eligió:
//         · el OPTION FILE (amistosos, salas, carreras nuevas)       → OptionFile::mover
//         · su LIGA MÁSTER interna (guardado ML0000000N)            → GuardadoLM::moverUsuarioAIA / moverEntreIA /
//                                                                     ficharParaUsuario (probado en el juego, prueba 19)
//       con sustituto por posición cuando se va un titular,
//    4. responde POST /liga/aplicado { version, huella_plantillas }.
//  Todo o nada: si un cambio no se puede aplicar al option file, no se toca
//  nada. En la Liga Máster, lo que todavía no se sabe hacer (agentes libres)
//  queda anotado como PENDIENTE y lo demás se aplica.
// =============================================================================

namespace mercado::sinc {

	/// Clave pública de la web (GET /clave-publica, 8 oct 2026). Si la web manda otro clave_id, NO se confía.
	inline const ClavePublica& claveWeb() {
		static const ClavePublica k = { "k67bd032d50", desdeBase64("emHq6k0YsiAANBET5YCgehooR0O4MvpdpOSAbnlxDAQ=") };
		return k;
	}

	/// A qué se aplican los cambios de la web (interruptores del usuario).
	struct Alcance {
		bool optionFile = true;    ///< amistosos, salas, carreras nuevas
		bool ligaMaster = true;    ///< su carrera de Liga Máster (si eligió un guardado)
	};

	/// Un cambio de liga tal como lo manda la web. 0 en `clubDesdePes`/`clubHaciaPes` = sin club (agente libre).
	struct Cambio {
		int64_t version = 0;
		int64_t phoenixId = 0;
		uint32_t pesId = 0;
		std::string clubDesde, clubHacia;      ///< nombres (para el informe)
		uint32_t clubDesdePes = 0, clubHaciaPes = 0;
		std::string tipo, fecha;
		// Condiciones del traspaso (opcionales; la web aún no las manda: 0 / vacío = lo decide el programa).
		uint64_t montoEur = 0;                 ///< «monto»: lo que paga el club que ficha
		uint64_t sueldoEur = 0;                ///< «sueldo»: sueldo anual del contrato nuevo (0 = el que tenía)
		uint64_t clausulaEur = 0;              ///< «clausula»
		std::string finContrato;               ///< «fin_contrato»: AAAA-MM-DD (vacío = 30/6 del año siguiente)
		uint16_t dorsal = 0;                   ///< «dorsal» (0 = como el juego: su número de club, de selección o el más alto libre)
	};

	struct ListaCambios {
		std::string liga;
		int64_t desde = 0, versionActual = 0;
		std::vector<Cambio> cambios;           ///< en orden de versión
	};

	/// Parsea el `contenido` (ya verificado) de /liga/cambios. Rechaza cambios sin pes_id o fuera de orden.
	Resultado<ListaCambios> parsearCambios(const std::string& contenido);

	/// Qué pasó con cada cambio.
	struct LineaInforme {
		int64_t version = 0;
		uint32_t pesId = 0;
		std::string texto;                     ///< «Bettinelli: Manchester City → Lanús»
		bool optionFile = false;               ///< aplicado al option file
		bool ligaMaster = false;               ///< aplicado a la Liga Máster
		std::string pendienteLM;               ///< vacío, o por qué quedó pendiente en la Liga Máster
	};

	struct Informe {
		int64_t versionAplicada = 0;           ///< última versión que quedó aplicada (la mayor de la lista)
		int aplicadosOption = 0, aplicadosLM = 0, pendientesLM = 0;
		std::vector<LineaInforme> lineas;
		std::string texto() const;             ///< resumen legible (para el log y la bitácora)
	};

	/// Aplica una lista de cambios. `option` y/o `liga` pueden ser nulos (según el alcance y lo que haya).
	/// `posicionDe(pes_id)` = posición del catálogo (0 PT … 12 DC) o -1; con ella se eligen los sustitutos.
	/// `nombreDe(pes_id)` solo para el informe (puede ser nulo).
	/// `edadDe(pes_id)` = edad del catálogo (0 si no se sabe): la usa el fichaje para el equipo del usuario (historial de medias).
	/// Todo o nada: si falla, ni el option file ni la Liga Máster cambian.
	Resultado<Informe> aplicarCambios(const ListaCambios& lista, int64_t versionYaAplicada, Alcance alcance,
		OptionFile* option, lm::GuardadoLM* liga,
		const std::function<int(uint32_t)>& posicionDe, const std::function<std::string(uint32_t)>& nombreDe = nullptr,
		const std::function<int(uint32_t)>& edadDe = nullptr);

	/// Cuerpo JSON de POST /liga/aplicado.
	std::string cuerpoAplicado(int64_t version, const std::string& huellaPlantillas, const std::string& liga = "");

}
