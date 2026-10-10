#pragma once

// =============================================================================
//  Phoenix Link · Interfaz web — piezas internas compartidas por los archivos
//  de acciones (AccionesWeb*.cpp), el estado (EstadoWeb.cpp) e InterfazWeb.cpp.
//  No incluir fuera de phoenix/web/.
// =============================================================================

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "InterfazWeb.h"
#include "AnfitrionWeb.h"
#include "Puente.h"
#include "MonitorRed.h"
#include "Partido.h"
#include "PerfilesSala.h"

namespace phoenix::web {

	/// Todo el estado vivo de la interfaz web (un solo objeto, hilo de la ventana).
	struct Interno {
		ContextoWeb ctx;
		std::unique_ptr<AnfitrionWeb> anfitrion;
		std::unique_ptr<Puente> puente;

		bool iniciada = false;
		bool permitida = true;
		bool panelClasico = false;      ///< la web pidió mostrar un panel ImGui
		bool avisoCerrado = false;      ///< el host ocultó el aviso de «no disponible»
		int reintentos = 0;
		double creadaEn = 0.0;

		// Lo que la interfaz está mirando (para mandar solo lo necesario)
		std::string seccion = "sala";
		std::string pestana;

		double ultimaFoto = 0.0;
		double ultimaMuestraRed = 0.0;
		size_t chatVistos = 0;
		size_t actividadVistos = 0;
		std::vector<std::string> listaPantallas;   ///< monitores (DX11), se lee una vez
		std::vector<std::string> listaGpus;

		MonitorRed red;
		Partido partido;
		std::unique_ptr<HistorialPartidos> historial;
		std::unique_ptr<PerfilesSala> perfilesSala;

		// Sesión (desde que se abrió la sala)
		double salaAbiertaDesde = -1.0;
		int picoInvitados = 0;
		int entradasSesion = 0;
		int partidosSesion = 0;
		std::set<uint32_t> presentesAntes;

		// Peticiones que esperan a la web (ticket de PhoenixLink → id del Puente)
		std::map<uint64_t, uint64_t> pendientesWeb;
		uint64_t siguienteTicket = 1;

		bool reiniciandoMandos = false;            ///< bandera de GamepadClient::resetAll

		/// Árbitro del partido (Arbitro.cpp): modo competitivo, pausa automática y goles del juego.
		struct Arbitro {
			bool competitivoActivo = false;        ///< Start/Back/Guía añadidos al bloqueo de los invitados
			bool lockPrevio = false;               ///< GamepadClient::lockButtons del anfitrión antes de activarlo
			std::string origen;                    ///< "juego" | "marcador" | "" (de dónde salió la fase)
			std::set<uint32_t> avisados;           ///< jugadores con un problema ya anunciado (no se repite)
			std::string ultimaPausa;               ///< texto de la última pausa automática
			bool ultimaConStart = false;           ///< esa pausa también pulsó Start en el juego
			int pulsando = -1;                     ///< mando (desde 0) con Start pulsado ahora
			double soltarEn = 0.0;
			int golesJuegoL = -1, golesJuegoV = -1;   ///< últimos goles leídos del juego
			int golesAnunciados = -1;              ///< goles del juego ya anunciados en el chat (-1 = aún sin sincronizar)
			std::string faseJuego;                 ///< última fase vista en estado.json
			bool avisoAbandono = false;            ///< ya se avisó de un partido abandonado en el juego
			double startPendienteHasta = 0.0;      ///< pausa automática: Start espera a que el reloj del juego corra
			double ultimoTick = 0.0;
		} arbitro;

		// Cambios que no pueden hacerse dentro de un aviso del propio WebView (se hacen en tick)
		std::string cambioInterfaz;                ///< "phoenix" | "clasica"
		bool recargarPendiente = false;
	};

	Interno& interno();

	// --- utilidades compartidas (definidas en InterfazWeb.cpp) ---------------
	double ahoraSeg();
	int64_t ahoraEpochMs();
	/// Ruta en la carpeta de configuración de Smash Soda (%APPDATA%\Trybuchet\Smash Soda\).
	std::string rutaConfig(const std::string& archivo);
	/// Lanza ErrorAccion("SIN_SALA") si no hay motor de host.
	ProveedorSala& salaObligatoria(Interno& in);
	Hosting& hostingObligatorio(Interno& in);
	/// Entero de `datos[clave]` dentro de [min, max] (lanza DATOS_INVALIDOS si falta).
	int entero(const nlohmann::json& datos, const char* clave, int minimo, int maximo);
	uint32_t idParsec(const nlohmann::json& datos, const char* clave = "parsecId");
	std::string texto(const nlohmann::json& datos, const char* clave, size_t maximo = 255);
	bool booleano(const nlohmann::json& datos, const char* clave);
	/// Copia texto al portapapeles de Windows.
	bool copiarAlPortapapeles(const std::string& texto);
	/// Abre un enlace https en el navegador del usuario.
	void abrirEnlace(const std::string& url);
	/// Mensaje del bot en el chat (lo ven los invitados y queda en Actividad).
	void mensajeDelBot(Interno& in, const std::string& texto);

	// --- piezas -----------------------------------------------------------
	void registrarAccionesApp(Interno& in);
	void registrarAccionesSala(Interno& in);
	void registrarAccionesMandos(Interno& in);
	void registrarAccionesGente(Interno& in);
	void registrarAccionesAjustes(Interno& in);
	void registrarAccionesPartido(Interno& in);
	void registrarAccionesModulos(Interno& in);

	nlohmann::json construirEstado(Interno& in);
	nlohmann::json construirBienvenida(Interno& in);
	/// Muestras de red, sesión, entradas/salidas, chat y actividad (≈ cada frame).
	void tickDatos(Interno& in);
	/// Árbitro del partido: modo competitivo, pausa automática y goles del juego (≈ cada frame; trabaja 2 veces por segundo).
	void tickArbitro(Interno& in);
	/// Valores del bloqueo de botones del anfitrión (sin lo que añade el modo competitivo).
	unsigned int mascaraBotonesPropia(Interno& in);
	bool bloqueoBotonesPropio(Interno& in);
	/// Alterna el bloqueo de botones del anfitrión respetando el modo competitivo.
	void alternarBloqueoBotones(Interno& in);
	/// Vuelve a aplicar el bloqueo tras cambiar la máscara del anfitrión (mandos.botonesBloq).
	void reaplicarArbitro(Interno& in);
	/// Termina el partido: historial, evento web «partida_fin» y (si anunciar) mensaje del bot + `extra`.
	/// nullopt si no había partido en juego o en pausa.
	std::optional<nlohmann::json> finalizarPartido(Interno& in, bool anunciar, const std::string& extra);
	/// Respuestas que llegan de la web (invitar, soltar rival).
	void tickResultadosWeb(Interno& in);
	/// Valores actuales que se guardan en un perfil de sala.
	nlohmann::json valoresPerfilSala(Interno& in);
	/// Aplica un perfil de sala por los mismos caminos que los controles.
	void aplicarPerfilSala(Interno& in, const nlohmann::json& valores);
	/// Formación local/visitante (cada lado ≥ 1, total ≤ mandos disponibles, máx. 8).
	void aplicarFormacion(Interno& in, int local, int visitante);

}
