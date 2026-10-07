#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// =============================================================================
//  Phoenix Soda · PhoenixLink: la app ↔ la web (API /v1, contrato 1.5.0)
// -----------------------------------------------------------------------------
//  Corre en un hilo propio: nunca traba el juego ni la interfaz. La interfaz le
//  pasa una «foto» de la sala (actualizar) y lee su estado para mostrarlo.
//  Si internet o la web fallan, la sala sigue funcionando igual.
// =============================================================================

namespace phoenix {

	struct InvitadoMuestra {
		std::string parsecId;
		std::string nombre;
		int pingMs = -1;
	};

	struct InstantaneaSala {
		bool abierta = false;
		std::string enlace;
		int plazasTotal = 4;
		int plazasLibres = 0;
		std::vector<InvitadoMuestra> invitados;
		// Preferencias del host (PhoenixPrefs)
		std::string visibilidad = "amigos";
		bool aceptaEspectadores = true;
		int limiteEspectadores = 4;
	};

	enum class EstadoLink { SinVincular, Vinculando, Conectado, SinConexion, Pausado };

	class PhoenixLink {
	public:
		static PhoenixLink& instancia();

		void iniciar();
		void detener();   ///< cierra la sala en la web si estaba publicada

		/// Desde el hilo de la interfaz (≈ 1 vez por segundo).
		void actualizar(const InstantaneaSala& foto);

		void emparejar(const std::string& codigo);
		void desvincular();
		void reintentar();

		// Para la interfaz
		EstadoLink estado();
		std::string usuario();
		std::string mensaje();     ///< último aviso legible (error o éxito)
		bool salaPublicada();      ///< la web ya conoce la sala abierta
		int jugadoresEnLista();    ///< jugadores en la lista de roles (incluye host)

	private:
		PhoenixLink() = default;
		void bucle();
		void pasoEmparejar(const std::string& codigo);
		void pasoAbrir(const InstantaneaSala& foto);
		void pasoLatido(const InstantaneaSala& foto);
		void pasoCerrar(const std::string& salaId);
		void aplicarRoles(const std::string& jsonRoles);
		void fallo(const std::string& codigo, const std::string& mensaje, int reintentarEn);

		std::thread _hilo;
		std::atomic<bool> _corriendo{ false };
		std::mutex _mutex;

		// Compartido (protegido por _mutex)
		InstantaneaSala _foto;
		std::string _codigoPendiente;
		std::string _token;
		std::string _usuario;
		std::string _mensaje;
		std::string _salaId;
		EstadoLink _estado = EstadoLink::SinVincular;
		bool _pausado = false;
		int _jugadores = 0;
		std::string _firmaPrefs;   // visibilidad/espectadores publicados
		long long _proximoLatidoMs = 0;
		long long _esperaHastaMs = 0;
		int _espera = 0;
		int _latidoSeg = 30;
	};

}
