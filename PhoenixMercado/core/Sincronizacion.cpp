#include "Sincronizacion.h"

#include <algorithm>
#include <nlohmann/json.hpp>

namespace mercado::sinc {

	namespace {
		constexpr int kPrimeraSeleccion = 528;   // en la Liga Máster los bloques 528+ son selecciones (ESTRUCTURA-ML.md §2)

		uint32_t u32Opcional(const nlohmann::json& j, const char* campo) {
			if (!j.contains(campo) || j[campo].is_null()) return 0;
			if (j[campo].is_number_unsigned() || j[campo].is_number_integer()) { const auto v = j[campo].get<int64_t>(); return v > 0 && v < 0x7fffffff ? uint32_t(v) : 0; }
			return 0;
		}
		std::string textoOpcional(const nlohmann::json& j, const char* campo) {
			return j.contains(campo) && j[campo].is_string() ? j[campo].get<std::string>() : std::string();
		}
	}

	Resultado<ListaCambios> parsearCambios(const std::string& contenido) {
		using R = Resultado<ListaCambios>;
		nlohmann::json j = nlohmann::json::parse(contenido, nullptr, false);
		if (j.is_discarded() || !j.is_object()) return R::mal("CAMBIOS_INVALIDOS", "El contenido no es un objeto JSON");
		if (!j.contains("cambios") || !j["cambios"].is_array()) return R::mal("CAMBIOS_INVALIDOS", "Falta la lista «cambios»");
		ListaCambios l;
		l.liga = textoOpcional(j, "liga");
		l.desde = j.contains("desde") && j["desde"].is_number() ? j["desde"].get<int64_t>() : 0;
		l.versionActual = j.contains("version_actual") && j["version_actual"].is_number() ? j["version_actual"].get<int64_t>() : 0;
		int64_t anterior = l.desde;
		for (const auto& c : j["cambios"]) {
			if (!c.is_object() || !c.contains("version") || !c["version"].is_number()) return R::mal("CAMBIOS_INVALIDOS", "Un cambio no tiene «version»");
			Cambio x;
			x.version = c["version"].get<int64_t>();
			if (x.version <= anterior) return R::mal("CAMBIOS_INVALIDOS", "Las versiones no vienen en orden creciente (" + std::to_string(x.version) + " tras " + std::to_string(anterior) + ")");
			anterior = x.version;
			x.phoenixId = c.contains("phoenix_id") && c["phoenix_id"].is_number() ? c["phoenix_id"].get<int64_t>() : 0;
			x.pesId = u32Opcional(c, "pes_id");
			x.clubDesde = textoOpcional(c, "club_desde");
			x.clubHacia = textoOpcional(c, "club_hacia");
			x.clubDesdePes = u32Opcional(c, "club_desde_pes");
			x.clubHaciaPes = u32Opcional(c, "club_hacia_pes");
			x.tipo = textoOpcional(c, "tipo");
			x.fecha = textoOpcional(c, "fecha");
			l.cambios.push_back(std::move(x));
		}
		if (l.versionActual < anterior) l.versionActual = anterior;
		return R::bien(std::move(l));
	}

	std::string Informe::texto() const {
		std::string t = "versión aplicada " + std::to_string(versionAplicada) + ": option file " + std::to_string(aplicadosOption)
			+ ", liga máster " + std::to_string(aplicadosLM) + ", pendientes LM " + std::to_string(pendientesLM) + "\n";
		for (const auto& l : lineas) {
			t += "  v" + std::to_string(l.version) + " " + l.texto + " [" + (l.optionFile ? "option " : "") + (l.ligaMaster ? "LM" : "") + "]";
			if (!l.pendienteLM.empty()) t += " · LM pendiente: " + l.pendienteLM;
			t += "\n";
		}
		return t;
	}

	std::string cuerpoAplicado(int64_t version, const std::string& huellaPlantillas, const std::string& liga) {
		nlohmann::json j = { {"version", version}, {"huella_plantillas", huellaPlantillas} };
		if (!liga.empty()) j["liga"] = liga;
		return j.dump();
	}

