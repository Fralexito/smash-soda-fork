#include "SalaClasificatoria.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "InterfazWebInterno.h"
#include "../../Hosting.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_clasificatoria {
		constexpr double kAvisoCadaSeg = 60.0;   // el mismo aviso a la misma persona, como mucho 1 vez por minuto

		struct EstadoClas {
			bool cargado = false;
			bool activa = false;
			double ultimoTick = 0.0;
			std::map<uint32_t, double> avisado;          // parsecId → último aviso (ahoraSeg)
			int64_t partidoAvisado = 0;                  // partido del que ya se avisó «no puntuará»
		};

		EstadoClas& estadoClas() {
			static EstadoClas e;
			return e;
		}

		std::string rutaClas() { return rutaConfig("phoenix-clasificatoria.json"); }

		void cargarClas(EstadoClas& e) {
			if (e.cargado) return;
			e.cargado = true;
			try {
				std::ifstream f(rutaClas(), std::ios::binary);
				if (!f) return;
				std::stringstream ss;
				ss << f.rdbuf();
				const json j = json::parse(ss.str(), nullptr, false);
				if (j.is_object()) e.activa = j.value("activa", false);
			}
			catch (...) {}
		}

		void guardarClas(const EstadoClas& e) {
			try {
				const std::string temporal = rutaClas() + ".tmp";
				{
					std::ofstream f(temporal, std::ios::binary | std::ios::trunc);
					if (!f) return;
					f << json{ {"version", 1}, {"activa", e.activa} }.dump(1, '\t');
				}
				std::error_code ec;
				std::filesystem::rename(temporal, rutaClas(), ec);
				if (ec) std::filesystem::remove(temporal, ec);
			}
			catch (...) {}
		}

		uint32_t idHostClas(Interno& in) {
			try { if (in.ctx.hosting != nullptr) return in.ctx.hosting->getHost().userID; } catch (...) {}
			return 0;
		}
	}

	using namespace detalle_clasificatoria;

	bool clasificatoriaActiva() {
		EstadoClas& e = estadoClas();
		cargarClas(e);
		return e.activa;
	}

	void fijarClasificatoria(bool si) {
		EstadoClas& e = estadoClas();
		cargarClas(e);
		if (si && !PhoenixLink::instancia().pcVinculada())
			throw ErrorAccion("PC_SIN_VINCULAR", "Vincula esta PC con la web de Phoenix para usar la sala clasificatoria.");
		if (e.activa == si) return;
		e.activa = si;
		e.avisado.clear();
		guardarClas(e);
	}

	void tickClasificatoria(Interno& in) {
		EstadoClas& e = estadoClas();
		cargarClas(e);
		if (!e.activa || in.ctx.sala == nullptr) return;
		const double ahora = ahoraSeg();
		if (ahora - e.ultimoTick < 1.0) return;
		e.ultimoTick = ahora;
		try {
			ProveedorSala& sala = *in.ctx.sala;
			PhoenixLink& pl = PhoenixLink::instancia();
			if (!sala.abierta() || !pl.pcVinculada()) return;
			const uint32_t idHost = idHostClas(in);

			// Partido ya empezado: no se saca a nadie (se rompería el partido), solo se avisa una vez
			const FasePartido fase = in.partido.fase();
			const bool enPartido = fase == FasePartido::EnJuego || fase == FasePartido::Pausado;
			std::set<uint32_t> delPartido;
			if (enPartido) {
				std::string sinCuenta;
				for (const LadoPartido* l : { &in.partido.ladoA(), &in.partido.ladoB() })
					for (const JugadorPartido& j : l->jugadores) {
						delPartido.insert(j.parsecId);
						if (j.parsecId != idHost && pl.cuentaWeb(j.parsecId) == 0) sinCuenta += (sinCuenta.empty() ? "" : ", ") + j.nombre;
					}
				if (!sinCuenta.empty() && e.partidoAvisado != in.partido.inicioMs()) {
					e.partidoAvisado = in.partido.inicioMs();
					mensajeDelBot(in, "Sala clasificatoria: " + sinCuenta + " no tiene su Parsec vinculado a la web. Este partido no puntuará.");
				}
			}

			const std::vector<AsientoVista> asientos = sala.asientos(8);
			for (size_t i = 0; i < asientos.size(); i++) {
				const AsientoVista& s = asientos[i];
				if (!s.ocupado || s.parsecId == 0 || s.parsecId == idHost) continue;
				if (enPartido && delPartido.count(s.parsecId)) continue;
				if (pl.cuentaWeb(s.parsecId) != 0) continue;   // vinculado o aún sin saber: juega
				sala.liberarMando(static_cast<int>(i));         // vuelve a «Mirando» y no retoma el mando solo
				auto it = e.avisado.find(s.parsecId);
				if (it == e.avisado.end() || ahora - it->second > kAvisoCadaSeg) {
					e.avisado[s.parsecId] = ahora;
					mensajeDelBot(in, s.jugador + ": esta sala es clasificatoria. Para jugar, vincula tu Parsec a tu cuenta en la web de Phoenix y vuelve a pedir puesto.");
				}
			}
		}
		catch (...) {
			// La sala clasificatoria nunca debe tumbar la interfaz.
		}
	}

	json estadoClasificatoria(Interno& in) {
		PhoenixLink& pl = PhoenixLink::instancia();
		json sinCuenta = json::array(), sinComprobar = json::array();
		try {
			if (in.ctx.hosting != nullptr) {
				for (Guest& g : in.ctx.hosting->getGuests()) {
					const int c = pl.cuentaWeb(g.userID);
					if (c == 0) sinCuenta.push_back({ {"parsecId", g.userID}, {"nombre", g.name} });
					else if (c < 0) sinComprobar.push_back({ {"parsecId", g.userID}, {"nombre", g.name} });
				}
			}
		}
		catch (...) {}
		return { {"activa", clasificatoriaActiva()}, {"pcVinculada", pl.pcVinculada()},
			{"sinCuenta", sinCuenta}, {"sinComprobar", sinComprobar} };
	}

	void comprobarLadosClasificatoria(Interno& in, const LadoPartido& a, const LadoPartido& b) {
		if (!clasificatoriaActiva()) return;
		PhoenixLink& pl = PhoenixLink::instancia();
		if (!pl.pcVinculada()) return;
		const uint32_t idHost = idHostClas(in);
		for (const LadoPartido* l : { &a, &b })
			for (const JugadorPartido& j : l->jugadores)
				if (j.parsecId != idHost && pl.cuentaWeb(j.parsecId) == 0)
					throw ErrorAccion("SIN_CUENTA", j.nombre + " no tiene su Parsec vinculado a la web: en una sala clasificatoria no puede jugar.");
	}

}
