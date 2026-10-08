#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// =============================================================================
//  Phoenix Link · PhoenixLink: la app ↔ la web (API /v1, contrato 1.6.0)
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
		std::string juego, parche, region;
	};

	enum class EstadoLink { SinVincular, Vinculando, Conectado, SinConexion, Pausado };

	/// Un amigo y su estado (GET /v1/presencia/amigos, contrato §19).
	struct AmigoWeb {
		std::string usuarioId, nombre, avatarUrl, estado, salaId, desde;
	};

	/// Resultado de una petición pedida desde la interfaz (invitar, soltar rival).
	struct ResultadoWeb {
		uint64_t ticket = 0;
		bool ok = false;
		std::string codigo;    ///< código estable del contrato si falló
		std::string mensaje;   ///< texto para mostrar
	};

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

		// --- Interfaz nueva (contrato 1.4.0 – 1.6.0) -----------------------
		/// La ventana de la app se ve: solo así se sondean los amigos (contrato §21).
		void ventanaVisible(bool si);
		/// Hay partido en juego: el latido manda «en_partida» y sale del radar (§22).
		void marcarPartido(bool enJuego);
		/// Encola un evento de sala (§6). `datosJson` = objeto JSON en texto ("{}" si nada).
		/// Se guarda en disco hasta poder enviarlo. Sin vínculo con la web se ignora.
		void evento(const std::string& tipo, const std::string& actorParsec, const std::string& datosJson);
		std::vector<AmigoWeb> amigos();
		bool amigosCargados();
		/// Invita a un amigo a la sala abierta (§20). El resultado llega por tomarResultados().
		void invitar(uint64_t ticket, const std::string& usuarioId);
		/// Libera el puesto de rival que aceptó en el radar (§22).
		void soltarRival(uint64_t ticket);
		std::vector<ResultadoWeb> tomarResultados();
		/// Cartas de jugador (§23) de quienes están en la sala: parsecId → JSON en texto.
		std::map<uint32_t, std::string> perfiles();
		/// Versión de la Liga Máster que informa la presencia (-1 = sin dato).
		long long versionLiga();
		std::string salaId();
		int eventosEnCola();

	private:
		PhoenixLink() = default;
		void bucle();
		void pasoEmparejar(const std::string& codigo);
		void pasoAbrir(const InstantaneaSala& foto);
		void pasoLatido(const InstantaneaSala& foto);
		void pasoCerrar(const std::string& salaId);
		void pasoDiagnostico(const std::string& salaId);
		void aplicarRoles(const std::string& jsonRoles);
		// Interfaz nueva: un paso opcional por vuelta, sin frenar el latido
		bool pasoOpcional(const std::string& token, const std::string& salaId, const InstantaneaSala& foto);
		bool pasoEventos(const std::string& token, const std::string& salaId);
		bool pasoPresencia(const std::string& token, const std::string& salaId);
		bool pasoAmigos(const std::string& token);
		bool pasoPerfiles(const std::string& token, const InstantaneaSala& foto);
		bool pasoPedidos(const std::string& token, const std::string& salaId);
		void guardarEventos();   ///< con _mutex tomado
		void cargarEventos();
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
		std::string _salaDiagnosticada;

		// Interfaz nueva (protegido por _mutex)
		bool _ventanaVisible = true;
		bool _enPartida = false;
		std::vector<std::string> _eventos;          ///< cada uno: objeto JSON en texto
		long long _proximoEnvioEventosMs = 0;
		long long _proximaPresenciaMs = 0;
		long long _proximoAmigosMs = 0;
		long long _proximoPerfilesMs = 0;
		long long _versionLiga = -1;
		std::string _etagAmigos;
		std::vector<AmigoWeb> _amigos;
		bool _amigosCargados = false;
		struct Pedido { uint64_t ticket = 0; std::string tipo, usuarioId; };
		std::vector<Pedido> _pedidos;
		std::vector<ResultadoWeb> _resultados;
		struct PerfilCache { std::string json; long long hastaMs = 0; };
		std::map<uint32_t, PerfilCache> _perfiles;  ///< json vacío = sin cuenta vinculada
		std::vector<uint32_t> _presentes;           ///< parsecIds en la sala (última foto)
	};

}
