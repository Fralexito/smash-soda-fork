#include "CambiosLM.h"
#include "Sincronizacion.h"

#include <set>

namespace mercado::lmweb {

	namespace {
		using json = nlohmann::json;
		constexpr int kPrimeraSeleccion = 528;          // bloques 528+ = selecciones (ESTRUCTURA-ML §2)
		constexpr double kMaxEuros = 40e9;              // como GuardadoLM::fijarFinanzas

		const std::set<std::string>& tipos() {
			static const std::set<std::string> t = { "traspaso", "ia_ia", "fichaje", "venta", "dinero", "valor", "stat", "deshacer" };
			return t;
		}
		/// «traspaso» = lo que manda la web (127): el motor decide en cada partida si es fichaje, venta o IA↔IA.
		bool esTraspaso(const std::string& t) { return t == "traspaso" || t == "ia_ia" || t == "fichaje" || t == "venta"; }

		uint32_t u32De(const json& j, const char* c) {
			if (!j.contains(c) || !j[c].is_number_integer()) return 0;
			const auto v = j[c].get<int64_t>();
			return v > 0 && v < 0x7fffffff ? uint32_t(v) : 0;
		}
		/// Euros opcionales: -1 = no viene (o null); -2 = inválido.
		long long eurosDe(const json& j, const char* c) {
			if (!j.contains(c) || j[c].is_null()) return -1;
			if (!j[c].is_number()) return -2;
			const double v = j[c].get<double>();
			if (v < 100 || v > kMaxEuros || v != double((long long)v) || (long long)v % 100) return -2;
			return (long long)v;
		}

		/// Un traspaso de la web en el formato de Sincronizacion.h (para usar el motor ya probado en el juego).
		Resultado<sinc::ListaCambios> comoCambioLiga(const json& d, int64_t version, uint32_t pid, uint32_t desdePes, uint32_t haciaPes) {
			json c = d;
			c["version"] = version;
			c["pes_id"] = pid;
			c["club_desde_pes"] = desdePes;
			c["club_hacia_pes"] = haciaPes;
			for (const char* q : { "club_desde", "club_hacia" }) if (!c.contains(q) || !c[q].is_string()) c[q] = "";
			return sinc::parsearCambios(json{ { "desde", version - 1 }, { "cambios", json::array({ c }) } }.dump());
		}

		/// Club (bloque, sin selecciones) donde está ahora el jugador en esta partida, o -1.
		int clubActual(const lm::GuardadoLM& g, uint32_t pid) {
			for (int k : g.equiposDe(pid)) if (k < kPrimeraSeleccion) return k;
			return -1;
		}
		uint32_t idOption(const lm::GuardadoLM& g, int k) {
			if (k < 0) return 0;
			auto r = g.idOptionDe(k);
			return r.ok() ? *r.valor : 0;
		}

		ResultadoCambio rechazo(int64_t v, const std::string& motivo) { ResultadoCambio r; r.version = v; r.estado = "rechazado"; r.motivo = motivo; return r; }
		ResultadoCambio pendiente(int64_t v, const std::string& motivo) { ResultadoCambio r; r.version = v; r.estado = "pendiente"; r.motivo = motivo; return r; }
		ResultadoCambio sinEfecto(int64_t v, const std::string& motivo) { ResultadoCambio r; r.version = v; r.estado = "sin_efecto"; r.motivo = motivo; return r; }

		struct Aplicador {
			lm::GuardadoLM& g;
			EstadoPartida& est;
			const Contexto& ctx;
			int kU;

			json finanzasJson(const lm::GuardadoLM& x) const {
				if (kU < 0) return nullptr;
				auto f = x.finanzas(kU);
				if (!f.ok()) return nullptr;
				return { { "presupuesto", f.valor->presupuestoFichajes }, { "tope_salarial", f.valor->topeSalarial } };
			}

