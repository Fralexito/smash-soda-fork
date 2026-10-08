#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "BaseDatosParche.h"
#include "OptionFile.h"

// =============================================================================
//  Phoenix Mercado · Emparejamiento  (Phoenix ID  ↔  ID local de cada parche)
// -----------------------------------------------------------------------------
//  Referencia = jugadores/clubes de la web (lm_jugadores / lm_clubes), con los
//  datos del parche con el que se cargó el catálogo.
//  Destino    = la base de datos + option file del parche instalado en esta PC.
//
//  Reglas (en orden):
//   1. Mismo ID y datos compatibles      → AUTOMATICO (por_id)
//   2. Mismo ID pero datos muy distintos → REVISAR  (ID reutilizado por otro jugador)
//   3. Búsqueda por datos (nombre, nacionalidad, edad, altura, posición):
//        puntaje alto y claramente mejor que el 2.º → AUTOMATICO (por_datos)
//        puntaje medio o empate                     → REVISAR
//        nada parecido                              → SIN_CANDIDATO
//  Nunca se aplica un fichaje con un emparejamiento que no sea AUTOMATICO o
//  confirmado a mano.
// =============================================================================

namespace mercado {

	enum class EstadoEmparejamiento { Automatico, Revisar, SinCandidato };
	const char* nombreEstado(EstadoEmparejamiento e);

	struct JugadorReferencia {
		int64_t phoenixId = 0;
		uint32_t pesIdReferencia = 0;   ///< ID en el parche de referencia (0 = desconocido)
		std::string nombre;
		int nacionalidad = 0, altura = 0, edad = 0, posicion = -1;
	};

	struct Candidato {
		uint32_t idLocal = 0;
		int puntaje = 0;               ///< 0–100
		std::string nombre;
	};

	struct ResultadoJugador {
		int64_t phoenixId = 0;
		EstadoEmparejamiento estado = EstadoEmparejamiento::SinCandidato;
		uint32_t idLocal = 0;
		std::string metodo;            ///< por_id | por_datos | —
		std::string motivo;            ///< explicación legible
		std::vector<Candidato> candidatos;   ///< hasta 3, para la revisión manual
	};

	struct ClubReferencia {
		int64_t phoenixId = 0;
		uint32_t pesTeamIdReferencia = 0;
		std::string nombre;
		std::vector<int64_t> jugadores;   ///< Phoenix IDs de su plantilla de referencia
	};

	struct ResultadoClub {
		int64_t phoenixId = 0;
		EstadoEmparejamiento estado = EstadoEmparejamiento::SinCandidato;
		uint32_t idLocal = 0;
		std::string metodo, motivo, nombreLocal;
		double coincidencia = 0;       ///< % de su plantilla encontrada en el club local
	};

	/// Normaliza nombres: minúsculas, sin acentos, sin signos ("Luka Modrić" → "luka modric").
	std::string normalizarNombre(const std::string& utf8);
	/// Parecido entre dos nombres, 0–1.
	double parecidoNombre(const std::string& a, const std::string& b);
	/// Puntaje 0–100 entre un jugador de referencia y una ficha local.
	int puntuar(const JugadorReferencia& r, const FichaJugador& f);

	class Emparejador {
	public:
		explicit Emparejador(const std::map<uint32_t, FichaJugador>& baseLocal);

		ResultadoJugador emparejar(const JugadorReferencia& r) const;
		std::vector<ResultadoJugador> emparejarTodos(const std::vector<JugadorReferencia>& refs) const;

		/// Clubes: mismo ID + coincidencia de plantilla (usando los jugadores ya emparejados).
		std::vector<ResultadoClub> emparejarClubes(const std::vector<ClubReferencia>& clubes,
			const std::vector<ResultadoJugador>& jugadores, const OptionFile& optionLocal) const;

		// Umbrales (públicos para poder ajustarlos y probarlos)
		int umbralAutomatico = 80, umbralRevisar = 55, margenMinimo = 15, umbralPorId = 65;

	private:
		const std::map<uint32_t, FichaJugador>& _base;
		std::map<std::string, std::vector<uint32_t>> _indice;   // palabra del nombre → IDs
	};

	/// Informe JSON (para la web o para revisar a mano).
	std::string informeJson(const std::vector<ResultadoJugador>& j, const std::vector<ResultadoClub>& c,
		const std::string& perfilParche);

}
