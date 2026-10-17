#pragma once

// =============================================================================
//  Phoenix Link · Identidad de la build
// -----------------------------------------------------------------------------
//  Interruptores en tiempo de compilación de la build Phoenix.
//  Todo el código propio de Phoenix vive en la carpeta phoenix/ y solo toca el
//  código del autor con ganchos mínimos, para facilitar sincronizar versiones
//  nuevas de Smash Soda.
// =============================================================================

namespace phoenix {

	/// Nombre visible de la build.
	inline constexpr const char* kNombreBuild = "Phoenix Link";

	/// Versión propia de Phoenix (independiente de la versión del autor).
	/// REGLA (Fralex, 2026-10-11): subirla en CADA cambio de Link (0.2.0 → 0.2.1 → …), para saber
	/// a simple vista si el .exe abierto ya trae el cambio. La UI muestra además la hora de compilación.
	inline constexpr const char* kVersion = "0.3.8";

	/// Caducidad de la beta privada (AAAA-MM-DD; "" = no caduca). Pasada esa fecha Link no abre y pide la versión nueva
	/// (ver PhoenixBeta.h). El PC del desarrollador queda exento con un archivo marca (ver esDueno en PhoenixBeta.h).
	inline constexpr const char* kCaduca = "2026-11-15";

	/// Etapa de la versión (se muestra junto al número en el informe de diagnóstico). beta 2 = base estable
	/// para que otra persona instale y use Link (2026-10-12).
	inline constexpr const char* kEtapa = "beta 2";

	/// Soda Arcade: su licencia de API prohíbe que los forks la usen sin permiso
	/// escrito. En la build Phoenix queda apagada por completo (login, publicar
	/// sala, contador de invitados, artwork).
	inline constexpr bool kSodaArcadeHabilitado = false;

	/// Buscador de actualizaciones del autor: apagado, para que la app nunca
	/// ofrezca reemplazar la build Phoenix por la versión original.
	inline constexpr bool kActualizacionesAutor = false;

}
