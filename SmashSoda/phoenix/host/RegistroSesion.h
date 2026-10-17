#pragma once

// =============================================================================
//  Phoenix Link · Registro de cada partida (sala abierta → sala cerrada).
//  Cada sondeo del hilo de latencia (~2 s) escribe una línea por invitado con lo
//  que mide Parsec (ping, codificar/decodificar, Mbps, cola, pérdida) más lo que
//  usaba Link (ancho, FPS, freno) y la CPU del PC. Al cerrar la sala escribe un
//  resumen en texto sencillo. Carpeta: %APPDATA%\Trybuchet\Smash Soda\phoenix-sesiones\
//  (se guardan las últimas 40). Todo desde el hilo de latencia: sin candados.
// =============================================================================

#include <cstdint>
#include <string>

struct ParsecGuest;

namespace phoenix::registro {

	/// Un sondeo: una línea por invitado. Abre el archivo de la partida la primera vez que hay invitados.
	void anotar(const ParsecGuest* guests, int n, uint32_t anchoTotalMbps, uint32_t fps, int frenoPct);
	/// La sala se cerró: escribe el resumen y cierra el archivo. Idempotente.
	void cerrarSesion();
	/// Carpeta de los registros (se crea si falta).
	std::wstring carpeta();

}