	Resultado<Informe> aplicarCambios(const ListaCambios& lista, int64_t versionYaAplicada, Alcance alcance,
		OptionFile* option, lm::GuardadoLM* liga,
		const std::function<int(uint32_t)>& posicionDe, const std::function<std::string(uint32_t)>& nombreDe) {
		using R = Resultado<Informe>;
		const bool conOption = alcance.optionFile && option;
		const bool conLM = alcance.ligaMaster && liga;
		if (!conOption && !conLM) return R::mal("SIN_DESTINO", "No hay option file ni Liga Máster a los que aplicar los cambios");

		// Copias de trabajo: solo al final, si todo salió bien, se adoptan (todo o nada).
		std::optional<OptionFile> of;
		std::optional<lm::GuardadoLM> ml;
		if (conOption) of = *option;
		if (conLM) ml = *liga;

		Informe inf;
		inf.versionAplicada = versionYaAplicada;
		auto nombre = [&](uint32_t pid) { return nombreDe ? nombreDe(pid) : "jugador " + std::to_string(pid); };
		auto errorEn = [&](const Cambio& c, const std::string& donde, const Error& e) {
			return R::mal(e.codigo, "v" + std::to_string(c.version) + " " + nombre(c.pesId) + " (" + donde + "): " + e.detalle);
		};

		for (const Cambio& c : lista.cambios) {
			if (c.version <= versionYaAplicada) continue;
			LineaInforme ln;
			ln.version = c.version; ln.pesId = c.pesId;
			ln.texto = nombre(c.pesId) + ": " + (c.clubDesdePes ? c.clubDesde : std::string("libre")) + " → " + (c.clubHaciaPes ? c.clubHacia : std::string("libre"));
			if (!c.pesId) {
				ln.texto += " (sin pes_id: no afecta al juego)";
				inf.lineas.push_back(ln);
				inf.versionAplicada = c.version;
				continue;
			}

			// ---- Option file ------------------------------------------------------
			if (of) {
				if (c.clubHaciaPes == 0) {
					if (c.clubDesdePes == 0) return errorEn(c, "option file", { "CAMBIO_INVALIDO", "sin club de origen ni de destino" });
					auto s = of->sugerirSustituto(c.clubDesdePes, c.pesId, posicionDe);
					if (!s.ok()) return errorEn(c, "option file", s.error);
					auto q = of->quitar(c.pesId, c.clubDesdePes, *s.valor);
					if (!q.ok()) return errorEn(c, "option file", q.error);
				}
				else {
					uint32_t sustituto = 0;
					if (c.clubDesdePes) {
						auto s = of->sugerirSustituto(c.clubDesdePes, c.pesId, posicionDe);
						if (!s.ok()) return errorEn(c, "option file", s.error);
						sustituto = *s.valor;
					}
					auto m = of->mover(c.pesId, c.clubHaciaPes, c.clubDesdePes, 0, sustituto);
					if (!m.ok()) return errorEn(c, "option file", m.error);
				}
				ln.optionFile = true;
				inf.aplicadosOption++;
			}

			// ---- Liga Máster --------------------------------------------------------
			if (ml) {
				std::string pendiente;
				if (c.clubHaciaPes == 0) pendiente = "dejar libre a un jugador en la Liga Máster todavía no se hace";
				else if (c.clubDesdePes == 0) pendiente = "fichar a un agente libre en la Liga Máster todavía no se hace";
				else {
					auto kD = ml->indicePorIdOption(c.clubHaciaPes);
					if (!kD.ok()) pendiente = "el club de destino (" + c.clubHacia + ") no está en esta Liga Máster";
					else {
						// Dónde está el jugador en ESTA carrera (la IA mueve gente por su cuenta): se parte de donde esté.
						int kO = -1;
						bool yaEsta = false;
						for (int k : ml->equiposDe(c.pesId)) {
							if (k >= kPrimeraSeleccion) continue;
							if (k == *kD.valor) yaEsta = true; else if (kO < 0) kO = k;
						}
						if (yaEsta) { ln.ligaMaster = true; inf.aplicadosLM++; ln.texto += " (ya estaba en el destino en la Liga Máster)"; }
						else if (kO < 0) pendiente = "el jugador no está en ningún club de esta Liga Máster";
						else if (ml->esEquipoUsuario(*kD.valor)) pendiente = "fichar PARA el equipo del usuario todavía no se hace";
						else {
							auto s = ml->sugerirSustituto(kO, c.pesId, posicionDe);
							if (!s.ok()) return errorEn(c, "liga máster", s.error);
							Resultado<uint16_t> m = ml->esEquipoUsuario(kO)
								? ml->moverUsuarioAIA(kO, *kD.valor, c.pesId, 0, *s.valor)
								: ml->moverEntreIA(kO, *kD.valor, c.pesId, 0, *s.valor);
							if (!m.ok()) return errorEn(c, "liga máster", m.error);
							ln.ligaMaster = true; inf.aplicadosLM++;
							auto eo = ml->equipo(kO);
							if (eo.ok() && c.clubDesdePes) {
								auto idO = ml->idOptionDe(kO);
								if (idO.ok() && *idO.valor != c.clubDesdePes) ln.texto += " (en la Liga Máster estaba en " + eo.valor->nombre + ")";
							}
						}
					}
				}
				if (!pendiente.empty()) { ln.pendienteLM = pendiente; inf.pendientesLM++; }
			}
			inf.lineas.push_back(ln);
			inf.versionAplicada = c.version;
		}
		if (lista.versionActual > inf.versionAplicada && lista.cambios.empty()) inf.versionAplicada = lista.versionActual;

		if (of) *option = std::move(*of);
		if (ml) *liga = std::move(*ml);
		return R::bien(std::move(inf));
	}

}
