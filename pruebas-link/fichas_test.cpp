// Pruebas de «estadísticas e información como cambios» (link/Fichas.cpp): campos y bits, lista, reglas de aplicación,
// convivencia con los fichajes y el flujo completo sobre juegos FALSOS en carpetas temporales. Nunca escribe en un juego real.
// Opcional: un 2.º argumento con la carpeta del juego real para leer COPIAS de su Player.bin (solo lectura).
// Compila en Windows (MSVC): ver pruebas-link/correr_fichas.bat.  Uso: fichas_test <carpeta temporal vacía> [carpeta del juego real]
#include "phoenix/link/Fichas.h"
#include "phoenix/link/FichasWeb.h"
#include "phoenix/link/Fichajes.h"
#include "phoenix/link/Entrega.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

namespace fs = std::filesystem;
using namespace phoenix::fichas;
namespace fj = phoenix::fichajes;

static int total = 0, fallos = 0;
#define CHECK(cond, msg) do { total++; if (!(cond)) { fallos++; std::cout << "  FALLO: " << msg << "\n"; } else std::cout << "  ok: " << msg << "\n"; } while (0)

static void escribir(const fs::path& p, const std::string& s) {
	fs::create_directories(p.parent_path());
	std::ofstream f(p, std::ios::binary | std::ios::trunc);
	f.write(s.data(), static_cast<std::streamsize>(s.size()));
}
static std::string leer(const fs::path& p) {
	std::ifstream f(p, std::ios::binary);
	std::stringstream ss; ss << f.rdbuf();
	return ss.str();
}

// ─── base falsa ──────────────────────────────────────────────────────────────────────────────────
// 1200 jugadores (5000…6199). Nombres «Jugador N»; 5001 = «Lamine Yamal», 5002 y 5003 = «Repetido». Último registro recortado
// 8 bytes, como el Player.bin real. Bits de relleno aleatorios (para que «solo cambian los bits pedidos» signifique algo).
static std::string playerBinFalso(size_t n = 1200) {
	std::string r(8, '\0');
	unsigned x = 7;
	for (size_t k = 0; k < n; k++) {
		std::string rec(312, '\0');
		for (char& c : rec) { x = x * 1103515245u + 12345u; c = static_cast<char>((x >> 16) & 0xFF); }
		const uint32_t id = 5000 + static_cast<uint32_t>(k);
		for (int i = 0; i < 4; i++) rec[static_cast<size_t>(i)] = static_cast<char>((id >> (8 * i)) & 0xFF);
		std::string nombre = id == 5001 ? "Lamine Yamal" : (id == 5002 || id == 5003) ? "Repetido" : "Jugador " + std::to_string(id);
		nombre.resize(61, '\0');
		rec.replace(243, 61, nombre);
		r += rec;
	}
	r.resize(r.size() - 8);
	return r;
}
static Lista lista(std::vector<Cambio> c) { Lista l; l.cambios = std::move(c); l.sha256 = std::to_string(l.cambios.size()); return l; }
static Cambio cam(uint32_t j, const std::string& campo, int valor, const std::string& nombre = "") { Cambio c; c.jugador = j; c.campo = campo; c.valor = valor; c.nombre = nombre; return c; }

