#pragma once

// =============================================================================
//  Phoenix Link · Medidor de retraso del mando (lógica pura, sin Windows: se
//  prueba aparte en pruebas-link/retraso_test.cpp).
// -----------------------------------------------------------------------------
//  Mide, por jugador, cómo LLEGAN al host los mensajes de su mando de Parsec:
//  intervalo entre llegadas mientras mueve el mando (media, p95), jitter
//  (variación entre intervalos seguidos), ritmo (mensajes por segundo) y
//  «saltos»: un hueco seguido de varios mensajes de golpe, la huella típica de
//  la red reteniendo y soltando paquetes (una pausa del jugador no la deja).
//  Junto al ping, la codificación/decodificación y la pérdida de paquetes de
//  Parsec da un «retraso estimado» con semáforo.
//
//  Lo que NO se puede medir desde el host: el retraso dentro de la casa del
//  invitado (su mando, su Bluetooth, su pantalla o tele).
//
//  MedidorMando lo usa UN solo hilo (el de entrada): mensaje() es O(1), sin
//  asignaciones ni bloqueos (ventana deslizante en arrays fijos).
// =============================================================================

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace phoenix::retraso {

	/// Mensajes que llegan con menos de esto entre sí son la MISMA llegada (un paquete).
	constexpr double kMismaLlegadaMs = 2.0;
	/// Intervalos menores que esto cuentan como «moviendo el mando» (los mayores son pausas del jugador).
	constexpr double kActivoMs = 100.0;
	/// Hueco máximo que aún puede ser un salto de red (más largo: el jugador soltó el mando).
	constexpr double kHuecoMaxMs = 250.0;
	/// Un salto es un hueco de al menos esto…
	constexpr double kSaltoMinMs = 40.0;

	/// Lo medido de la llegada del mando de un jugador.
	struct MedidaMando {
		uint32_t userId = 0;
		int muestras = 0;          ///< intervalos «moviendo el mando» en la ventana
		double mediaMs = -1.0;     ///< intervalo medio entre llegadas
		double p95Ms = -1.0;       ///< el 95 % de las llegadas tardan menos que esto
		double jitterMs = -1.0;    ///< variación media entre intervalos seguidos
		int saltosMin = 0;         ///< saltos en los últimos 60 s
		int ritmo = 0;             ///< mensajes en el último segundo completo
		double ultimoHaceMs = -1.0;///< hace cuánto llegó su último mensaje
	};

	/// Ventana deslizante por jugador (hasta kJugadores). Un solo hilo.
	class MedidorMando {
	public:
		static constexpr int kJugadores = 16;
		static constexpr int kIntervalos = 128;
		static constexpr int kSaltos = 64;

		/// Llega un mensaje de mando (estado, eje o botón) de `userId`. O(1), sin asignaciones.
		void mensaje(uint32_t userId, double ahoraMs);
		/// Avanza el reloj sin mensajes (cierra segundos y llegadas pendientes). Barato.
		void tick(double ahoraMs);
		/// Resumen de cada jugador visto (asigna: llamar ≈ 1 vez por segundo).
		std::vector<MedidaMando> medir(double ahoraMs) const;
		void olvidar(uint32_t userId);
		void limpiar();

	private:
		struct Ranura {
			uint32_t userId = 0;
			bool usada = false;
			double ultimo = -1.0;            ///< último mensaje
			double llegada = -1.0;           ///< inicio de la llegada en curso
			int enLlegada = 0;               ///< mensajes en la llegada en curso
			bool abierta = false;            ///< la llegada en curso aún no se evaluó
			bool candidato = false;          ///< la llegada en curso vino tras un hueco sospechoso
			double tipicoMs = 0.0;           ///< intervalo típico (media móvil de los activos)
			double tipicoMsgs = 1.0;         ///< mensajes por llegada típicos
			std::array<float, kIntervalos> iv{};
			int ivN = 0, ivPos = 0;
			std::array<double, kSaltos> saltos{};
			int saltosN = 0, saltosPos = 0;
			double segundo = -1.0;           ///< inicio del segundo en curso
			int enSegundo = 0, ritmo = 0;
		};

		Ranura* ranura(uint32_t userId, double ahoraMs);
		static void cerrarLlegada(Ranura& r);
		static void avanzarSegundo(Ranura& r, double ahoraMs);

		std::array<Ranura, kJugadores> _r{};
	};

	// ---- Evaluación (retraso estimado + semáforo) ---------------------------

	struct UmbralesRetraso {
		int totalVerde = 80;        ///< ms de retraso estimado
		int totalAmbar = 130;
		double perdidaAmbar = 2.0;  ///< % de paquetes reenviados
		double perdidaRojo = 5.0;
		int saltosAmbar = 6;        ///< saltos por minuto
		int saltosRojo = 20;
	};

	struct DatosParsec {
		int pingMs = -1;            ///< ida y vuelta (-1 = sin dato)
		double codificarMs = 0.0;   ///< el host codifica el vídeo
		double decodificarMs = 0.0; ///< el invitado lo decodifica
		double perdidaPct = 0.0;    ///< % de paquetes de vídeo reenviados
	};

	struct Evaluacion {
		int totalMs = -1;           ///< retraso estimado (-1 = sin datos)
		std::string semaforo = "sin_datos";  ///< verde | ambar | rojo | sin_datos
		std::string motivo;         ///< la causa principal, corta, en español («ping alto»…)
	};

	/// total = ping + codificar + decodificar + jitter del mando. Manda la peor regla.
	Evaluacion evaluar(const MedidaMando* m, const DatosParsec& p, const UmbralesRetraso& u = UmbralesRetraso());

	/// % de paquetes reenviados (retransmisiones / enviados) en una ventana de ~10 muestras.
	class PerdidaPaquetes {
	public:
		/// Contadores acumulados de Parsec (packetsSent, fastRTs + slowRTs). Devuelve el % actual.
		double registrar(uint32_t id, uint32_t enviados, uint32_t reenviados);
		void olvidar(uint32_t id);
	private:
		struct Muestras { std::array<uint32_t, 10> e{}, r{}; int n = 0, pos = 0; uint32_t id = 0; };
		std::vector<Muestras> _m;
	};

	/// Antispam del aviso en el chat: rojo seguido ≥ 10 s durante un partido;
	/// como mucho un aviso por jugador cada 3 min y uno en total cada 30 s.
	class AvisosRetraso {
	public:
		static constexpr double kRojoSeguido = 10.0;
		static constexpr double kPorJugador = 180.0;
		static constexpr double kGlobal = 30.0;
		/// Llamar 1 vez por segundo por jugador. true = toca avisar ahora.
		bool actualizar(uint32_t id, const std::string& semaforo, bool enPartido, double ahoraSeg);
		void limpiar() { _j.clear(); _ultimoGlobal = -1e9; }
	private:
		struct Jugador { uint32_t id = 0; double rojoDesde = -1.0; double avisado = -1e9; };
		std::vector<Jugador> _j;
		double _ultimoGlobal = -1e9;
	};

}
