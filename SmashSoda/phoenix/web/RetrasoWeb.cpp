#include "InterfazWebInterno.h"

#include <cmath>

#include "../../Hosting.h"
#include "../core/ProveedorSala.h"
#include "../host/RetrasoMando.h"

// =============================================================================
//  Phoenix Link · Medidor de retraso del mando (lado de la interfaz)
// -----------------------------------------------------------------------------
//  Una vez por segundo junta lo medido en el hilo de entrada (cómo llegan los
//  mensajes del mando) con lo que da Parsec (ping, codificar, decodificar,
//  paquetes reenviados), lo evalúa y lo deja listo para el estado («retraso»).
//  Durante un partido avisa en el chat si alguien sigue en rojo (con antispam).
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;
	namespace rt = phoenix::retraso;

	void tickRetraso(Interno& in) {
		try {
			ProveedorSala& sala = *in.ctx.sala;
			Hosting& h = *in.ctx.hosting;
			const double ahora = ahoraSeg();
			if (ahora - in.ultimoRetraso < 1.0) return;
			in.ultimoRetraso = ahora;

			if (!sala.abierta()) {
				if (!in.retrasos.empty()) {
					in.retrasos.clear();
					in.avisosRetraso.limpiar();
					rt::pedirLimpieza();
				}
				return;
			}

			const std::vector<rt::MedidaMando> medidas = rt::instantanea();
			const uint32_t idHost = h.getHost().userID;
			const bool enPartido = in.partido.fase() == FasePartido::EnJuego;
			std::map<uint32_t, FilaRetraso> nuevas;
			for (Guest& g : h.getGuests()) {
				if (g.userID == idHost || g.fake) continue;
				FilaRetraso f;
				f.nombre = g.name;
				for (const rt::MedidaMando& m : medidas) if (m.userId == g.userID) { f.m = m; f.hayMando = true; }
				const MyMetrics mm = h.getMetrics(g.id);
				const ParsecMetrics& pm = mm.metrics;
				f.p.pingMs = pm.networkLatency > 0.0f ? static_cast<int>(pm.networkLatency + 0.5f) : -1;
				f.p.codificarMs = pm.encodeLatency > 0.0f ? pm.encodeLatency : 0.0;
				f.p.decodificarMs = pm.decodeLatency > 0.0f ? pm.decodeLatency : 0.0;
				f.p.perdidaPct = in.perdidaRetraso.registrar(g.userID, pm.packetsSent, pm.fastRTs + pm.slowRTs);
				f.colaFrames = static_cast<int>(pm.queuedFrames);
				f.e = rt::evaluar(f.hayMando ? &f.m : nullptr, f.p);
				nuevas[g.userID] = f;
			}
			for (const auto& par : in.retrasos) {
				if (!nuevas.count(par.first)) in.perdidaRetraso.olvidar(par.first);
			}
			in.retrasos.swap(nuevas);

			// Aviso en el chat: solo jugadores del partido, rojo ≥ 10 s seguidos (antispam en AvisosRetraso)
			for (const LadoPartido* lado : { &in.partido.ladoA(), &in.partido.ladoB() }) {
				for (const JugadorPartido& jp : lado->jugadores) {
					auto it = in.retrasos.find(jp.parsecId);
					if (it == in.retrasos.end()) continue;
					const FilaRetraso& f = it->second;
					if (in.avisosRetraso.actualizar(jp.parsecId, f.e.semaforo, enPartido, ahora)) {
						std::string t = "Aviso de retraso: " + (jp.nombre.empty() ? f.nombre : jp.nombre) + " va con retraso";
						if (f.e.totalMs >= 0) t += " (~" + std::to_string(f.e.totalMs) + " ms";
						else t += " (";
						if (!f.e.motivo.empty()) t += (f.e.totalMs >= 0 ? ", " : "") + f.e.motivo;
						t += "). El anfitrión lo ve en Sala > Conexión.";
						mensajeDelBot(in, t);
					}
				}
			}
		}
		catch (...) {
			// El medidor nunca debe tumbar la interfaz.
		}
	}

	json estadoRetraso(Interno& in) {
		json lista = json::array();
		auto redondo = [](double v) { return v < 0.0 ? -1.0 : std::round(v * 10.0) / 10.0; };
		for (const auto& par : in.retrasos) {
			const FilaRetraso& f = par.second;
			json x = {
				{"parsecId", par.first}, {"nombre", f.nombre},
				{"total", f.e.totalMs}, {"semaforo", f.e.semaforo}, {"motivo", f.e.motivo},
				{"ping", f.p.pingMs}, {"codificar", redondo(f.p.codificarMs)}, {"decodificar", redondo(f.p.decodificarMs)},
				{"perdida", redondo(f.p.perdidaPct)}, {"cola", f.colaFrames},
			};
			if (f.hayMando) {
				x["mando"] = {
					{"muestras", f.m.muestras}, {"media", redondo(f.m.mediaMs)}, {"p95", redondo(f.m.p95Ms)},
					{"jitter", redondo(f.m.jitterMs)}, {"saltos", f.m.saltosMin}, {"ritmo", f.m.ritmo},
					{"haceMs", f.m.ultimoHaceMs < 0.0 ? -1 : static_cast<int>(f.m.ultimoHaceMs)},
				};
			}
			lista.push_back(x);
		}
		return lista;
	}

}