static void pruebaCampos() {
	std::cout << "[campos y bits]\n";
	CHECK(campos().size() == 46, "46 campos: 20 de campo, 5 de portero, 2 pie malo, forma, lesiones, 13 posiciones, 4 básicos");
	std::set<std::string> claves; std::set<int> bitsUsados; bool solapan = false;
	for (const Campo& c : campos()) {
		claves.insert(c.clave);
		for (int i = 0; i < c.bits; i++) if (!bitsUsados.insert(c.bit + i).second) solapan = true;
	}
	CHECK(claves.size() == campos().size(), "claves únicas");
	CHECK(!solapan, "ningún campo pisa los bits de otro");
	CHECK(campo("velocidad") && campo("velocidad")->bit == 306 && campo("altura") && campo("altura")->sumar == 100 && !campo("no_existe"), "búsqueda por clave (velocidad = bit 306, mismos bits que PhoenixSync)");

	Ficha f; std::string err;
	CHECK(!f.cargar(std::string(100, 'a'), err), "Player.bin de tamaño raro se rechaza");
	CHECK(f.cargar(playerBinFalso(), err) && f.jugadores() == 1200, "carga 1200 jugadores (último registro recortado)");
	std::string entero = playerBinFalso(); entero.append(8, '\0');
	Ficha g;
	CHECK(g.cargar(entero, err) && g.jugadores() == 1200, "también acepta el último registro entero");
	CHECK(f.nombrePlegado(5001) == fj::plegarNombre("Lamine Yamal"), "lee el nombre del jugador");
	CHECK(f.buscarNombre(fj::plegarNombre("Repetido")).size() == 2, "buscar por nombre: encuentra los dos «Repetido»");
	const Ficha base = f;
	bool extremos = true;
	for (const Campo& c : campos()) {
		int leido = 0;
		if (!(f.poner(5010, c.clave, c.maximo).empty() && f.leer(5010, c.clave, leido) && leido == c.maximo)) extremos = false;
		if (!(f.poner(5010, c.clave, c.minimo).empty() && f.leer(5010, c.clave, leido) && leido == c.minimo)) extremos = false;
	}
	CHECK(extremos, "poner/leer en el mínimo y el máximo de los 46 campos");
	const auto d = f.distintos(base);
	CHECK(d.size() == 1 && d[0] == 5010, "solo cambió el registro del jugador tocado");
	CHECK(f.poner(5010, "velocidad", 100) == "fuera_de_rango" && f.poner(5010, "velocidad", 39) == "fuera_de_rango", "fuera de rango se rechaza");
	CHECK(f.poner(999, "velocidad", 80) == "jugador_no_existe" && f.poner(5010, "nada", 80) == "campo_desconocido", "jugador o campo inexistente se rechaza");
}

static void pruebaLista() {
	std::cout << "[lista]\n";
	Lista l; std::string err;
	CHECK(leerLista(R"({"version":1,"cambios":[{"id":"a1","player_id":5001,"jugador":"Lamine Yamal","campo":"velocidad","valor":95},{"jugador":"Jugador 5020","campo":"pos_dc","valor":2}]})", l, err)
		&& l.cambios.size() == 2 && l.cambios[0].jugador == 5001 && l.cambios[0].id == "a1" && l.cambios[1].jugador == 0, "lista válida (por id y por nombre)");
	CHECK(!leerLista("{roto", l, err), "JSON roto se rechaza");
	CHECK(!leerLista(R"({"version":2,"cambios":[]})", l, err), "versión desconocida se rechaza");
	CHECK(!leerLista(R"({"version":1,"cambios":[{"player_id":5001,"campo":"velocidad","valor":"95"}]})", l, err), "valor que no es entero se rechaza");
	CHECK(!leerLista(R"({"version":1,"cambios":[{"campo":"velocidad","valor":95}]})", l, err), "sin jugador se rechaza");
	CHECK(!leerLista(R"({"version":1,"cambios":[{"player_id":5001,"valor":95}]})", l, err), "sin campo se rechaza");
	CHECK(leerLista(R"({"version":1,"cambios":[]})", l, err) && l.cambios.empty(), "lista vacía vale");
}