			/// Mueve `pid` de donde esté a `haciaPes` sobre la copia `x`. `d` = campos del cambio (monto, sueldo…).
			ResultadoCambio traspaso(lm::GuardadoLM& x, int64_t v, const std::string& tipo, const json& d, uint32_t pid, uint32_t desdePes, uint32_t haciaPes) {
				const int kO = clubActual(x, pid);
				if (kO < 0) return rechazo(v, "el jugador no está en ningún club de esta partida (agentes libres: todavía no)");
				const uint32_t actualPes = idOption(x, kO);
				auto kD = x.indicePorIdOption(haciaPes);
				if (!kD.ok()) return rechazo(v, "el club de destino no está en esta partida");
				if (*kD.valor == kO) return sinEfecto(v, "el jugador ya estaba en ese club");
				if (tipo == "fichaje" && *kD.valor != kU) return rechazo(v, "fichaje: el club de destino no es el tuyo en esta partida");
				if (tipo == "venta" && kO != kU) return rechazo(v, "venta: el jugador ya no está en tu club en esta partida");
				if (tipo == "ia_ia" && (kO == kU || *kD.valor == kU)) return rechazo(v, "IA↔IA: uno de los clubes es el tuyo (usa fichaje o venta)");
				ResultadoCambio r; r.version = v;
				r.antes = { { "club_pes", actualPes } };
				if (auto f = finanzasJson(x); !f.is_null()) r.antes["finanzas"] = f;
				if (desdePes && desdePes != actualPes) r.motivo = "en esta partida estaba en otro club (" + std::to_string(actualPes) + ")";
				auto lista = comoCambioLiga(d, v, pid, actualPes, haciaPes);
				if (!lista.ok()) return rechazo(v, lista.error.detalle);
				auto inf = sinc::aplicarCambios(*lista.valor, v - 1, { false, true }, nullptr, &x, ctx.posicionDe, nullptr, ctx.edadDe);
				if (!inf.ok()) return rechazo(v, inf.error.detalle);
				if (inf.valor->pendientesLM > 0 && !inf.valor->lineas.empty()) return rechazo(v, inf.valor->lineas.back().pendienteLM);
				r.estado = "aplicado";
				r.despues = { { "club_pes", haciaPes } };
				if (auto f = finanzasJson(x); !f.is_null()) r.despues["finanzas"] = f;
				return r;
			}

