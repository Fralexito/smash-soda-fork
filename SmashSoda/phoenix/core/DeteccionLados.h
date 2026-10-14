#pragma once

// =============================================================================
//  Phoenix Link · Detección del lado de cada mando (lógica pura, sin Windows; se prueba en
//  pruebas-link/lados_test.cpp). Ver docs/DETECCION-LADOS.md.
// -----------------------------------------------------------------------------
//  Idea: el futbolista que maneja un mando corre hacia donde apunta su stick izquierdo (con un
//  pequeño retraso). Se cruza el stick de cada mando con la velocidad de cada fichita del Campo 2D
//  (core/CampoVivo): la que lo sigue de forma sostenida es la suya, y su equipo (A/B del Campo) es
//  el del mando. El controlado cambia a menudo, pero siempre dentro del mismo equipo: se vota cuadro
//  a cuadro y se decide cuando un equipo gana con claridad.
//  La cámara no se supone: se prueban las 4 orientaciones (signo de x y de z) y se queda la que mejor
//  explica a todos los mandos.
//  Para pasar de A/B a local/visita hace falta un ancla (ver LadoDefendido y equipoQueMarco).
// =============================================================================

#include <cstdint>
#include <deque>
#include <map>
#include <utility>
#include <vector>

namespace phoenix::lados {

	/// Una fichita del Campo: posición en metros y equipo (0 = A, 1 = B; otros valores se ignoran).
	struct Ficha { float x = 0, z = 0; int eq = -1; };

	/// Stick izquierdo de un mando, normalizado a −1…1 (arriba = ly positivo, como XInput).
	struct Stick { int asiento = 0; float lx = 0, ly = 0; };

	struct Veredicto {
		int asiento = 0;
		int eq = -1;            ///< 0 = A, 1 = B, -1 = aún no se sabe
		int votosA = 0, votosB = 0;
		double proporcion = 0;  ///< votos del ganador / votos totales
		int apretado = 0;       ///< lecturas con el stick apretado (los votos salen de aquí)
		bool decidido = false;
	};

	class Detector {
	public:
		/// Una lectura del Campo (~20/s) con los sticks de ese instante. t en segundos, creciente.
		void alimentar(double t, const std::vector<Ficha>& fichas, const std::vector<Stick>& sticks);
		/// Veredicto por asiento con la orientación de cámara que mejor encaja.
		std::vector<Veredicto> veredictos() const;
		/// Fichitas que ahora mismo maneja cada mando: pares (índice en `fichas`, asiento). Solo con correlación sostenida alta del
		/// stick con la carrera de esa fichita (con la orientación de cámara que mejor encaja). Pensado para colorear la luz del poseedor.
		std::vector<std::pair<int, int>> controlados(const std::vector<Ficha>& fichas) const;
		/// Orientación elegida (bit 0: x invertida, bit 1: z invertida) y si gana con margen claro.
		int orientacion() const;
		bool orientacionClara() const;
		void reiniciar() { *this = Detector(); }

		// Ajustes (públicos para las pruebas)
		static constexpr double kRetraso = 0.25;      ///< s entre el stick y el movimiento del futbolista
		static constexpr double kTau = 1.0;           ///< s de memoria de la correlación por fichita
		static constexpr float kStickMin = 0.7f;      ///< stick «apretado»
		static constexpr float kVelMin = 1.5f;        ///< m/s: fichita en movimiento
		static constexpr double kCorrMin = 0.6;       ///< correlación sostenida mínima para votar
		static constexpr int kVotosMin = 200;         ///< ≈ 10 s de stick apretado a 20 lecturas/s
		static constexpr double kProporcionMin = 0.8;
		static constexpr double kTasaMin = 0.35;      ///< votos / lecturas con stick: un mando que no maneja a nadie apenas vota

	private:
		struct Pista { int id; float x, z, vx = 0, vz = 0; int eq; bool conVel = false; };
		struct Muestra { double t; float lx, ly; };
		struct PorAsiento {
			std::deque<Muestra> hist;
			std::map<int, double> S[4];       ///< correlación con memoria, por orientación y fichita
			int votos[4][2] = {};
			int apretado = 0;
		};
		std::vector<Pista> pistas_;
		int sigId_ = 1;
		double tPrev_ = -1;
		std::map<int, PorAsiento> asientos_;
		void seguir(double t, const std::vector<Ficha>& fichas);
	};

	/// Qué lado defiende cada equipo (el de su portero): cuenta, con memoria de ~20 s, en qué área chica
	/// tiene alguien cada equipo. Se adapta solo al descanso.
	class LadoDefendido {
	public:
		void alimentar(double t, const std::vector<Ficha>& fichas);
		/// −1 = defiende x < 0, +1 = defiende x > 0, 0 = no se sabe todavía.
		int lado(int eq) const;
	private:
		double p_[2][2] = {};   ///< [equipo][lado 0 = izquierda, 1 = derecha] presencia con memoria
		double tPrev_ = -1;
	};

	/// Gol en la portería del lado `ladoPorteria` (−1 o +1): marcó el equipo que NO la defiende
	/// (vale también en propia puerta). −1 si no se sabe.
	int equipoQueMarco(int ladoPorteria, const LadoDefendido& ld);

	/// ¿El balón entró en una portería? Devuelve −1/+1 (lado) o 0.
	int porteriaDelGol(float bx, float by, float bz);

}

namespace phoenix::lados {

	/// Ancla A/B → local: devuelve el equipo del Campo (0/1) que es el local, o −1 si no hay ancla o se contradicen.
	/// anclaGol: deducida de un gol (segura). anclaSaque: el equipo que defiende x < 0 en el 1.er tiempo (PES pone ahí al local).
	int equipoLocal(int anclaGol, int anclaSaque);

	/// Un jugador del partido con lado detectado: equipo del Campo (0/1) y si Link lo tiene como local.
	struct Participante { int eqCampo = -1; bool linkLocal = false; };

	enum class Decision { Nada, Coinciden, Corregir };
	/// Con el equipo local del Campo: Corregir si TODOS los detectados están al revés en Link, Coinciden si todos
	/// están bien, Nada si no hay ninguno o hay mezcla (ante la duda no se toca).
	Decision decidir(const std::vector<Participante>& ps, int eqLocal);

}