static void pruebaAplicar() {
	std::cout << "[aplicar en memoria]\n";
	Ficha base; std::string err;
	base.cargar(playerBinFalso(), err);
	int antes = 0; base.leer(5001, "velocidad", antes);
	const int nuevo = antes == 95 ? 94 : 95;
	Resultado r = aplicar(base, lista({ cam(5001, "velocidad", nuevo, "Lamine Yamal") }));
	Ficha nueva; nueva.cargar(r.crudo, err);
	int v = 0;
	CHECK(r.error.empty() && r.aplicados.size() == 1 && nueva.leer(5001, "velocidad", v) && v == nuevo, "cambia la velocidad");
	CHECK(r.aplicados[0].antes == antes && r.cambiados == 1, "guarda el valor de antes (para el historial)");
	r = aplicar(base, lista({ cam(5001, "velocidad", antes) }));
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].igual && r.cambiados == 0 && r.crudo == base.crudo(), "mismo valor que la base: no cambia nada");
	r = aplicar(base, lista({ cam(5001, "velocidad", 80), cam(5001, "velocidad", 90) }));
	nueva.cargar(r.crudo, err);
	CHECK(r.aplicados.size() == 1 && nueva.leer(5001, "velocidad", v) && v == 90 && r.aplicados[0].antes == antes, "dos cambios del mismo campo: gana el último");
	r = aplicar(base, lista({ cam(0, "finalizacion", 99, "Jugador 5100") }));
	nueva.cargar(r.crudo, err);
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].jugador == 5100 && nueva.leer(5100, "finalizacion", v) && v == 99, "por nombre: encuentra al jugador");
	r = aplicar(base, lista({ cam(0, "finalizacion", 99, "Repetido") }));
	CHECK(r.aplicados.empty() && r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_ambiguo", "nombre repetido: ambiguo, no se toca");
	r = aplicar(base, lista({ cam(0, "finalizacion", 99, "Nadie") }));
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_no_encontrado", "nombre que no está: se salta");
	r = aplicar(base, lista({ cam(5030, "velocidad", 90, "Lamine Yamal") }));
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].jugador == 5001, "id de otro jugador en este parche: se usa el nombre (Lamine es 5001 aquí)");
	r = aplicar(base, lista({ cam(5030, "velocidad", 90, "Nadie Conocido") }));
	CHECK(r.aplicados.empty() && r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_distinto" && r.crudo == base.crudo(), "id reusado y nombre que no está: no se toca al jugador equivocado");
	r = aplicar(base, lista({ cam(99999, "velocidad", 90, "Lamine Yamal") }));
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].jugador == 5001, "id que no existe en este parche + nombre: lo encuentra por nombre");
	r = aplicar(base, lista({ cam(99999, "velocidad", 90) }));
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_no_existe", "id que no existe y sin nombre: se salta");
	r = aplicar(base, lista({ cam(5001, "velocidad", 120), cam(5001, "altura", 230), cam(5001, "magia", 5) }));
	CHECK(r.aplicados.empty() && r.saltados.size() == 3 && r.saltados[0].codigo == "fuera_de_rango" && r.saltados[2].codigo == "campo_desconocido", "fuera de rango y campo desconocido: se saltan");
	std::vector<Cambio> muchos;
	for (const Campo& c : campos()) muchos.push_back(cam(5050, c.clave, c.minimo));
	muchos.push_back(cam(5060, "pie_malo_uso", 4));
	r = aplicar(base, lista(muchos));
	nueva.cargar(r.crudo, err);
	const auto d = nueva.distintos(base);
	CHECK(r.error.empty() && r.aplicados.size() == campos().size() + 1, "todos los campos a la vez pasan la comprobación de bits");
	bool soloEsos = true; for (uint32_t id : d) if (id != 5050 && id != 5060) soloEsos = false;
	CHECK(soloEsos && !d.empty(), "solo cambian los registros de los jugadores pedidos");
}

