#include "LadosAuto.h"

#include <set>

#include "InterfazWebInterno.h"
#include "../../Hosting.h"
#include "../PhoenixPrefs.h"
#include "../core/DeteccionLados.h"
#include "../core/LadoPantalla.h"
#include "../core/MandoHost.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"

namespace phoenix::web {

	namespace detalle_lados_auto {
		constexpr double kVentanaSeg = 300.0;     // se mira como mucho 5 min desde el pitazo (luego el lector se apaga)
		constexpr int kAsientoAnfitrion = 9;      // mando físico del anfitrión (sin puesto: juega directo en PES)

		struct EstadoLados {
			int64_t partido = 0;                  // inicioMs del partido medido (0 = aún antes del pitazo)
			double desde = 0.0;                   // ahoraSeg del pitazo
			lados::Detector det;
			lados::LadoDefendido ld;
			bool resuelto = false;
			const char* estado = "";
			int anclaGol = -1, anclaSaque = -1;
			double porteriaT = -1e9; int porteriaMarco = -1;   // último balón dentro de una portería y quién marcó
			int golesVistos = -1;
			std::string ultimaPantalla;
		};

		EstadoLados& estadoLados() {
			static EstadoLados e;
			return e;
		}

		void reiniciar() { estadoLados() = EstadoLados(); }

		bool enJuego(Interno& in) {
			const FasePartido f = in.partido.fase();
			return f == FasePartido::EnJuego || f == FasePartido::Pausado;
		}

		/// ID Parsec de quien juega con ese mando (0 = nadie conocido).
		uint32_t idDeAsiento(Interno& in, int asiento) {
			try {
				uint32_t idHost = 0;
				if (in.ctx.hosting != nullptr) { Guest& g = in.ctx.hosting->getHost(); if (g.isValid()) idHost = g.userID; }
				if (asiento == kAsientoAnfitrion) return idHost;
				const std::vector<AsientoVista> v = in.ctx.sala->asientos(8);
				const size_t k = static_cast<size_t>(asiento - 1);
				if (k < v.size() && v[k].ocupado && v[k].parsecId != 0) return v[k].parsecId;
				if (asiento - 1 == MandoHost::activo() - 1) return idHost;   // el anfitrión sentado con su mando de Link
			}
			catch (...) {}
			return 0;
		}

		/// 1 = Link lo tiene en el lado A, 0 = en el B, -1 = no juega este partido.
		int ladoEnLink(Interno& in, uint32_t id) {
			if (id == 0) return -1;
			for (const JugadorPartido& j : in.partido.ladoA().jugadores) if (j.parsecId == id) return 1;
			for (const JugadorPartido& j : in.partido.ladoB().jugadores) if (j.parsecId == id) return 0;
			return -1;
		}
	}

	using namespace detalle_lados_auto;

	// ---- Pantalla de mandos de PES (antes del pitazo): lado de cada casilla = puesto ----------------------
	namespace detalle_lados_auto {
		ladopantalla::Estabilizador& estabilizador() {
			static ladopantalla::Estabilizador e;
			return e;
		}
		// Al pitazo la pantalla de mandos ya no está: se recuerda el último lado estable que se leyó (kRecuerdoSeg)
		constexpr double kRecuerdoSeg = 300.0;
		char recuerdo[ladopantalla::kCasillas] = { '?', '?', '?', '?', '?', '?', '?', '?' };
		double tRecuerdo = -1e9;

		void pantallaTick(double ahora) {
			if (!PhoenixPrefs::get().ladosAuto) return;
			ladopantalla::Lectura l;
			if (ladopantalla::leer(l)) estabilizador().alimentar(ahora, l);   // sin lector (o lectura dudosa) no hace nada
			if (estabilizador().vivo(ahora)) {
				for (int i = 0; i < ladopantalla::kCasillas; i++) recuerdo[i] = estabilizador().lado(i, ahora);
				tRecuerdo = ahora;
			}
		}
	}

	char ladoPantallaPuesto(int i) {
		if (!PhoenixPrefs::get().ladosAuto || i < 0 || i >= ladopantalla::kCasillas) return '?';
		const double ahora = ahoraSeg();
		if (estabilizador().vivo(ahora)) return estabilizador().lado(i, ahora);
		return ahora - tRecuerdo <= kRecuerdoSeg ? recuerdo[i] : '?';
	}

