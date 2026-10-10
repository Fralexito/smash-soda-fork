// =============================================================================
//  Phoenix Sync · Pruebas de «option file compartido» (sin internet, sin Windows)
//  Rutas multiparche, respaldos, entrega para Link, operaciones, conflictos,
//  anti-bucle y DOS PCs simuladas contra una web falsa en memoria.
// =============================================================================

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "../core/AsignacionBase.h"
#include "../core/ClienteSync.h"
#include "../terceros/miniz/miniz.h"
#include "../core/Entrega.h"
#include "../core/Grupo.h"
#include "../core/Respaldos.h"
#include "../core/RutasJuego.h"
#include "../core/Sha256.h"
#include "../core/SyncCompartido.h"
#include <nlohmann/json.hpp>

using namespace mercado;
namespace fs = std::filesystem;
using json = nlohmann::json;

static int fallos = 0, total = 0;
#define CHECK(cond) do { total++; if (!(cond)) { fallos++; std::printf("  FALLA %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void escribir(const fs::path& p, const std::string& t) {
	fs::create_directories(p.parent_path());
	std::ofstream(p, std::ios::binary | std::ios::trunc) << t;
}
static std::string leer(const fs::path& p) { std::ifstream f(p, std::ios::binary); return std::string(std::istreambuf_iterator<char>(f), {}); }

// --- PlayerAssignment.bin sintético (WESYS + zlib, filas de 16 B) -------------------
struct FilaPA { uint32_t jug, eq; uint8_t dorsal, orden, banderas; };
static std::string hacerPA(const std::vector<FilaPA>& filas) {
	std::vector<uint8_t> plano(filas.size() * 16, 0);
	for (size_t i = 0; i < filas.size(); i++) {
		uint8_t* e = &plano[i * 16];
		auto pon = [](uint8_t* p, uint32_t v) { for (int k = 0; k < 4; k++) p[k] = uint8_t(v >> (8 * k)); };
		pon(e, uint32_t(i + 1)); pon(e + 4, filas[i].jug); pon(e + 8, filas[i].eq);
		e[12] = uint8_t(filas[i].dorsal - 1); e[13] = uint8_t(filas[i].orden << 2); e[14] = filas[i].banderas;
	}
	mz_ulong n = mz_compressBound(mz_ulong(plano.size()));
	std::vector<uint8_t> c(n);
	mz_compress2(c.data(), &n, plano.data(), mz_ulong(plano.size()), 6);
	std::string out("\xff\x10\x81WESYS", 8);
	auto u32 = [&](uint32_t v) { for (int k = 0; k < 4; k++) out += char(uint8_t(v >> (8 * k))); };
	u32(uint32_t(n)); u32(uint32_t(plano.size()));
	out.append(reinterpret_cast<const char*>(c.data()), n);
	return out;
}

// --- Option file FALSO: JSON de plantillas rellenado a 4096 bytes (mismo tamaño siempre) ---
static const size_t kTam = 4096;
static void escribirOption(const fs::path& p, const grupo::Plantillas& pl, const std::string& huella = "BASE-1") {
	std::string t = json{ { "huella", huella }, { "pl", grupo::plantillasAJson(pl) } }.dump();
	t.resize(kTam, ' ');
	escribir(p, t);
}
struct OptionFalso : sync::AccesoOption {
	int lecturas = 0;
	Resultado<sync::LecturaOption> leer(const std::string& ruta) override {
		lecturas++;
		const json j = json::parse(::leer(aRuta(ruta)), nullptr, false);
		if (j.is_discarded()) return Resultado<sync::LecturaOption>::mal("OPTION_INVALIDO", "a medias");
		sync::LecturaOption l;
		l.plantillas = grupo::plantillasDeJson(j["pl"]);
		l.huellaEquipos = j.value("huella", "");
		for (const auto& [eq, v] : l.plantillas) for (auto x : v) l.dorsales[eq][x] = static_cast<uint16_t>(x % 99 + 1);
		return Resultado<sync::LecturaOption>::bien(l);
	}
	struct Ed : grupo::Editor {
		grupo::Plantillas p;
		grupo::Plantillas plantillas() const override { return p; }
		Resultado<bool> mover(uint32_t j, uint32_t hacia, uint32_t desde, uint16_t) override {
			auto& v = p[desde]; v.erase(std::remove(v.begin(), v.end(), j), v.end()); p[hacia].push_back(j); return Resultado<bool>::bien(true);
		}
		Resultado<bool> quitar(uint32_t j, uint32_t desde) override {
			auto& v = p[desde]; v.erase(std::remove(v.begin(), v.end(), j), v.end()); return Resultado<bool>::bien(true);
		}
	};
	Resultado<sync::AplicacionOption> aplicar(const std::string& ruta, const std::vector<grupo::Operacion>& ops, const std::string& salida) override {
		auto l = leer(ruta);
		if (!l.ok()) return Resultado<sync::AplicacionOption>{ std::nullopt, l.error };
		Ed ed; ed.p = l.valor->plantillas;
		sync::AplicacionOption a;
		a.resultados = grupo::aplicarLote(ed, ops);
		for (const auto& r : a.resultados) if (r.estado == "aplicada" && r.motivo.empty()) a.escritas++;
		a.plantillasDespues = ed.p;
		if (a.escritas) escribirOption(aRuta(salida), ed.p, l.valor->huellaEquipos);
		return Resultado<sync::AplicacionOption>::bien(a);
	}
};

