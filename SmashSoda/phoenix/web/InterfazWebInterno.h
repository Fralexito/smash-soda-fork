#pragma once

// =============================================================================
//  Phoenix Link · Interfaz web — piezas internas compartidas por los archivos
//  de acciones (AccionesWeb*.cpp), el estado (EstadoWeb.cpp) e InterfazWeb.cpp.
//  No incluir fuera de phoenix/web/.
// =============================================================================

#include <cstdint>
#include <map>
#include <memory>
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

	nlohmann::json construirEstado(Interno& in);
	nlohmann::json construirBienvenida(Interno& in);
	/// Muestras de red, sesión, entradas/salidas, chat y actividad (≈ cada frame).
	void tickDatos(Interno& in);
	/// Respuestas que llegan de la web (invitar, soltar rival).
	void tickResultadosWeb(Interno& in);
	/// Valores actuales que se guardan en un perfil de sala.
	nlohmann::json valoresPerfilSala(Interno& in);
	/// Aplica un perfil de sala por los mismos caminos que los controles.
	void aplicarPerfilSala(Interno& in, const nlohmann::json& valores);
	/// Formación local/visitante (cada lado ≥ 1, total ≤ mandos disponibles, máx. 8).
	void aplicarFormacion(Interno& in, int local, int visitante);

}
