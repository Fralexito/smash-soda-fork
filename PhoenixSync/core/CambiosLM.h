#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "LigaMaster.h"
#include "Tipos.h"

// =============================================================================
//  Phoenix Sync · Cambios de Liga Máster que manda la web, POR PARTIDA (127)
// -----------------------------------------------------------------------------
//  La web (formulario único, destino «lm» o «ambos») crea cambios para la Liga
//  Máster de cada usuario. Llegan FIRMADOS (Firma.h) con este contenido:
//    { "formato":"phoenix-lm/cambios-1", "desde":N, "version_actual":M,
//      "cambios":[ { "version":7, "tipo":"…", "partida":null|"ML0000000B:1234", … } ] }
//  Tipos:
//    traspaso                 pes_id, club_desde_pes, club_hacia_pes (+ monto, sueldo, clausula, fin_contrato, dorsal)
//                              Lo manda la web: en cada partida el motor ve si es fichaje (IA → tú), venta o IA↔IA.
//    ia_ia · fichaje · venta  lo mismo, pero exigiendo ese papel (si en la partida no cuadra, se rechaza).
//                              Todos usan el motor ya probado en el juego (Sincronizacion.h → LigaMaster).
//    dinero                   presupuesto y/o tope_salarial (euros; null = no cambiar)
//    valor                    pes_id, valor (euros)
//    stat                     lo aplica el motor de estadísticas de LM (126); si no está conectado, queda PENDIENTE
//    deshacer                 deshace = versión: vuelve a dejar lo que había ANTES de esa versión en ESTA partida
//                              (lo sabe el diario de la partida; si aquí nunca se aplicó, no hace nada)
//  Cada partida lleva su propio estado (versión aplicada, diario, pendientes).
//  Todo o nada por cambio: si uno no se puede aplicar, se rechaza ese y los demás siguen.
//  Este módulo NO toca archivos: trabaja sobre un GuardadoLM en memoria (PartidasLM.h escribe).
// =============================================================================

namespace mercado::lmweb {

	inline constexpr const char* kFormato = "phoenix-lm/cambios-1";

	struct CambioLM {
		int64_t version = 0;
		std::string tipo;          ///< ia_ia, fichaje, venta, dinero, valor, stat, deshacer
		std::string partida;       ///< vacío = todas las partidas del usuario
		nlohmann::json datos;      ///< el objeto tal como vino (los campos de cada tipo)
	};

	struct ListaLM {
		int64_t desde = 0, versionActual = 0;
		std::vector<CambioLM> cambios;   ///< en orden de versión
	};

	/// Parsea el contenido (ya verificado con la firma). Rechaza tipos desconocidos, versiones fuera de orden y campos inválidos.
	Resultado<ListaLM> parsear(const std::string& contenido);

	/// Estado de UNA partida en este PC (se guarda en JSON junto a los demás datos de Sync).
	struct EstadoPartida {
		int64_t versionAplicada = 0;
		nlohmann::json diario = nlohmann::json::object();      ///< "<version>": { tipo, antes:{…} } — para deshacer
		std::vector<CambioLM> pendientes;                      ///< esperan algo (motor de stats, juego…)
		nlohmann::json aJson() const;
		static EstadoPartida desdeJson(const nlohmann::json& j);
	};

	/// Resultado de un cambio en una partida (lo que se informa a la web).
	struct ResultadoCambio {
		int64_t version = 0;
		std::string estado;        ///< aplicado · rechazado · pendiente · sin_efecto
		std::string motivo;        ///< texto corto (vacío si aplicado)
		nlohmann::json antes = nlohmann::json::object();     ///< valores previos (club, dinero, valor) — historial y deshacer
		nlohmann::json despues = nlohmann::json::object();
	};

	struct Contexto {
		std::function<int(uint32_t)> posicionDe;    ///< catálogo (0 PT … 12 DC) o -1
		std::function<int(uint32_t)> edadDe;        ///< catálogo (0 = no se sabe)
		/// Motor de stats de LM (126). Nulo = los «stat» quedan pendientes. Devuelve { antes, despues } o error.
		std::function<Resultado<nlohmann::json>(lm::GuardadoLM&, const CambioLM&)> aplicarStat;
	};

	struct Informe {
		std::vector<ResultadoCambio> resultados;
		int aplicados = 0, rechazados = 0, pendientes = 0;
		bool modificado = false;   ///< hay que escribir el guardado
		std::string texto() const;
	};

	/// Aplica a `g` (y a `estado`) los cambios de la lista posteriores a estado.versionAplicada que tocan a la partida
	/// `partidaId`, después de reintentar los pendientes. Cada cambio es todo o nada; el llamador decide si escribe.
	Informe aplicar(const ListaLM& lista, const std::string& partidaId, lm::GuardadoLM& g, EstadoPartida& estado, const Contexto& ctx);

	/// Equipo del usuario en la partida (bloque 0–527 con las tablas del usuario) o -1.
	int equipoUsuario(const lm::GuardadoLM& g);
	/// Identificador estable de la partida: «<ranura>:<ID option del club del usuario>» (p. ej. «ML0000000B:101»).
	std::string idPartida(const std::string& ranura, const lm::GuardadoLM& g);

}