// ─── juego falso ─────────────────────────────────────────────────────────────────────────────────
static const std::string OLMOS = "cpk.root = \".\\olmosjr23\\Database\"\r\n";
static const std::string MODS = "\r\nlua.module = \"CGP_data.lua\"\r\n";
static fs::path pesdb(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb"; }

static std::string asignacionFalsa() {
	std::vector<fj::Fila> f;
	uint32_t id = 1;
	for (uint32_t k = 0; k < 1100; k++) f.push_back({ id++, 5000 + k, 1000 + k / 25, ((k % 25) << 10) | (k % 25 + 1) });
	return fj::escribirAsignacion(f);
}
static void escribirBase(const fs::path& sider, const std::string& player) {
	const fs::path d = sider / "olmosjr23" / "Database" / "common" / "etc" / "pesdb";
	escribir(d / "PlayerAssignment.bin", fj::escribirBin(asignacionFalsa()));
	escribir(d / "Player.bin", fj::escribirBin(player));
}
static int leerCampo(const fs::path& archivo, uint32_t jugador, const char* clave) {
	std::string crudo, e; Ficha f; int v = -1;
	if (fj::leerBin(leer(archivo), crudo, e) && f.cargar(crudo, e)) f.leer(jugador, clave, v);
	return v;
}
static int contarCopias(const fs::path& phx) {
	int n = 0; std::error_code ec;
	for (fs::directory_iterator it(phx, ec), fin; !ec && it != fin; it.increment(ec)) if (it->path().filename().string().rfind("_antes-de-fichas-", 0) == 0) n++;
	return n;
}

static void pruebaJuegoFalso(const fs::path& tmp) {
	std::cout << "[juego falso: aplicar, idempotencia, deshacer, convivencia con fichajes, quitar]\n";
	const fs::path juego = tmp / "juego";
	escribir(juego / "PES2021.exe", "MZ");
	const fs::path raiz = juego / "SiderAddons";
	escribir(raiz / "sider.ini", OLMOS + MODS);
	const std::string player = playerBinFalso();
	escribirBase(raiz, player);
	// un modo del cambiador con OTRO parche: allí Lamine es el 5030 y el 5001 es «Fulano»
	const fs::path modo = juego / "ConmeGol Extras" / "Otro Parche" / "SiderAddons";
	escribir(modo / "sider.ini", OLMOS + MODS);
	std::string otro = player;
	{ std::string n1(61, '\0'), n2(61, '\0'); n1.replace(0, 12, "Lamine Yamal"); n2.replace(0, 6, "Fulano");
	  otro.replace(8 + 30 * 312 + 243, 61, n1); otro.replace(8 + 1 * 312 + 243, 61, n2); }
	escribirBase(modo, otro);

	Ficha base; std::string err; base.cargar(player, err);
	int velBase = 0; base.leer(5001, "velocidad", velBase);
	int vel30 = 0; base.leer(5030, "velocidad", vel30);
	const int nuevo = (velBase == 95 || vel30 == 95) ? (velBase == 94 || vel30 == 94 ? 93 : 94) : 95;
	const Lista l = lista({ cam(5001, "velocidad", nuevo, "Lamine Yamal"), cam(5100, "altura", 190), cam(5200, "pos_dc", 2) });

	Informe inf = aplicarAlJuego(juego, lista({}));
	CHECK(inf.ok && !fs::exists(pesdb(raiz) / "Player.bin") && !fs::exists(pesdb(modo) / "Player.bin"), "lista vacía: no escribe nada");
	auto an = analizar(juego, l);
	CHECK(an.size() == 2 && an[0].baseOk && an[1].baseOk && an[0].estado == "sin_aplicar", "análisis: los dos sitios tienen base, aún sin aplicar");
	CHECK(!fs::exists(pesdb(raiz) / "Player.bin"), "el análisis no escribió nada");

	inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok && inf.escritos == 2, "aplicar: escrito en la raíz y en el modo (" + inf.mensaje + ")");
	CHECK(leerCampo(pesdb(raiz) / "Player.bin", 5001, "velocidad") == nuevo, "raíz: Lamine (5001) con la velocidad nueva");
	CHECK(leerCampo(pesdb(modo) / "Player.bin", 5030, "velocidad") == nuevo, "otro parche: Lamine es 5030 y recibe el cambio");
	CHECK(leerCampo(pesdb(modo) / "Player.bin", 5001, "velocidad") == velBase, "otro parche: el 5001 (Fulano) NO se toca");
	CHECK(leerCampo(pesdb(raiz) / "Player.bin", 5100, "altura") == 190 && leerCampo(pesdb(raiz) / "Player.bin", 5200, "pos_dc") == 2, "altura y posición puestas");
	const std::string ini = leer(raiz / "sider.ini");
	CHECK(ini.find("Phoenix-DB") != std::string::npos && ini.find("Phoenix-DB") < ini.find("olmosjr23"), "sider.ini: Phoenix-DB por delante de la base");
	const std::string estado = leer(raiz / "livecpk" / "Phoenix-DB" / "fichas_aplicadas.json");
	CHECK(estado.find("\"antes\"") != std::string::npos && estado.find("salidaSha") != std::string::npos, "fichas_aplicadas.json con antes/después y huella");
	CHECK(analizar(juego, l)[0].estado == "aplicado", "estado: aplicado");
	const std::string bytes1 = leer(pesdb(raiz) / "Player.bin");
	inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok && inf.escritos == 0 && leer(pesdb(raiz) / "Player.bin") == bytes1, "aplicar otra vez: no reescribe (mismos bytes)");

	// los fichajes NO apartan nuestro Player.bin (no es «ajeno»)
	fj::Lista lf; lf.cambios.push_back({ 5000, 1003, 0, "" }); lf.sha256 = "x";
	const auto anF = fj::analizar(juego, lf);
	CHECK(anF.size() == 2 && !anF[0].conPlayerBinAjeno && !anF[1].conPlayerBinAjeno, "fichajes: el Player.bin de las estadísticas no cuenta como ajeno");
	const fj::Informe infF = fj::aplicarAlJuego(juego, lf);
	CHECK(infF.ok && infF.escritos == 2 && leer(pesdb(raiz) / "Player.bin") == bytes1, "fichajes aplicados: el Player.bin de las estadísticas sigue ahí, intacto");

	// deshacer = quitar el cambio de la lista → vuelve el valor de la base
	inf = aplicarAlJuego(juego, lista({ cam(5100, "altura", 190), cam(5200, "pos_dc", 2) }));
	CHECK(inf.ok && leerCampo(pesdb(raiz) / "Player.bin", 5001, "velocidad") == velBase && leerCampo(pesdb(raiz) / "Player.bin", 5100, "altura") == 190, "deshacer un cambio: Lamine vuelve a su velocidad, la altura se queda");
	CHECK(contarCopias(raiz / "livecpk" / "Phoenix-DB") >= 1, "hay copia del Player.bin anterior");
	// sin ningún cambio efectivo → se retira y el juego vuelve a la base
	int alturaBase = 0; base.leer(5100, "altura", alturaBase);
	int dcBase = 0; base.leer(5200, "pos_dc", dcBase);
	inf = aplicarAlJuego(juego, lista({ cam(5100, "altura", alturaBase), cam(5200, "pos_dc", dcBase) }));
	CHECK(inf.ok && inf.retirados == 2 && !fs::exists(pesdb(raiz) / "Player.bin") && !fs::exists(pesdb(modo) / "Player.bin"), "todo igual que la base: se retira el Player.bin (" + inf.mensaje + ")");
	CHECK(fs::exists(pesdb(raiz) / "PlayerAssignment.bin"), "los fichajes siguen ahí");

	// copias viejas: se quedan solo las 3 más nuevas
	for (int k = 0; k < 5; k++) fs::create_directories(raiz / "livecpk" / "Phoenix-DB" / ("_antes-de-fichas-2000010" + std::to_string(k) + "-000000"));
	inf = aplicarAlJuego(juego, lista({ cam(5100, "altura", 171) }));
	CHECK(inf.ok && contarCopias(raiz / "livecpk" / "Phoenix-DB") == 3, "las copias viejas se podan (quedan 3)");

	// alguien cambió nuestro Player.bin → estado «cambiado», quitar no lo borra
	escribir(pesdb(raiz) / "Player.bin", fj::escribirBin(playerBinFalso(1100)));
	CHECK(analizar(juego, lista({ cam(5100, "altura", 199) }))[0].estado == "cambiado", "Player.bin cambiado por otra herramienta: estado «cambiado»");
	inf = quitarDelJuego(juego);
	CHECK(inf.ok && fs::exists(pesdb(raiz) / "Player.bin") && !fs::exists(pesdb(modo) / "Player.bin"), "quitar: no borra el ajeno de la raíz; quita el nuestro del modo");
	inf = aplicarAlJuego(juego, lista({ cam(5100, "altura", 199) }));
	CHECK(inf.ok && leerCampo(pesdb(raiz) / "Player.bin", 5100, "altura") == 199, "aplicar sobre uno ajeno: lo reemplaza (con copia)");
	inf = quitarDelJuego(juego);
	CHECK(inf.ok && !fs::exists(pesdb(raiz) / "Player.bin") && !fs::exists(pesdb(modo) / "Player.bin"), "quitar: el juego vuelve a la base en los dos sitios");

	// juego sin base legible: no se crea nada
	const fs::path roto = tmp / "roto";
	escribir(roto / "PES2021.exe", "MZ");
	escribir(roto / "SiderAddons" / "sider.ini", OLMOS);
	escribir(roto / "SiderAddons" / "olmosjr23" / "Database" / "common" / "etc" / "pesdb" / "PlayerAssignment.bin", "basura");
	inf = aplicarAlJuego(roto, l);
	CHECK(inf.ok && inf.escritos == 0 && !fs::exists(pesdb(roto / "SiderAddons") / "Player.bin"), "base ilegible: no se crea nada");
	CHECK(!aplicarAlJuego(fs::path(), l).ok, "sin carpeta de juego: no hace nada");
}

