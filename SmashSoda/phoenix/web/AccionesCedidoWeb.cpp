#include "InterfazWebInterno.h"

#include <random>

#include "../PhoenixPrefs.h"
#include "../core/ModoCedido.h"
#include "../host/Cedido.h"
#include "../../Hosting.h"
#include "../../core/Cache.h"
#include "../../widgets/HostSettingsWidget.h"

// =============================================================================
//  Acciones de la interfaz web: «Modo cedido» (Gente › persona › CEDER MI PC).
//  El anfitrión cede su PC a un invitado: solo él tiene teclado y ratón, y solo
//  dentro de Link y PES. Link queda limitado (Sala, Partido, Mandos) y el motor
//  rechaza lo demás (portero del Puente, código PC_CEDIDA).
//  Se sale con el PIN del anfitrión, con Ctrl+Alt+F12 en su teclado físico, o
//  solo: si el invitado sale, la sala se cierra, la interfaz deja de verse o el
//  guardián falla. Al salir, el invitado pierde teclado y ratón.
//  Guía: docs/MODO-CEDIDO.md · lógica pura: phoenix/core/ModoCedido.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_cedido {
		cedido::Freno gFreno;
		double gWebOcultaDesde = -1.0;
		std::string gUltimoFin;
		bool gPesAbierto = false;
		double gPesMirado = -10.0;

		struct Juego { std::string nombre, ruta, parametros; };

		/// Juego de la biblioteca elegido en Sala › Juegos (el que abre «ABRIR PES»).
		Juego juegoElegido(Interno& in) {
			Juego j;
			if (in.ctx.ajustesSala == nullptr) return j;
			std::string nombre, biblioteca;
			int plazas = 0, limite = 0;
			bool limitador = false, pendiente = false;
			in.ctx.ajustesSala->phoenixLeerOpciones(nombre, plazas, limitador, limite, biblioteca, pendiente);
			for (const GameData& g : Cache::cache.gameList.getGames()) {
				if (g.name == biblioteca && biblioteca != "Default") { j.nombre = g.name; j.ruta = g.path; j.parametros = g.parameters; break; }
			}
			return j;
		}

		std::vector<std::string> extras() {
			std::vector<std::string> v;
			std::string actual;
			for (char c : PhoenixPrefs::get().cedidoExtras + ",") {
				if (c == ',') { if (!actual.empty()) v.push_back(actual); actual.clear(); }
				else if (c != ' ') actual += c;
			}
			return v;
		}

		std::string salAleatoria() {
			std::random_device rd;   // en MSVC usa el generador seguro del sistema
			static const char* hx = "0123456789abcdef";
			std::string s;
			for (int i = 0; i < 16; ++i) s += hx[rd() & 15];
			return s;
		}

		const Guest* buscarInvitado(Hosting& h, uint32_t id) {
			for (Guest& g : h.getGuests()) if (g.userID == id) return &g;
			return nullptr;
		}

		/// Apaga el modo, quita teclado y ratón al invitado y avisa en el chat. Idempotente.
		void terminarCedido(Interno& in, const std::string& motivo) {
			if (!cedido::activo()) return;
			const uint32_t id = cedido::invitado();
			const std::string nombre = cedido::nombreInvitado();
			cedido::terminar();
			gWebOcultaDesde = -1.0;
			gUltimoFin = motivo;
			try {
				if (in.ctx.sala != nullptr && id != 0) {
					in.ctx.sala->permitirTeclado(id, false);
					in.ctx.sala->permitirRaton(id, false);
				}
			}
			catch (...) {}
			mensajeDelBot(in, " La PC vuelve al anfitrión (" + motivo + "). " + nombre + " ya no tiene teclado ni ratón.");
		}
	}

	using namespace detalle_cedido;

	void registrarAccionesCedido(Interno& in) {
		Puente& p = *in.puente;

		// Portero: con la PC cedida, solo lo de jugar (lista blanca en ModoCedido.cpp).
		p.fijarPortero([](const std::string& accion, const json& datos) -> std::optional<std::string> {
			if (!cedido::activo() || cedido::accionPermitida(accion, datos)) return std::nullopt;
			return std::string("La PC está cedida: eso solo lo puede hacer el anfitrión.");
		}, "PC_CEDIDA");

		p.registrar("cedido.empezar", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (cedido::activo()) throw ErrorAccion("YA_CEDIDA", "La PC ya está cedida.");
			Hosting& h = hostingObligatorio(in);
			if (!h.isRunning()) throw ErrorAccion("SALA_CERRADA", "Abre la sala primero.");
			const uint32_t id = idParsec(d);
			const Guest* g = buscarInvitado(h, id);
			if (g == nullptr) throw ErrorAccion("NO_ESTA", "Esa persona no está en la sala.");
			Guest& host = h.getHost();
			if (host.isValid() && host.userID == id) throw ErrorAccion("DATOS_INVALIDOS", "No puedes cederte la PC a ti mismo.");
			PhoenixPrefs& pr = PhoenixPrefs::get();
			if (pr.cedidoPin.empty()) {
				const std::string nuevo = d.contains("pinNuevo") && d["pinNuevo"].is_string() ? d["pinNuevo"].get<std::string>() : "";
				if (!cedido::pinConFormato(nuevo)) throw ErrorAccion("PIN_INVALIDO", "Elige un PIN de 4 a 8 números para poder recuperar la PC.");
				pr.cedidoPin = cedido::guardarPin(nuevo, salAleatoria());
				pr.guardar();
			}
			const Juego juego = juegoElegido(in);
			cedido::Cesion c;
			c.parsecId = id;
			c.nombre = g->name;
			c.ventanaLink = in.ctx.ventana;
			c.carpetaJuego = pr.carpetaJuego;
			c.exeJuego = juego.ruta;
			c.extras = extras();
			cedido::empezar(c);
			gFreno.acierto();
			gWebOcultaDesde = -1.0;
			gUltimoFin.clear();
			mensajeDelBot(in, " " + (host.isValid() ? host.name : std::string("El anfitrión")) + " cedió su PC a " + c.nombre
				+ ": maneja Phoenix Link y PES con teclado y ratón. Nada más de Windows.");
			return json{ {"ok", true} };
		});

		p.registrar("cedido.salir", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (!cedido::activo()) return json{ {"ok", true} };
			const double ahora = ahoraSeg();
			if (!gFreno.puedeIntentar(ahora)) {
				throw ErrorAccion("PIN_ESPERA", "Demasiados intentos. Espera " + std::to_string(static_cast<int>(gFreno.espera(ahora)) + 1) + " s.");
			}
			const std::string pin = d.contains("pin") && d["pin"].is_string() ? d["pin"].get<std::string>() : "";
			if (!cedido::pinCorrecto(pin, PhoenixPrefs::get().cedidoPin)) {
				gFreno.fallo(ahora);
				throw ErrorAccion("PIN_INCORRECTO", "PIN incorrecto.");
			}
			gFreno.acierto();
			terminarCedido(in, "PIN del anfitrión");
			return json{ {"ok", true} };
		});

		p.registrar("cedido.pin", [](const json& d, uint64_t) -> std::optional<json> {
			if (cedido::activo()) throw ErrorAccion("PC_CEDIDA", "Primero recupera la PC.");
			PhoenixPrefs& pr = PhoenixPrefs::get();
			const std::string actual = d.contains("actual") && d["actual"].is_string() ? d["actual"].get<std::string>() : "";
			const std::string nuevo = d.contains("nuevo") && d["nuevo"].is_string() ? d["nuevo"].get<std::string>() : "";
			const double ahora = ahoraSeg();
			if (!pr.cedidoPin.empty()) {
				if (!gFreno.puedeIntentar(ahora)) throw ErrorAccion("PIN_ESPERA", "Demasiados intentos. Espera un minuto.");
				if (!cedido::pinCorrecto(actual, pr.cedidoPin)) { gFreno.fallo(ahora); throw ErrorAccion("PIN_INCORRECTO", "El PIN actual no es correcto."); }
			}
			if (!cedido::pinConFormato(nuevo)) throw ErrorAccion("PIN_INVALIDO", "El PIN nuevo debe tener de 4 a 8 números.");
			gFreno.acierto();
			pr.cedidoPin = cedido::guardarPin(nuevo, salAleatoria());
			pr.guardar();
			return json{ {"ok", true} };
		});

		p.registrar("cedido.abrirPes", [&in](const json&, uint64_t) -> std::optional<json> {
			const Juego j = juegoElegido(in);
			std::string error;
			if (!cedido::abrirPes(j.ruta, j.parametros, PhoenixPrefs::get().carpetaJuego, error)) throw ErrorAccion("NO_SE_PUDO", error);
			gPesMirado = -10.0;   // que el estado lo vuelva a mirar ya
			return json{ {"ok", true} };
		});
	}

	void tickCedido(Interno& in) {
		if (!cedido::activo()) return;
		try {
			std::string motivo;
			if (cedido::tomarAtajo()) { terminarCedido(in, std::string("el anfitrión pulsó ") + cedido::ATAJO); return; }
			if (cedido::tomarFallo(motivo)) { terminarCedido(in, "fallo del guardián: " + motivo); return; }
			if (in.ctx.hosting == nullptr || !in.ctx.hosting->isRunning()) { terminarCedido(in, "la sala se cerró"); return; }
			if (buscarInvitado(*in.ctx.hosting, cedido::invitado()) == nullptr) { terminarCedido(in, cedido::nombreInvitado() + " salió de la sala"); return; }
			// Si la interfaz web no se ve (cayó, panel clásico, respaldo ImGui), Link quedaría sin límites: se apaga.
			const bool webVisible = in.anfitrion && in.anfitrion->visible() && in.permitida && !in.panelClasico;
			const double ahora = ahoraSeg();
			if (webVisible) gWebOcultaDesde = -1.0;
			else if (gWebOcultaDesde < 0.0) gWebOcultaDesde = ahora;
			else if (ahora - gWebOcultaDesde > 10.0) { terminarCedido(in, "la interfaz de Link dejó de verse"); return; }
		}
		catch (...) {
			terminarCedido(in, "error inesperado");
		}
	}

	json estadoCedido(Interno& in) {
		const double ahora = ahoraSeg();
		const Juego j = juegoElegido(in);
		if (ahora - gPesMirado >= 2.0) {
			gPesMirado = ahora;
			gPesAbierto = cedido::pesAbierto(PhoenixPrefs::get().carpetaJuego, j.ruta);
		}
		return {
			{"activo", cedido::activo()},
			{"parsecId", cedido::invitado()},
			{"nombre", cedido::activo() ? cedido::nombreInvitado() : std::string()},
			{"desde", cedido::activo() ? cedido::desdeMs() : 0},
			{"pinGuardado", !PhoenixPrefs::get().cedidoPin.empty()},
			{"atajo", cedido::ATAJO},
			{"juego", j.nombre},
			{"pesAbierto", gPesAbierto},
			{"espera", static_cast<int>(gFreno.espera(ahora) + 0.999)},
			{"ultimoFin", gUltimoFin},
		};
	}

}
