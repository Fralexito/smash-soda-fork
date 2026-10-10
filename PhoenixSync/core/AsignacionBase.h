#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Grupo.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · PlayerAssignment.bin (plantillas de la BASE)
// -----------------------------------------------------------------------------
//  Probado por FRALEX (2026-10-09 20:08): con «Datos Actual. en vivo → Activar»
//  el juego toma los equipos de la base (este archivo de Phoenix-DB), no del
//  option file. Por eso cada fichaje del grupo se escribe AQUÍ también.
//  Formato: cabecera WESYS (16 B) + zlib → registros de 16 B:
//    +0 índice u32 · +4 jugador u32 · +8 equipo u32 · +12 dorsal−1 u8 ·
//    +13 orden<<2 u8 · +14 banderas u8 (0x20 = capitán?) · +15 0
//  Siempre se trabaja sobre el archivo de Phoenix-DB (hecho a partir del de
//  olmos), así no se pierden los datos del parche. Mover = se cambia el
//  registro en su sitio (mismo índice): nunca se añaden ni se borran filas.
// =============================================================================

namespace mercado::base {

	struct Fila {
		uint32_t indice = 0, jugador = 0, equipo = 0;
		uint8_t dorsal = 0;      ///< 1..99
		uint8_t orden = 0;       ///< 0..39
		uint8_t banderas = 0, extra = 0;
		uint8_t bajosOrden = 0;  ///< los 2 bits bajos del byte +13 (se conservan tal cual)
	};

	class Asignaciones {
	public:
		/// Lee un PlayerAssignment.bin (con su cabecera WESYS).
		static Resultado<Asignaciones> abrir(const std::string& ruta);
		static Resultado<Asignaciones> desdeBytes(const std::vector<uint8_t>& archivo);

		const std::vector<Fila>& filas() const { return _filas; }
		grupo::Plantillas plantillas() const;

		/// Mueve al jugador de `desde` a `hacia` (mismo registro). dorsal 0 o ocupado = el más alto libre.
		Resultado<bool> mover(uint32_t jugador, uint32_t hacia, uint32_t desde, uint16_t dorsal);

		/// Archivo completo (WESYS + zlib nivel 9) listo para guardar.
		Resultado<std::vector<uint8_t>> bytes() const;
		/// Guarda en un archivo NUEVO (falla si existe).
		Resultado<bool> guardarComo(const std::string& ruta) const;

	private:
		std::vector<uint8_t> _cabecera;   // los 8 primeros bytes originales de la cabecera WESYS
		std::vector<Fila> _filas;
	};

	/// Aplica las operaciones (en orden) a la base. Mismas reglas que el option file
	/// (grupo::decidir). «quitar» todavía no se aplica en la base (queda como conflicto).
	std::vector<grupo::ResultadoOp> aplicarOperaciones(Asignaciones& a, const std::vector<grupo::Operacion>& ops);

}