static void pruebaWeb(const fs::path& tmp) {
	std::cout << "[cambios de la web (/v1/cambios) → lista → resultados del informe]\n";
	namespace w = phoenix::fichas::web;
	const std::string U1 = "11111111-1111-4111-8111-111111111111", U2 = "22222222-2222-4222-8222-222222222222", U3 = "33333333-3333-4333-8333-333333333333",
		U4 = "44444444-4444-4444-8444-444444444444", U5 = "55555555-5555-4555-8555-555555555555";
	const std::string p1 = R"({"ok":true,"ultimo_seq":5,"cambios":[
		{"seq":1,"id":")" + U1 + R"(","version":1,"tipo":"stats","destino":"base","estado":"activo","datos":{"jugador_id":5001,"jugador":"Lamine Yamal","valores":{"velocidad":85,"finalizacion":88}}},
		{"seq":2,"id":"aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa","version":1,"tipo":"fichaje","destino":"base","estado":"activo","datos":{}},
		{"seq":3,"id":")" + U2 + R"(","version":1,"tipo":"info","destino":"lm","estado":"activo","datos":{"jugador":"Lamine Yamal","valores":{"altura":200}}},
		{"seq":4,"id":")" + U3 + R"(","version":1,"tipo":"stats","destino":"ambos","estado":"activo","datos":{"jugador_id":5001,"jugador":"Lamine Yamal","valores":{"velocidad":95}}},
		{"seq":5,"id":")" + U4 + R"(","version":1,"tipo":"info","destino":"base","estado":"activo","datos":{"jugador_id":90012345,"jugador":"Jugador 5100","valores":{"altura":190,"pos_dc":2}}}]})";
	w::Almacen a; std::string e;
	CHECK(w::leerPagina(p1, a, e) == 5 && a.ultimoSeq == 5 && a.cambios.size() == 3, "página: se quedan solo stats/info para la base (no fichajes ni solo-LM)");
	CHECK(w::leerPagina("{roto", a, e) < 0 && a.cambios.size() == 3, "respuesta rota: no toca lo que había");
	Lista l = w::listaDesde(a);
	int vel = 0; bool porNombre = false;
	for (const Cambio& c : l.cambios) { if (c.campo == "velocidad") vel = c.valor; if (c.campo == "altura" && c.jugador == 0 && c.nombre == "Jugador 5100") porNombre = true; }
	CHECK(l.cambios.size() == 4 && vel == 95, "lista: el último cambio de velocidad gana (95)");
	CHECK(porNombre, "id sintético (≥ 90.000.000): se aplica por nombre");
	// deshacer en la web = un cambio nuevo con null → vuelve al valor de la base
	const std::string p2 = R"({"ok":true,"cambios":[{"seq":7,"id":")" + U5 + R"(","version":1,"tipo":"info","destino":"base","estado":"activo","datos":{"jugador":"Jugador 5100","jugador_id":90012345,"valores":{"pos_dc":null},"deshace":")" + U4 + R"("}}]})";
	CHECK(w::leerPagina(p2, a, e) == 1 && a.ultimoSeq == 7, "página 2: llega el deshacer");
	l = w::listaDesde(a);
	bool quedaDc = false; for (const Cambio& c : l.cambios) if (c.campo == "pos_dc") quedaDc = true;
	CHECK(!quedaDc && l.cambios.size() == 3, "deshacer: pos_dc sale de la lista (vuelve al de la base)");
	w::Almacen b;
	CHECK(w::leer(w::guardar(a), b) && b.ultimoSeq == 7 && b.cambios.size() == a.cambios.size(), "guardar/leer el almacén: ida y vuelta");

	// sobre un juego falso
	const fs::path juego = tmp / "juego-web";
	escribir(juego / "PES2021.exe", "MZ");
	escribir(juego / "SiderAddons" / "sider.ini", OLMOS + MODS);
	escribirBase(juego / "SiderAddons", playerBinFalso());
	Informe inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok && leerCampo(pesdb(juego / "SiderAddons") / "Player.bin", 5001, "velocidad") == 95 && leerCampo(pesdb(juego / "SiderAddons") / "Player.bin", 5100, "altura") == 190, "aplicar la lista de la web en el juego falso");
	auto rs = w::resultados(a, inf.objetivos, false);
	std::map<std::string, std::string> res, mot;
	for (const auto& r : rs) { res[r.cambioId] = r.resultado; mot[r.cambioId] = r.motivo; }
	CHECK(res[U3] == "aplicado" && mot[U3].find("velocidad") != std::string::npos && mot[U3].find("→95") != std::string::npos, "informe: el de velocidad 95, aplicado con «antes→después» (" + mot[U3] + ")");
	CHECK(res[U1] == "aplicado" && mot[U1].find("finalizacion") != std::string::npos, "informe: el primero sigue aplicado por su finalización");
	CHECK(res[U4] == "aplicado" && res[U5] == "aplicado", "informe: el cambio deshecho y su deshacer");
	rs = w::resultados(a, inf.objetivos, true, "PES está abierto");
	bool todosPend = true; for (const auto& r : rs) if (r.resultado != "pendiente") todosPend = false;
	CHECK(todosPend, "PES abierto: todo «pendiente»");
	// un cambio que otro posterior pisa entero → «reemplazado»
	w::Almacen c;
	w::leerPagina(R"({"cambios":[{"seq":1,"id":")" + U1 + R"(","tipo":"stats","destino":"base","estado":"activo","datos":{"jugador_id":5001,"jugador":"Lamine Yamal","valores":{"velocidad":70}}},{"seq":2,"id":")" + U2 + R"(","tipo":"stats","destino":"base","estado":"activo","datos":{"jugador_id":5001,"jugador":"Lamine Yamal","valores":{"velocidad":71}}}]})", c, e);
	inf = aplicarAlJuego(juego, w::listaDesde(c));
	rs = w::resultados(c, inf.objetivos, false);
	CHECK(rs.size() == 2 && rs[0].resultado == "reemplazado" && rs[1].resultado == "aplicado", "informe: pisado entero → reemplazado");
	// un nombre que no existe en esta base → saltado
	w::Almacen d;
	w::leerPagina(R"({"cambios":[{"seq":9,"id":")" + U1 + R"(","tipo":"stats","destino":"base","estado":"activo","datos":{"jugador":"Nadie Conocido","valores":{"velocidad":80}}}]})", d, e);
	inf = aplicarAlJuego(juego, w::listaDesde(d));
	rs = w::resultados(d, analizar(juego, w::listaDesde(d)), false);
	CHECK(rs.size() == 1 && rs[0].resultado == "saltado" && rs[0].motivo.find("jugador_no_encontrado") != std::string::npos, "informe: jugador que no está en esta base → saltado con motivo");
}

