#include "Grupo.h"
#include "Sha256.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace mercado::grupo {

	std::string aTexto(Modo m) { return m == Modo::Automatico ? "automatico" : "autorizacion"; }

	Config parsearConfig(const json& d) {
		Config c;
		if (!d.is_object()) { c.motivoNoLeida = "respuesta sin datos"; return c; }
		const std::string modo = d.value("modo", "");
		c.modo = (modo == "automatico") ? Modo::Automatico : Modo::Autorizacion;   // cualquier otra cosa = lo seguro
		c.actualizadoPor = d.contains("actualizado_por") && d["actualizado_por"].is_string() ? d["actualizado_por"].get<std::string>() : "";
		c.actualizadoEn = d.contains("actualizado_en") && d["actualizado_en"].is_string() ? d["actualizado_en"].get<std::string>() : "";
		c.etag = d.contains("etag") && d["etag"].is_string() ? d["etag"].get<std::string>() : "";
		c.leida = (modo == "automatico" || modo == "autorizacion");
		if (!c.leida) c.motivoNoLeida = "modo desconocido: " + modo;
		return c;
	}

	std::string huellaEquipos(const OptionFile& of) {
		std::vector<std::pair<uint32_t, std::string>> v;
		for (const auto& e : of.equipos()) v.push_back({ e.id, e.nombre });
		std::sort(v.begin(), v.end());
		std::string t;
		for (const auto& [id, n] : v) t += std::to_string(id) + "|" + n + "\n";
		return sha256::deTexto(t);
	}

	std::string motivoIncompatible(const Compat& local, const Compat& remoto) {
		if (remoto.formato != 1) return "formato de cambio desconocido (" + std::to_string(remoto.formato) + "): actualiza Phoenix Sync";
		if (remoto.parche.empty() || remoto.huellaBase.empty()) return "el cambio no dice de qué parche viene";
		if (local.huellaBase.empty()) return "no se pudo leer tu option file para comparar el parche";
		if (local.huellaBase != remoto.huellaBase)
			return "tu parche es distinto al de quien lo publicó (" + remoto.parche + ")";
		return "";   // el nombre del parche puede variar (modo del switcher); manda la huella de la tabla de equipos
	}

	json aJson(const Operacion& o) {
		// Nombres del contrato publicado (v1.8.0): op_id, base_seq, parche, huella_bd. El resto son extras opcionales.
		json j = { { "op_id", o.id }, { "tipo", o.tipo }, { "jugador_id", o.jugador }, { "equipo_origen", o.equipoOrigen },
			{ "equipo_destino", o.equipoDestino }, { "dorsal", o.dorsal }, { "autor_pc", o.autorPc }, { "creado_en", o.creadoEn },
			{ "base_seq", o.baseSeq }, { "base", o.base }, { "sha_resultado", o.shaResultado }, { "resumen", o.resumen },
			{ "parche", o.compat.parche }, { "huella_bd", o.compat.huellaBase }, { "formato", o.compat.formato } };
		if (o.seq) j["seq"] = o.seq;
		if (!o.autor.empty()) j["autor"] = o.autor;
		return j;
	}

	Resultado<Operacion> deJson(const json& j) {
		using R = Resultado<Operacion>;
		try {
			Operacion o;
			auto s = [&](const char* k) { return j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : std::string(); };
			auto u = [&](const char* k) -> int64_t { return j.contains(k) && j[k].is_number_integer() ? j[k].get<int64_t>() : 0; };
			o.id = s("op_id");
			if (o.id.empty()) o.id = s("id");   // nombre anterior (compatibilidad)
			o.seq = u("seq");
			o.tipo = s("tipo");
			o.jugador = static_cast<uint32_t>(u("jugador_id"));
			o.equipoOrigen = static_cast<uint32_t>(u("equipo_origen"));
			o.equipoDestino = static_cast<uint32_t>(u("equipo_destino"));
			const int64_t d = u("dorsal");
			o.dorsal = (d >= 1 && d <= 99) ? static_cast<uint16_t>(d) : 0;
			o.autor = s("autor"); o.autorPc = s("autor_pc"); o.creadoEn = s("creado_en");
			o.baseSeq = u("base_seq");
			o.base = s("base"); o.shaResultado = s("sha_resultado"); o.resumen = s("resumen");
			if (j.contains("huella_bd") || j.contains("parche")) {   // contrato publicado: campos planos
				o.compat.parche = s("parche");
				o.compat.huellaBase = s("huella_bd");
				o.compat.formato = j.contains("formato") && j["formato"].is_number_integer() ? j["formato"].get<int>() : 1;
			}
			else if (j.contains("compat") && j["compat"].is_object()) {
				const auto& c = j["compat"];
				o.compat.parche = c.value("parche", "");
				o.compat.huellaBase = c.value("huella_base", "");
				o.compat.formato = c.contains("formato") && c["formato"].is_number_integer() ? c["formato"].get<int>() : 0;
			}
			else o.compat.formato = 0;
			if (o.id.empty() || o.id.size() > 64) return R::mal("OPERACION_INVALIDA", "sin id");
			if (o.tipo != "mover" && o.tipo != "quitar") return R::mal("OPERACION_INVALIDA", "tipo desconocido: " + o.tipo);
			if (!o.jugador) return R::mal("OPERACION_INVALIDA", "sin jugador");
			if (o.tipo == "mover" && !o.equipoDestino) return R::mal("OPERACION_INVALIDA", "sin equipo destino");
			if (o.tipo == "quitar" && !o.equipoOrigen) return R::mal("OPERACION_INVALIDA", "sin equipo origen");
			return R::bien(std::move(o));
		}
		catch (const std::exception& e) { return R::mal("OPERACION_INVALIDA", e.what()); }
	}

	Plantillas plantillasDe(const OptionFile& of) {
		Plantillas p;
		for (const auto& [eq, plazas] : of.plantillas()) {
			auto& v = p[eq];
			for (const auto& x : plazas) if (x.jugador) v.push_back(x.jugador);
		}
		return p;
	}

	json plantillasAJson(const Plantillas& p) {
		json j = json::object();
		for (const auto& [eq, v] : p) j[std::to_string(eq)] = v;
		return j;
	}

	Plantillas plantillasDeJson(const json& j) {
		Plantillas p;
		if (!j.is_object()) return p;
		for (auto it = j.begin(); it != j.end(); ++it) {
			try { p[static_cast<uint32_t>(std::stoul(it.key()))] = it.value().get<std::vector<uint32_t>>(); }
			catch (...) {}
		}
		return p;
	}

	std::vector<Movimiento> diferencias(const Plantillas& antes, const Plantillas& ahora) {
		std::map<uint32_t, std::set<uint32_t>> a, b;   // jugador → equipos
		for (const auto& [eq, v] : antes) for (auto j : v) a[j].insert(eq);
		for (const auto& [eq, v] : ahora) for (auto j : v) b[j].insert(eq);
		std::set<uint32_t> jugadores;
		for (const auto& [j, _] : a) jugadores.insert(j);
		for (const auto& [j, _] : b) jugadores.insert(j);
		std::vector<Movimiento> r;
		static const std::set<uint32_t> vacio;
		for (auto j : jugadores) {
			const auto& sa = a.count(j) ? a.at(j) : vacio;
			const auto& sb = b.count(j) ? b.at(j) : vacio;
			std::vector<uint32_t> salio, llego;
			std::set_difference(sa.begin(), sa.end(), sb.begin(), sb.end(), std::back_inserter(salio));
			std::set_difference(sb.begin(), sb.end(), sa.begin(), sa.end(), std::back_inserter(llego));
			size_t i = 0;
			for (; i < salio.size() && i < llego.size(); i++) r.push_back({ j, salio[i], llego[i] });
			for (size_t k = i; k < salio.size(); k++) r.push_back({ j, salio[k], 0 });
			for (size_t k = i; k < llego.size(); k++) r.push_back({ j, 0, llego[k] });
		}
		return r;
	}

	Decision decidir(const Operacion& op, const Plantillas& actual) {
		auto esta = [&](uint32_t eq) {
			auto it = actual.find(eq);
			return it != actual.end() && std::find(it->second.begin(), it->second.end(), op.jugador) != it->second.end();
		};
		if (op.tipo == "quitar") {
			if (!actual.count(op.equipoOrigen)) return { Veredicto::Conflicto, "el equipo " + std::to_string(op.equipoOrigen) + " no existe en tu option file" };
			if (!esta(op.equipoOrigen)) return { Veredicto::YaAplicada, "el jugador ya no estaba en ese equipo" };
			return { Veredicto::Aplicar, "" };
		}
		if (!actual.count(op.equipoDestino)) return { Veredicto::Conflicto, "el equipo destino " + std::to_string(op.equipoDestino) + " no existe en tu option file" };
		if (op.equipoOrigen && !actual.count(op.equipoOrigen)) return { Veredicto::Conflicto, "el equipo origen " + std::to_string(op.equipoOrigen) + " no existe en tu option file" };
		const bool enDestino = esta(op.equipoDestino);
		const bool enOrigen = op.equipoOrigen ? esta(op.equipoOrigen) : false;
		if (enDestino && !enOrigen) return { Veredicto::YaAplicada, "el jugador ya estaba en el equipo destino" };
		if (enDestino && enOrigen) return { Veredicto::Conflicto, "el jugador está en los dos equipos" };
		if (!op.equipoOrigen) return { Veredicto::Conflicto, "altas sin equipo de origen: todavía no se aplican" };
		if (!enOrigen) return { Veredicto::Conflicto, "el jugador ya no está en el equipo de origen (¿otro fichaje lo movió antes?)" };
		if (actual.at(op.equipoDestino).size() >= 40) return { Veredicto::Conflicto, "el equipo destino está lleno (40 jugadores)" };
		return { Veredicto::Aplicar, "" };
	}

	uint32_t EditorOption::sustituto(uint32_t equipo, uint32_t jugador) const {
		if (!_pos) return 0;
		auto s = _of.sugerirSustituto(equipo, jugador, _pos);
		return s.ok() ? *s.valor : 0;
	}

	Resultado<bool> EditorOption::mover(uint32_t jugador, uint32_t hacia, uint32_t desde, uint16_t dorsal) {
		auto r = _of.mover(jugador, hacia, desde, dorsal, 0);
		if (!r.ok() && r.error.codigo == "FALTA_SUSTITUTO") {
			const uint32_t s = sustituto(desde, jugador);
			if (!s) return Resultado<bool>::mal("FALTA_SUSTITUTO", "era titular y no se pudo elegir quién lo reemplaza");
			r = _of.mover(jugador, hacia, desde, dorsal, s);
		}
		if (!r.ok() && r.error.codigo == "SIN_DORSAL" && dorsal) r = _of.mover(jugador, hacia, desde, 0, sustituto(desde, jugador));
		return r;
	}

	Resultado<bool> EditorOption::quitar(uint32_t jugador, uint32_t desde) {
		auto r = _of.quitar(jugador, desde, 0);
		if (!r.ok() && r.error.codigo == "FALTA_SUSTITUTO") {
			const uint32_t s = sustituto(desde, jugador);
			if (!s) return Resultado<bool>::mal("FALTA_SUSTITUTO", "era titular y no se pudo elegir quién lo reemplaza");
			r = _of.quitar(jugador, desde, s);
		}
		return r;
	}

	std::vector<ResultadoOp> aplicarLote(Editor& ed, const std::vector<Operacion>& ops) {
		std::vector<ResultadoOp> r;
		for (const auto& op : ops) {
			const Decision d = decidir(op, ed.plantillas());
			if (d.v == Veredicto::YaAplicada) { r.push_back({ op.id, "aplicada", d.motivo }); continue; }
			if (d.v == Veredicto::Conflicto) { r.push_back({ op.id, "conflicto", d.motivo }); continue; }
			auto x = (op.tipo == "quitar") ? ed.quitar(op.jugador, op.equipoOrigen) : ed.mover(op.jugador, op.equipoDestino, op.equipoOrigen, op.dorsal);
			if (x.ok()) r.push_back({ op.id, "aplicada", "" });
			else r.push_back({ op.id, "conflicto", x.error.codigo + (x.error.detalle.empty() ? "" : ": " + x.error.detalle) });
		}
		return r;
	}

	// --- Estado ---------------------------------------------------------------------
	bool Estado::esConocida(const std::string& h) const { return std::find(conocidas.begin(), conocidas.end(), h) != conocidas.end(); }
	void Estado::anotarConocida(const std::string& h) {
		if (h.empty() || esConocida(h)) return;
		conocidas.push_back(h);
		while (conocidas.size() > 50) conocidas.pop_front();
	}
	void Estado::anotarHistorial(LineaHistorial l) {
		historial.push_back(std::move(l));
		while (historial.size() > 200) historial.pop_front();
	}
	bool Estado::debePublicar(const std::string& h) const {
		return !h.empty() && h != ultimoHashVisto && h != ultimoHashSubido && !esConocida(h);
	}

	json estadoAJson(const Estado& e) {
		json ent = json::array(), hist = json::array(), pub = json::array();
		for (const auto& x : e.entrantes) ent.push_back({ { "op", aJson(x.op) }, { "estado", x.estado }, { "motivo", x.motivo } });
		json conf = json::array();
		for (const auto& c : e.porConfirmar) conf.push_back({ { "id", c.id }, { "estado", c.estado }, { "motivo", c.motivo } });
		for (const auto& h : e.historial) hist.push_back({ { "fecha", h.fecha }, { "op", h.opId }, { "resumen", h.resumen }, { "autor", h.autor }, { "estado", h.estado }, { "motivo", h.motivo } });
		for (const auto& o : e.porPublicar) pub.push_back(aJson(o));
		// Los conjuntos se recortan para que el archivo no crezca sin fin.
		auto recorte = [](const std::set<std::string>& s) {
			std::vector<std::string> v(s.begin(), s.end());
			if (v.size() > 5000) v.erase(v.begin(), v.begin() + static_cast<long>(v.size() - 5000));
			return v;
		};
		return {
			{ "formato", "phoenix-sync/grupo-1" },
			{ "grupo_id", e.grupoId }, { "publicar", e.publicar }, { "pausado", e.pausado }, { "base_en_entrega", e.baseEnEntrega },
			{ "ultima_seq", e.ultimaSeq }, { "etag_operaciones", e.etagOperaciones }, { "etag_config", e.etagConfig },
			{ "config", { { "modo", aTexto(e.config.modo) }, { "actualizado_por", e.config.actualizadoPor },
				{ "actualizado_en", e.config.actualizadoEn }, { "leida", e.config.leida }, { "motivo", e.config.motivoNoLeida } } },
			{ "propias", recorte(e.propias) }, { "aplicadas", recorte(e.aplicadas) },
			{ "conocidas", std::vector<std::string>(e.conocidas.begin(), e.conocidas.end()) },
			{ "ultimo_hash_visto", e.ultimoHashVisto }, { "ultimo_hash_subido", e.ultimoHashSubido },
			{ "hay_foto", e.hayFoto }, { "foto", plantillasAJson(e.foto) },
			{ "por_publicar", pub }, { "entrantes", ent }, { "por_confirmar", conf }, { "historial", hist },
			{ "ultima", { { "entrega", e.ultima.entregaId }, { "respaldo", e.ultima.respaldo }, { "ops", e.ultima.ops }, { "fecha", e.ultima.fecha } } },
			{ "entrega_en_curso", e.entregaEnCurso } };
	}

	Estado estadoDeJson(const json& j) {
		Estado e;
		if (!j.is_object()) return e;
		e.grupoId = j.value("grupo_id", "");
		e.publicar = j.value("publicar", true);
		e.pausado = j.value("pausado", false);
		e.baseEnEntrega = j.value("base_en_entrega", false);
		e.ultimaSeq = j.value("ultima_seq", int64_t(0));
		e.etagOperaciones = j.value("etag_operaciones", "");
		e.etagConfig = j.value("etag_config", "");
		if (j.contains("config")) {
			const auto& c = j["config"];
			e.config.modo = c.value("modo", "") == "automatico" ? Modo::Automatico : Modo::Autorizacion;
			e.config.actualizadoPor = c.value("actualizado_por", "");
			e.config.actualizadoEn = c.value("actualizado_en", "");
			e.config.leida = c.value("leida", false);
			e.config.motivoNoLeida = c.value("motivo", "");
		}
		for (const auto& s : j.value("propias", std::vector<std::string>{})) e.propias.insert(s);
		for (const auto& s : j.value("aplicadas", std::vector<std::string>{})) e.aplicadas.insert(s);
		for (const auto& s : j.value("conocidas", std::vector<std::string>{})) e.conocidas.push_back(s);
		e.ultimoHashVisto = j.value("ultimo_hash_visto", "");
		e.ultimoHashSubido = j.value("ultimo_hash_subido", "");
		e.hayFoto = j.value("hay_foto", false);
		if (j.contains("foto")) e.foto = plantillasDeJson(j["foto"]);
		for (const auto& o : j.value("por_publicar", json::array())) if (auto r = deJson(o); r.ok()) e.porPublicar.push_back(*r.valor);
		for (const auto& x : j.value("entrantes", json::array())) {
			auto r = deJson(x.value("op", json::object()));
			if (r.ok()) e.entrantes.push_back({ *r.valor, x.value("estado", "pendiente"), x.value("motivo", "") });
		}
		for (const auto& c : j.value("por_confirmar", json::array()))
			e.porConfirmar.push_back({ c.value("id", ""), c.value("estado", ""), c.value("motivo", "") });
		for (const auto& h : j.value("historial", json::array()))
			e.historial.push_back({ h.value("fecha", ""), h.value("op", ""), h.value("resumen", ""), h.value("autor", ""), h.value("estado", ""), h.value("motivo", "") });
		if (j.contains("ultima")) {
			const auto& u = j["ultima"];
			e.ultima = { u.value("entrega", ""), u.value("respaldo", ""), u.value("ops", std::vector<std::string>{}), u.value("fecha", "") };
		}
		e.entregaEnCurso = j.value("entrega_en_curso", "");
		return e;
	}

	Resultado<Estado> cargarEstado(const std::string& ruta) {
		try {
			std::error_code ec;
			if (!fs::exists(aRuta(ruta), ec)) return Resultado<Estado>::bien(Estado{});
			std::ifstream f(aRuta(ruta), std::ios::binary);
			return Resultado<Estado>::bien(estadoDeJson(json::parse(f)));
		}
		catch (const std::exception& e) { return Resultado<Estado>::mal("ESTADO_ILEGIBLE", e.what()); }
	}

	Resultado<bool> guardarEstado(const std::string& ruta, const Estado& e) {
		try {
			const fs::path p = aRuta(ruta);
			fs::create_directories(p.parent_path());
			const fs::path tmp = aRuta(ruta + ".tmp");
			{
				std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
				f << estadoAJson(e).dump(1);
				if (!f) return Resultado<bool>::mal("ARCHIVO_ERROR", "No se pudo guardar el estado");
			}
			fs::rename(tmp, p);
			return Resultado<bool>::bien(true);
		}
		catch (const std::exception& ex) { return Resultado<bool>::mal("ARCHIVO_ERROR", ex.what()); }
	}

	std::string resumenDe(const Operacion& op, const std::function<std::string(uint32_t)>& nombreEquipo) {
		auto eq = [&](uint32_t id) { std::string n = nombreEquipo ? nombreEquipo(id) : ""; return n.empty() ? "equipo " + std::to_string(id) : n; };
		if (!op.resumen.empty()) return op.resumen;
		if (op.tipo == "quitar") return "Jugador " + std::to_string(op.jugador) + " deja " + eq(op.equipoOrigen);
		return "Jugador " + std::to_string(op.jugador) + ": " + (op.equipoOrigen ? eq(op.equipoOrigen) : "libre") + " → " + eq(op.equipoDestino);
	}

}
