#pragma once

// =============================================================================
//  Phoenix Soda · Identidad de la build
// -----------------------------------------------------------------------------
//  Interruptores en tiempo de compilación de la build Phoenix.
//  Todo el código propio de Phoenix vive en la carpeta phoenix/ y solo toca el
//  código del autor con ganchos mínimos, para facilitar sincronizar versiones
//  nuevas de Smash Soda.
// =============================================================================

namespace phoenix {

	/// Nombre visible de la build.
	inline constexpr const char* kNombreBuild = "Phoenix Soda";

	/// Versión propia de Phoenix (independiente de la versión del autor).
	inline constexpr const char* kVersion = "0.1.0";

	/// Soda Arcade: su licencia de API prohíbe que los forks la usen sin permiso
	/// escrito. En la build Phoenix queda apagada por completo (login, publicar
	/// sala, contador de invitados, artwork).
	inline constexpr bool kSodaArcadeHabilitado = false;

	/// Buscador de actualizaciones del autor: apagado, para que la app nunca
	/// ofrezca reemplazar la build Phoenix por la versión original.
	inline constexpr bool kActualizacionesAutor = false;

}
