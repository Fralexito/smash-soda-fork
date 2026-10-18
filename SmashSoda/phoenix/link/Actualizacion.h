#pragma once

#include <cstdint>
#include <functional>
#include <string>

// =============================================================================
//  Phoenix Link · Auto-actualización (web: migración 128, función «mercado»)
// -----------------------------------------------------------------------------
//  1. Link pregunta GET /mercado/v1/link/actualizacion?version=<la suya> con su token.
//  2. La web responde un SOBRE firmado con Ed25519 (misma clave que Phoenix Sync,
//     incrustada abajo) con versión, sha256 y tamaño del zip, + una URL firmada.
//  3. Link baja el zip a %LOCALAPPDATA%\Phoenix Link\actualizacion, comprueba
//     tamaño y sha256, lo descomprime y lanza PhoenixActualizador.exe (copiado
//     allí), que espera a que Link se cierre, guarda lo anterior, copia lo nuevo,
//     abre Link y, si no arranca, vuelve a la versión anterior.
//  Nunca toca %APPDATA% (ajustes, cuenta, fichajes) ni la carpeta Trybuchet.
//  Todo bloqueante: usar SOLO desde un hilo propio (nunca desde la interfaz).
// =============================================================================

namespace phoenix::actualizacion {

	inline constexpr const char* kFormato = "phoenix-link/actualizacion-1";

	/// Clave pública de la web (la misma de GET /mercado/v1/clave-publica, 8 oct 2026).
	struct Clave { std::string id; std::string publicaB64; };
	const Clave& claveWeb();

	bool versionValida(const std::string& v);
	/// <0 si a<b, 0 si iguales, >0 si a>b (numérico: 0.3.9 < 0.3.16). Una inválida cuenta como la más vieja.
	int compararVersion(const std::string& a, const std::string& b);

	struct Manifiesto {
		std::string version, etapa, sha256, notas, generado;
		uint64_t tamano = 0;
		bool obligatoria = false;
	};

	/// Abre el sobre { contenido, firma, clave_id, algoritmo }: solo si el algoritmo es Ed25519, el clave_id es el
	/// esperado, la firma vale y el contenido tiene el formato y campos correctos. `codigo` explica el fallo.
	bool abrirSobre(const std::string& sobreJson, const Clave& clave, Manifiesto& m, std::string& codigo);

	/// sha256 en hex minúsculas ("" si no se pudo leer).
	std::string sha256Archivo(const std::wstring& ruta);

	/// Baja `url` a `destino`. Corta si llega más de `tamano` bytes. Si llega menos: DESCARGA_CORTADA.
	/// Solo https (o http://127.0.0.1 si `permitirLocal`, para las pruebas).
	bool descargar(const std::string& url, const std::wstring& destino, uint64_t tamano,
		const std::function<void(uint64_t)>& progreso, bool permitirLocal, std::string& codigo, int timeoutMs = 30000);

	/// Descomprime con tar.exe de Windows (o Expand-Archive si no hay). Comprueba que nada quede fuera de `carpeta`
	/// ni haya enlaces, y que esté PhoenixLink.exe en la raíz.
	bool extraerZip(const std::wstring& zip, const std::wstring& carpeta, std::string& codigo);

	struct Opciones {
		std::wstring trabajo;              ///< carpeta de trabajo (vacía = carpetaTrabajo())
		Clave clave = claveWeb();
		std::string versionActual;
		std::function<void(uint64_t, uint64_t)> progreso;   ///< bajado, total
		bool permitirLocal = false;        ///< solo pruebas: acepta http://127.0.0.1
	};
	struct Preparada {
		Manifiesto m;
		std::wstring carpetaNueva;   ///< lo nuevo, ya descomprimido y comprobado
		std::wstring trabajo;
	};
	/// `datosJson` = el objeto «datos» de la respuesta (hay, sobre, url). Hace todo menos instalar.
	bool preparar(const std::string& datosJson, const Opciones& o, Preparada& p, std::string& codigo);

	std::wstring carpetaTrabajo();   ///< %LOCALAPPDATA%\Phoenix Link\actualizacion
	std::wstring carpetaExe();       ///< carpeta de PhoenixLink.exe, sin barra final

	/// Copia PhoenixActualizador.exe (el de la versión nueva si lo trae) a la carpeta de trabajo y lo lanza:
	/// esperará a que este proceso termine. Después hay que cerrar Link.
	bool lanzarActualizador(const Preparada& p, const std::string& versionActual, std::string& codigo);

	/// Llamar una vez cuando Link ya está andando bien: si se abrió con «--ok <archivo>», lo escribe (el actualizador lo espera).
	void avisarArranqueOk();

	/// Lo que dejó el actualizador la última vez (y lo borra). estado: instalada | restaurada | fallo.
	struct Resultado { std::string version, anterior, estado, detalle; };
	bool tomarResultado(Resultado& r, const std::wstring& trabajo = std::wstring());

	/// Texto simple para la persona.
	std::string mensajeDe(const std::string& codigo);

}
