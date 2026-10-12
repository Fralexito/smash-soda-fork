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
		int correcciones = 0;      ///< goles tocados a mano (partido.gol) durante el partido
		int pausas = 0;            ///< veces que se pausó el marcador
		std::string cierre;        ///< "completo" (el juego confirmó el final) | "manual" | "" (registros viejos)
		std::string origenLados;   ///< "host" (los eligió el anfitrión) | "mandos" (Link los armó solo al pitazo)
		int correccionesLados = 0; ///< veces que se corrigió quién es local (CORREGIR LADOS)
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
		/// Marcador entero (lo usa el árbitro con los goles que da el juego). Entre 0 y 99.
		bool fijarGoles(int golesA, int golesB);
		/// Termina y devuelve el registro (solo si estaba en juego o en pausa).
		std::optional<RegistroPartido> finalizar(double ahoraSeg);
		void cancelar();

		/// Los lados juegan con los mandos del otro (tras «Cambiar lados»).
		void invertirMandos() { _invertido = !_invertido; }
		bool mandosInvertidos() const { return _invertido; }
		/// Solo cambia quién es local en Link (no mueve mandos): para cuando en el juego se eligió el otro lado.
		bool corregirLados();
		/// "host" por defecto al preparar; el árbitro pone "mandos" cuando arma los lados solo.
		void origenLados(const std::string& o) { _origenLados = o; }
		const std::string& origenLados() const { return _origenLados; }
		int correccionesLados() const { return _correccionesLados; }

		FasePartido fase() const { return _fase; }
		const LadoPartido& ladoA() const { return _a; }
		const LadoPartido& ladoB() const { return _b; }
		std::string marcador() const;
		int segundosJugados(double ahoraSeg) const;
		int64_t inicioMs() const { return _inicioMs; }
		int correcciones() const { return _correcciones; }
		int pausas() const { return _pausas; }
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
		int _correcciones = 0;
		int _pausas = 0;
		std::string _origenLados = "host";
		int _correccionesLados = 0;
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
