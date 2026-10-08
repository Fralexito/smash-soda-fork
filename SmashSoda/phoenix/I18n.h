#pragma once

#include <string>
#include <utility>
#include <vector>

// =============================================================================
//  Phoenix Link · Idiomas
// -----------------------------------------------------------------------------
//  Uso: phoenix::T("nav.sala")  → texto en el idioma activo.
//  Orden de búsqueda: idioma activo → español (base) → la propia clave.
//
//  Idiomas incluidos: es, en, pt, fr, it, de.
//  Ampliable sin recompilar: un archivo
//    %APPDATA%\Trybuchet\Smash Soda\idiomas\<codigo>.json   ({"clave": "texto"})
//  agrega un idioma nuevo o corrige textos de uno existente.
// =============================================================================

namespace phoenix {

	const char* T(const char* clave);

	/// Traduce un texto original en inglés de Smash Soda (conserva sufijos "##id").
	/// Si no hay traducción para el idioma activo, devuelve el original.
	std::string Tr(const std::string& original);

	class I18n {
	public:
		static void establecer(const std::string& codigo);
		static const std::string& actual();

		/// (código, nombre nativo) de todos los idiomas disponibles.
		static std::vector<std::pair<std::string, std::string>> disponibles();
	};

}