	bool ladoPantallaVivo() { return PhoenixPrefs::get().ladosAuto && estabilizador().vivo(ahoraSeg()); }

	bool ladosQuiere(Interno& in) {
		try {
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			if (!pr.ladosAuto || in.ctx.sala == nullptr || !in.ctx.sala->abierta()) return false;
			const EstadoLados& e = estadoLados();
			// Saliendo al campo: el lector tiene que estar listo para ver el saque inicial (reparte los equipos)
			const juego::PantallaJuego pj = PhoenixLink::instancia().pantallaJuego();
			if (pj.valida && pj.pantalla == "saliendo" && !enJuego(in)) return true;
			if (!enJuego(in) || e.resuelto || in.partido.correccionesLados() > 0) return false;
			return e.partido == 0 || ahoraSeg() - e.desde < kVentanaSeg;
		}
		catch (...) { return false; }
	}

	namespace {
		lados::Detector& detectorControl() { static lados::Detector d; return d; }
		std::vector<std::pair<int, int>>& controlGuardado() { static std::vector<std::pair<int, int>> v; return v; }
		int64_t& controlPartido() { static int64_t p = -1; return p; }
	}

	const std::vector<std::pair<int, int>>& controlActual() { return controlGuardado(); }

	void controlFoto(Interno& in, const phoenix::campo::FotoCampo& fc, double ahora) {
		try {
			if (in.ctx.sala == nullptr || fc.estado != phoenix::campo::EstadoCampo::Activo || !fc.equiposListos || fc.fase != "vivo") { controlGuardado().clear(); return; }
			if (controlPartido() != in.partido.inicioMs()) { controlPartido() = in.partido.inicioMs(); detectorControl().reiniciar(); controlGuardado().clear(); }
			std::vector<lados::Ficha> fichas;
			fichas.reserve(fc.j.size());
			for (const phoenix::campo::Pieza& p : fc.j) fichas.push_back({ p.x, p.z, p.eq });
			std::vector<lados::Stick> sticks;
			for (const AsientoVista& a : in.ctx.sala->asientos(8))
				if (a.conectado) sticks.push_back({ a.numero, a.ejeLX / 32767.0f, a.ejeLY / 32767.0f });
			int16_t lx = 0, ly = 0;
			if (in.ctx.sala->hayMandoPropio() && in.ctx.sala->ejesMandoPropio(lx, ly)) sticks.push_back({ kAsientoAnfitrion, lx / 32767.0f, ly / 32767.0f });
			if (sticks.empty()) { controlGuardado().clear(); return; }
			detectorControl().alimentar(ahora, fichas, sticks);
			controlGuardado() = detectorControl().controlados(fichas);
		}
		catch (...) { controlGuardado().clear(); }
	}

	void ladosFoto(Interno& in, const phoenix::campo::FotoCampo& fc, double ahora) {
		try {
			EstadoLados& e = estadoLados();
			if (e.resuelto || in.ctx.sala == nullptr || fc.estado != phoenix::campo::EstadoCampo::Activo) return;
			std::vector<lados::Ficha> fichas;
			fichas.reserve(fc.j.size());
			for (const phoenix::campo::Pieza& p : fc.j) fichas.push_back({ p.x, p.z, p.eq });
			if (!fc.equiposListos) return;   // sin saque de centro visto, las fichitas no tienen equipo
			std::vector<lados::Stick> sticks;
			for (const AsientoVista& a : in.ctx.sala->asientos(8))
				if (a.conectado) sticks.push_back({ a.numero, a.ejeLX / 32767.0f, a.ejeLY / 32767.0f });
			int16_t lx = 0, ly = 0;
			if (in.ctx.sala->hayMandoPropio() && in.ctx.sala->ejesMandoPropio(lx, ly)) sticks.push_back({ kAsientoAnfitrion, lx / 32767.0f, ly / 32767.0f });
			e.det.alimentar(ahora, fichas, sticks);
			e.ld.alimentar(ahora, fichas);
			if (fc.hayBola && ahora - e.porteriaT > 5.0) {
				const int lado = lados::porteriaDelGol(fc.bx, fc.by, fc.bz);
				if (lado != 0) { e.porteriaT = ahora; e.porteriaMarco = lados::equipoQueMarco(lado, e.ld); }
			}
		}
		catch (...) {}
	}