static void pruebaCopiaReal(const fs::path& real) {
	std::cout << "[COPIA en memoria del Player.bin del juego real (solo lectura)]\n";
	for (const fj::Objetivo& o : fj::objetivos(real)) {
		const fj::BaseJugadores b = fj::leerPlayerBinDeBase(real, o);
		if (!b.ok) { std::cout << "  (" << o.nombre << ": " << b.error << ")\n"; continue; }
		Ficha f; std::string e;
		CHECK(f.cargar(b.crudo, e), o.nombre + ": Player.bin real se carga (" + std::to_string(f.jugadores()) + " jugadores)");
		int v = 0;
		if (f.leer(162114, "velocidad", v)) std::cout << "  " << o.nombre << ": 162114 velocidad = " << v << ", nombre = " << f.nombrePlegado(162114) << "\n";
		const Resultado r = aplicar(f, lista({ cam(162114, "velocidad", 99, "Lamine Yamal") }));
		CHECK(r.error.empty(), o.nombre + ": un cambio sobre la copia pasa la comprobación de bits (" + std::to_string(r.aplicados.size()) + " aplicado)");
	}
}

int main(int argc, char** argv) {
	if (argc < 2) { std::cout << "uso: fichas_test <carpeta temporal vacía> [carpeta del juego real]\n"; return 2; }
	const fs::path tmp = fs::path(argv[1]);
	fs::create_directories(tmp);
	pruebaCampos();
	pruebaLista();
	pruebaAplicar();
	pruebaJuegoFalso(tmp);
	pruebaWeb(tmp);
	if (argc > 2) pruebaCopiaReal(phoenix::entrega::deU8(argv[2]));
	std::cout << "\n" << (total - fallos) << "/" << total << " pruebas OK\n";
	return fallos == 0 ? 0 : 1;
}
