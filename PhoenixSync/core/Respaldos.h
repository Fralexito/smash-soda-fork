#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <string>
#include <vector>
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Respaldos completos en la PC del usuario
// -----------------------------------------------------------------------------
//  Dónde:  %APPDATA%\Phoenix Mercado\respaldos\<AAAA-MM-DD_HHMMSS>\
//            save\…                     (la carpeta save completa)
//            phoenix-db\<modo>\Player.bin
//            manifiesto.json            (qué, de dónde, tamaño, sha256, motivo)
//  Reglas: solo COPIA (nunca mueve ni borra originales); comprueba espacio;
//  cada archivo va a .tmp, se verifica su sha256 y se renombra; el respaldo se
//  arma en «<nombre>.parcial» y solo al final pasa a su nombre (si falla, se
//  borra lo parcial, que es nuestro); nunca pisa uno existente (-2, -3…).
//  Es OTRA capa: no toca los respaldos de Link (EDIT00000000.phoenix-*, Player.bin.anterior).
// =============================================================================

namespace mercado::respaldos {

	inline constexpr const char* kFormato = "phoenix-sync/respaldo-1";

	/// Qué respaldar. `etiqueta` = subcarpeta dentro del respaldo ("save", "phoenix-db/principal").
	struct Origen {
		std::string etiqueta;
		std::string ruta;          ///< carpeta (se copia completa, sin subcarpetas) o archivo
	};

	struct Opciones {
		std::string raiz;          ///< …\respaldos
		std::string motivo;        ///< "antes de aplicar", "diario", "manual", "antes de restaurar"…
		std::string versionSync;
		/// Espacio libre en bytes donde va el respaldo (para pruebas). Vacío = std::filesystem::space.
		std::function<uint64_t(const std::string&)> espacioLibre;
		/// Hora actual (para pruebas). Vacío = reloj del sistema.
		std::function<std::time_t()> ahora;
		uint64_t margenBytes = 50ull * 1024 * 1024;   ///< deja siempre 50 MB libres
	};

	struct ArchivoRespaldado {
		std::string etiqueta, relativo, rutaOriginal, sha256;
		long long bytes = 0;
	};

	struct Info {
		std::string nombre;        ///< «2026-10-09_191233» (o con -2)
		std::string carpeta;       ///< ruta completa
		std::string creadoEn, motivo, versionSync;
		long long bytes = 0;
		std::vector<ArchivoRespaldado> archivos;
		std::vector<std::string> omitidos;   ///< «etiqueta: motivo» (carpeta que no existe, etc.)
	};

	/// Hace un respaldo. Si no hay NADA que copiar devuelve error NADA_QUE_RESPALDAR.
	/// Errores: SIN_ESPACIO, COPIA_CORRUPTA, ARCHIVO_ERROR… (y no deja nada a medias).
	Resultado<Info> crear(const std::vector<Origen>& origenes, const Opciones& op);

	/// Respaldos propios (con manifiesto válido), del más nuevo al más viejo.
	std::vector<Info> listar(const std::string& raiz);

	struct Retencion { int ultimos = 10; int dias = 7; };
	/// Conserva los `ultimos` y, aparte, el primero de cada uno de los últimos `dias` días.
	/// Solo borra carpetas que son respaldos nuestros. Devuelve los nombres borrados.
	std::vector<std::string> aplicarRetencion(const std::string& raiz, const Retencion& r, std::time_t ahora);

	/// ¿Hace falta el respaldo diario? (no hay ninguno de hoy).
	bool faltaDiario(const std::string& raiz, std::time_t ahora);

	/// Restaura: 1) respaldo del estado actual (motivo «antes de restaurar»; si falla, no sigue),
	/// 2) coloca cada archivo del respaldo en su ruta original (.tmp → verificar → reemplazar).
	/// Nunca crea carpetas del juego: si la carpeta original ya no existe, ese archivo se salta.
	/// `origenesActuales` = lo que se respalda antes (normalmente los mismos orígenes de siempre).
	struct ResultadoRestaurar { Info respaldoPrevio; int colocados = 0; std::vector<std::string> saltados; };
	Resultado<ResultadoRestaurar> restaurar(const std::string& raiz, const std::string& nombre,
		const std::vector<Origen>& origenesActuales, const Opciones& op);

	/// Copia segura de un archivo: destino.tmp → sha256 igual → renombrar (reemplaza). Devuelve el sha256.
	Resultado<std::string> copiarVerificado(const std::string& origen, const std::string& destino);

	/// Sello local «AAAA-MM-DD_HHMMSS».
	std::string sello(std::time_t t);

}
