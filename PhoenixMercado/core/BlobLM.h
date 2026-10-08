#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Mercado · El «blob» comprimido del guardado de Liga Máster (ESTRUCTURA-ML §14, §20)
// -----------------------------------------------------------------------------
//  Zona comprimida con zlib por tramos de 256 KB. Guarda, entre otras cosas, la
//  ficha de Liga Máster de cada jugador (156 B: competiciones inscritas, sueldo,
//  contrato…) y el calendario/resultados. Formato (posiciones absolutas en los
//  DATOS descifrados; «rel» = desde el byte siguiente a la palabra de tamaño):
//    0x11403a8          u32 LE  tamaño de la zona (desde 0x11403ac) MENOS 12: el último tramo acaba 12 B después
//    rel 0x00..0x13     20 B    fijos (0x0007458c, 0, 0, 0x12091120, 0)
//    rel 0x14           u32 BE  suma de los campos «descomprimido» de los tramos
//    rel 0x18           u32 BE  suma de los campos «comprimido»
//    rel 0x1c           por tramo: [descomprimido u32 BE][comprimido u32 BE][fin u32 BE] + datos zlib
//                       fin = rel del separador + comprimido (0 en el último). Descomprimido: 4.680.156 B.
//  Al reescribir solo se recomprimen los tramos que cambiaron; el resto se copia
//  byte a byte. El archivo cambia de tamaño y todo lo que va detrás se desplaza
//  (por eso el resto del programa localiza las tablas por anclas, no por posición).
// =============================================================================

namespace mercado::lm {

	class BlobLM {
	public:
		/// Lee y descomprime el blob de unos DATOS de Liga Máster. Valida toda la estructura.
		static Resultado<BlobLM> leer(const std::vector<uint8_t>& datos);

		/// Contenido descomprimido (4.680.156 B en el ConmeGOL 26). Se puede modificar sin cambiar su tamaño.
		std::vector<uint8_t>& contenido() { return _plano; }
		const std::vector<uint8_t>& contenido() const { return _plano; }

		/// Devuelve unos DATOS nuevos con el blob reescrito (los tramos sin cambios se copian tal cual).
		Resultado<std::vector<uint8_t>> aplicar(const std::vector<uint8_t>& datos) const;

		/// Ficha de Liga Máster de un jugador dentro del blob (156 B, empieza 2 B antes del campo `reg`). -1 si no está.
		long long fichaDe(uint32_t reg, uint32_t pid) const;

		static constexpr size_t kOfsTam = 0x11403a8;
		static constexpr size_t kTamTramo = 262144;
		static constexpr size_t kTamFicha = 156;

	private:
		struct Tramo {
			uint32_t campoDesc = 0, campoComp = 0, campoFin = 0;   // tal como estaban
			std::vector<uint8_t> comprimido;                       // bytes zlib originales
			size_t ini = 0, len = 0;                               // dónde cae en el contenido descomprimido
		};
		std::vector<uint8_t> _cabecera;   // los 0x1c bytes rel antes del primer separador
		std::vector<Tramo> _tramos;
		std::vector<uint8_t> _plano, _planoOriginal;
		size_t _finZona = 0;              // posición absoluta del primer byte tras el blob
	};

}