			ResultadoCambio uno(const CambioLM& c) {
				const int64_t v = c.version;
				const json& d = c.datos;
				lm::GuardadoLM x = g;   // copia de trabajo: todo o nada por cambio
				ResultadoCambio r;
				json diario;
				if (esTraspaso(c.tipo)) {
					const uint32_t pid = u32De(d, "pes_id");
					r = traspaso(x, v, c.tipo, d, pid, u32De(d, "club_desde_pes"), u32De(d, "club_hacia_pes"));
					if (r.estado == "aplicado") diario = { { "tipo", c.tipo }, { "pes_id", pid }, { "antes", r.antes }, { "despues", r.despues } };
				}
				else if (c.tipo == "dinero") {
					if (kU < 0) return rechazo(v, "no se encontró tu club en esta partida");
					auto f = x.finanzas(kU);
					if (!f.ok()) return rechazo(v, f.error.detalle);
					const long long p = eurosDe(d, "presupuesto"), t = eurosDe(d, "tope_salarial");
					r.version = v;
					r.antes = finanzasJson(x);
					auto n = x.fijarFinanzas(kU, p > 0 ? uint64_t(p) : 0, t > 0 ? uint64_t(t) : 0);
					if (!n.ok()) return rechazo(v, n.error.detalle);
					r.estado = "aplicado";
					r.despues = finanzasJson(x);
					diario = { { "tipo", "dinero" }, { "antes", r.antes } };
				}
				else if (c.tipo == "valor") {
					const uint32_t pid = u32De(d, "pes_id");
					auto a = x.valorMercado(pid);
					if (!a.ok()) return rechazo(v, a.error.detalle);
					auto n = x.fijarValorMercado(pid, uint64_t(eurosDe(d, "valor")));
					if (!n.ok()) return rechazo(v, n.error.detalle);
					r.version = v; r.estado = "aplicado";
					r.antes = { { "valor", *a.valor } };
					r.despues = { { "valor", *n.valor } };
					diario = { { "tipo", "valor" }, { "pes_id", pid }, { "antes", r.antes } };
				}
				else if (c.tipo == "stat") {
					if (!ctx.aplicarStat) return pendiente(v, "el motor de estadísticas de Liga Máster todavía no está conectado");
					auto s = ctx.aplicarStat(x, c);
					if (!s.ok()) return rechazo(v, s.error.detalle);
					r.version = v; r.estado = "aplicado";
					r.antes = s.valor->value("antes", json::object());
					r.despues = s.valor->value("despues", json::object());
					diario = { { "tipo", "stat" }, { "datos", d }, { "antes", r.antes } };
				}
				else if (c.tipo == "deshacer") {
					const std::string clave = std::to_string(d.value("deshace", int64_t(0)));
					if (!est.diario.contains(clave)) return sinEfecto(v, "esa versión no se aplicó en esta partida");
					json& e = est.diario[clave];
					if (e.contains("deshecho")) return sinEfecto(v, "ya estaba deshecha");
					const std::string t = e.value("tipo", "");
					const json& an = e["antes"];
					if (esTraspaso(t)) {
						const uint32_t pid = e.value("pes_id", 0u), vuelta = an.value("club_pes", 0u);
						if (!vuelta) return rechazo(v, "el jugador no tenía club antes (dejar libre: todavía no)");
						const int kNow = clubActual(x, pid);
						const std::string tipoVuelta = kNow == kU ? "venta" : (x.indicePorIdOption(vuelta).ok() && *x.indicePorIdOption(vuelta).valor == kU ? "fichaje" : "ia_ia");
						r = traspaso(x, v, tipoVuelta, json::object(), pid, 0, vuelta);
						if (r.estado != "aplicado") return r;
						// El dinero que movió aquel traspaso vuelve a su sitio (solo lo que cambió entonces, no lo de después).
						if (kU >= 0 && an.contains("finanzas") && e.contains("despues") && e["despues"].contains("finanzas")) {
							const long long gastado = an["finanzas"].value("presupuesto", 0ll) - e["despues"]["finanzas"].value("presupuesto", 0ll);
							auto f = x.finanzas(kU);
							if (gastado != 0 && f.ok()) {
								const long long nuevo = (long long)f.valor->presupuestoFichajes + gastado;
								if (nuevo >= 100) {
									auto n = x.fijarFinanzas(kU, uint64_t(nuevo), 0);
									if (!n.ok()) return rechazo(v, n.error.detalle);
								}
							}
							r.despues["finanzas"] = finanzasJson(x);
						}
					}
					else if (t == "dinero") {
						if (kU < 0) return rechazo(v, "no se encontró tu club en esta partida");
						r.version = v;
						r.antes = finanzasJson(x);
						auto n = x.fijarFinanzas(kU, an.value("presupuesto", uint64_t(0)), an.value("tope_salarial", uint64_t(0)));
						if (!n.ok()) return rechazo(v, n.error.detalle);
						r.despues = finanzasJson(x);
					}
					else if (t == "valor") {
						const uint32_t pid = e.value("pes_id", 0u);
						auto a = x.valorMercado(pid);
						if (!a.ok()) return rechazo(v, a.error.detalle);
						auto n = x.fijarValorMercado(pid, an.value("valor", uint64_t(0)));
						if (!n.ok()) return rechazo(v, n.error.detalle);
						r.version = v; r.antes = { { "valor", *a.valor } }; r.despues = { { "valor", *n.valor } };
					}
					else if (t == "stat") {
						if (!ctx.aplicarStat) return pendiente(v, "el motor de estadísticas de Liga Máster todavía no está conectado");
						CambioLM inv; inv.version = v; inv.tipo = "deshacer"; inv.datos = e;
						auto s = ctx.aplicarStat(x, inv);
						if (!s.ok()) return rechazo(v, s.error.detalle);
						r.version = v;
						r.antes = s.valor->value("antes", json::object());
						r.despues = s.valor->value("despues", json::object());
					}
					else return rechazo(v, "tipo de cambio desconocido en el diario");
					r.estado = "aplicado";
					r.antes["deshace"] = d["deshace"];
					g = std::move(x);
					e["deshecho"] = v;
					return r;
				}
				else return rechazo(v, "tipo desconocido: " + c.tipo);

				if (r.estado == "aplicado") {
					g = std::move(x);
					if (!diario.is_null()) est.diario[std::to_string(v)] = diario;
				}
				return r;
			}
		};
	}

