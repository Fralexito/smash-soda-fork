#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../core/Config.h"
#include "../../core/MasterOfPuppets.h"
#include "../../services/Hotseat.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../core/AccionesSala.h"
#include "../core/MandoHost.h"

// =============================================================================
//  Acciones de la interfaz web: mandos, solicitudes, entrada por Parsec,
//  herramientas del motor de mandos y turnos (hotseat).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_mandos {
		int indiceMando(Interno& in, const json& d, const char* clave = "indice") {
			const int total = static_cast<int>(salaObligatoria(in).asientos(8).size());
			if (total == 0) throw ErrorAccion("SIN_MANDOS", "No hay mandos virtuales: revisa ViGEmBus en Diagnóstico.");
			return entero(d, clave, 0, total - 1);
		}

		GamepadClient& clienteMandos(Interno& in) {
			GamepadClient& gc = hostingObligatorio(in).getGamepadClient();
			if (gc.isSlave) throw ErrorAccion("MANDOS_OCUPADOS", "Con turnos o torneo activos no se pueden tocar los mandos desde aquí.");
			return gc;
		}
	}

	using namespace detalle_mandos;

	void registrarAccionesMandos(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("mandos.conectar", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).conectarMando(indiceMando(in, d));
			return json::object();
		});
		p.registrar("mandos.desconectar", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).desconectarMando(indiceMando(in, d));
			return json::object();
		});
		p.registrar("mandos.bloquear", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).alternarBloqueo(indiceMando(in, d));
			return json::object();
		});
		p.registrar("mandos.liberar", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).liberarMando(indiceMando(in, d));
			return json::object();
		});
		p.registrar("mandos.asignar", [&in](const json& d, uint64_t) -> std::optional<json> {
			const int indice = indiceMando(in, d);
			if (!AccionesSala::asignarConectando(salaObligatoria(in), indice, idParsec(d))) {
				throw ErrorAccion("NO_PUEDE_JUGAR", "Ese invitado no puede tomar mando (la lista de la web lo marca como espectador).");
			}
			return json::object();
		});
		p.registrar("mandos.intercambiar", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (!AccionesSala::intercambiar(salaObligatoria(in), indiceMando(in, d, "a"), indiceMando(in, d, "b"))) {
				throw ErrorAccion("NADA_QUE_MOVER", "Los dos mandos están libres.");
			}
			return json::object();
		});
		p.registrar("mandos.formacion", [&in](const json& d, uint64_t) -> std::optional<json> {
			aplicarFormacion(in, entero(d, "local", 1, 7), entero(d, "visitante", 1, 7));
			return json::object();
		});
		p.registrar("mandos.tomar", [&in](const json& d, uint64_t) -> std::optional<json> {
			ProveedorSala& sala = salaObligatoria(in);
			const int n = entero(d, "numero", 0, 8);
			if (n > 0 && !sala.abierta()) throw ErrorAccion("SALA_CERRADA", "Abre la sala para tomar un mando.");
			MandoHost::tomar(&sala, n);
			return json{ {"activo", MandoHost::activo()} };
		});

		// ---- Herramientas del motor de mandos (barra del panel original) --------
		p.registrar("mandos.herramienta", [&in](const json& d, uint64_t) -> std::optional<json> {
			const std::string que = texto(d, "nombre", 32);
			Hosting& h = hostingObligatorio(in);
			GamepadClient& gc = clienteMandos(in);
			if (que == "reiniciar") {
				if (!in.reiniciandoMandos) {
					in.reiniciandoMandos = true;
					gc.resetAll(in.reiniciandoMandos); // la bandera vuelve a false al terminar
				}
			}
			else if (que == "desconectarTodos") gc.disconnectAllGamepads();
			else if (que == "ordenar") gc.sortGamepads();
			else if (que == "bloquearTodo") h.toggleGamepadLock();
			else if (que == "bloquearBotones") alternarBloqueoBotones(in);   // respeta el modo competitivo
			else throw ErrorAccion("DATOS_INVALIDOS", "Herramienta desconocida.");
			return json::object();
		});

		// ---- Elegir botones (bloqueo parcial): se guarda y se aplica al instante -----
		p.registrar("mandos.botonesBloq", [&in](const json& d, uint64_t) -> std::optional<json> {
			Hosting& h = hostingObligatorio(in);
			const unsigned int validos = 0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010 | 0x0020 | 0x0040 | 0x0080
				| 0x0100 | 0x0200 | 0x0400 | 0x1000 | 0x2000 | 0x4000 | 0x8000;
			const unsigned int mascara = static_cast<unsigned int>(entero(d, "mascara", 0, 0xFFFF)) & validos;
			Config::cfg.input.lockedGamepadButtons = mascara;
			Config::cfg.input.lockedGamepadLeftTrigger = booleano(d, "lt");
			Config::cfg.input.lockedGamepadRightTrigger = booleano(d, "rt");
			Config::cfg.input.lockedGamepadLX = booleano(d, "lx");
			Config::cfg.input.lockedGamepadLY = booleano(d, "ly");
			Config::cfg.input.lockedGamepadRX = booleano(d, "rx");
			Config::cfg.input.lockedGamepadRY = booleano(d, "ry");
			h._lockedGamepad.wButtons = mascara;
			h._lockedGamepad.bLeftTrigger = Config::cfg.input.lockedGamepadLeftTrigger;
			h._lockedGamepad.bRightTrigger = Config::cfg.input.lockedGamepadRightTrigger;
			h._lockedGamepad.sThumbLX = Config::cfg.input.lockedGamepadLX;
			h._lockedGamepad.sThumbLY = Config::cfg.input.lockedGamepadLY;
			h._lockedGamepad.sThumbRX = Config::cfg.input.lockedGamepadRX;
			h._lockedGamepad.sThumbRY = Config::cfg.input.lockedGamepadRY;
			reaplicarArbitro(in);   // con el partido en juego, Start/Back/Guía siguen bloqueados
			Config::cfg.Save();
			return json::object();
		});

		// ---- Árbitro del partido: preferencias (se guardan en phoenix-ui.json) ---------
		p.registrar("mandos.competitivo", [&in](const json& d, uint64_t) -> std::optional<json> {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.competitivo = booleano(d, "si");
			pr.guardar();
			return json::object();   // el árbitro lo aplica o lo quita en su siguiente vuelta (≤ 0,5 s)
		});
		p.registrar("mandos.pausaAuto", [&in](const json& d, uint64_t) -> std::optional<json> {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.pausaAuto = booleano(d, "si");
			pr.guardar();
			if (!pr.pausaAuto) in.arbitro.avisados.clear();
			return json::object();
		});
		p.registrar("mandos.soloAutorizados", [](const json& d, uint64_t) -> std::optional<json> {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.soloAutorizados = booleano(d, "si");
			pr.guardar();
			return json::object();   // el árbitro lo pasa a PhoenixRoles en su siguiente vuelta (≤ 0,5 s)
		});
		p.registrar("mandos.marcadorAuto", [&in](const json& d, uint64_t) -> std::optional<json> {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			pr.marcadorAuto = booleano(d, "si");
			pr.guardar();
			return json::object();
		});

		// ---- Teclado: mapa de teclas por perfil (se guarda en cada cambio) ----------
		p.registrar("teclado.asignar", [&in](const json& d, uint64_t) -> std::optional<json> {
			KeyboardMap& km = hostingObligatorio(in).getGamepadClient().getKeyMap();
			const uint32_t usuario = static_cast<uint32_t>(entero(d, "userId", 0, 2147483647));
			const std::string boton = texto(d, "boton", 16);
			if (!km.isValidButtonName(boton)) throw ErrorAccion("DATOS_INVALIDOS", "Botón desconocido.");
			const int tecla = entero(d, "tecla", 0, 0x1FF);
			if (tecla != 0) {
				bool permitida = false;
				for (uint16_t k : _allowedSettionKeys) if (k == tecla) { permitida = true; break; }
				if (!permitida) throw ErrorAccion("TECLA_NO_PERMITIDA", "Esa tecla no se puede usar.");
			}
			const std::string nombre = d.contains("nombre") && d["nombre"].is_string() && !d["nombre"].get<std::string>().empty()
				? texto(d, "nombre", 64) : std::string("Invitado ") + std::to_string(usuario);
			if (!km.mapButton(nombre, usuario, boton, static_cast<uint16_t>(tecla))) throw ErrorAccion("SIN_PERFIL", "No se pudo guardar la tecla.");
			return json::object();
		});
		p.registrar("teclado.crear", [&in](const json& d, uint64_t) -> std::optional<json> {
			KeyboardMap& km = hostingObligatorio(in).getGamepadClient().getKeyMap();
			const uint32_t usuario = static_cast<uint32_t>(entero(d, "userId", 1, 2147483647));
			km.createProfile(texto(d, "nombre", 64), usuario);   // si ya existe no hace nada
			return json::object();
		});
		p.registrar("teclado.reiniciar", [&in](const json& d, uint64_t) -> std::optional<json> {
			KeyboardMap& km = hostingObligatorio(in).getGamepadClient().getKeyMap();
			if (!km.resetProfile(static_cast<uint32_t>(entero(d, "userId", 0, 2147483647)))) throw ErrorAccion("SIN_PERFIL", "Ese perfil no existe.");
			return json::object();
		});
		p.registrar("teclado.borrar", [&in](const json& d, uint64_t) -> std::optional<json> {
			KeyboardMap& km = hostingObligatorio(in).getGamepadClient().getKeyMap();
			const uint32_t usuario = static_cast<uint32_t>(entero(d, "userId", 0, 2147483647));
			if (usuario == 0) throw ErrorAccion("NO_PERMITIDO", "El perfil predeterminado no se puede borrar.");
			if (!km.deleteProfile(usuario)) throw ErrorAccion("SIN_PERFIL", "Ese perfil no existe.");
			return json::object();
		});

		// ---- Marionetas: mando maestro y títeres (mismos pasos del panel original) ----
		p.registrar("marionetas.motor", [&in](const json& d, uint64_t) -> std::optional<json> {
			clienteMandos(in);
			MasterOfPuppets& mp = MasterOfPuppets::instance;
			const bool sdl = booleano(d, "sdl");
			if (mp.isSDLEngine == sdl) return json::object();
			mp.isSDLEngine = sdl;
			if (sdl) mp.fetchSDLGamepads();
			// los numeros de mando cambian de motor: se suelta el maestro para no dejar uno equivocado
			mp.setMasterIndex(-1);
			GamepadClient::instance.isPuppetMaster = false;
			return json::object();
		});
		p.registrar("marionetas.actualizar", [&in](const json&, uint64_t) -> std::optional<json> {
			clienteMandos(in);
			if (MasterOfPuppets::instance.isSDLEngine) MasterOfPuppets::instance.fetchSDLGamepads();
			return json::object();
		});
		p.registrar("marionetas.maestro", [&in](const json& d, uint64_t) -> std::optional<json> {
			GamepadClient& gc = clienteMandos(in);
			MasterOfPuppets& mp = MasterOfPuppets::instance;
			const int i = entero(d, "indice", -1, 15);
			if (i >= 0) {
				std::lock_guard<std::mutex> cierre(mp.inputMutex);
				const size_t total = mp.isSDLEngine ? mp.getSDLGamepads().size() : mp.getXInputGamepads().size();
				if (static_cast<size_t>(i) >= total) throw ErrorAccion("MANDO_NO_ENCONTRADO", "Ese mando ya no está conectado.");
			}
			mp.setMasterIndex(i == mp.getMasterIndex() ? -1 : i);
			gc.isPuppetMaster = mp.getMasterIndex() >= 0;
			if (!gc.isPuppetMaster) {
				for (AGamepad* g : gc.gamepads) if (g != nullptr && g->isPuppet) g->clearState();
			}
			return json::object();
		});
		p.registrar("marionetas.titere", [&in](const json& d, uint64_t) -> std::optional<json> {
			GamepadClient& gc = clienteMandos(in);
			const size_t i = static_cast<size_t>(entero(d, "indice", 0, 15));
			if (i >= gc.gamepads.size() || gc.gamepads[i] == nullptr) throw ErrorAccion("SIN_MANDOS", "Ese mando virtual no existe.");
			std::lock_guard<std::mutex> cierre(MasterOfPuppets::instance.inputMutex);
			gc.gamepads[i]->isPuppet = booleano(d, "si");
			if (!gc.gamepads[i]->isPuppet) gc.gamepads[i]->clearState();
			return json::object();
		});
		p.registrar("marionetas.tipo", [&in](const json& d, uint64_t) -> std::optional<json> {
			clienteMandos(in);
			MasterOfPuppets& mp = MasterOfPuppets::instance;
			if (!mp.isSDLEngine) return json::object();
			std::lock_guard<std::mutex> cierre(mp.inputMutex);
			std::vector<SDLGamepad>& v = mp.getSDLGamepads();
			const size_t i = static_cast<size_t>(entero(d, "indice", 0, 15));
			if (i >= v.size()) throw ErrorAccion("MANDO_NO_ENCONTRADO", "Ese mando ya no está conectado.");
			v[i].cycleType();
			return json::object();
		});

		p.registrar("mandos.cantidad", [&in](const json& d, uint64_t) -> std::optional<json> {
			GamepadClient& gc = clienteMandos(in);
			// PES admite como máximo 8 mandos en total (mismo tope que el panel original)
			int xbox = entero(d, "xbox", 0, 8);
			int ds4 = entero(d, "ds4", 0, 8);
			if (xbox + ds4 > 8) throw ErrorAccion("FUERA_DE_RANGO", "Máximo 8 mandos en total.");
			if (xbox + ds4 < 1) throw ErrorAccion("FUERA_DE_RANGO", "Deja al menos un mando.");
			Config::cfg.input.xboxPuppetCount = static_cast<unsigned int>(xbox);
			Config::cfg.input.ds4PuppetCount = static_cast<unsigned int>(ds4);
			gc.resize(static_cast<size_t>(xbox), static_cast<size_t>(ds4));
			Config::cfg.Save();
			PhoenixPrefs& pr = PhoenixPrefs::get();
			const int total = xbox + ds4;
			if (pr.mandosActivos > total) {
				pr.mandosActivos = (std::max)(2, total);
				pr.equipoLocal = (std::max)(1, (std::min)(pr.equipoLocal, pr.mandosActivos - 1));
				pr.guardar();
			}
			return json::object();
		});

		// ---- Solicitudes (!cambio / !equipo) y entrada por Parsec ---------------
		p.registrar("solicitud.aceptar", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (!AccionesSala::aceptarSolicitud(salaObligatoria(in), idParsec(d))) throw ErrorAccion("NO_EXISTE", "La solicitud ya venció.");
			return json::object();
		});
		p.registrar("solicitud.rechazar", [](const json& d, uint64_t) -> std::optional<json> {
			AccionesSala::rechazarSolicitud(idParsec(d));
			return json::object();
		});
		p.registrar("espera.decidir", [&in](const json& d, uint64_t) -> std::optional<json> {
			const std::string como = texto(d, "como", 16);
			DecisionEspera dec = DecisionEspera::Espectador;
			if (como == "jugador") dec = DecisionEspera::Jugador;
			else if (como == "expulsar") dec = DecisionEspera::Expulsar;
			else if (como != "espectador") throw ErrorAccion("DATOS_INVALIDOS", "Decisión desconocida.");
			AccionesSala::decidirEspera(salaObligatoria(in), idParsec(d), dec);
			return json::object();
		});

		// ---- Turnos (hotseat) ----------------------------------------------------
		p.registrar("turnos.activar", [](const json& d, uint64_t) -> std::optional<json> {
			Config::cfg.hotseat.enabled = booleano(d, "si"); // como la casilla «Hotseat»
			Config::cfg.Save();
			return json::object();
		});
		p.registrar("turnos.ajustes", [](const json& d, uint64_t) -> std::optional<json> {
			// Mismos rangos y efectos que la pestaña Settings del panel Hotseat
			if (d.contains("juegoMin")) {
				const int previo = Config::cfg.hotseat.playTime;
				Config::cfg.hotseat.playTime = entero(d, "juegoMin", 1, 999);
				Config::cfg.Save();
				Hotseat::instance.applySettingsDelta(Config::cfg.hotseat.playTime - previo, 0, Config::cfg.hotseat.reminderInterval);
			}
			if (d.contains("reinicioMin")) {
				const int previo = Config::cfg.hotseat.resetTime;
				Config::cfg.hotseat.resetTime = entero(d, "reinicioMin", Config::cfg.hotseat.playTime, 999);
				Config::cfg.Save();
				Hotseat::instance.applySettingsDelta(0, Config::cfg.hotseat.resetTime - previo, Config::cfg.hotseat.reminderInterval);
			}
			if (d.contains("recordatorioMin")) {
				Config::cfg.hotseat.reminderInterval = entero(d, "recordatorioMin", 0, (std::max)(0, Config::cfg.hotseat.playTime - 1));
				Config::cfg.Save();
				Hotseat::instance.applySettingsDelta(0, 0, Config::cfg.hotseat.reminderInterval);
			}
			return json::object();
		});
	}

}
