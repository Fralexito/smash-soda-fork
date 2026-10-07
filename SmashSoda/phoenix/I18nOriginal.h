#pragma once

#include <map>
#include <string>

// =============================================================================
//  Phoenix Soda · Traducción de los textos originales de Smash Soda
// -----------------------------------------------------------------------------
//  Los paneles del autor escriben sus textos en inglés. En vez de editar cada
//  panel, Widget.cpp pasa cada etiqueta, ayuda, botón y pestaña por
//  phoenix::Tr(), que busca la clave "en:<texto original>".
//  Otros idiomas: agregar claves "en:<texto>" en idiomas/<codigo>.json.
// =============================================================================

namespace phoenix {

	/// Tabla español de los textos originales (clave = "en:" + texto en inglés).
	const std::map<std::string, std::string>& textosOriginalesEs();

}
