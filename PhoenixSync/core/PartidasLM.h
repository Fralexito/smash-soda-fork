#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "CambiosLM.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Partidas de Liga Máster del PC y escritura segura (127)
// -----------------------------------------------------------------------------
//  Reglas (las mismas de Sync):
//   · SOLO con PES cerrado: así ninguna ranura está cargada en el juego.
//   · Respaldo completo de la ranura ANTES (respaldos\<fecha>\lm\ML0000000N).
//   · Se escribe un archivo NUEVO (ML….phoenix-nuevo), se verifica releyéndolo y
//     solo entonces se cambia de nombre; el anterior queda como ML….phoenix-antes.
//   · Si la ranura cambió mientras se trabajaba (otra huella), no se toca nada.
//   · El estado de cada partida (versión, diario para deshacer, pendientes) va en
//     <carpetaEstado>\<ranura>_<club>.json (tmp → renombrar).
//  Las partidas se reconocen por su FORMA (se abren y se busca el club del
//  usuario), nunca por el nombre del parche: vale en cualquier PC y parche.
// =============================================================================

namespace mercado::lmweb {

	struct PartidaPC {
		std::string ruta;          ///< …\save\ML0000000B
		std::string ranura;        ///< ML0000000B
		std::string id;            ///< ML0000000B:101 (ver idPartida)
		uint32_t clubPes = 0;      ///< ID del club del usuario en el option file
		std::string clubNombre;
		std::string fecha;         ///< fecha de la partida AAAA-MM-DD (vacía si no se pudo leer)
		std::string carpetaSave;
		nlohmann::json aJson() const;
	};

	/// ¿El nombre es de una ranura de Liga Máster? (ML + 8 cifras hexadecimales, sin extensión)
	bool esRanuraLM(const std::string& nombre);

	/// Partidas de Liga Máster en esas carpetas save. Las que no se pueden abrir van a `avisos` (no se tocan).
	std::vector<PartidaPC> buscarPartidas(const std::vector<std::string>& carpetasSave, std::vector<std::string>* avisos = nullptr);

	struct OpcionesEscritura {
		std::string carpetaEstado;                  ///< %APPDATA%\Phoenix Mercado\lm
		std::string raizRespaldos;                  ///< %APPDATA%\Phoenix Mercado\respaldos
		std::string versionSync;
		std::function<bool()> juegoAbierto;         ///< nulo = se da por cerrado (solo pruebas)
	};

	struct ResultadoPartida {
		PartidaPC partida;
		Informe informe;
		bool escrito = false;
		std::string respaldo;      ///< nombre del respaldo hecho antes de escribir
		std::string sha256Antes, sha256Despues;
		int64_t versionAplicada = 0;
		std::string error;         ///< vacío = bien; si no, NADA cambió (ni el archivo ni el estado)
	};

	/// Aplica la lista a UNA partida con todas las reglas de arriba.
	ResultadoPartida aplicarEnPartida(const PartidaPC& p, const ListaLM& lista, const Contexto& ctx, const OpcionesEscritura& op);

	EstadoPartida leerEstado(const std::string& carpetaEstado, const PartidaPC& p);

	/// Informe para la web (POST /mercado/v1/liga/aplicado-pc): por partida, versión aplicada y resultado de cada cambio.
	nlohmann::json informeParaWeb(const std::vector<ResultadoPartida>& rs);

	/// Una vuelta completa: busca las partidas, baja lo nuevo desde la versión más baja de todas (por páginas) y lo
	/// aplica en cada una. `contenidoDesde(desde)` = contenido YA verificado con la firma de la web (lo da la app).
	struct Vuelta {
		std::vector<ResultadoPartida> partidas;
		std::vector<std::string> avisos;      ///< ranuras que no se reconocen (no se tocan)
		nlohmann::json informe;               ///< informeParaWeb(partidas)
	};
	Resultado<Vuelta> vuelta(const std::vector<std::string>& carpetasSave, const std::function<Resultado<std::string>(int64_t)>& contenidoDesde,
		const Contexto& ctx, const OpcionesEscritura& op);

}