	Resultado<ListaLM> parsear(const std::string& contenido) {
		using R = Resultado<ListaLM>;
		json j = json::parse(contenido, nullptr, false);
		if (j.is_discarded() || !j.is_object()) return R::mal("CAMBIOS_INVALIDOS", "El contenido no es un objeto JSON");
		if (j.value("formato", "") != kFormato) return R::mal("CAMBIOS_INVALIDOS", "Formato desconocido (se esperaba " + std::string(kFormato) + ")");
		if (!j.contains("cambios") || !j["cambios"].is_array()) return R::mal("CAMBIOS_INVALIDOS", "Falta la lista «cambios»");
		ListaLM l;
		l.desde = j.value("desde", int64_t(0));
		l.versionActual = j.value("version_actual", int64_t(0));
		int64_t anterior = l.desde;
		for (const auto& c : j["cambios"]) {
			if (!c.is_object() || !c.contains("version") || !c["version"].is_number_integer()) return R::mal("CAMBIOS_INVALIDOS", "Un cambio no tiene «version»");
			CambioLM x;
			x.version = c["version"].get<int64_t>();
			const std::string v = "v" + std::to_string(x.version) + ": ";
			if (x.version <= anterior) return R::mal("CAMBIOS_INVALIDOS", v + "las versiones no vienen en orden creciente");
			anterior = x.version;
			x.tipo = c.value("tipo", "");
			if (!tipos().count(x.tipo)) return R::mal("CAMBIOS_INVALIDOS", v + "tipo desconocido «" + x.tipo + "»");
			if (c.contains("partida") && !c["partida"].is_null()) {
				if (!c["partida"].is_string() || c["partida"].get<std::string>().size() > 80) return R::mal("CAMBIOS_INVALIDOS", v + "«partida» inválida");
				x.partida = c["partida"].get<std::string>();
			}
			x.datos = c;
			if (esTraspaso(x.tipo)) {
				if (!u32De(c, "pes_id") || !u32De(c, "club_hacia_pes")) return R::mal("CAMBIOS_INVALIDOS", v + "faltan pes_id o club_hacia_pes");
				auto prueba = comoCambioLiga(c, x.version, u32De(c, "pes_id"), u32De(c, "club_desde_pes"), u32De(c, "club_hacia_pes"));
				if (!prueba.ok()) return R::mal("CAMBIOS_INVALIDOS", v + prueba.error.detalle);
			}
			else if (x.tipo == "dinero") {
				const long long p = eurosDe(c, "presupuesto"), t = eurosDe(c, "tope_salarial");
				if (p == -2 || t == -2) return R::mal("CAMBIOS_INVALIDOS", v + "importe inválido (múltiplo de 100 €, de 100 € a 40.000 millones)");
				if (p < 0 && t < 0) return R::mal("CAMBIOS_INVALIDOS", v + "dinero sin presupuesto ni tope_salarial");
			}
			else if (x.tipo == "valor") {
				if (!u32De(c, "pes_id") || eurosDe(c, "valor") < 0) return R::mal("CAMBIOS_INVALIDOS", v + "valor necesita pes_id y valor (múltiplo de 100 €)");
			}
			else if (x.tipo == "stat") {
				if (!u32De(c, "pes_id")) return R::mal("CAMBIOS_INVALIDOS", v + "stat sin pes_id");
			}
			else if (x.tipo == "deshacer") {
				if (!c.contains("deshace") || !c["deshace"].is_number_integer() || c["deshace"].get<int64_t>() < 1 || c["deshace"].get<int64_t>() >= x.version)
					return R::mal("CAMBIOS_INVALIDOS", v + "«deshace» tiene que ser una versión anterior");
			}
			l.cambios.push_back(std::move(x));
		}
		if (l.versionActual < anterior) l.versionActual = anterior;
		return R::bien(std::move(l));
	}

