#include "CampoWeb.h"

#include <cmath>

#include "InterfazWebInterno.h"
#include "../PhoenixPrefs.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_campo_web {
		constexpr double kCampoWebCadaSeg = 1.0;    // envío a la web sin nadie mirando con sesión: 1 por segundo
		constexpr double kCampoWebCadaSegMirando = 0.25;   // con alguien viendo el tablero (liga_mirando): 4 por segundo
		constexpr float kCampoWebAlcance = 2.0f;    // metros: el jugador más cercano al balón a esta distancia lo «tiene»

		struct EstadoCampoWeb {
			int64_t partido = 0;                    // inicioMs del partido medido
			double ultimoEnvio = 0.0;
			long long conBalon[2] = { 0, 0 };       // lecturas con el balón en poder del equipo A / B
		};

		EstadoCampoWeb& estadoCampoWeb() {
			static EstadoCampoWeb e;
			return e;
		}

		double redondeo(double v) { return std::round(v * 10.0) / 10.0; }
	}

	using namespace detalle_campo_web;

	nlohmann::json estadisticasJson(const phoenix::campo::EstadisticasPartido& e) {
		json j;
		const double total = e.tPosesion[0] + e.tPosesion[1];
		if (total >= 2.0) { const double a = std::round(1000.0 * e.tPosesion[0] / total) / 10.0; j["pos"] = json::array({ a, std::round((100.0 - a) * 10.0) / 10.0 }); }
		auto par = [&](const char* k, const int* v) { j[k] = json::array({ v[0], v[1] }); };
		par("pas", e.pases); par("per", e.perdidas); par("rec", e.recuperaciones); par("tir", e.tiros); par("pue", e.tirosPuerta);
		par("cor", e.corners); par("ban", e.saquesBanda); par("met", e.saquesMeta);
		return j;
	}

	bool campoWebQuiere(Interno& in) {
		try {
			const PhoenixPrefs& pr = PhoenixPrefs::get();
			if (!pr.campoWeb || in.ctx.sala == nullptr || !in.ctx.sala->abierta()) return false;
			const FasePartido f = in.partido.fase();
			if (f != FasePartido::EnJuego && f != FasePartido::Pausado) return false;
			// Las mismas salas que salen en «En vivo ahora»: oficiales siempre, amistosos si la sala no es privada
			if (in.arbitro.partidoOficial <= 0 && pr.visibilidad == "privada") return false;
			return PhoenixLink::instancia().salaPublicada();
		}
		catch (...) {
			return false;
		}
	}

	void campoWebFoto(Interno& in, const phoenix::campo::FotoCampo& fc, const json& ev, double ahora) {
		try {
			EstadoCampoWeb& e = estadoCampoWeb();
			if (e.partido != in.partido.inicioMs()) {
				e = EstadoCampoWeb();
				e.partido = in.partido.inicioMs();
			}
			// Posesión: el equipo de quien lleva el balón (core/CampoVivo: Poseedor, con histéresis), solo en juego en vivo
			if (fc.fase == "vivo" && fc.equiposListos && fc.poseedor >= 0 && fc.poseedor < static_cast<int>(fc.j.size()) && in.partido.fase() == FasePartido::EnJuego) {
				const int eq = fc.j[static_cast<size_t>(fc.poseedor)].eq;
				if (eq == 0 || eq == 1) e.conBalon[eq]++;
			}
			if (ahora - e.ultimoEnvio < (in.mirandoWeb.empty() ? kCampoWebCadaSeg : kCampoWebCadaSegMirando)) return;
			e.ultimoEnvio = ahora;
			json c = ev;   // {j:[[x,z,eq]…], b:[x,z,y]|null, t?:tanda}
			c["v"] = 1;
			c["est"] = estadisticasJson(fc.est);   // estadísticas derivadas (aproximadas)
			c["fase"] = fc.fase;   // vivo | pausa | repeticion | otro (en repeticion las posiciones van congeladas)
			c["equipos"] = fc.equiposListos;   // false = aún sin colores (se ponen en el siguiente saque de centro)
			const long long total = e.conBalon[0] + e.conBalon[1];
			if (total >= 40) {   // ~2 s de lecturas con balón: antes no significa nada
				const double a = 100.0 * static_cast<double>(e.conBalon[0]) / static_cast<double>(total);
				c["posesion"] = json::array({ redondeo(a), redondeo(100.0 - a) });
			}
			PhoenixLink::instancia().ligaCampo(c.dump(-1, ' ', false, json::error_handler_t::replace));
		}
		catch (...) {
			// El tablero de la web nunca debe tumbar la interfaz.
		}
	}

}
