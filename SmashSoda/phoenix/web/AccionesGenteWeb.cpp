#include "InterfazWebInterno.h"

#include <algorithm>

#include "../../Hosting.h"
#include "../../core/Cache.h"
#include "../../core/Config.h"
#include "../core/ProveedorSala.h"
#include "../link/PhoenixLink.h"

// =============================================================================
//  Acciones de la interfaz web: gente en la sala, moderación (listas de
//  baneados, moderadores, VIP e historial), chat y amigos de la web.
// =============================================================================

namespace phoenix::web {

	using json = nlohmann::json;

	namespace detalle_gente {
		/// Nombre de un invitado presente (o el que mandó la interfaz si ya se fue).
		std::string nombreDe(Interno& in, uint32_t id, const json& d) {
			try {
				for (Guest& g : hostingObligatorio(in).getGuests()) if (g.userID == id) return g.name;
				for (GuestData& g : hostingObligatorio(in).getGuestHistory()) if (g.userID == id) return g.name;
			}
			catch (const ErrorAccion&) { throw; }
			catch (...) {}
			return d.contains("nombre") && d["nombre"].is_string() ? texto(d, "nombre", 60) : std::string("#" + std::to_string(id));
		}

		json lista(const std::vector<GuestData>& v) {
			json arr = json::array();
			for (const GuestData& g : v) arr.push_back({ {"parsecId", g.userID}, {"nombre", g.name}, {"motivo", g.reason} });
			return arr;
		}

		bool esUuid(const std::string& s) {
			if (s.size() != 36) return false;
			for (size_t i = 0; i < s.size(); i++) {
				const char c = s[i];
				if (i == 8 || i == 13 || i == 18 || i == 23) { if (c != '-') return false; }
				else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
			}
			return true;
		}
	}

	using namespace detalle_gente;

	void registrarAccionesGente(Interno& in) {
		Puente& p = *in.puente;

		p.registrar("gente.mod", [&in](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = idParsec(d);
			salaObligatoria(in).alternarMod(id, nombreDe(in, id, d));
			return json{ {"mod", salaObligatoria(in).esMod(id)} };
		});
		p.registrar("gente.vip", [&in](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = idParsec(d);
			salaObligatoria(in).alternarVip(id, nombreDe(in, id, d));
			return json{ {"vip", salaObligatoria(in).esVip(id)} };
		});
		p.registrar("gente.teclado", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).permitirTeclado(idParsec(d), booleano(d, "si"));
			return json::object();
		});
		p.registrar("gente.raton", [&in](const json& d, uint64_t) -> std::optional<json> {
			salaObligatoria(in).permitirRaton(idParsec(d), booleano(d, "si"));
			return json::object();
		});
		p.registrar("gente.expulsar", [&in](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = idParsec(d);
			salaObligatoria(in).expulsar(id);
			PhoenixLink::instancia().evento("expulsion", std::to_string(id), json{ {"motivo", "manual"} }.dump());
			return json::object();
		});
		p.registrar("gente.banear", [&in](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = idParsec(d);
			if (Cache::cache.isSodaCop(id)) throw ErrorAccion("NO_BANEABLE", "Es policía de Soda: no se puede banear.");
			if (!salaObligatoria(in).banear(id, nombreDe(in, id, d))) throw ErrorAccion("NO_SE_PUDO", "No se pudo banear.");
			PhoenixLink::instancia().evento("expulsion", std::to_string(id), json{ {"motivo", "ban"} }.dump());
			return json::object();
		});

		// ---- Moderación ---------------------------------------------------------
		p.registrar("moderacion.listas", [&in](const json&, uint64_t) -> std::optional<json> {
			json historial = json::array();
			try {
				const std::vector<GuestData>& h = hostingObligatorio(in).getGuestHistory();
				// Más reciente primero
				for (auto it = h.rbegin(); it != h.rend(); ++it) historial.push_back({ {"parsecId", it->userID}, {"nombre", it->name} });
			}
			catch (const ErrorAccion&) { throw; }
			catch (...) {}
			return json{
				{"baneados", lista(Cache::cache.banList.getGuests())},
				{"mods", lista(Cache::cache.modList.getGuests())},
				{"vips", lista(Cache::cache.vipList.getGuests())},
				{"historial", historial},
			};
		});
		p.registrar("moderacion.desbanear", [](const json& d, uint64_t) -> std::optional<json> {
			if (!Cache::cache.banList.unban(idParsec(d))) throw ErrorAccion("NO_EXISTE", "No estaba baneado.");
			return json::object();
		});
		p.registrar("moderacion.quitarMod", [](const json& d, uint64_t) -> std::optional<json> {
			if (!Cache::cache.modList.unmod(idParsec(d))) throw ErrorAccion("NO_EXISTE", "No era moderador.");
			return json::object();
		});
		p.registrar("moderacion.quitarVip", [](const json& d, uint64_t) -> std::optional<json> {
			if (!Cache::cache.vipList.unVIP(idParsec(d))) throw ErrorAccion("NO_EXISTE", "No era VIP.");
			return json::object();
		});
		p.registrar("moderacion.banear", [&in](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = idParsec(d);
			if (Cache::cache.isSodaCop(id)) throw ErrorAccion("NO_BANEABLE", "Es policía de Soda: no se puede banear.");
			if (!salaObligatoria(in).banear(id, nombreDe(in, id, d))) throw ErrorAccion("NO_SE_PUDO", "No se pudo banear.");
			return json::object();
		});
		p.registrar("moderacion.motivo", [](const json& d, uint64_t) -> std::optional<json> {
			const uint32_t id = idParsec(d);
			const std::string motivo = texto(d, "motivo", 200);
			for (GuestData& g : Cache::cache.banList.getGuests()) {
				if (g.userID == id) {
					g.reason = motivo; // como «Add reason» del panel original
					Cache::cache.banList.SaveToFile();
					return json::object();
				}
			}
			throw ErrorAccion("NO_EXISTE", "No está en la lista de baneados.");
		});

		// ---- Chat -----------------------------------------------------------------
		p.registrar("chat.enviar", [&in](const json& d, uint64_t) -> std::optional<json> {
			std::string t = texto(d, "texto", 1000);
			while (!t.empty() && (t.back() == ' ' || t.back() == '\n' || t.back() == '\r')) t.pop_back();
			if (t.empty()) throw ErrorAccion("DATOS_INVALIDOS", "Mensaje vacío.");
			hostingObligatorio(in).sendHostMessage(t.c_str()); // como el botón Enviar del chat
			return json::object();
		});

		// ---- Amigos de la web ------------------------------------------------------
		p.registrar("amigos.invitar", [&in](const json& d, uint64_t id) -> std::optional<json> {
			const std::string usuario = texto(d, "usuarioId", 40);
			if (!esUuid(usuario)) throw ErrorAccion("DATOS_INVALIDOS", "Amigo inválido.");
			if (in.ctx.sala == nullptr || !in.ctx.sala->abierta()) throw ErrorAccion("SALA_CERRADA", "Abre tu sala para invitar.");
			const uint64_t ticket = in.siguienteTicket++;
			in.pendientesWeb[ticket] = id;
			PhoenixLink::instancia().invitar(ticket, usuario);
			return std::nullopt; // responde cuando contesta la web
		});
	}

}
