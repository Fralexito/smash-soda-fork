#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../core/Config.h"
#include "../../widgets/HostSettingsWidget.h"
#include "../PhoenixPrefs.h"
#include "../I18n.h"
#include "../ui/Shell.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"

// =============================================================================
//  Acciones de la interfaz web: aplicación (tema, idioma, interfaz) y Sala.
//  Cada acción usa el mismo camino que su control original.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_sala {
		/// Opciones de sala que cambian por los métodos phoenix de HostSettingsWidget.
		void aplicarOpciones(Interno& in, const json& d) {
			if (in.ctx.ajustesSala == nullptr) throw ErrorAccion("SIN_SALA");
			HostSettingsWidget& w = *in.ctx.ajustesSala;
			if (d.contains("nombre")) w.phoenixCambiarNombre(texto(d, "nombre", 50));
			if (d.contains("plazas")) w.phoenixCambiarPlazas(entero(d, "plazas", 0, 20));
			if (d.contains("limitador") || d.contains("limite")) {
				std::string nombre, biblioteca;
				int plazas = 0, limite = 0;
				bool limitador = false, pendiente = false;
				w.phoenixLeerOpciones(nombre, plazas, limitador, limite, biblioteca, pendiente);
				if (d.contains("limitador")) limitador = booleano(d, "limitador");
				if (d.contains("limite")) limite = entero(d, "limite", 0, 64);
				w.phoenixCambiarLimitador(limitador, limite);
			}
			if (d.contains("biblioteca")) w.phoenixElegirBiblioteca(texto(d, "biblioteca", 255));
			if (d.contains("quiosco") && !w.phoenixCambiarQuiosco(booleano(d, "quiosco"))) {
				throw ErrorAccion("QUIOSCO_SIN_JUEGO", "El modo quiosco necesita un juego de la biblioteca.");
			}
			if (d.contains("overlay")) w.phoenixCambiarOverlay(booleano(d, "overlay"));
			if (d.contains("turnos")) {
				// Igual que la casilla «Hotseat» de la configuración de sala
				Config::cfg.hotseat.enabled = booleano(d, "turnos");
				Config::cfg.Save();
			}
		}

		void aplicarPhoenix(const json& d) {
			PhoenixPrefs& pr = PhoenixPrefs::get();
			if (d.contains("visibilidad")) {
				const std::string v = texto(d, "visibilidad", 16);
				if (v != "publica" && v != "amigos" && v != "privada") throw ErrorAccion("DATOS_INVALIDOS", "Visibilidad desconocida.");
				pr.visibilidad = v;
			}
			if (d.contains("espectadores")) pr.espectadores = booleano(d, "espectadores");
			if (d.contains("limiteEspectadores")) pr.limiteEspectadores = entero(d, "limiteEspectadores", 1, 16);
			if (d.contains("entradaParsec")) pr.entradaParsec = booleano(d, "entradaParsec");
			if (d.contains("juego")) pr.juego = texto(d, "juego", 60);
			if (d.contains("parche")) pr.parche = texto(d, "parche", 60);
			if (d.contains("region")) pr.region = texto(d, "region", 40);
			pr.guardar();
		}
	}

	using namespace detalle_sala;

	void aplicarFormacion(Interno& in, int local, int visitante) {
		int maximo = 8;
		if (in.ctx.sala != nullptr) maximo = (std::max)(2, (std::min)(8, static_cast<int>(in.ctx.sala->asientos(8).size())));
		if (local < 1 || visitante < 1 || local + visitante > maximo) {
			throw ErrorAccion("FUERA_DE_RANGO", "Cada equipo necesita al menos un mando y en total máximo " + std::to_string(maximo) + ".");
		}
		PhoenixPrefs& pr = PhoenixPrefs::get();
		pr.equipoLocal = local;
		pr.mandosActivos = local + visitante;
		pr.guardar();
	}

	// =========================================================================
	json valoresPerfilSala(Interno& in) {
		const PhoenixPrefs& pr = PhoenixPrefs::get();
		json v = {
			{"turnos", Config::cfg.hotseat.enabled},
			{"overlay", Config::cfg.overlay.enabled},
			{"visibilidad", pr.visibilidad}, {"espectadores", pr.espectadores},
			{"limiteEspectadores", pr.limiteEspectadores}, {"entradaParsec", pr.entradaParsec},
			{"juego", pr.juego}, {"parche", pr.parche}, {"region", pr.region},
			{"local", pr.equipoLocal}, {"visitante", pr.mandosActivos - pr.equipoLocal},
			{"fps", Config::cfg.video.fps}, {"mbps", Config::cfg.video.bandwidth},
		};
		if (in.ctx.ajustesSala != nullptr) {
			std::string nombre, biblioteca;
			int plazas = 0, limite = 0;
			bool limitador = false, pendiente = false;
			in.ctx.ajustesSala->phoenixLeerOpciones(nombre, plazas, limitador, limite, biblioteca, pendiente);
			v["plazas"] = plazas;
			v["limitador"] = limitador;
			v["limite"] = limite;
			v["nombre"] = nombre;
		}
		return v;
	}

	void aplicarPerfilSala(Interno& in, const json& v) {
		// Solo claves conocidas; cada una por su camino normal. Lo que falte se deja como está.
		json opciones = json::object();
		for (const char* k : { "nombre", "plazas", "limitador", "limite", "turnos", "overlay" }) {
			if (v.contains(k)) opciones[k] = v[k];
		}
		if (!opciones.empty()) aplicarOpciones(in, opciones);
		json phoenix = json::object();
		for (const char* k : { "visibilidad", "espectadores", "limiteEspectadores", "entradaParsec", "juego", "parche", "region" }) {
			if (v.contains(k)) phoenix[k] = v[k];
		}
		if (!phoenix.empty()) aplicarPhoenix(phoenix);
		if (v.contains("local") && v.contains("visitante")) {
			try { aplicarFormacion(in, entero(v, "local", 1, 7), entero(v, "visitante", 1, 7)); }
			catch (const ErrorAccion&) {} // formación imposible con los mandos de hoy: se ignora
		}
		if (v.contains("fps") && v.contains("mbps") && in.ctx.sala != nullptr) {
			in.ctx.sala->aplicarVideo(entero(v, "fps", 10, 250), entero(v, "mbps", 1, 1000));
		}
	}

	// =========================================================================
	void registrarAccionesApp(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("ui.seccion", [&in](const json& d, uint64_t) -> std::optional<json> {
			in.seccion = texto(d, "seccion", 24);
			in.pestana = d.contains("pestana") && d["pestana"].is_string() ? texto(d, "pestana", 24) : std::string();
			return json::object();
		});

		p.registrar("ui.tema", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string t = texto(d, "tema", 16);
			if (t != "galaxy" && t != "sudario") throw ErrorAccion("DATOS_INVALIDOS", "Tema desconocido.");
			PhoenixPrefs::get().tema = t;
			PhoenixPrefs::get().guardar();
			return json::object();
		});

		p.registrar("ui.idioma", [](const json& d, uint64_t) -> std::optional<json> {
			const std::string codigo = texto(d, "idioma", 8);
			bool existe = false;
			for (const auto& par : I18n::disponibles()) if (par.first == codigo) existe = true;
			if (!existe) throw ErrorAccion("DATOS_INVALIDOS", "Idioma no disponible.");
			PhoenixPrefs::get().idioma = codigo;
			PhoenixPrefs::get().guardar();
			I18n::establecer(codigo); // la interfaz ImGui también cambia
			return json::object();
		});

		p.registrar("ui.interfaz", [&in](const json& d, uint64_t) -> std::optional<json> {
			const std::string modo = texto(d, "modo", 16);
			if (modo != "phoenix" && modo != "clasica") throw ErrorAccion("DATOS_INVALIDOS", "Interfaz desconocida.");
			in.cambioInterfaz = modo; // se aplica en el próximo frame
			return json::object();
		});

		p.registrar("ui.panelClasico", [&in](const json& d, uint64_t) -> std::optional<json> {
			const int seccion = entero(d, "seccion", 0, 3);
			const int pestana = d.contains("pestana") ? entero(d, "pestana", 0, 10) : 0;
			PhoenixPrefs::get().interfazPhoenix = true;
			Shell::irA(seccion, pestana);
			in.panelClasico = true;
			return json::object();
		});

		p.registrar("ui.copiar", [](const json& d, uint64_t) -> std::optional<json> {
			if (!copiarAlPortapapeles(texto(d, "texto", 4000))) throw ErrorAccion("PORTAPAPELES", "No se pudo copiar.");
			return json::object();
		});

		p.registrar("ui.abrir", [](const json& d, uint64_t) -> std::optional<json> {
			abrirEnlace(texto(d, "url", 500));
			return json::object();
		});

		p.registrar("ui.recargar", [&in](const json&, uint64_t) -> std::optional<json> {
			in.recargarPendiente = true;
			return json::object();
		});
	}

	// =========================================================================
	void registrarAccionesSala(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("sala.abrir", [&in](const json&, uint64_t) -> std::optional<json> {
			ProveedorSala& sala = salaObligatoria(in);
			// Prueba «Solo PES 2021» (nivel estricto): sin PES abierto no se abre la sala
			if (PhoenixPrefs::get().modoPes == "estricto" && phoenix::buzon::detectarJuego().puente == phoenix::buzon::Puente::JuegoCerrado)
				throw ErrorAccion("PES_CERRADO", Tr(std::string("Abre PES 2021 primero: tienes activado «Solo PES 2021» (SYNC).")));
			std::string error;
			if (!sala.abrir(error)) throw ErrorAccion("NO_SE_PUDO_ABRIR", Tr(error.empty() ? std::string("No se pudo abrir la sala.") : error));
			return json::object();
		});

		p.registrar("sala.cerrar", [&in](const json&, uint64_t) -> std::optional<json> {
			salaObligatoria(in).cerrar();
			return json::object();
		});

		p.registrar("sala.copiarEnlace", [&in](const json&, uint64_t) -> std::optional<json> {
			ProveedorSala& sala = salaObligatoria(in);
			if (!sala.abierta()) throw ErrorAccion("SALA_CERRADA", "Abre la sala para tener enlace.");
			const std::string enlace = sala.enlace();
			if (enlace.empty() || !copiarAlPortapapeles(enlace)) throw ErrorAccion("PORTAPAPELES", "No se pudo copiar el enlace.");
			return json{ {"enlace", enlace} };
		});

		p.registrar("sala.opciones", [&in](const json& d, uint64_t) -> std::optional<json> {
			aplicarOpciones(in, d);
			std::string nombre, biblioteca;
			int plazas = 0, limite = 0;
			bool limitador = false, pendiente = false;
			in.ctx.ajustesSala->phoenixLeerOpciones(nombre, plazas, limitador, limite, biblioteca, pendiente);
			return json{ {"pendiente", pendiente} };
		});

		p.registrar("sala.aplicar", [&in](const json&, uint64_t) -> std::optional<json> {
			if (in.ctx.ajustesSala == nullptr) throw ErrorAccion("SIN_SALA");
			std::string error;
			if (!in.ctx.ajustesSala->phoenixAplicarCambios(error)) throw ErrorAccion("NO_SE_PUDO_APLICAR", Tr(error));
			return json::object();
		});

		p.registrar("sala.phoenix", [](const json& d, uint64_t) -> std::optional<json> {
			aplicarPhoenix(d);
			return json::object();
		});

		p.registrar("sala.calidad", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).aplicarVideo(entero(d, "fps", 10, 250), entero(d, "mbps", 1, 1000));
			return json::object();
		});

		// ---- Vínculo con la web ------------------------------------------------
		p.registrar("web.vincular", [](const json& d, uint64_t) -> std::optional<json> {
			std::string codigo;
			for (char c : texto(d, "codigo", 20)) if (c >= '0' && c <= '9') codigo += c;
			if (codigo.size() != 6) throw ErrorAccion("CODIGO_INVALIDO", "El código tiene 6 dígitos.");
			PhoenixLink::instancia().emparejar(codigo);
			return json::object();
		});
		p.registrar("web.reintentar", [](const json&, uint64_t) -> std::optional<json> {
			PhoenixLink::instancia().reintentar();
			return json::object();
		});
		p.registrar("web.desvincular", [](const json&, uint64_t) -> std::optional<json> {
			PhoenixLink::instancia().desvincular();
			return json::object();
		});
		// Chat general (contrato §27)
		p.registrar("chatGlobal.enviar", [&in](const json& d, uint64_t id) -> std::optional<json> {
			const std::string x = texto(d, "texto", 400);
			if (x.empty()) throw ErrorAccion("DATOS_INVALIDOS", "Escribe un mensaje.");
			const uint64_t ticket = in.siguienteTicket++;
			in.pendientesWeb[ticket] = id;
			PhoenixLink::instancia().enviarChatGlobal(ticket, x);
			return std::nullopt; // responde cuando contesta la web
		});
		p.registrar("chatGlobal.abierto", [](const json& d, uint64_t) -> std::optional<json> {
			PhoenixLink::instancia().chatGlobalAbierto(d.contains("abierto") && d["abierto"].is_boolean() && d["abierto"].get<bool>());
			return json::object();
		});
		p.registrar("web.soltarRival", [&in](const json&, uint64_t id) -> std::optional<json> {
			const uint64_t ticket = in.siguienteTicket++;
			in.pendientesWeb[ticket] = id;
			PhoenixLink::instancia().soltarRival(ticket);
			return std::nullopt; // responde cuando contesta la web
		});

		// ---- Perfiles de sala --------------------------------------------------
		p.registrar("perfilesSala.lista", [&in](const json&, uint64_t) -> std::optional<json> {
			json lista = json::array();
			for (const PerfilSala& ps : in.perfilesSala->lista()) lista.push_back({ {"nombre", ps.nombre}, {"valores", ps.valores} });
			return json{ {"perfiles", lista}, {"actual", valoresPerfilSala(in)} };
		});
		p.registrar("perfilesSala.guardar", [&in](const json& d, uint64_t) -> std::optional<json> {
			const std::string nombre = PerfilesSala::normalizar(texto(d, "nombre", 80));
			if (nombre.empty()) throw ErrorAccion("DATOS_INVALIDOS", "Ponle un nombre al perfil.");
			if (!in.perfilesSala->guardar(nombre, valoresPerfilSala(in))) {
				throw ErrorAccion("NO_SE_PUDO_GUARDAR", "No se pudo guardar (máximo 20 perfiles).");
			}
			return json{ {"nombre", nombre} };
		});
		p.registrar("perfilesSala.aplicar", [&in](const json& d, uint64_t) -> std::optional<json> {
			auto ps = in.perfilesSala->obtener(texto(d, "nombre", 80));
			if (!ps) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
			aplicarPerfilSala(in, ps->valores);
			return json{ {"nombre", ps->nombre} };
		});
		p.registrar("perfilesSala.borrar", [&in](const json& d, uint64_t) -> std::optional<json> {
			if (!in.perfilesSala->borrar(texto(d, "nombre", 80))) throw ErrorAccion("NO_EXISTE", "Ese perfil ya no existe.");
			return json::object();
		});
	}

}
