#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "MapeoMandos.h"

// =============================================================================
//  PERFILES DE MAPEO por persona (parsecId), guardados en
//  %APPDATA%\Trybuchet\Smash Soda\phoenix-mapeos.json. GamepadClient los consulta
//  en cada mensaje de mando: sin perfiles cuesta una lectura atómica.
//  También guarda «lo que llega y lo que sale» de UNA persona observada
//  (la que la interfaz tiene abierta en Mandos › Mapeo) y su última pulsación
//  nueva, para el asistente paso a paso.
// =============================================================================

namespace phoenix::mapeo {

	/// Perfil de esa persona (false si no tiene o es identidad). Hilo de los mandos.
	bool perfilDe(uint32_t parsecId, Perfil& salida);

	/// Lo llama GamepadClient con cada mensaje: barato si nadie está observado.
	void anotar(uint32_t parsecId, const Estado& crudo, const Estado& salida);

	// ---- Interfaz (hilo de la ventana) ----------------------------------------
	void poner(uint32_t parsecId, const std::string& nombre, const Perfil& p);   ///< guarda en disco
	void quitar(uint32_t parsecId);                                               ///< restablecer
	nlohmann::json listar();             ///< {"<id>": {nombre, perfil, actualizado}}
	void observar(uint32_t parsecId);    ///< 0 = nadie
	/// {parsecId, crudo{b,lt,rt,lx,ly,rx,ry}, salida{…}, ultima, n, edad} de la persona observada
	nlohmann::json vista();
}
