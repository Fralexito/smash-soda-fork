#include "PartidasLM.h"
#include "Respaldos.h"
#include "Sha256.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace mercado::lmweb {

	namespace fs = std::filesystem;
	using json = nlohmann::json;

	namespace {
		std::string textoFecha(const lm::Fecha& f) {
			if (!f.valida()) return "";
			char b[16]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", int(f.anio), int(f.mes), int(f.dia));
			return b;
		}
		std::string rutaEstado(const std::string& carpeta, const PartidaPC& p) {
			std::string n = p.id;
			for (char& c : n) if (!std::isalnum((unsigned char)c)) c = '_';
			return deRuta(aRuta(carpeta) / (n + ".json"));
		}
		bool escribirAtomico(const std::string& ruta, const std::string& texto) {
			std::error_code ec;
			fs::create_directories(aRuta(ruta).parent_path(), ec);
			const fs::path tmp = aRuta(ruta + ".tmp");
			{ std::ofstream f(tmp, std::ios::binary | std::ios::trunc); if (!f) return false; f << texto; if (!f) return false; }
			fs::rename(tmp, aRuta(ruta), ec);
			if (ec) { fs::remove(tmp, ec); return false; }
			return true;
		}
	}

	nlohmann::json PartidaPC::aJson() const {
		return { { "partida", id }, { "ranura", ranura }, { "club_pes", clubPes }, { "club_nombre", clubNombre }, { "fecha_partida", fecha } };
	}

	bool esRanuraLM(const std::string& n) {
		if (n.size() != 10 || n[0] != 'M' || n[1] != 'L') return false;
		for (size_t i = 2; i < n.size(); i++) if (!std::isxdigit((unsigned char)n[i])) return false;
		return true;
	}

	std::vector<PartidaPC> buscarPartidas(const std::vector<std::string>& carpetasSave, std::vector<std::string>* avisos) {
		std::vector<PartidaPC> r;
		for (const auto& carpeta : carpetasSave) {
			std::error_code ec;
			for (fs::directory_iterator it(aRuta(carpeta), ec), fin; !ec && it != fin; it.increment(ec)) {
				const std::string nombre = deRuta(it->path().filename());
				std::error_code e2;
				if (!esRanuraLM(nombre) || !it->is_regular_file(e2)) continue;
				const std::string ruta = deRuta(it->path());
				auto g = lm::GuardadoLM::abrir(ruta);
				if (!g.ok()) { if (avisos) avisos->push_back(nombre + ": " + g.error.detalle); continue; }
				const int k = equipoUsuario(*g.valor);
				if (k < 0) { if (avisos) avisos->push_back(nombre + ": no se encontró el club del usuario (no se toca)"); continue; }
				PartidaPC p;
				p.ruta = ruta; p.ranura = nombre; p.carpetaSave = carpeta;
				p.id = idPartida(nombre, *g.valor);
				auto id = g.valor->idOptionDe(k); if (id.ok()) p.clubPes = *id.valor;
				auto e = g.valor->equipo(k); if (e.ok()) p.clubNombre = e.valor->nombre;
				auto f = g.valor->fechaActual(); if (f.ok()) p.fecha = textoFecha(*f.valor);
				r.push_back(std::move(p));
			}
		}
		return r;
	}

	EstadoPartida leerEstado(const std::string& carpetaEstado, const PartidaPC& p) {
		std::ifstream f(aRuta(rutaEstado(carpetaEstado, p)), std::ios::binary);
		if (!f) return {};
		return EstadoPartida::desdeJson(json::parse(f, nullptr, false));
	}

	ResultadoPartida aplicarEnPartida(const PartidaPC& p, const ListaLM& lista, const Contexto& ctx, const OpcionesEscritura& op) {
		ResultadoPartida r;
		r.partida = p;
		auto abierto = [&] { return op.juegoAbierto && op.juegoAbierto(); };
		if (abierto()) { r.error = "JUEGO_ABIERTO: PES está abierto; la Liga Máster se actualiza cuando se cierre"; return r; }
		r.sha256Antes = sha256::deArchivo(p.ruta);
		if (r.sha256Antes.empty()) { r.error = "ARCHIVO_ERROR: no se pudo leer " + p.ranura; return r; }
		auto g = lm::GuardadoLM::abrir(p.ruta);
		if (!g.ok()) { r.error = g.error.codigo + ": " + g.error.detalle; return r; }
		if (idPartida(p.ranura, *g.valor) != p.id) { r.error = "PARTIDA_CAMBIO: la ranura " + p.ranura + " ya es otra partida"; return r; }

		EstadoPartida est = leerEstado(op.carpetaEstado, p);
		r.informe = aplicar(lista, p.id, *g.valor, est, ctx);
		r.versionAplicada = est.versionAplicada;

		if (r.informe.modificado) {
			// 1) Respaldo de la ranura tal como está.
			respaldos::Opciones ro; ro.raiz = op.raizRespaldos; ro.motivo = "antes de Liga Máster web (" + p.ranura + ")"; ro.versionSync = op.versionSync;
			auto rb = respaldos::crear({ { "lm", p.ruta } }, ro);
			if (!rb.ok()) { r.error = rb.error.codigo + ": " + rb.error.detalle; return r; }
			r.respaldo = rb.valor->nombre;
			// 2) Archivo nuevo, verificado al guardarse.
			const std::string nuevo = p.ruta + ".phoenix-nuevo", antes = p.ruta + ".phoenix-antes";
			std::error_code ec;
			fs::remove(aRuta(nuevo), ec);
			auto gu = g.valor->guardarComo(nuevo);
			if (!gu.ok()) { fs::remove(aRuta(nuevo), ec); r.error = gu.error.codigo + ": " + gu.error.detalle; return r; }
			// 3) Nada cambió mientras tanto: juego cerrado y la ranura con la misma huella.
			if (abierto() || sha256::deArchivo(p.ruta) != r.sha256Antes) {
				fs::remove(aRuta(nuevo), ec);
				r.error = abierto() ? "JUEGO_ABIERTO: PES se abrió mientras tanto; no se tocó nada" : "PARTIDA_CAMBIO: la ranura cambió mientras tanto; no se tocó nada";
				return r;
			}
			// 4) Cambio de nombre: original → .phoenix-antes, nuevo → original (si falla, se deja como estaba).
			fs::remove(aRuta(antes), ec);
			fs::rename(aRuta(p.ruta), aRuta(antes), ec);
			if (ec) { fs::remove(aRuta(nuevo), ec); r.error = "ARCHIVO_ERROR: no se pudo apartar la ranura original"; return r; }
			fs::rename(aRuta(nuevo), aRuta(p.ruta), ec);
			if (ec) {
				std::error_code e2; fs::rename(aRuta(antes), aRuta(p.ruta), e2);
				r.error = "ARCHIVO_ERROR: no se pudo colocar la ranura nueva (se dejó la original)";
				return r;
			}
			r.escrito = true;
			r.sha256Despues = sha256::deArchivo(p.ruta);
		}
		// 5) Estado de la partida (solo si todo lo anterior salió bien).
		if (!escribirAtomico(rutaEstado(op.carpetaEstado, p), est.aJson().dump(1))) {
			r.error = "ESTADO_ERROR: la partida se actualizó pero no se pudo guardar su estado";
		}
		return r;
	}

	nlohmann::json informeParaWeb(const std::vector<ResultadoPartida>& rs) {
		json partidas = json::array();
		for (const auto& r : rs) {
			json res = json::array();
			for (const auto& c : r.informe.resultados) {
				json x = { { "version", c.version }, { "estado", c.estado } };
				if (!c.motivo.empty()) x["motivo"] = c.motivo.substr(0, 200);
				if (!c.antes.empty()) x["antes"] = c.antes;
				if (!c.despues.empty()) x["despues"] = c.despues;
				res.push_back(std::move(x));
			}
			json p = r.partida.aJson();
			p["version_aplicada"] = r.versionAplicada;
			p["resultados"] = res;
			if (!r.error.empty()) p["error"] = r.error.substr(0, 200);
			if (!r.sha256Despues.empty()) p["huella"] = r.sha256Despues;
			partidas.push_back(std::move(p));
		}
		return { { "formato", "phoenix-lm/aplicado-1" }, { "partidas", partidas } };
	}

	Resultado<Vuelta> vuelta(const std::vector<std::string>& carpetasSave, const std::function<Resultado<std::string>(int64_t)>& contenidoDesde,
		const Contexto& ctx, const OpcionesEscritura& op) {
		using R = Resultado<Vuelta>;
		Vuelta v;
		const auto ps = buscarPartidas(carpetasSave, &v.avisos);
		if (ps.empty()) { v.informe = informeParaWeb({}); return R::bien(std::move(v)); }
		int64_t desde = INT64_MAX;
		for (const auto& p : ps) desde = std::min(desde, leerEstado(op.carpetaEstado, p).versionAplicada);
		ListaLM todo; todo.desde = desde;
		for (int pagina = 0; pagina < 50; pagina++) {   // la web manda hasta 500 por página
			auto c = contenidoDesde(desde);
			if (!c.ok()) return R::mal(c.error.codigo, c.error.detalle);
			auto l = parsear(*c.valor);
			if (!l.ok()) return R::mal(l.error.codigo, l.error.detalle);
			todo.versionActual = std::max(todo.versionActual, l.valor->versionActual);
			if (l.valor->cambios.empty()) break;
			for (auto& x : l.valor->cambios) todo.cambios.push_back(std::move(x));
			desde = todo.cambios.back().version;
			if (desde >= l.valor->versionActual) break;
		}
		for (const auto& p : ps) v.partidas.push_back(aplicarEnPartida(p, todo, ctx, op));
		v.informe = informeParaWeb(v.partidas);
		return R::bien(std::move(v));
	}

}
