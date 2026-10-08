#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// =============================================================================
//  Phoenix Link · Partido (marcador, reloj e historial)
// -----------------------------------------------------------------------------
//  Lado A y lado B con sus jugadores (ID Parsec + nombre). El reloj descuenta
//  las pausas. Al finalizar queda un registro en el historial local
//  (phoenix-partidos.json, últimos 200). La parte de mandos, chat y web la
//  hace InterfazWeb; aquí solo hay lógica pura (se prueba en Linux).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	struct JugadorPartido {
		uint32_t parsecId = 0;
		std::string nombre;
	};

	struct LadoPartido {
		std::string nombre;
		std::vector<JugadorPartido> jugadores;
		int goles = 0;
	};

	enum class FasePartido { Libre, Listo, EnJuego, Pausado };

	struct RegistroPartido {
		int64_t id = 0;            ///< epoch ms del inicio (único en la práctica)
		int64_t inicioMs = 0;
		int duracionSeg = 0;
		LadoPartido a, b;
		std::string ganador;       ///< "a" | "b" | "empate"
		json comoJson() const;
		static std::optional<RegistroPartido> desdeJson(const json& j);
	};

	class Partido {
	public:
		/// Deja el partido listo para empezar (cada lado necesita al menos un jugador).
		bool preparar(LadoPartido a, LadoPartido b);
		bool iniciar(int64_t epochMs, double ahoraSeg);
		bool pausar(bool pausa, double ahoraSeg);
		/// lado 0 = A, 1 = B. El marcador queda entre 0 y 99.
		bool gol(int lado, int delta);
		/// Termina y devuelve el registro (solo si estaba en juego o en pausa).
		std::optional<RegistroPartido> finalizar(double ahoraSeg);
		void cancelar();

		/// Los lados juegan con los mandos del otro (tras «Cambiar lados»).
		void invertirMandos() { _invertido = !_invertido; }
		bool mandosInvertidos() const { return _invertido; }

		FasePartido fase() const { return _fase; }
		const LadoPartido& ladoA() const { return _a; }
		const LadoPartido& ladoB() const { return _b; }
		std::string marcador() const;
		int segundosJugados(double ahoraSeg) const;
		json comoJson(double ahoraSeg) const;
		static const char* nombreFase(FasePartido f);

	private:
		FasePartido _fase = FasePartido::Libre;
		LadoPartido _a, _b;
		int64_t _inicioMs = 0;
		double _inicioSeg = 0.0;
		double _pausadoDesde = 0.0;
		double _pausaTotal = 0.0;
		bool _invertido = false;
	};

	class HistorialPartidos {
	public:
		explicit HistorialPartidos(std::string ruta) : _ruta(std::move(ruta)) {}
		/// Más reciente primero. Si el archivo falta o está dañado: lista vacía.
		std::vector<RegistroPartido> lista() const;
		/// Agrega y guarda (escritura atómica: temporal + renombrar). Conserva los últimos 200.
		bool agregar(const RegistroPartido& r);
		static constexpr size_t kMaximo = 200;
	private:
		std::string _ruta;
	};

}
