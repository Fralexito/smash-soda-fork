#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Sobre cifrado de los guardados de PES 2021
// -----------------------------------------------------------------------------
//  Los guardados (EDIT…, ML…) son: cabecera + descripción + logo + DATOS + serial,
//  cifrados con libpesXcrypter y la clave de PES 2021. Esta clase abre el sobre,
//  entrega los DATOS descifrados y vuelve a cifrar con CUALQUIER tamaño de datos
//  (la Liga Máster crece o encoge al fichar o vender). Conserva cabecera, logo y
//  serial del original. El juego no valida el hash de la cabecera: así se
//  comprobó con guardados editados cargados en el juego (8 oct 2026).
//
//  La descripción mide 384 bytes: los primeros 128 son un nombre que el menú
//  «Cargar» de Liga Máster NO muestra; del byte 128 en adelante va el «texto
//  info» (equipo/liga, fecha, competición, separados por \n) y ESE sí lo muestra.
//
//  Nunca sobrescribe: guardarComo falla si el destino existe, escribe primero
//  un temporal, lo relee y solo entonces lo renombra.
// =============================================================================

namespace mercado {

	class SobrePes {
	public:
		/// Lee y descifra. `tipoEsperado`: "EDIT" o "ML" (vacío = no comprobar).
		static Resultado<SobrePes> abrir(const std::string& ruta, const std::string& tipoEsperado = "");
		static Resultado<SobrePes> desdeBytes(std::vector<uint8_t> cifrado, const std::string& tipoEsperado = "");

		const std::string& tipo() const { return _tipo; }        ///< "EDIT", "ML"…
		const std::string& version() const { return _version; }  ///< "eFootball PES 2021 SEASON UPDATE"
		const std::vector<uint8_t>& datos() const { return _datos; }
		/// Mueve los DATOS descifrados fuera del sobre (el sobre conserva lo demás).
		std::vector<uint8_t> tomarDatos() { return std::move(_datos); }

		/// Texto info de la descripción (lo que muestra el menú Cargar).
		const std::string& textoInfo() const { return _textoInfo; }
		void ponerTextoInfo(const std::string& texto) { _textoInfo = texto; _infoCambiado = true; }

		/// Cifra `datos` (de cualquier tamaño) con la cabecera, logo y serial del original.
		Resultado<std::vector<uint8_t>> cifrar(const std::vector<uint8_t>& datos) const;
		/// Cifra y escribe en `rutaNueva` (nunca sobrescribe, verifica releyendo). Devuelve el SHA-256.
		Resultado<std::string> guardarComo(const std::string& rutaNueva, const std::vector<uint8_t>& datos) const;

	private:
		std::vector<uint8_t> _cifrado;   // original: se reabre al cifrar para reutilizar cabecera/logo/serial
		std::vector<uint8_t> _datos;
		std::string _tipo, _version, _textoInfo;
		bool _infoCambiado = false;
	};

}
