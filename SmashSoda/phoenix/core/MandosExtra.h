#pragma once

// =============================================================================
//  Phoenix Link · Mandos extra para PES 2021 (puestos 9 en adelante, EXPERIMENTAL)
// -----------------------------------------------------------------------------
//  settings.dat de PES solo tiene 8 casillas de jugador (ver OrdenPes.h), pero el
//  motor del partido tiene 24 ranuras y el gestor de mandos admite 32. Para los
//  puestos 9+ Link escribe un archivo NUEVO junto a settings.dat (no lo toca):
//
//    Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\phoenix_mandos_extra.dat
//
//    0x00  "PHXM"                    firma ASCII
//    0x04  u32 versión = 1           (little endian)
//    0x08  u32 cantidad = N
//    0x0C  N casillas de 0x40 bytes, MISMO formato que una casilla de settings.dat:
//          GUID de instancia DirectInput (16 bytes) + 24 botones de 2 bytes (48).
//          Casilla i = puesto 9+i. GUID a cero = puesto desenchufado.
//
//  Lo lee el módulo de Sider de mandos extra (aparte; Link no lo instala). Sin ese
//  módulo PES ignora el archivo. Mismas reglas que OrdenPes: solo con PES cerrado,
//  copia «.phoenix-<fecha>» antes del primer cambio de la sesión y escritura atómica.
//  Probado en pruebas-link/mandosextra_test.cpp.
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace phoenix::mandosextra {

	constexpr uint32_t kVersion = 1;
	constexpr size_t kCabecera = 12;
	constexpr size_t kCasilla = 0x40;
	constexpr size_t kMapa = 48;
	/// Puestos que caben en settings.dat; del 9 en adelante van a este archivo.
	constexpr int kPuestosSettings = 8;

	/// Archivo con N = guids.size() casillas; cada una GUID (16 bytes, vacío = ceros) + mapa (48 bytes).
	std::vector<unsigned char> construir(const std::vector<std::vector<unsigned char>>& guids,
		const std::vector<unsigned char>& mapa);
	/// Firma, versión 1 y tamaño exacto (12 + N·0x40).
	bool esValido(const std::vector<unsigned char>& a);
	/// Cantidad de casillas de un archivo válido (0 si no lo es).
	uint32_t cantidad(const std::vector<unsigned char>& a);
	/// GUID de la casilla i (16 bytes; vacío si no existe o está a cero).
	std::vector<unsigned char> guidDe(const std::vector<unsigned char>& a, size_t i);
	/// Mapa de botones de la casilla 1 de settings.dat (612 bytes, «WECF»); «defecto» si no es válido o está a cero.
	std::vector<unsigned char> mapaDeCasilla1(const std::vector<unsigned char>& settings,
		const std::vector<unsigned char>& defecto);

	/// Ruta del archivo (vacía si no hay carpeta Documentos).
	std::wstring ruta();
	/// Escritura atómica (archivo .phoenix-tmp + MoveFileEx). Si «copiar» y ya hay un archivo, antes lo copia a
	/// <ruta>.phoenix-<fecha> y devuelve el nombre en «copia». Falso (y el archivo de antes intacto) si algo falla.
	bool escribir(const std::wstring& ruta, const std::vector<unsigned char>& datos, bool copiar,
		std::string& copia, std::string& error);
}