	void ladosRevisar(Interno& in, const juego::EstadoPartidoJuego& j, double ahora) {
		try {
			EstadoLados& e = estadoLados();
			pantallaTick(ahora);   // lado de cada mando en la pantalla de mandos (antes del pitazo), si hay lector
			if (!PhoenixPrefs::get().ladosAuto) { if (e.partido != 0 || e.golesVistos >= 0) reiniciar(); return; }
			// Partido nuevo: al salir al campo, o si cambia el partido de Link
			const juego::PantallaJuego pj = PhoenixLink::instancia().pantallaJuego();
			const std::string pantalla = pj.valida ? pj.pantalla : std::string();
			if (pantalla == "saliendo" && e.ultimaPantalla != "saliendo" && !enJuego(in)) reiniciar();
			estadoLados().ultimaPantalla = pantalla;
			if (!enJuego(in)) return;
			if (e.partido != in.partido.inicioMs()) {
				if (e.partido != 0) { reiniciar(); estadoLados().ultimaPantalla = pantalla; }
				e.partido = in.partido.inicioMs();
				e.desde = ahora;
			}
			if (e.resuelto) return;
			if (in.partido.correccionesLados() > 0) { e.resuelto = true; e.estado = ""; return; }   // ya lo corrigió alguien a mano
			if (ahora - e.desde >= kVentanaSeg) { e.resuelto = true; e.estado = "dudas"; return; }
			e.estado = "midiendo";

			// Anclas A/B → local. Saque: en el 1.er tiempo el local defiende x < 0 (lo hace PES). Gol: marcó el que no defiende esa portería.
			if (j.valido && j.periodo == 1) {
				const int a = e.ld.lado(0), b = e.ld.lado(1);
				if (a != 0 && a == -b) e.anclaSaque = a < 0 ? 0 : 1;
			}
			if (j.valido) {
				const int n = static_cast<int>(j.goles.size());
				if (e.golesVistos < 0) e.golesVistos = n;
				else if (n > e.golesVistos) {
					if (ahora - e.porteriaT < 20.0 && e.porteriaMarco >= 0)
						e.anclaGol = j.goles.back().local ? e.porteriaMarco : 1 - e.porteriaMarco;
					e.golesVistos = n;
				}
			}
			const int eqLocal = lados::equipoLocal(e.anclaGol, e.anclaSaque);
			if (eqLocal < 0) return;

			// Jugadores detectados (con veredicto firme) y si Link los tiene de local
			std::vector<lados::Participante> ps;
			for (const lados::Veredicto& v : e.det.veredictos()) {
				if (!v.decidido) continue;
				const int enLink = ladoEnLink(in, idDeAsiento(in, v.asiento));
				if (enLink < 0) continue;
				ps.push_back({ v.eq, (enLink == 1) != in.partido.mandosInvertidos() });
			}
			const lados::Decision d = lados::decidir(ps, eqLocal);
			if (d == lados::Decision::Nada) return;
			e.resuelto = true;
			if (d == lados::Decision::Coinciden) { in.partido.origenLados("juego"); e.estado = "coinciden"; return; }
			if (!in.partido.corregirLados()) { e.estado = "dudas"; return; }
			in.partido.origenLados("juego");
			in.arbitro.golesJuegoL = in.arbitro.golesJuegoV = -1;   // el marcador se vuelve a leer del juego con los lados nuevos
			e.estado = "corregidos";
			const bool inv = in.partido.mandosInvertidos();
			const LadoPartido& local = inv ? in.partido.ladoB() : in.partido.ladoA();
			const LadoPartido& visita = inv ? in.partido.ladoA() : in.partido.ladoB();
			mensajeDelBot(in, "Link vio en el juego los lados: " + local.nombre + " es el local y " + visita.nombre + " la visita (corregido solo).");
		}
		catch (...) {}
	}

	const char* ladosEstado() { return estadoLados().estado; }

}
