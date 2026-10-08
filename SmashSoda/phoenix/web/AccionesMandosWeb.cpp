#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../core/Config.h"
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
			else if (que == "bloquearBotones") h.toggleGamepadLockButtons();
			else throw ErrorAccion("DATOS_INVALIDOS", "Herramienta desconocida.");
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