// --- Web FALSA en memoria (compartida por las dos PCs) ---------------------------------
struct WebFalsa {
	bool existe = true;              // false = la web aún no tiene las rutas (404)
	bool caida = false;              // sin respuesta
	std::string modo = "automatico";
	std::vector<json> ops;           // con seq
	std::map<std::string, std::string> confirmaciones;   // "pc|id" → estado
	int publicaciones = 0;
};
struct HttpFalso : ClienteHttp {
	WebFalsa& w; std::string pc;
	HttpFalso(WebFalsa& web, std::string nombre) : w(web), pc(std::move(nombre)) {}
	RespuestaHttp peticion(const std::string& metodo, const std::string& url, const std::string& cuerpo, const std::vector<std::string>& cab) override {
		for (const auto& c : cab) if (c.rfind("Authorization: Bearer phx_", 0) != 0 && c.rfind("Authorization", 0) == 0) return { 401, R"({"ok":false,"error":{"codigo":"TOKEN_INVALIDO"}})", "" };
		if (w.caida) return { 0, "", "sin red" };
		if (!w.existe) return { 404, "Not Found", "" };
		auto ok = [](const json& d) { return RespuestaHttp{ 200, json{ { "ok", true }, { "datos", d } }.dump(), "" }; };
		if (url.find("/v1/sync/config") != std::string::npos)
			return ok({ { "modo", w.modo }, { "actualizado_por", "FRALEX" }, { "actualizado_en", "2026-10-09T19:00:00" }, { "etag", "c-" + w.modo } });
		if (metodo == "GET" && url.find("/v1/sync/operaciones") != std::string::npos) {
			const int64_t desde = std::stoll(url.substr(url.find("desde=") + 6));
			json lista = json::array();
			for (const auto& o : w.ops) if (o["seq"].get<int64_t>() > desde) lista.push_back(o);
			return ok({ { "operaciones", lista }, { "hasta", (int64_t)w.ops.size() }, { "etag", "o" + std::to_string(w.ops.size()) } });
		}
		if (metodo == "POST" && url.size() >= 9 && url.substr(url.size() - 9) == "/aplicada") {
			const json j = json::parse(cuerpo);
			w.confirmaciones[pc + "|" + j["op_id"].get<std::string>()] = j["estado"];
			return ok({ { "ok", true } });
		}
		if (metodo == "POST" && url.find("/v1/sync/operaciones") != std::string::npos) {
			json j = json::parse(cuerpo);
			for (const auto& o : w.ops) if (o["op_id"] == j["op_id"]) return { 409, R"({"ok":false,"error":{"codigo":"OPERACION_DUPLICADA"}})", "" };
			j["seq"] = (int64_t)w.ops.size() + 1; j["autor"] = pc; j.erase("grupo_id");
			w.ops.push_back(j); w.publicaciones++;
			return ok({ { "seq", j["seq"] } });
		}
		return { 404, "", "" };
	}
	bool descargar(const std::string&, const std::string&, long long, std::string&) override { return false; }
};
struct TokenFalso : FuenteToken {
	std::string token() override { return "phx_secreto"; }
	bool guardar(const std::string&) override { return true; }
	void borrar() override {}
	std::string nombreModo() const override { return "compartido"; }
};

