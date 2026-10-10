#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "BuzonJuego.h"
#include "EstadoJuego.h"

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

	/// Estado del puente con el juego (buzon de avisos web -> Sider).
	enum class EstadoBuzon { Apagado, JuegoCerrado, NoInstalado, Conectado, SinConexion };
	struct AvisoBuzon {
		long long id = 0;
		std::string texto, hora;   ///< hora hh:mm (Lima)
		bool escrito = false;      ///< ya esta en el archivo del juego
	};
	struct InfoBuzon {
		EstadoBuzon estado = EstadoBuzon::JuegoCerrado;
		std::string ultimo;   ///< hora hh:mm (Lima) del aviso mas nuevo escrito
		std::string juego;    ///< carpeta del juego detectado (UTF-8), vacia si esta cerrado
		std::string parche;   ///< nombre del parche detectado (UTF-8), vacio si el juego esta cerrado
		std::vector<AvisoBuzon> avisos;   ///< los 5 mas nuevos
	};

	/// Ultima entrega de datos (Phoenix Sync -> juego). estado: ninguna | esperando | colocada | rechazada | deshecha
	struct InfoEntrega {
		std::string estado = "ninguna";
		std::string id, resumen, motivo, fecha;
		bool puedeDeshacer = false;
	};

	/// Un amigo y su estado (GET /v1/presencia/amigos, contrato §19).
	struct AmigoWeb {
		std::string usuarioId, nombre, avatarUrl, estado, salaId, desde;
	};

	/// Un mensaje del chat general (contrato §27): el mismo chat global de la web.
	struct MensajeGlobal {
		long long id = 0;
		std::string usuarioId, nombre, texto, hora, rol;   ///< hora hh:mm (Lima)
		bool propio = false;
	};
	struct InfoChatGlobal {
		bool cargado = false;      ///< ya llego al menos una respuesta
		bool pausado = false;      ///< el staff apago chat_global
		int esperaSeg = 0;         ///< anti-spam fijado por el admin (0 = sin espera)
		long long rev = 0;         ///< sube cada vez que cambia la lista
		std::string error;         ///< ultimo problema legible (vacio = todo bien)
		std::vector<MensajeGlobal> mensajes;   ///< los ultimos 100, el mas viejo primero
	};

	/// Una noticia de «última hora» (GET /v1/noticias/ultima-hora, la escribe el staff en la web).
	struct NoticiaWeb {
		long long id = 0;
		std::string texto, nivel, enlace, hora;   ///< nivel: info | importante | urgente; hora hh:mm (Lima)
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
		/// Puente con el juego (contrato §25). Solo lectura de estado.
		InfoBuzon buzon();
		/// Repartidor de datos: estado de la ultima entrega y orden de deshacerla (la hace el hilo de Link).
		InfoEntrega entrega();
		void deshacerEntrega();
		/// Barra de «última hora»: noticias vigentes que el staff publica en la web.
		std::vector<NoticiaWeb> noticias();
		bool noticiasCargadas();
		/// Chat general (§27). Solo se pregunta con la ventana visible: rapido con el panel abierto, lento (para el contador) con el panel cerrado.
		InfoChatGlobal chatGlobal();
		void chatGlobalAbierto(bool abierto);

		/// Para el overlay (PhoenixGlass): textos JSON listos para el WebSocket local (ticker y mensajes nuevos del chat general).
		std::vector<std::string> mensajesOverlay();
		void enviarChatGlobal(uint64_t ticket, const std::string& texto);
		/// PES2021.exe esta abierto (se mira cada ~5 s en el hilo de Link).
		bool pesAbierto() const { return _pesAbierto.load(); }
		/// Partido según phoenix.lua (estado.json, leído ~1 vez por segundo en el hilo de Link).
		/// valido = false si no hay archivo, tiene más de 5 s o el hilo lleva más de 3 s sin leerlo.
		juego::EstadoPartidoJuego estadoJuego();
		/// Texto de sala.txt para el HUD del juego (lineas clave=valor). El hilo de Link lo escribe atómico
		/// en content\phoenix\ cuando cambia (y cada 4 s con la hora, para que el juego sepa que Link sigue vivo).
		void salaJuego(const std::string& texto);

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
		bool pasoBuzon(const std::string& token);
		bool pasoChat(const std::string& token);
		bool pasoNoticias(const std::string& token);
		bool pasoEntrega();
		void avisoLocal(const std::string& texto);
		bool volcarBuzon(const std::filesystem::path& carpeta, std::vector<long long>* incluidosWeb);
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
		// Última hora (protegido por _mutex)
		std::vector<NoticiaWeb> _noticias;
		bool _noticiasCargadas = false;
		long long _proximoNoticiasMs = 0;
		std::string _noticiasEtag;
		// Overlay (protegido por _mutex, salvo _overlayVistoMs)
		std::atomic<long long> _overlayVistoMs{ 0 };
		long long _overlayProximoMs = 0;
		long long _overlayTickerMs = 0;
		std::string _overlayFirmaTicker;
		long long _overlayUltimoChat = 0;
		bool _overlayChatIniciado = false;
		bool overlayReciente() const;
		// Chat general (protegido por _mutex)
		std::vector<MensajeGlobal> _chatMsgs;
		std::vector<std::pair<uint64_t, std::string>> _chatEnvios;   ///< ticket + texto por enviar
		std::set<long long> _chatMios;
		long long _chatUltimoId = 0, _chatRev = 0, _proximoChatMs = 0;
		std::string _chatEtag, _chatError;
		bool _chatAbierto = false, _chatCargado = false, _chatPausado = false;
		int _chatEspera = 0, _chatSondeoSeg = 5;
		struct Pedido { uint64_t ticket = 0; std::string tipo, usuarioId; };
		std::vector<Pedido> _pedidos;
		std::vector<ResultadoWeb> _resultados;
		struct PerfilCache { std::string json; long long hastaMs = 0; };
		std::map<uint32_t, PerfilCache> _perfiles;  ///< json vacío = sin cuenta vinculada
		// Buzon del juego: el hilo de Link es el unico que toca estos; _buzonInfo se protege con _mutex
		InfoBuzon _buzonInfo;
		long long _buzonDetectaMs = 0;
		long long _proximoBuzonMs = 0;
		long long _buzonEntregaMs = 0;
		int _buzonEspera = 0;
		int _buzonSondeoSeg = 15;
		std::string _buzonEtag;
		std::string _buzonUltimoTexto;
		std::vector<long long> _buzonPorEntregar;
		std::wstring _buzonCarpeta;   ///< carpeta del buzon (solo si el juego esta abierto y el puente instalado)
		int _buzonPuente = 0;        ///< 0 cerrado, 1 no instalado, 2 listo
		std::vector<phoenix::buzon::Aviso> _buzonWeb;     ///< ultimos avisos que dio la web
		std::vector<phoenix::buzon::Aviso> _buzonLocal;   ///< avisos propios de Link (entregas de datos), id negativo
		long long _buzonLocalId = 0;
		std::atomic<bool> _pesAbierto{ false };
		long long _pesProximoMs = 0;
		// Repartidor de datos: lo toca solo el hilo de Link; _entregaInfo se protege con _mutex
		InfoEntrega _entregaInfo;
		std::atomic<bool> _entregaDeshacer{ false };
		long long _entregaProximoMs = 0;
		bool _entregaCargada = false;
		std::wstring _entregaCarpeta;
		std::vector<uint32_t> _presentes;           ///< parsecIds en la sala (última foto)
		// Estado del partido (estado.json): _estadoCarpeta solo la toca el hilo de Link; _estadoJuego y _estadoLeidoMs, con _mutex
		std::wstring _estadoCarpeta;
		juego::EstadoPartidoJuego _estadoJuego;
		long long _estadoLeidoMs = 0;
		long long _estadoValidoMs = 0;
		// sala.txt (HUD del juego): _salaJuegoTexto con _mutex; el resto solo el hilo de Link
		std::string _salaJuegoTexto, _salaJuegoEscrito;
		long long _salaJuegoProximoMs = 0, _salaJuegoEscritoMs = 0;
		// modo.txt (phoenix.lua v0.18): solo el hilo de Link
		std::wstring _juegoCarpeta;   ///< carpeta de PES2021.exe (la última vista abierta)
		std::string _modoEscrito;
		long long _modoProximoMs = 0;
	};

}