	nlohmann::json EstadoPartida::aJson() const {
		json p = json::array();
		for (const auto& c : pendientes) p.push_back(c.datos);
		return { { "formato", "phoenix-lm/estado-partida-1" }, { "version_aplicada", versionAplicada }, { "diario", diario }, { "pendientes", p } };
	}

	EstadoPartida EstadoPartida::desdeJson(const nlohmann::json& j) {
		EstadoPartida e;
		if (!j.is_object()) return e;
		e.versionAplicada = j.value("version_aplicada", int64_t(0));
		if (j.contains("diario") && j["diario"].is_object()) e.diario = j["diario"];
		if (j.contains("pendientes") && j["pendientes"].is_array())
			for (const auto& c : j["pendientes"]) {
				CambioLM x;
				x.version = c.value("version", int64_t(0));
				x.tipo = c.value("tipo", "");
				if (c.contains("partida") && c["partida"].is_string()) x.partida = c["partida"].get<std::string>();
				x.datos = c;
				if (x.version > 0 && tipos().count(x.tipo)) e.pendientes.push_back(std::move(x));
			}
		return e;
	}

	std::string Informe::texto() const {
		std::string t = "Liga Máster: " + std::to_string(aplicados) + " aplicados, " + std::to_string(rechazados) + " rechazados, "
			+ std::to_string(pendientes) + " pendientes\n";
		for (const auto& r : resultados) t += "  v" + std::to_string(r.version) + " " + r.estado + (r.motivo.empty() ? "" : " · " + r.motivo) + "\n";
		return t;
	}

	int equipoUsuario(const lm::GuardadoLM& g) {
		for (int k = 0; k < kPrimeraSeleccion; k++) if (g.esEquipoUsuario(k)) return k;
		return -1;
	}

	std::string idPartida(const std::string& ranura, const lm::GuardadoLM& g) {
		return ranura + ":" + std::to_string(idOption(g, equipoUsuario(g)));
	}

	Informe aplicar(const ListaLM& lista, const std::string& partidaId, lm::GuardadoLM& g, EstadoPartida& estado, const Contexto& ctx) {
		Informe inf;
		Aplicador ap{ g, estado, ctx, equipoUsuario(g) };
		auto anotar = [&](const CambioLM& c, ResultadoCambio r, std::vector<CambioLM>& siguenPendientes) {
			if (r.estado == "aplicado") { inf.aplicados++; inf.modificado = true; }
			else if (r.estado == "rechazado") inf.rechazados++;
			else if (r.estado == "pendiente") { inf.pendientes++; siguenPendientes.push_back(c); }
			inf.resultados.push_back(std::move(r));
		};
		std::vector<CambioLM> siguen;
		// 1) Lo que quedó pendiente de antes, en su orden.
		for (const auto& c : estado.pendientes) anotar(c, ap.uno(c), siguen);
		// 2) Lo nuevo que toca a esta partida.
		std::set<int64_t> yaPendientes;
		for (const auto& c : estado.pendientes) yaPendientes.insert(c.version);
		for (const auto& c : lista.cambios) {
			if (c.version <= estado.versionAplicada) continue;
			estado.versionAplicada = c.version;
			if (!c.partida.empty() && c.partida != partidaId) continue;
			if (yaPendientes.count(c.version)) continue;
			anotar(c, ap.uno(c), siguen);
		}
		if (lista.cambios.empty() && lista.desde <= estado.versionAplicada && lista.versionActual > estado.versionAplicada)
			estado.versionAplicada = lista.versionActual;
		estado.pendientes = std::move(siguen);
		return inf;
	}

}