/// Una PC simulada: Documentos, juego, datos, motor… y un «Link» falso que coloca las entregas.
struct PC {
	std::string nombre;
	fs::path raiz, docs, juego, datos, save;
	int64_t reloj = 1760050000;
	HttpFalso http; TokenFalso tok; OptionFalso of;
	sync::ClienteSync web; sync::Motor motor;
	PC(const fs::path& base, const std::string& n, WebFalsa& w)
		: nombre(n), raiz(base / n), docs(raiz / "Documentos"), juego(raiz / "PES 2021"), datos(raiz / "AppData" / "Phoenix Mercado"),
		save(docs / "KONAMI" / "eFootball PES 2021 SEASON UPDATE" / "239200" / "save"),
		http(w, n), web(http, tok), motor(web, of, entorno()) {}
	sync::Entorno entorno() {
		sync::Entorno e;
		e.documentos = [this] { return std::vector<std::string>{ deRuta(docs) }; };
		e.carpetaJuego = [this] { return deRuta(juego); };
		e.carpetaDatos = deRuta(datos);
		e.ahora = [this] { return reloj; };
		e.juegoAbierto = [] { return false; };
		e.nombrePc = nombre;
		e.segundosEstable = 10;
		e.espacioLibre = [](const std::string&) { return uint64_t(1) << 40; };
		return e;
	}
	fs::path edit() const { return save / "EDIT00000000"; }
	/// Lo que hace Phoenix Link: si hay entrega.json válido, coloca y mueve a entregados\<id>.
	bool linkColoca() {
		const fs::path ent = datos / "entrega";
		if (!fs::exists(ent / "entrega.json")) return false;
		const json e = json::parse(leer(ent / "entrega.json"));
		for (const auto& a : e["archivos"]) {
			if (sha256::deArchivo(deRuta(ent / a["nombre"].get<std::string>())) != a["sha256"].get<std::string>()) return false;
			if (a["nombre"] == "PlayerAssignment.bin")
				fs::copy_file(ent / "PlayerAssignment.bin", juego / "SiderAddons" / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "PlayerAssignment.bin", fs::copy_options::overwrite_existing);
			if (a["nombre"] == "EDIT00000000") {
				if (fs::file_size(ent / "EDIT00000000") != fs::file_size(edit())) return false;
				fs::copy_file(ent / "EDIT00000000", edit(), fs::copy_options::overwrite_existing);
			}
		}
		const fs::path dst = ent / "entregados" / e["id"].get<std::string>();
		fs::create_directories(dst);
		fs::rename(ent / "entrega.json", dst / "entrega.json");
		for (const auto& a : e["archivos"]) fs::rename(ent / a["nombre"].get<std::string>(), dst / a["nombre"].get<std::string>());
		return true;
	}
	/// Avanza el reloj y da vueltas (como el hilo real cada ~3 s).
	void pasar(int segundos) { for (int s = 0; s < segundos; s += 3) { reloj += 3; motor.ciclo(); linkColoca(); } }
	/// El usuario ficha en su PC (cambia el option file y su fecha).
	void fichar(uint32_t jug, uint32_t desde, uint32_t hacia) {
		auto l = of.leer(deRuta(edit())); auto p = l.valor->plantillas;
		auto& v = p[desde]; v.erase(std::remove(v.begin(), v.end(), jug), v.end()); p[hacia].push_back(jug);
		escribirOption(edit(), p);
		fs::last_write_time(edit(), fs::last_write_time(edit()) + std::chrono::seconds(7));
	}
	bool esta(uint32_t jug, uint32_t eq) {
		auto l = of.leer(deRuta(edit())); const auto& v = l.valor->plantillas[eq];
		return std::find(v.begin(), v.end(), jug) != v.end();
	}
};

