#pragma once

#include <optional>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Dónde están los archivos del juego (multiparche)
// -----------------------------------------------------------------------------
//  Nada de ConmeGOL escrito a mano. Se busca igual que Phoenix Link:
//   · Option file: <Documentos>\KONAMI\<edición>\<steamid>\save\EDIT00000000
//     en TODAS las carpetas «Documentos» candidatas (normal y OneDrive) y en
//     todas las ediciones y steamid; manda el EDIT modificado más reciente.
//   · Player.bin: <cualquier SiderAddons bajo la carpeta del juego>\livecpk\
//     Phoenix-DB\common\etc\pesdb\Player.bin  (raíz del juego y modos del switcher).
//  Si algo no existe se salta, nunca falla.
// =============================================================================

namespace mercado::rutas {

	struct CarpetaSave {
		std::string ruta;        ///< …\save
		std::string edicion;     ///< nombre de la carpeta bajo KONAMI
		std::string steamid;
		long long modificado = 0;  ///< fecha del EDIT00000000 (segundos, para comparar)
	};

	/// Todas las carpetas save con EDIT00000000, de la más reciente a la más vieja.
	std::vector<CarpetaSave> buscarSaves(const std::vector<std::string>& carpetasDocumentos);
	/// La carpeta activa (EDIT más reciente) o nada.
	std::optional<CarpetaSave> saveActivo(const std::vector<std::string>& carpetasDocumentos);

	struct BaseParche {
		std::string modo;        ///< "principal" (raíz del juego) o la ruta relativa del modo, con '/' (p. ej. «ConmeGol Extras/Latam»)
		std::string playerBin;   ///< ruta completa del Player.bin
	};

	/// Todos los Player.bin de Phoenix-DB bajo la carpeta del juego (hasta 4 niveles). Vacío si no hay.
	std::vector<BaseParche> buscarPlayerBin(const std::string& carpetaJuego);

	/// Nombre del parche para la etiqueta de compatibilidad: el modo activo que deja el
	/// switcher (version_actual.txt, si existe) o, si no, el nombre de la carpeta del juego.
	std::string nombreParche(const std::string& carpetaJuego);

	/// Etiqueta segura para carpetas (sin \ / : * ? " < > |).
	std::string etiquetaSegura(const std::string& texto);

}