int main() {
	const fs::path tmp = fs::temp_directory_path() / "phoenix-sync-compartido";
	fs::remove_all(tmp);

	// ---------------------------------------------------------------------------------
	std::printf("Rutas multiparche\n");
	{
		const fs::path d1 = tmp / "rutas" / "Documents", d2 = tmp / "rutas" / "OneDrive" / "Documentos";
		escribir(d1 / "KONAMI" / "eFootball PES 2021 SEASON UPDATE" / "111" / "save" / "EDIT00000000", "viejo");
		escribir(d2 / "KONAMI" / "Otra Edicion" / "222" / "save" / "EDIT00000000", "nuevo");
		fs::last_write_time(d2 / "KONAMI" / "Otra Edicion" / "222" / "save" / "EDIT00000000",
			fs::last_write_time(d1 / "KONAMI" / "eFootball PES 2021 SEASON UPDATE" / "111" / "save" / "EDIT00000000") + std::chrono::hours(1));
		fs::create_directories(d1 / "KONAMI" / "Sin save" / "333");                   // carpeta sin save: se salta
		const auto todas = rutas::buscarSaves({ deRuta(d1), deRuta(d2), deRuta(tmp / "no-existe"), "" });
		CHECK(todas.size() == 2);
		const auto act = rutas::saveActivo({ deRuta(d1), deRuta(d2) });
		CHECK(act && act->steamid == "222" && act->edicion == "Otra Edicion");
		CHECK(!rutas::saveActivo({ deRuta(tmp / "nada") }));

		const fs::path j = tmp / "rutas" / "Juego Inventado";
		const std::string wesys = std::string("WESYS") + std::string(11, '\0') + "datos";
		escribir(j / "SiderAddons" / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "Player.bin", wesys);
		escribir(j / "Extras Raros" / "Modo Beta" / "SiderAddons" / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "Player.bin", wesys);
		escribir(j / "Extras Raros" / "Modo Sin DB" / "SiderAddons" / "sider.ini", "x");   // modo sin Phoenix-DB
		const auto bases = rutas::buscarPlayerBin(deRuta(j));
		CHECK(bases.size() == 2);
		CHECK(bases.size() == 2 && bases[0].modo == "principal" && bases[1].modo == "Extras Raros/Modo Beta");
		CHECK(rutas::buscarPlayerBin(deRuta(tmp / "no-existe")).empty());
		CHECK(rutas::nombreParche(deRuta(j)) == "Juego Inventado");
		escribir(j / "version_actual.txt", "\xEF\xBB\xBF" "Mi Parche 27\r\n");
		CHECK(rutas::nombreParche(deRuta(j)) == "Mi Parche 27");
		CHECK(rutas::etiquetaSegura("a/b:c") == "a_b_c");
	}

	// ---------------------------------------------------------------------------------
	std::printf("Respaldos\n");
	{
		const fs::path save = tmp / "resp" / "save", pb = tmp / "resp" / "juego" / "Player.bin", raiz = tmp / "resp" / "respaldos";
		escribir(save / "EDIT00000000", std::string(5000, 'E'));
		escribir(save / "otro_save", "liga master");
		escribir(pb, "WESYS player");
		std::time_t reloj = 1760050000;
		uint64_t libre = uint64_t(1) << 40;
		respaldos::Opciones op;
		op.raiz = deRuta(raiz); op.motivo = "prueba"; op.versionSync = "t";
		op.ahora = [&] { return reloj; };
		op.espacioLibre = [&](const std::string&) { return libre; };
		const std::vector<respaldos::Origen> orig = { { "save", deRuta(save) }, { "phoenix-db/principal", deRuta(pb) }, { "phoenix-db/falta", deRuta(tmp / "nada.bin") } };

		auto a = respaldos::crear(orig, op);
		CHECK(a.ok() && a.valor->archivos.size() == 3 && a.valor->omitidos.size() == 1);
		if (a.ok()) CHECK(sha256::deArchivo(deRuta(fs::path(aRuta(a.valor->carpeta)) / "save" / "EDIT00000000")) == sha256::deArchivo(deRuta(save / "EDIT00000000")));
		auto b = respaldos::crear(orig, op);   // mismo segundo → no pisa
		CHECK(b.ok() && a.ok() && b.valor->nombre == a.valor->nombre + "-2");
		libre = 1000;                          // sin espacio
		const auto antes = respaldos::listar(op.raiz).size();
		auto c = respaldos::crear(orig, op);
		CHECK(!c.ok() && c.error.codigo == "SIN_ESPACIO" && respaldos::listar(op.raiz).size() == antes);
		libre = uint64_t(1) << 40;
		CHECK(!respaldos::crear({ { "save", deRuta(tmp / "vacia") } }, op).ok());

		// Restaurar: cambia el original, restaura y vuelve a ser idéntico; antes respalda lo actual.
		const std::string hOrig = sha256::deArchivo(deRuta(save / "EDIT00000000"));
		escribir(save / "EDIT00000000", std::string(5000, 'X'));
		reloj += 5;
		auto r = respaldos::restaurar(op.raiz, a.valor->nombre, orig, op);
		CHECK(r.ok() && r.valor->colocados == 3 && !r.valor->respaldoPrevio.nombre.empty());
		CHECK(sha256::deArchivo(deRuta(save / "EDIT00000000")) == hOrig);
		CHECK(!respaldos::restaurar(op.raiz, "../../etc", orig, op).ok());
		// Respaldo dañado: no toca nada.
		escribir(aRuta(a.valor->carpeta) / "save" / "otro_save", "dañado");
		CHECK(!respaldos::restaurar(op.raiz, a.valor->nombre, orig, op).ok());

		// Retención: 10 días con 3 respaldos por día.
		const fs::path raiz2 = tmp / "resp" / "retencion";
		op.raiz = deRuta(raiz2);
		for (int d = 0; d < 10; d++) for (int k = 0; k < 3; k++) {
			reloj = 1760050000 + d * 86400 + k * 3600;
			CHECK(respaldos::crear(orig, op).ok());
		}
		escribir(raiz2 / "no-es-mio" / "x.txt", "x");   // carpeta ajena: jamás se borra
		const auto borrados = respaldos::aplicarRetencion(op.raiz, { 10, 7 }, reloj);
		const auto quedan = respaldos::listar(op.raiz);
		// 10 últimos (días 9,8,7 y uno del 6) + primero de los días 3..6 que no estén ya → 10 + 4 = 14
		CHECK(quedan.size() == 14 && borrados.size() == 16);
		CHECK(fs::exists(raiz2 / "no-es-mio" / "x.txt"));
		CHECK(!respaldos::faltaDiario(op.raiz, reloj) && respaldos::faltaDiario(op.raiz, reloj + 86400));
	}

	// ---------------------------------------------------------------------------------
	std::printf("Entrega para Link\n");
	{
		const fs::path ent = tmp / "entrega", src = tmp / "src";
		escribir(src / "EDIT00000000", std::string(4096, 'Z'));
		escribir(src / "Player.bin", std::string("WESYS") + std::string(20, 'p'));
		escribir(src / "MalPlayer.bin", std::string(30, 'p'));
		CHECK(entrega::idValido("sync-2026_x") && !entrega::idValido("a/b") && !entrega::idValido(std::string(65, 'a')) && !entrega::idValido(""));
		CHECK(entrega::idValido(entrega::nuevoId()) && entrega::nuevoId() != entrega::nuevoId());
		CHECK(entrega::recortarResumen("Fichaje: Muñoz → Barça", 12).size() <= 12);
		CHECK(json::parse("\"" + entrega::recortarResumen("ññññññññ", 5) + "\"").is_string());   // UTF-8 válido

		auto w = entrega::escribir(deRuta(ent), "id-1", "Lamine al City", { { "EDIT00000000", deRuta(src / "EDIT00000000") }, { "Player.bin", deRuta(src / "Player.bin") } }, "2026-10-09T19:00:00");
		CHECK(w.ok());
		const json e = json::parse(leer(ent / "entrega.json"));
		CHECK(e["version"] == 1 && e["id"] == "id-1" && e["resumen"] == "Lamine al City" && e["archivos"].size() == 2);
		for (const auto& a : e["archivos"]) CHECK(sha256::deArchivo(deRuta(ent / a["nombre"].get<std::string>())) == a["sha256"].get<std::string>());
		CHECK(e["archivos"][0]["sha256"].get<std::string>().size() == 64);
		CHECK(!fs::exists(ent / "entrega.json.tmp") && !fs::exists(ent / "EDIT00000000.tmp"));
		CHECK(entrega::hayPendiente(deRuta(ent)));
		auto w2 = entrega::escribir(deRuta(ent), "id-2", "x", { { "EDIT00000000", deRuta(src / "EDIT00000000") } }, "t");
		CHECK(!w2.ok() && w2.error.codigo == "ENTREGA_PENDIENTE");   // no se apila
		fs::create_directories(ent / "entregados" / "id-1"); fs::remove(ent / "entrega.json");
		CHECK(entrega::estadoDe(deRuta(ent), "id-1") == 1);
		CHECK(entrega::escribir(deRuta(ent), "id-1", "x", { { "EDIT00000000", deRuta(src / "EDIT00000000") } }, "t").error.codigo == "ENTREGA_INVALIDA");   // id repetido
		CHECK(entrega::escribir(deRuta(ent), "id-3", "x", { { "sider.ini", deRuta(src / "EDIT00000000") } }, "t").error.codigo == "ENTREGA_INVALIDA");
		CHECK(entrega::escribir(deRuta(ent), "id-4", "x", { { "Player.bin", deRuta(src / "MalPlayer.bin") } }, "t").error.codigo == "ENTREGA_INVALIDA");
		CHECK(!entrega::hayPendiente(deRuta(ent)));
	}

	// ---------------------------------------------------------------------------------
	std::printf("Operaciones, choques e idempotencia\n");
	{
		grupo::Plantillas antes = { { 1, { 10, 11, 12 } }, { 2, { 20, 21 } }, { 3, {} } };
		grupo::Plantillas ahora = { { 1, { 11, 12 } }, { 2, { 20, 21, 10 } }, { 3, { 99 } } };
		const auto d = grupo::diferencias(antes, ahora);
		CHECK(d.size() == 2);
		CHECK(d.size() == 2 && d[0].jugador == 10 && d[0].desde == 1 && d[0].hacia == 2);
		CHECK(d.size() == 2 && d[1].jugador == 99 && d[1].desde == 0 && d[1].hacia == 3);
		CHECK(grupo::diferencias(antes, antes).empty());

		grupo::Operacion op; op.id = "a"; op.jugador = 10; op.equipoOrigen = 1; op.equipoDestino = 2;
		CHECK(grupo::decidir(op, antes).v == grupo::Veredicto::Aplicar);
		CHECK(grupo::decidir(op, ahora).v == grupo::Veredicto::YaAplicada);
		grupo::Operacion choque = op; choque.id = "b"; choque.equipoDestino = 3;   // mismo jugador a otro club
		CHECK(grupo::decidir(choque, ahora).v == grupo::Veredicto::Conflicto);
		grupo::Operacion lleno = op; grupo::Plantillas pl = antes; pl[2] = std::vector<uint32_t>(40, 5);
		CHECK(grupo::decidir(lleno, pl).v == grupo::Veredicto::Conflicto);
		grupo::Operacion noEq = op; noEq.equipoDestino = 77;
		CHECK(grupo::decidir(noEq, antes).v == grupo::Veredicto::Conflicto);

		OptionFalso::Ed ed; ed.p = antes;
		auto r1 = grupo::aplicarLote(ed, { op, choque });   // el primero gana, el segundo choca
		CHECK(r1.size() == 2 && r1[0].estado == "aplicada" && r1[1].estado == "conflicto");
		auto r2 = grupo::aplicarLote(ed, { op });            // idempotente: no duplica
		CHECK(r2[0].estado == "aplicada" && !r2[0].motivo.empty() && std::count(ed.p[2].begin(), ed.p[2].end(), 10u) == 1);

		op.compat = { "P", "H", 1 };
		auto rt = grupo::deJson(grupo::aJson(op));
		CHECK(rt.ok() && rt.valor->id == "a" && rt.valor->equipoDestino == 2 && rt.valor->compat.huellaBase == "H");
		CHECK(!grupo::deJson({ { "id", "x" }, { "tipo", "borrar" }, { "jugador_id", 1 } }).ok());
		CHECK(grupo::motivoIncompatible({ "A", "H", 1 }, { "B", "H", 1 }).empty());
		CHECK(!grupo::motivoIncompatible({ "A", "H", 1 }, { "A", "OTRA", 1 }).empty());
		CHECK(!grupo::motivoIncompatible({ "A", "H", 1 }, { "A", "H", 2 }).empty());

		CHECK(grupo::parsearConfig({ { "modo", "automatico" } }).modo == grupo::Modo::Automatico);
		CHECK(grupo::parsearConfig({ { "modo", "AUTOMATICO" } }).modo == grupo::Modo::Autorizacion);
		CHECK(grupo::parsearConfig(json()).modo == grupo::Modo::Autorizacion);

		grupo::Estado e; e.grupoId = "g"; e.propias.insert("p"); e.anotarConocida("h1"); e.foto = antes; e.hayFoto = true;
		e.porConfirmar.push_back({ "x", "aplicada", "" });
		const auto e2 = grupo::estadoDeJson(grupo::estadoAJson(e));
		CHECK(e2.grupoId == "g" && e2.propias.count("p") && e2.esConocida("h1") && e2.foto == antes && e2.porConfirmar.size() == 1);
		CHECK(!e2.debePublicar("h1") && e2.debePublicar("h2"));
	}

	// ---------------------------------------------------------------------------------
	std::printf("Cliente web: ritmo, 404, 429 y secretos\n");
	{
		sync::Ritmo r;
		CHECK(r.puede("x", 0));
		r.exito("x", 100); CHECK(!r.puede("x", 104) && r.puede("x", 105));
		r.fallo("y", 0); CHECK(r.siguiente("y") == 2);
		r.fallo("y", 0); CHECK(r.siguiente("y") == 4);
		for (int i = 0; i < 20; i++) r.fallo("y", 0);
		CHECK(r.siguiente("y") == 300);
		r.fallo("z", 0, 42); CHECK(r.siguiente("z") == 42);

		WebFalsa w; w.existe = false;
		HttpFalso h(w, "pc"); TokenFalso t;
		std::string registro;
		sync::ClienteSync c(h, t, [&](const std::string&, const std::string& x) { registro += x; });
		bool sin = false;
		auto cf = c.config("g", "", sin);
		CHECK(!cf.ok() && cf.error.codigo == "RUTA_NO_ENCONTRADA");
		w.existe = true;
		auto l = c.operaciones("g", 0, "");
		CHECK(l.ok() && l.valor->operaciones.empty());
		CHECK(registro.find("phx_secreto") == std::string::npos);
	}

	// ---------------------------------------------------------------------------------
	std::printf("Dos PCs (FRALEX y amigo) contra la web falsa\n");
	{
		WebFalsa web;
		const grupo::Plantillas inicial = { { 108, { 1, 2, 3 } }, { 172, { 7, 8 } }, { 173, { 9 } }, { 50, { 4, 5 } } };
		PC fralex(tmp / "pcs", "FRALEX", web), amigo(tmp / "pcs", "AMIGO", web);
		for (PC* p : { &fralex, &amigo }) {
			escribirOption(p->edit(), inicial);
			escribir(p->juego / "SiderAddons" / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "Player.bin", "WESYS....");
			CHECK(p->motor.cargar().ok());
			p->motor.elegirGrupo("grupo-prueba");
		}
		fralex.pasar(30); amigo.pasar(30);            // primera foto, respaldo diario
		CHECK(web.publicaciones == 0);
		CHECK(!sync::Motor(fralex.web, fralex.of, fralex.entorno()).respaldos().empty());

		// 1) FRALEX ficha (Julián 7: 172 → 108). Sube UNA sola vez.
		fralex.fichar(7, 172, 108);
		fralex.pasar(60);
		CHECK(web.publicaciones == 1);
		fralex.pasar(60);
		CHECK(web.publicaciones == 1);                // no se repite

		// 2) Automático: el amigo lo recibe, respalda, entrega a Link y Link coloca.
		amigo.pasar(30);
		CHECK(amigo.esta(7, 108) && !amigo.esta(7, 172));
		CHECK(web.confirmaciones["AMIGO|" + web.ops[0]["op_id"].get<std::string>()] == "aplicada");
		const auto respAmigo = amigo.motor.respaldos();
		CHECK(std::any_of(respAmigo.begin(), respAmigo.end(), [](const auto& r) { return r.motivo == "antes de aplicar cambios del grupo"; }));
		// 3) Anti-bucle: el amigo NO vuelve a subirlo, y nadie hace ping-pong.
		amigo.pasar(90); fralex.pasar(90); amigo.pasar(90);
		CHECK(web.publicaciones == 1);

		// 4) En el otro sentido: el amigo ficha (jugador 9: 173 → 50) y FRALEX lo recibe.
		amigo.fichar(9, 173, 50);
		amigo.pasar(60);
		CHECK(web.publicaciones == 2);
		fralex.pasar(30);
		CHECK(fralex.esta(9, 50));
		fralex.pasar(90); amigo.pasar(90);
		CHECK(web.publicaciones == 2);

		// 5) Choque: los dos fichan al MISMO jugador a la vez a clubes distintos.
		fralex.fichar(1, 108, 172);
		amigo.fichar(1, 108, 173);
		fralex.pasar(30); amigo.pasar(30);
		CHECK(web.publicaciones == 4);
		fralex.pasar(30); amigo.pasar(30);
		CHECK(fralex.esta(1, 172) && amigo.esta(1, 173));   // cada uno conserva el suyo: nada se pisó
		int confl = 0;
		for (const auto& x : fralex.motor.estado().entrantes) if (x.estado == "conflicto") confl++;
		for (const auto& x : amigo.motor.estado().entrantes) if (x.estado == "conflicto") confl++;
		CHECK(confl == 2);

		// 6) Interruptor en «autorización»: queda pendiente hasta pulsar Aplicar.
		web.modo = "autorizacion";
		fralex.pasar(40); amigo.pasar(40);
		fralex.fichar(2, 108, 50);
		fralex.pasar(40);
		amigo.pasar(40);
		CHECK(!amigo.esta(2, 50));
		bool pend = false; for (const auto& x : amigo.motor.estado().entrantes) if (x.estado == "pendiente") pend = true;
		CHECK(pend);
		auto ap = amigo.motor.aplicar();
		CHECK(ap.ok() && *ap.valor == 1);
		amigo.linkColoca();
		CHECK(amigo.esta(2, 50));
		// Aplicar otra vez la misma: no duplica.
		CHECK(amigo.motor.aplicar().ok());

		// 7) Rechazar.
		fralex.fichar(3, 108, 172);
		fralex.pasar(40); amigo.pasar(40);
		std::string idPend; for (const auto& x : amigo.motor.estado().entrantes) if (x.estado == "pendiente") idPend = x.op.id;
		CHECK(!idPend.empty() && amigo.motor.rechazar(idPend).ok());
		amigo.pasar(30);
		CHECK(!amigo.esta(3, 172));

		// 8) Web caída → actúa con autorización aunque la web dijera «automático».
		web.modo = "automatico";
		amigo.pasar(40);
		CHECK(amigo.motor.resumen()["modo"] == "automatico");
		web.caida = true;
		amigo.pasar(40);
		CHECK(amigo.motor.resumen()["modo"] == "autorizacion");
		web.caida = false;

		// 9) Deshacer último cambio del grupo (vuelve el option file previo, sin publicarse).
		web.modo = "automatico";
		amigo.pasar(40);
		fralex.fichar(4, 50, 173);
		fralex.pasar(40); amigo.pasar(40);
		CHECK(amigo.esta(4, 173));
		const int pubAntes = web.publicaciones;
		auto dz = amigo.motor.deshacerUltimo();
		CHECK(dz.ok());
		amigo.linkColoca();
		CHECK(!amigo.esta(4, 173) && amigo.esta(4, 50));
		amigo.pasar(60);
		CHECK(web.publicaciones == pubAntes);   // deshacer no se reparte al grupo

		// 10) Parche distinto: no se aplica.
		escribirOption(amigo.edit(), amigo.of.leer(deRuta(amigo.edit())).valor->plantillas, "OTRO-PARCHE");
		fs::last_write_time(amigo.edit(), fs::last_write_time(amigo.edit()) + std::chrono::seconds(9));
		amigo.pasar(30);
		fralex.fichar(5, 50, 108);
		fralex.pasar(40); amigo.pasar(40);
		CHECK(!amigo.esta(5, 108));
		bool inc = false; for (const auto& x : amigo.motor.estado().entrantes) if (x.estado == "incompatible") inc = true;
		CHECK(inc);

		// 11) Estado guardado y recargado (reinicio de Sync).
		sync::Motor otro(amigo.web, amigo.of, amigo.entorno());
		CHECK(otro.cargar().ok() && otro.estado().grupoId == "grupo-prueba" && otro.estado().ultimaSeq == (int64_t)web.ops.size());
		CHECK(otro.resumen()["modo"] == "autorizacion");   // hasta leer la web: lo seguro
	}

	// ---------------------------------------------------------------------------------
	std::printf("Archivo a medias y web inexistente\n");
	{
		WebFalsa web; web.existe = false;
		PC pc(tmp / "pcs2", "SOLO", web);
		escribirOption(pc.edit(), { { 1, { 1 } }, { 2, {} } });
		pc.motor.cargar(); pc.motor.elegirGrupo("g");
		pc.pasar(30);
		pc.fichar(1, 1, 2);
		pc.pasar(3);
		escribir(pc.edit(), "{a medias");     // se está escribiendo
		pc.pasar(30);                          // ilegible: no sube nada, no se rompe
		CHECK(web.publicaciones == 0);
		escribirOption(pc.edit(), { { 1, {} }, { 2, { 1 } } });
		fs::last_write_time(pc.edit(), fs::last_write_time(pc.edit()) + std::chrono::seconds(30));
		pc.pasar(30);
		CHECK(pc.motor.estado().porPublicar.size() == 1);   // detectado; espera a que exista la ruta
		CHECK(pc.motor.resumen()["modo"] == "autorizacion");
		web.existe = true;
		pc.pasar(400);
		CHECK(web.publicaciones == 1 && pc.motor.estado().porPublicar.empty());
	}


	// ---------------------------------------------------------------------------------
	std::printf("PlayerAssignment.bin (base para «Activar»)\n");
	{
		// Barça 108: 3 jugadores (dorsales 10, 19, 28); Madrid 109: Vinícius (117047, dorsal 7, capitán)
		const std::string pa = hacerPA({ { 1, 108, 10, 0, 0 }, { 2, 108, 19, 1, 0 }, { 3, 108, 28, 2, 0 }, { 117047, 109, 7, 0, 0x20 }, { 5, 109, 9, 1, 0 } });
		auto a = base::Asignaciones::desdeBytes(std::vector<uint8_t>(pa.begin(), pa.end()));
		CHECK(a.ok() && a.valor->filas().size() == 5);
		if (a.ok()) {
			auto pl = a.valor->plantillas();
			CHECK(pl[108] == (std::vector<uint32_t>{ 1, 2, 3 }) && pl[109] == (std::vector<uint32_t>{ 117047, 5 }));
			grupo::Operacion op; op.id = "v"; op.jugador = 117047; op.equipoOrigen = 109; op.equipoDestino = 108; op.dorsal = 19;   // 19 ocupado
			auto r = base::aplicarOperaciones(*a.valor, { op });
			CHECK(r[0].estado == "aplicada" && r[0].motivo.empty());
			pl = a.valor->plantillas();
			CHECK(pl[108].back() == 117047 && pl[109] == (std::vector<uint32_t>{ 5 }));
			for (const auto& f : a.valor->filas()) {
				if (f.jugador == 117047) CHECK(f.equipo == 108 && f.orden == 3 && f.dorsal == 99 && f.banderas == 0 && f.indice == 4);
				if (f.jugador == 5) CHECK(f.orden == 0);   // subió un puesto en el Madrid
			}
			CHECK(base::aplicarOperaciones(*a.valor, { op })[0].motivo.size() > 0);   // idempotente: ya estaba
			grupo::Operacion q = op; q.id = "q"; q.tipo = "quitar"; q.equipoOrigen = 108; q.equipoDestino = 0;
			CHECK(base::aplicarOperaciones(*a.valor, { q })[0].estado == "conflicto");
			auto b = a.valor->bytes();
			CHECK(b.ok());
			if (b.ok()) {
				CHECK(std::string(b.valor->begin(), b.valor->begin() + 8) == pa.substr(0, 8));
				auto re = base::Asignaciones::desdeBytes(*b.valor);
				CHECK(re.ok() && re.valor->plantillas() == a.valor->plantillas());
			}
			// Plantilla llena
			std::vector<FilaPA> llena; for (uint32_t i = 0; i < 40; i++) llena.push_back({ 1000 + i, 200, uint8_t(i + 1), uint8_t(i), 0 });
			llena.push_back({ 77, 201, 5, 0, 0 });
			const std::string pl2 = hacerPA(llena);
			auto a2 = base::Asignaciones::desdeBytes(std::vector<uint8_t>(pl2.begin(), pl2.end()));
			CHECK(a2.ok() && !a2.valor->mover(77, 200, 201, 0).ok());
		}
		CHECK(!base::Asignaciones::desdeBytes({ 1, 2, 3 }).ok());
	}

	// ---------------------------------------------------------------------------------
	std::printf("Fichaje que entra con «Activar» (base en la entrega)\n");
	{
		WebFalsa web;
		const grupo::Plantillas inicial = { { 108, { 1, 2, 3 } }, { 109, { 117047, 5 } } };
		PC fralex(tmp / "pcs3", "FRALEX", web), amigo(tmp / "pcs3", "AMIGO", web);
		const fs::path pesdb = fs::path("SiderAddons") / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb";
		for (PC* p : { &fralex, &amigo }) {
			escribirOption(p->edit(), inicial);
			escribir(p->juego / pesdb / "Player.bin", "WESYS....");
			escribir(p->juego / pesdb / "PlayerAssignment.bin", hacerPA({ { 1, 108, 10, 0, 0 }, { 2, 108, 19, 1, 0 }, { 3, 108, 28, 2, 0 }, { 117047, 109, 7, 0, 0x20 }, { 5, 109, 9, 1, 0 } }));
			p->motor.cargar(); p->motor.elegirGrupo("g");
		}
		// Sin el interruptor: la entrega NO lleva PlayerAssignment (Link actual la rechazaría).
		fralex.pasar(30); amigo.pasar(30);
		fralex.fichar(117047, 109, 108);
		fralex.pasar(40); 
		const std::string antes = leer(amigo.juego / pesdb / "PlayerAssignment.bin");
		amigo.pasar(30);
		CHECK(amigo.esta(117047, 108));
		CHECK(leer(amigo.juego / pesdb / "PlayerAssignment.bin") == antes);
		// Con el interruptor: el siguiente fichaje va también a la base.
		amigo.motor.ponerBaseEnEntrega(true);
		fralex.fichar(5, 109, 108);
		fralex.pasar(40); amigo.pasar(30);
		CHECK(amigo.esta(5, 108));
		auto pa = base::Asignaciones::abrir(deRuta(amigo.juego / pesdb / "PlayerAssignment.bin"));
		CHECK(pa.ok());
		if (pa.ok()) {
			auto pl = pa.valor->plantillas();
			CHECK(std::find(pl[108].begin(), pl[108].end(), 5u) != pl[108].end());
			// El primero (antes del interruptor) no estaba en la base: queda donde estaba (es lo esperado).
			CHECK(std::find(pl[109].begin(), pl[109].end(), 117047u) != pl[109].end());
		}
		// Respaldo con PlayerAssignment y deshacer que lo devuelve.
		bool conPA = false;
		for (const auto& r : amigo.motor.respaldos()) for (const auto& f : r.archivos) if (f.relativo.find("PlayerAssignment.bin") != std::string::npos) conPA = true;
		CHECK(conPA);
		CHECK(amigo.motor.deshacerUltimo().ok());
		amigo.linkColoca();
		auto pa2 = base::Asignaciones::abrir(deRuta(amigo.juego / pesdb / "PlayerAssignment.bin"));
		CHECK(pa2.ok() && pa2.valor->plantillas()[109] == (std::vector<uint32_t>{ 117047, 5 }));
		CHECK(!amigo.esta(5, 108));
	}

	fs::remove_all(tmp);
	std::printf("\n%d/%d pruebas OK\n", total - fallos, total);
	return fallos ? 1 : 0;
}
