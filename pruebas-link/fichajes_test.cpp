// Pruebas de «fichajes como cambios» (link/Fichajes.cpp): formatos (zlib, .bin de PES, CPK), lista de cambios, reglas de
// aplicación, y el flujo completo sobre juegos FALSOS en carpetas temporales. Nunca escribe en un juego real.
// Opcional: un 3er argumento con la carpeta del juego real (p. ej. D:\Frank\Games_\Conmegol Patch) para probar con COPIAS
// de sus bases (solo lectura del juego real; todo se copia a la carpeta temporal).
// Compila en Windows (MSVC): ver pruebas-link/correr_fichajes.bat.  Uso: fichajes_test <carpeta temporal vacía> [carpeta del juego real]
#include "phoenix/link/Fichajes.h"
#include "phoenix/link/ModulosJuego.h"
#include "phoenix/link/Entrega.h"

#include <algorithm>`n#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace fs = std::filesystem;
using namespace phoenix::fichajes;
using phoenix::entrega::deU8;

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
static std::string deHex(const std::string& h) {
	std::string r;
	for (size_t i = 0; i + 1 < h.size(); i += 2) r.push_back(static_cast<char>(std::stoi(h.substr(i, 2), nullptr, 16)));
	return r;
}
static int contar(const std::string& t, const std::string& frag) {
	int n = 0; size_t i = 0;
	while ((i = t.find(frag, i)) != std::string::npos) { n++; i += frag.size(); }
	return n;
}
static bool igualFilas(const std::vector<Fila>& a, const std::vector<Fila>& b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); i++) if (a[i].id != b[i].id || a[i].jugador != b[i].jugador || a[i].equipo != b[i].equipo || a[i].empaq != b[i].empaq) return false;
	return true;
}

// ─── base falsa ──────────────────────────────────────────────────────────────────────────────────
// 50 equipos (1000…1049) con 30 jugadores; el 1049 con 40 (lleno). Jugadores 5000… en equipos; 100000… libres. Un jugador (7000) en dos equipos.
struct BaseFalsa { std::vector<Fila> filas; std::set<uint32_t> jugadores; };
static BaseFalsa baseFalsa() {
	BaseFalsa b;
	uint32_t id = 1, jug = 5000;
	for (uint32_t e = 1000; e < 1050; e++) {
		const int n = e == 1049 ? 40 : 30;
		for (int k = 0; k < n; k++) { b.filas.push_back({ id++, jug, e, (static_cast<uint32_t>(k) << 10) | static_cast<uint32_t>(k + 1) }); b.jugadores.insert(jug); jug++; }
	}
	for (uint32_t j = 100000; j < 100200; j++) b.jugadores.insert(j);   // sin equipo
	b.jugadores.insert(7000);
	b.filas.push_back({ id++, 7000, 1001, (30u << 10) | 99u });
	b.filas.push_back({ id++, 7000, 1002, (30u << 10) | 98u });
	return b;
}
static std::string playerBinFalso(const std::set<uint32_t>& ids) {
	std::string r(8, '\0');
	for (uint32_t j : ids) { std::string rec(312, '\0'); for (int i = 0; i < 4; i++) rec[static_cast<size_t>(i)] = static_cast<char>((j >> (8 * i)) & 0xFF); r += rec; }
	return r;
}
static Lista lista(std::vector<Cambio> c, bool estricta = false) { Lista l; l.cambios = std::move(c); l.estricta = estricta; return l; }

static const Fila* fila(const std::vector<Fila>& v, uint32_t jug, uint32_t eq) {
	for (const Fila& f : v) if (f.jugador == jug && f.equipo == eq) return &f;
	return nullptr;
}
static int enEquipo(const std::vector<Fila>& v, uint32_t eq) { int n = 0; for (const Fila& f : v) if (f.equipo == eq) n++; return n; }

static void pruebaFormatos() {
	std::cout << "[formatos]\n";
	// zlib real (nivel 9, huffman dinámico) generado con zlib de Python
	const std::string dyn = deHex("78da25965762ed300805ffb52b4082fdefc833f89524379629a780cea97373fadeac1767e655f48d39e73c3e67d63d1c68be569c775f4ee5c93399c183c97be7be3e197d1eafdc36ce4cdc2246573767f8e1f423f43951393375e2c4d4eb4e1ef1fdbe9343b6ec32fe99c7c9e638259186a0bcf82883ac37f830c1797e2039398b179be7c197081e3d8a2f721e82bdc9b2582aa6dca487e869c264d280796fbce140f35ebd247e96889c6da17db5a8185c0ee169241ef0b47d531767ac3c48f15254e602e349112d0e06602d10b4cf23d20f87f2f650eabbd302d7803bc587a262e283c7a1c51bdd43f9e4e6b7227bef252a8f010c5c1eadf1944f41b543059c963e3213125c8181da299b64a2460f44e063c505002a0aa99e0708b16d501ced791ab0409c60a04bb2816392f4164f947b7f4e890ff2e4ea951070921e59bd213c4dcb281c9be8522ca9268a94f04b87b481cac6d6151b4128e73c61a317e97fe02a428f1822db2d040bdb1301ba01f87cca2cac29e2d22d75f3971303123f15af72a17b928a66e7020b5fd4f78551fb0616456b71c96f9ecd4078935556aea710703e59a7d55629f66ac5e8462b9d2242353ca822ce108aae61bee55f75b6fe32bcaabb063206ec0efab35441c8356081bfe5d1c9116d9e0c823cfe8e37902ef642f5170f5cc8273d18dc553120940e1da1372b84d223c03e5d2fc5a1dd20133f6a25394d4918354860b1c81c9b3f8ac227900d51b54624a4251917b1f2bd5abbd0ab6f6a4091abd6a2144b2aec0af8385dab4301a2e51b2fdbb9d345d5d299a301a5cae38aebace846ad330b1042aa70304b717b8eae5c07c06facf6539b3053061eac5352298e6e20c3c4ab75a3aa5a0d85ded1850593482c7a757f6265637d14b4fc21dd4b1dc114d318a54b2fc30d40740350d2459a0b30a48e81466bbcc7690619e890e5ac7b290e761895aa94d79c4bc5f8e2a8c60f74e3bb9cb72e8e51a7518a4986be281da4901081e8b51c3fce3cf2a3535800ae56af2bb0c521b4eaedff15a596bf3f619cb6f5023538fb9f92fc8d810d6a94328482a31308f6ceb4d356bbe13ed11ad593629aae15aae48cb0a78dd77a4adb331cc08989c42c453ebcde32911258dbe53131a43e8b6ab9774e041d892edbaa1dddc87cd11fe73aaa183d95868f1d6f68e8bad518c1a5cfa49a3e9c42bdb30bf1f459ec52d33ab15193a6010d25db29b74818792dd6bb1e9fa068acdc86d221eef42431c4600d570565a7e0f04738c9279534c52e252346626359aa6e6066fcc6de20e3beb540fe2f31fc106fd9c288e95e0a47b9f3ef384cf2dfa52d2b2ee3cdf79660e617c8d065dd766fb0915c39c705a6ca6fafc3392f2bcf0d0fcf962d182b443f605218502d6fdd636924b4bed0faee03048a6466016d59de99b3ed8fe30335eb3dfeee20bb82ff7bf5fcbd83bd7b1040f0023f037d202b6ca9c8b43ebf758639c90c2d4deec6d5260a7489ab26a1dbe9ac5d14b5220f05e810010e80d987c0e8f5c0ddbe0b3585f70acc718621be50334603edddad3ba0ae3e8eb5338808695bb76b968fa9e74741b1549c1fee74fea1728d083fcc1ac6943702a53dbf25c749e55cd1bd12d41a4169cf3e741c1e59b608ef2de653e90b232d09acc93404ceb2cebdb08ce25d9fef5e6b17596b4ec70bd95180f41cb7fc927e5d0429bbde3a72f3ddf35fcd28fab9cc75064b4757c2e45534d702579d16eba45890dcef08e3498177ccdf34e14fbcde7b3729bb887fd40a776da35e7abcc0b8f319c8191b2f5db4ab829cbd19496d39acffbdeea58599a997bc729cdd127b2b7c2b05f73ffcf3341d2dde87689bc5c5c4da225d847b37a379374839079ce07075bc8ff0275d725e02d93c0e2cd77ceda572bc384d2ecc3ad19abd38b4c39e36bc8b78452c77b3f4781d9e9da84e02db6319b97ff63ae185e697a2b47d4e938d88");
	std::string out, err;
	// el mismo generador que la prueba de Python
	std::string esperado; { unsigned x = 42; for (int i = 0; i < 3000; i++) { x = (x * 1103515245u + 12345u) & 0x7fffffffu; esperado.push_back(((x >> 8) % 5) ? static_cast<char>(65 + ((x >> 16) % 7)) : '\n'); } }
	CHECK(inflarZlib(dyn, out, 1 << 20, err) && out == esperado, "inflate: flujo zlib real con códigos dinámicos (" + std::to_string(dyn.size()) + " bytes → " + std::to_string(out.size()) + ")");
	CHECK(inflarZlib(deHex("7801cbc8cf4954c880130119f9a9799915301a00a98a0b23"), out, 1000, err) && out == "hola hola hola Phoenix Phoenix", "inflate: bloque de códigos fijos");
	std::string malo = dyn; malo[100] ^= 0x55;
	CHECK(!inflarZlib(malo, out, 1 << 20, err), "inflate: un byte dañado se detecta (" + err + ")");
	CHECK(!inflarZlib(dyn.substr(0, 200), out, 1 << 20, err), "inflate: flujo cortado se rechaza");
	CHECK(!inflarZlib(dyn, out, 100, err), "inflate: el límite de tamaño se respeta");
	// zlib sin compresión y vuelta
	std::string grande(200000, 'x'); for (size_t i = 0; i < grande.size(); i += 7) grande[i] = static_cast<char>(i);
	const std::string z = zlibAlmacenado(grande);
	CHECK(inflarZlib(z, out, 1 << 20, err) && out == grande, "zlib almacenado: ida y vuelta de 200 KB (varios bloques)");
	CHECK(inflarZlib(zlibAlmacenado(""), out, 100, err) && out.empty(), "zlib almacenado: vacío");
	// .bin de PES
	const std::string bin = escribirBin(grande);
	CHECK(static_cast<unsigned char>(bin[0]) == 0xFF && bin.compare(3, 5, "WESYS") == 0, "bin: cabecera FF 10 81 WESYS");
	std::string crudo;
	CHECK(leerBin(bin, crudo, err) && crudo == grande, "bin: ida y vuelta");
	std::string roto = bin; roto[9] ^= 1;
	CHECK(!leerBin(roto, crudo, err), "bin: tamaño de cabecera incoherente se rechaza");
	CHECK(!leerBin("no soy un bin", crudo, err), "bin: basura se rechaza");
	// PlayerAssignment / Player
	const BaseFalsa b = baseFalsa();
	std::vector<Fila> leidas;
	CHECK(leerAsignacion(escribirAsignacion(b.filas), leidas, err) && igualFilas(leidas, b.filas), "PlayerAssignment: ida y vuelta de " + std::to_string(b.filas.size()) + " filas");
	CHECK(escribirAsignacion(b.filas).size() == b.filas.size() * 16, "PlayerAssignment: 16 bytes por fila");
	CHECK(!leerAsignacion(std::string(16 * 1000 + 3, 'a'), leidas, err), "PlayerAssignment: tamaño que no es múltiplo de 16 se rechaza");
	CHECK(!leerAsignacion(std::string(16 * 1500, '\0'), leidas, err), "PlayerAssignment: filas con jugador 0 se rechazan");
	std::set<uint32_t> ids;
	CHECK(leerJugadores(playerBinFalso(b.jugadores), ids, err) && ids == b.jugadores, "Player.bin: lee los ids de jugador");
	CHECK(!leerJugadores(std::string(100, 'a'), ids, err), "Player.bin: tamaño raro se rechaza");
}

static void pruebaLista() {
	std::cout << "[lista de cambios]\n";
	Lista l; std::string err;
	CHECK(leerLista(R"({"version":1,"regla":"relajada","cambios":[{"player_id":114506,"team_id":101,"origen":5,"nota":"prueba"},{"player_id":2,"team_id":3}]})", l, err) && l.cambios.size() == 2 && l.cambios[0].origen == 5 && !l.estricta, "lista válida (relajada por defecto)");
	CHECK(leerLista(R"({"version":1,"regla":"estricta","cambios":[]})", l, err) && l.estricta && l.cambios.empty(), "regla estricta y lista vacía");
	CHECK(!leerLista("{no json", l, err), "JSON roto se rechaza");
	CHECK(!leerLista(R"({"version":3,"cambios":[]})", l, err), "versión desconocida se rechaza (" + err + ")");
	CHECK(!leerLista(R"({"version":1,"cambios":[{"player_id":0,"team_id":3}]})", l, err), "player_id 0 se rechaza");
	CHECK(!leerLista(R"({"version":1,"cambios":[{"player_id":"x","team_id":3}]})", l, err), "player_id que no es número se rechaza");
	CHECK(!leerLista(R"({"version":1,"regla":"loca","cambios":[]})", l, err), "regla desconocida se rechaza");
	CHECK(!leerLista(R"({"version":1})", l, err), "falta la lista «cambios»");
}

static void pruebaAplicar() {
	std::cout << "[reglas de aplicación sobre una base falsa]\n";
	const BaseFalsa b = baseFalsa();
	std::string err;
	Resultado r = aplicar(b.filas, b.jugadores, lista({ { 5000, 1003, 0, "" } }));
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].que == "movido" && fila(r.filas, 5000, 1003) && !fila(r.filas, 5000, 1000), "mover: el jugador cambia de equipo");
	CHECK(r.filas.size() == b.filas.size() && enEquipo(r.filas, 1000) == 29 && enEquipo(r.filas, 1003) == 31, "mover: 29 + 31, mismas filas en total");
	CHECK(validar(b.filas, r.filas, err), "mover: el resultado pasa la validación");
	CHECK(fila(r.filas, 5000, 1003)->id == b.filas[0].id, "mover: conserva el id de fila");
	r = aplicar(b.filas, b.jugadores, lista({ { 100005, 1004, 0, "" } }));
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].que == "alta" && r.filas.size() == b.filas.size() + 1 && fila(r.filas, 100005, 1004), "alta: jugador sin equipo entra");
	CHECK(validar(b.filas, r.filas, err), "alta: pasa la validación");
	r = aplicar(b.filas, b.jugadores, lista({ { 5000, 1000, 0, "" } }));
	CHECK(r.aplicados.empty() && r.yaEstaban == 1 && r.filas.size() == b.filas.size(), "ya estaba: no cambia nada");
	r = aplicar(b.filas, b.jugadores, lista({ { 999999, 1003, 0, "" }, { 5001, 4242, 0, "" } }));
	CHECK(r.saltados.size() == 2 && r.saltados[0].codigo == "jugador_no_existe" && r.saltados[1].codigo == "equipo_no_existe" && r.aplicados.empty() && igualFilas(r.filas, b.filas), "jugador o equipo inexistente: se salta con motivo");
	r = aplicar(b.filas, b.jugadores, lista({ { 5002, 1049, 0, "" } }));
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "equipo_lleno" && fila(r.filas, 5002, 1000), "equipo lleno (40): se salta y el jugador se queda");
	r = aplicar(b.filas, b.jugadores, lista({ { 7000, 1010, 0, "" } }));
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "ambiguo", "jugador con dos equipos y sin origen: ambiguo, no se toca");
	r = aplicar(b.filas, b.jugadores, lista({ { 7000, 1010, 1002, "" } }));
	CHECK(r.aplicados.size() == 1 && fila(r.filas, 7000, 1010) && fila(r.filas, 7000, 1001) && !fila(r.filas, 7000, 1002), "con origen: mueve solo esa fila (conserva la otra)");
	r = aplicar(b.filas, b.jugadores, lista({ { 5000, 1010, 1003, "" } }, false));
	CHECK(r.aplicados.size() == 1 && r.aplicados[0].que == "movido_origen_distinto" && fila(r.filas, 5000, 1010), "regla relajada: el origen no coincide pero se mueve");
	r = aplicar(b.filas, b.jugadores, lista({ { 5000, 1010, 1003, "" } }, true));
	CHECK(r.aplicados.empty() && r.saltados.size() == 1 && r.saltados[0].codigo == "origen_cambiado" && fila(r.filas, 5000, 1000), "regla estricta: el origen no coincide, no se toca");
	std::vector<Cambio> muchos;
	for (uint32_t k = 0; k < 15; k++) muchos.push_back({ 100000 + k, 1005, 0, "" });
	r = aplicar(b.filas, b.jugadores, lista(muchos));
	CHECK(r.aplicados.size() == 10 && r.saltados.size() == 5 && enEquipo(r.filas, 1005) == 40, "capacidad: entran 10 (30→40) y 5 se saltan");
	CHECK(validar(b.filas, r.filas, err), "capacidad: pasa la validación");
	std::vector<Fila> conHueco = b.filas;
	conHueco.erase(std::remove_if(conHueco.begin(), conHueco.end(), [](const Fila& f) { return f.equipo == 1006 && (f.empaq >> 10) == 3; }), conHueco.end());
	r = aplicar(conHueco, b.jugadores, lista({ { 100100, 1006, 0, "" } }));
	CHECK(fila(r.filas, 100100, 1006) && (fila(r.filas, 100100, 1006)->empaq >> 10) == 3, "puesto: usa el primer hueco libre de la plantilla");
	CHECK(validar(conHueco, r.filas, err), "puesto: sin repetidos");
	r = aplicar(b.filas, b.jugadores, lista({ { 5000, 1003, 0, "" }, { 100005, 1004, 0, "" } }));
	Resultado r2 = aplicar(r.filas, b.jugadores, lista({ { 5000, 1003, 0, "" }, { 100005, 1004, 0, "" } }));
	CHECK(r2.aplicados.empty() && r2.yaEstaban == 2 && igualFilas(r2.filas, r.filas), "idempotente: aplicar dos veces = igual");
	std::vector<Fila> mal = b.filas; mal.push_back(mal[0]);
	CHECK(!validar(b.filas, mal, err), "validar: id de fila repetido");
	mal = b.filas; mal.push_back({ 99999, mal[0].jugador, mal[0].equipo, 77u << 10 });
	CHECK(!validar(b.filas, mal, err), "validar: mismo jugador dos veces en un equipo");
	mal = b.filas; mal.push_back({ 99999, 100001, 1000, mal[0].empaq });
	CHECK(!validar(b.filas, mal, err), "validar: puesto repetido en un equipo");
}

// ─── juego falso con olmosjr23 ───────────────────────────────────────────────────────────────────
static const std::string FACES = "cpk.root = \".\\livecpk\\Faces\"\r\n";
static const std::string OLMOS = "cpk.root = \".\\olmosjr23\\Database\"\r\n";
static const std::string MODS = "\r\nlua.module = \"CGP_data.lua\"\r\n";

static void escribirBaseOlmos(const fs::path& sider, const BaseFalsa& b) {
	const fs::path d = sider / "olmosjr23" / "Database" / "common" / "etc" / "pesdb";
	escribir(d / "PlayerAssignment.bin", escribirBin(escribirAsignacion(b.filas)));
	escribir(d / "Player.bin", escribirBin(playerBinFalso(b.jugadores)));
}
static fs::path pa(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "PlayerAssignment.bin"; }
static fs::path pb(const fs::path& sider) { return sider / "livecpk" / "Phoenix-DB" / "common" / "etc" / "pesdb" / "Player.bin"; }

static void pruebaJuegoFalso(const fs::path& tmp) {
	std::cout << "[juego falso con base olmosjr23: aplicar, idempotencia, quitar]\n";
	const BaseFalsa b = baseFalsa();
	const fs::path juego = tmp / "juego26";
	escribir(juego / "PES2021.exe", "MZ");
	const fs::path raiz = juego / "SiderAddons";
	escribir(raiz / "sider.ini", FACES + OLMOS + MODS);
	escribirBaseOlmos(raiz, b);
	const fs::path modo = juego / "ConmeGol Extras" / "ConmeGOL Patch 26" / "SiderAddons";
	escribir(modo / "sider.ini", "cpk.root = \".\\livecpk\\Phoenix-DB\"\r\n" + OLMOS + FACES + MODS);
	escribirBaseOlmos(modo, b);
	escribir(pb(modo), escribirBin(playerBinFalso({ 1, 2, 3 })));   // un Player.bin ajeno (tapa jugadores)

	const Lista l = lista({ { 5000, 1003, 0, "" }, { 100005, 1004, 0, "" }, { 999999, 1003, 0, "" } });
	Informe inf = aplicarAlJuego(juego, lista({}));
	CHECK(inf.ok && !fs::exists(pa(raiz)) && !fs::exists(pa(modo)), "lista vacía: no escribe nada");
	const auto an = analizar(juego, l);
	CHECK(an.size() == 2 && an[0].baseOk && an[0].aplicados == 2 && an[0].saltados.size() == 1 && an[0].estado == "sin_aplicar", "análisis: 2 aplicables, 1 saltado, aún sin aplicar");
	CHECK(an[0].baseOrigen.find("olmosjr23") != std::string::npos, "análisis: la base es olmosjr23\\Database (la que usa el juego)");
	CHECK(an[1].conPlayerBinAjeno, "análisis: detecta el Player.bin ajeno en Phoenix-DB del modo");
	CHECK(!fs::exists(pa(raiz)), "el análisis no escribió nada");

	inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok && inf.escritos == 2, "aplicar: escrito en la raíz y en el modo (" + inf.mensaje + ")");
	std::string crudo, err; std::vector<Fila> filas;
	CHECK(leerBin(leer(pa(raiz)), crudo, err) && leerAsignacion(crudo, filas, err), "el PlayerAssignment.bin generado se lee como bin de PES");
	CHECK(filas.size() == b.filas.size() + 1 && fila(filas, 5000, 1003) && fila(filas, 100005, 1004) && !fila(filas, 999999, 1003), "contenido: 1 movido, 1 alta, el inexistente no entra");
	CHECK(validar(b.filas, filas, err), "contenido: pasa la validación");
	CHECK(!fs::exists(pb(modo)), "el Player.bin ajeno se apartó de Phoenix-DB");
	bool copia = false;
	for (const auto& e : fs::directory_iterator(modo / "livecpk" / "Phoenix-DB")) if (e.path().filename().string().rfind("_antes-de-fichajes-", 0) == 0 && fs::exists(e.path() / "Player.bin")) copia = true;
	CHECK(copia, "el Player.bin apartado se guardó en una copia (no se borró)");
	const std::string iniRaiz = leer(raiz / "sider.ini");
	CHECK(contar(iniRaiz, "Phoenix-DB") == 1 && iniRaiz.find("Phoenix-DB") < iniRaiz.find("olmosjr23"), "sider.ini de la raíz: una línea Phoenix-DB antes de la base");
	CHECK(contar(leer(modo / "sider.ini"), "Phoenix-DB") == 1, "sider.ini del modo: sigue con una sola línea");
	int copiasIni = 0; for (const auto& e : fs::directory_iterator(raiz)) if (e.path().filename().string().rfind("sider.ini.phoenix-", 0) == 0) copiasIni++;
	CHECK(copiasIni == 1, "se guardó copia del sider.ini de la raíz");
	const std::string antes = leer(pa(raiz)), antesModo = leer(pa(modo)), iniAntes = leer(raiz / "sider.ini");
	CHECK(analizar(juego, l)[0].estado == "aplicado", "estado: aplicado");
	inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok && leer(pa(raiz)) == antes && leer(pa(modo)) == antesModo && leer(raiz / "sider.ini") == iniAntes, "aplicar otra vez: mismos bytes y mismo sider.ini");
	escribir(pa(modo), "otra herramienta");
	CHECK(analizar(juego, l)[1].estado == "cambiado", "estado: cambiado por otra herramienta");
	inf = quitarDelJuego(juego);
	CHECK(inf.ok && !fs::exists(pa(raiz)) && fs::exists(pa(modo)) && leer(pa(modo)) == "otra herramienta", "quitar: borra lo de Link y respeta lo que cambió otra herramienta");
	escribir(pa(modo), antesModo);
	inf = quitarDelJuego(juego);
	CHECK(inf.ok && !fs::exists(pa(modo)) && !fs::exists(modo / "livecpk" / "Phoenix-DB" / "fichajes_aplicados.json"), "quitar: vuelve a la base original");
	inf = quitarDelJuego(juego);
	CHECK(inf.ok && inf.mensaje.find("ya usa la base original") != std::string::npos, "quitar otra vez: nada que hacer");
	inf = aplicarAlJuego(juego, l);
	inf = aplicarAlJuego(juego, lista({ { 999999, 1003, 0, "" } }));
	CHECK(inf.ok && !fs::exists(pa(raiz)), "si ya no aplica ningún cambio, se retira lo que generó Link");
}

// ─── nombres (lista versión 2) ───────────────────────────────────────────────────────────────────
static std::string playerBinConNombres(const std::map<uint32_t, std::string>& nombres, const std::set<uint32_t>& ids) {
	std::string r(8, '\0');
	for (uint32_t j : ids) {
		std::string rec(312, '\0');
		for (int i = 0; i < 4; i++) rec[static_cast<size_t>(i)] = static_cast<char>((j >> (8 * i)) & 0xFF);
		const auto n = nombres.find(j);
		if (n != nombres.end()) rec.replace(243, (std::min<size_t>)(n->second.size(), 60), n->second.substr(0, 60));
		r += rec;
	}
	return r;
}
static std::string teamBinFalso(const std::map<uint32_t, std::string>& equipos) {
	std::string r;
	for (const auto& e : equipos) {
		std::string rec(1532, '\0');
		for (int i = 0; i < 4; i++) { rec[static_cast<size_t>(i)] = static_cast<char>((e.first >> (8 * i)) & 0xFF); rec[static_cast<size_t>(8 + i)] = static_cast<char>((e.first >> (8 * i)) & 0xFF); }
		rec.replace(368, (std::min<size_t>)(e.second.size(), 60), e.second.substr(0, 60));
		r += rec;
	}
	return r;
}
static std::map<uint32_t, std::string> equiposFalsos() {
	std::map<uint32_t, std::string> m;
	for (uint32_t e = 1000; e < 1150; e++) m[e] = "Equipo " + std::to_string(e);
	m[1001] = "Real Madrid CF"; m[1002] = "FC Barcelona"; m[1003] = "Atl\xE9tico de Madrid";   // Latin-1
	m[1010] = "Club Repetido"; m[1011] = "Club Repetido";
	return m;
}

static void pruebaNombres() {
	std::cout << "[nombres: lista versión 2]\n";
	CHECK(plegarNombre("Kylian Mbapp\xE9") == "kylianmbappe" && plegarNombre("Kylian Mbapp\xC3\xA9") == "kylianmbappe", "plegar: Latin-1 y UTF-8 dan lo mismo («Mbappé» = mbappe)");
	CHECK(plegarNombre("R. CRUZ") == plegarNombre("r cruz") && plegarNombre("Real Madrid CF") == "realmadridcf", "plegar: mayúsculas, espacios y signos no cuentan");
	CHECK(plegarNombre("Atl\xE9tico de Madrid") != plegarNombre("Real Madrid CF"), "plegar: nombres distintos siguen siendo distintos");
	const BaseFalsa b = baseFalsa();
	std::map<uint32_t, std::string> nombres;
	for (uint32_t j : b.jugadores) nombres[j] = "Jugador " + std::to_string(j);
	nombres[5000] = "Lionel Messi"; nombres[5031] = "Lionel Messi"; nombres[100010] = "Lionel Messi";   // 5000 en 1000, 5031 en 1001, 100010 sin equipo
	nombres[5062] = "Ni\xF1o Unico";
	std::set<uint32_t> ids; std::map<uint32_t, std::string> leidos; std::string err;
	CHECK(leerJugadores(playerBinConNombres(nombres, b.jugadores), ids, err, &leidos) && leidos[5000] == "lionelmessi" && leidos[5062] == "ninounico" && ids == b.jugadores, "Player.bin: lee los nombres completos (a +243)");
	std::map<uint32_t, std::string> eq;
	CHECK(leerEquipos(teamBinFalso(equiposFalsos()), eq, err) && eq[1001] == "realmadridcf" && eq[1003] == "atleticodemadrid", "Team.bin: lee los nombres de equipo (registros de 1532, nombre a +368)");
	Nombres nm; nm.jugador = leidos; nm.equipo = eq;

	Lista l;
	CHECK(leerLista(R"({"version":2,"cambios":[{"jugador":"Niño Unico","equipo_destino":"Real Madrid CF"},{"player_id":5000,"equipo_destino":"FC Barcelona","equipo_origen":"Equipo 1000"},{"jugador":"X","team_id":3}]})", l, err) && l.cambios.size() == 3 && l.usaNombres(), "lista v2: acepta nombres, ids y mezcla");
	CHECK(!leerLista(R"({"version":1,"cambios":[{"jugador":"X","equipo_destino":"Y"}]})", l, err), "lista v1: los nombres se rechazan («version»: 2) (" + err + ")");
	CHECK(!leerLista(R"({"version":2,"cambios":[{"player_id":5,"jugador":"X","team_id":3}]})", l, err), "lista v2: id y nombre del mismo campo a la vez se rechaza");
	CHECK(!leerLista(R"({"version":2,"cambios":[{"jugador":"X"}]})", l, err), "lista v2: falta el equipo destino");
	CHECK(leerLista(R"({"version":1,"cambios":[{"player_id":5000,"team_id":1003}]})", l, err) && !l.usaNombres(), "lista v1 sigue siendo válida");

	auto cam = [](const char* jug, const char* dest, const char* ori = "") { Cambio c; c.jugadorNombre = jug; c.equipoNombre = dest; c.origenNombre = ori; return c; };
	Resultado r = aplicar(b.filas, b.jugadores, lista({ cam("Niño Unico", "Real Madrid CF") }), &nm);
	CHECK(r.aplicados.size() == 1 && fila(r.filas, 5062, 1001), "nombre único: el jugador (con ñ) pasa al Real Madrid CF (equipo 1001)");
	r = aplicar(b.filas, b.jugadores, lista({ cam("niño unico", "REAL MADRID cf") }), &nm);
	CHECK(r.aplicados.size() == 1, "mayúsculas y acentos no estorban");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Lionel Messi", "FC Barcelona") }), &nm);
	CHECK(r.aplicados.empty() && r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_ambiguo", "jugador con el mismo nombre 3 veces y sin origen: ambiguo, no se toca");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Lionel Messi", "FC Barcelona", "Equipo 1000") }), &nm);
	CHECK(r.aplicados.size() == 1 && fila(r.filas, 5000, 1002) && fila(r.filas, 5031, 1001), "el equipo de origen desambigua: mueve al Messi de «Equipo 1000»");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Lionel Messi", "FC Barcelona", "Equipo 1020") }), &nm);
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_no_encontrado", "origen donde no está ese jugador: no se encuentra");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Niño Unico", "Club Repetido") }), &nm);
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "equipo_ambiguo", "dos equipos con el mismo nombre: ambiguo");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Niño Unico", "Equipo Inventado") }), &nm);
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "equipo_no_encontrado", "equipo que no existe: no se encuentra");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Nadie Conocido", "FC Barcelona") }), &nm);
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "jugador_no_encontrado", "jugador que no existe: no se encuentra");
	r = aplicar(b.filas, b.jugadores, lista({ cam("Niño Unico", "FC Barcelona") }), nullptr);
	CHECK(r.saltados.size() == 1 && r.saltados[0].codigo == "nombres_no_disponibles", "sin nombres en la base: se salta con motivo");
	CHECK(validar(b.filas, aplicar(b.filas, b.jugadores, lista({ cam("Niño Unico", "Real Madrid CF"), cam("Lionel Messi", "FC Barcelona", "Equipo 1000") }), &nm).filas, err), "nombres: el resultado pasa la validación");
}

// ─── CPK falso (sin comprimir, tabla @UTF enmascarada) y DpFileList ─────────────────────────────
struct ColUtf { std::string nombre; int tipo; };   // tipo: 4 = u32, 6 = u64, 0xA = texto
struct ValUtf { unsigned long long n = 0; std::string s; };
static void be32(std::string& s, uint32_t v) { for (int i = 3; i >= 0; i--) s.push_back(static_cast<char>((v >> (8 * i)) & 0xFF)); }
static void be64(std::string& s, unsigned long long v) { for (int i = 7; i >= 0; i--) s.push_back(static_cast<char>((v >> (8 * i)) & 0xFF)); }
static void le64s(std::string& s, unsigned long long v) { for (int i = 0; i < 8; i++) s.push_back(static_cast<char>((v >> (8 * i)) & 0xFF)); }
static std::string tablaUtf(const std::vector<ColUtf>& cols, const std::vector<std::vector<ValUtf>>& filas) {
	std::string cad = "T"; cad.push_back('\0');
	std::map<std::string, uint32_t> offs;
	auto off = [&](const std::string& s) { auto i = offs.find(s); if (i != offs.end()) return i->second; const uint32_t o = static_cast<uint32_t>(cad.size()); cad += s; cad.push_back('\0'); offs[s] = o; return o; };
	std::string colsB;
	for (const ColUtf& c : cols) { colsB.push_back(static_cast<char>((5 << 4) | c.tipo)); be32(colsB, off(c.nombre)); }
	int ancho = 0;
	for (const ColUtf& c : cols) ancho += c.tipo == 6 ? 8 : 4;
	std::string rows;
	for (const auto& f : filas) for (size_t i = 0; i < cols.size(); i++) {
		if (cols[i].tipo == 0xA) be32(rows, off(f[i].s)); else if (cols[i].tipo == 6) be64(rows, f[i].n); else be32(rows, static_cast<uint32_t>(f[i].n));
	}
	const uint32_t rowsOff = static_cast<uint32_t>(24 + colsB.size()), strOff = static_cast<uint32_t>(rowsOff + rows.size()), dataOff = static_cast<uint32_t>(strOff + cad.size());
	std::string t = "@UTF";
	be32(t, 0);   // tamaño: se rellena al final
	t.push_back(0); t.push_back(0);
	t.push_back(static_cast<char>(rowsOff >> 8)); t.push_back(static_cast<char>(rowsOff & 0xFF));
	be32(t, strOff); be32(t, dataOff); be32(t, 0);
	t.push_back(static_cast<char>(cols.size() >> 8)); t.push_back(static_cast<char>(cols.size() & 0xFF));
	t.push_back(static_cast<char>(ancho >> 8)); t.push_back(static_cast<char>(ancho & 0xFF));
	be32(t, static_cast<uint32_t>(filas.size()));
	t += colsB; t += rows; t += cad;
	const uint32_t tam = static_cast<uint32_t>(t.size() - 8);
	for (int i = 0; i < 4; i++) t[static_cast<size_t>(4 + i)] = static_cast<char>((tam >> (8 * (3 - i))) & 0xFF);
	return t;
}
static std::string enmascarar(std::string t) {
	unsigned k = 0x655F;
	for (char& c : t) { c = static_cast<char>(static_cast<unsigned char>(c) ^ (k & 0xFF)); k = (k * 0x4115) & 0xFFFF; }
	return t;
}
struct ArchivoCpk { std::string carpeta, nombre, datos; };
static std::string construirCpk(const std::vector<ArchivoCpk>& archivos) {
	const unsigned long long contenido = 2048;
	std::string datos;
	std::vector<std::vector<ValUtf>> filas;
	for (const ArchivoCpk& a : archivos) {
		ValUtf d, n, fs, es, fo;
		d.s = a.carpeta; n.s = a.nombre; fs.n = es.n = a.datos.size(); fo.n = datos.size();
		filas.push_back({ d, n, fs, es, fo });
		datos += a.datos;
		while (datos.size() % 16) datos.push_back('\0');
	}
	const std::string toc = tablaUtf({ { "DirName", 0xA }, { "FileName", 0xA }, { "FileSize", 4 }, { "ExtractSize", 4 }, { "FileOffset", 6 } }, filas);
	const unsigned long long tocOff = contenido + datos.size();
	ValUtf c, t;
	c.n = contenido; t.n = tocOff;
	const std::string cab = tablaUtf({ { "ContentOffset", 6 }, { "TocOffset", 6 } }, { { c, t } });
	std::string r = "CPK "; be32(r, 0xFF); le64s(r, cab.size()); r += enmascarar(cab);
	r.resize(static_cast<size_t>(contenido), '\0');
	r += datos;
	r += "TOC "; be32(r, 0xFF); le64s(r, toc.size()); r += enmascarar(toc);
	return r;
}
static std::string dpFileList(const std::vector<std::pair<std::string, int>>& v) {
	std::string r;
	for (const auto& e : v) {
		std::string rec(48, '\0');
		rec[4] = static_cast<char>(e.second);
		rec.replace(16, (std::min<size_t>)(31, e.first.size()), e.first.substr(0, 31));
		r += rec;
	}
	return r;
}
static std::vector<ArchivoCpk> archivosBase(const BaseFalsa& b, const std::map<uint32_t, std::string>& nombres = {}, bool conTeam = true) {
	std::vector<ArchivoCpk> v = {
		{ "common/etc/pesdb", "PlayerAssignment.bin", escribirBin(escribirAsignacion(b.filas)) },
		{ "common/etc/pesdb", "Player.bin", escribirBin(playerBinConNombres(nombres, b.jugadores)) } };
	if (conTeam) v.push_back({ "common/etc/pesdb", "Team.bin", escribirBin(teamBinFalso(equiposFalsos())) });
	return v;
}
static BaseFalsa baseOtra() {   // otra base: menos jugadores en equipos y otros equipos
	BaseFalsa b = baseFalsa();
	b.filas.erase(b.filas.begin() + 100, b.filas.begin() + 140);
	return b;
}
static const Analisis* por(const std::vector<Analisis>& v, const std::string& nombre) { for (const Analisis& a : v) if (a.objetivo.nombre == nombre) return &a; return nullptr; }
static std::string unir(const std::vector<std::string>& v) { std::string r; for (const std::string& s : v) r += s + " | "; return r; }

static void pruebaCpk() {
	std::cout << "[CPK falso: tabla @UTF enmascarada, lectura sin cargar el archivo]\n";
	const BaseFalsa b = baseFalsa();
	const std::string cpk = construirCpk(archivosBase(b));
	std::string bytes, err;
	CHECK(extraerDeCpk(cpk, "common/etc/pesdb", "PlayerAssignment.bin", bytes, err) && bytes == escribirBin(escribirAsignacion(b.filas)), "CPK: extrae un archivo de la carpeta pesdb");
	CHECK(!extraerDeCpk(cpk, "common/etc/pesdb", "NoExiste.bin", bytes, err), "CPK: archivo que no está");
	std::vector<EntradaCpk> es;
	const LectorCpk lector = [&cpk](uint64_t d, size_t n, std::string& out) { if (d + n > cpk.size()) return false; out = cpk.substr(static_cast<size_t>(d), n); return true; };
	CHECK(entradasCpk(lector, cpk.size(), "common/etc/pesdb", es, err) && es.size() == 3, "CPK: la tabla lista 3 archivos sin leer el contenido");
	CHECK(!entradasCpk(lector, 10, "x", es, err) && !extraerDeCpk("CPK basura", "a", "b", bytes, err), "CPK: basura se rechaza");
	const auto rg = leerDpFileList(dpFileList({ { "dt80_100E_x64.cpk", 8 }, { "CGP_database.cpk", 1 } }));
	CHECK(rg.size() == 2 && rg.at("cgp_database.cpk") == 1 && rg.at("dt80_100e_x64.cpk") == 8, "DpFileList.bin: nombre → prioridad");
}

// ─── detector de base por evidencia: estructuras falsas ─────────────────────────────────────────
static fs::path juegoNuevo(const fs::path& tmp, const std::string& n, const std::string& ini) {
	const fs::path j = tmp / n;
	escribir(j / "PES2021.exe", "MZ");
	escribir(j / "SiderAddons" / "sider.ini", ini);
	return j;
}
static const Lista kLista = lista({ { 5000, 1003, 0, "" } });

static void pruebaDeteccion(const fs::path& tmp) {
	std::cout << "[detector de parche/base: estructuras falsas]\n";
	const BaseFalsa b = baseFalsa(), otra = baseOtra();
	const std::string vanilla = construirCpk({ { "common/etc/pesdb", "PlayerAssignment.bin", std::string(151729, 'v') }, { "common/etc/pesdb", "Player.bin", std::string(1724196, 'v') } });
	const std::string nombresIni = FACES + MODS;

	{   // 1) PES vanilla: solo dt*.cpk con datos propios del juego (sin formato WESYS)
		const fs::path j = juegoNuevo(tmp, "d_vanilla", nombresIni);
		escribir(j / "download" / "dt80_100E_x64.cpk", vanilla); escribir(j / "download" / "dt80_700E_x64.cpk", vanilla);
		escribir(j / "download" / "DpFileList.bin", dpFileList({ { "dt80_100E_x64.cpk", 8 }, { "dt80_700E_x64.cpk", 2 } }));
		const auto an = analizar(j, kLista);
		CHECK(an.size() == 1 && !an[0].baseOk && an[0].parche == "PES 2021 original" && an[0].estado == "sin_base", "vanilla: se reconoce «PES 2021 original» pero su base no se lee (" + (an.empty() ? std::string() : an[0].error.substr(0, 90)) + ")");
		CHECK(!an.empty() && an[0].error.find("dt80_700E") != std::string::npos && !an[0].queHacer.empty(), "vanilla: dice qué detectó (usa dt80_700E, la de más prioridad) y qué hacer");
		const Informe inf = aplicarAlJuego(j, kLista);
		CHECK(inf.ok && inf.escritos == 0 && !fs::exists(j / "SiderAddons" / "livecpk"), "vanilla: no se crea nada");
	}
	{   // 2) parche con CPK propio que gana al vanilla por prioridad (DpFileList)
		const fs::path j = juegoNuevo(tmp, "d_cpkpropio", nombresIni);
		escribir(j / "download" / "dt80_700E_x64.cpk", vanilla);
		escribir(j / "download" / "MiParche.cpk", construirCpk(archivosBase(b)));
		escribir(j / "download" / "DpFileList.bin", dpFileList({ { "dt80_700E_x64.cpk", 2 }, { "MiParche.cpk", 1 } }));
		const auto an = analizar(j, kLista);
		CHECK(an.size() == 1 && an[0].baseOk && an[0].aplicados == 1 && an[0].baseOrigen.find("MiParche.cpk") != std::string::npos, "CPK propio: gana por prioridad sobre el CPK original (" + (an.empty() ? std::string() : an[0].baseOrigen) + ")");
		CHECK(!an.empty() && an[0].baseGenerica && an[0].parche.find("parche desconocido") == 0 && !an[0].comoLoSupe.empty(), "CPK propio: «parche desconocido» pero se usa como base genérica, con la evidencia (" + (an.empty() ? std::string() : unir(an[0].comoLoSupe).substr(0, 120)) + ")");
		const Informe inf = aplicarAlJuego(j, kLista);
		std::string crudo, err; std::vector<Fila> filas;
		CHECK(inf.ok && inf.escritos == 1 && leerBin(leer(pa(j / "SiderAddons")), crudo, err) && leerAsignacion(crudo, filas, err) && fila(filas, 5000, 1003), "CPK propio: genera el PlayerAssignment sobre ESA base");
	}
	{   // 3) parche con la base en una carpeta (cpk.root con otro nombre) y un CPK por debajo
		const fs::path j = juegoNuevo(tmp, "d_carpeta", "cpk.root = \".\\livecpk\\Mi Base Rara\"\r\n" + FACES + MODS);
		escribir(j / "download" / "OtroParche.cpk", construirCpk(archivosBase(otra)));
		const fs::path d = j / "SiderAddons" / "livecpk" / "Mi Base Rara" / "common" / "etc" / "pesdb";
		escribir(d / "PlayerAssignment.bin", escribirBin(escribirAsignacion(b.filas)));
		escribir(d / "Player.bin", escribirBin(playerBinFalso(b.jugadores)));
		const auto an = analizar(j, kLista);
		CHECK(an.size() == 1 && an[0].baseOk && an[0].baseOrigen.find("Mi Base Rara") != std::string::npos && an[0].filasBase == static_cast<int>(b.filas.size()), "base en carpeta: gana la cpk.root sobre el CPK (nombre de carpeta cualquiera)");
	}
	{   // 4) varias cpk.root con base: manda la primera
		const fs::path j = juegoNuevo(tmp, "d_varias", "cpk.root = \".\\livecpk\\Primera\"\r\ncpk.root = \".\\livecpk\\Segunda\"\r\n" + MODS);
		for (const auto& e : std::vector<std::pair<std::string, BaseFalsa>>{ { "Primera", otra }, { "Segunda", b } }) {
			const fs::path d = j / "SiderAddons" / "livecpk" / e.first / "common" / "etc" / "pesdb";
			escribir(d / "PlayerAssignment.bin", escribirBin(escribirAsignacion(e.second.filas)));
			escribir(d / "Player.bin", escribirBin(playerBinFalso(e.second.jugadores)));
		}
		const auto an = analizar(j, kLista);
		CHECK(an.size() == 1 && an[0].baseOk && an[0].filasBase == static_cast<int>(otra.filas.size()) && an[0].baseOrigen.find("Primera") != std::string::npos, "varias bases en cpk.root: gana la de más prioridad (la primera)");
	}
	{   // 5) dos CPK con la misma prioridad y bases distintas: no se adivina
		const fs::path j = juegoNuevo(tmp, "d_empate", nombresIni);
		escribir(j / "download" / "A.cpk", construirCpk(archivosBase(b)));
		escribir(j / "download" / "B.cpk", construirCpk(archivosBase(otra)));
		const auto an = analizar(j, kLista);
		CHECK(an.size() == 1 && !an[0].baseOk && an[0].error.find("misma prioridad") != std::string::npos, "empate de prioridad entre dos CPK: no se adivina (" + (an.empty() ? std::string() : an[0].error.substr(0, 80)) + ")");
		CHECK(aplicarAlJuego(j, kLista).escritos == 0 && !fs::exists(j / "SiderAddons" / "livecpk"), "empate: no se crea nada");
	}
	{   // 6) cambiador con varios modos, cada uno con su CPK (bases distintas) y uno dañado
		const fs::path j = juegoNuevo(tmp, "d_cambiador", nombresIni);
		for (const auto& e : std::vector<std::pair<std::string, BaseFalsa>>{ { "Parche Uno", b }, { "Parche Dos", otra } }) {
			const fs::path m = j / "ConmeGol Extras" / e.first;
			escribir(m / "download" / "CGP_database.cpk", construirCpk(archivosBase(e.second)));
			escribir(m / "SiderAddons" / "x.txt", "x");
		}
		escribir(j / "ConmeGol Extras" / "Parche Roto" / "download" / "CGP_database.cpk", "esto no es un cpk");
		escribir(j / "ConmeGol Extras" / "Parche Roto" / "SiderAddons" / "x.txt", "x");
		escribir(j / "version_actual.txt", "Parche Dos");
		escribir(j / "download" / "CGP_database.cpk", construirCpk(archivosBase(otra)));
		const auto an = analizar(j, kLista);
		const Analisis *u = por(an, "Parche Uno"), *d = por(an, "Parche Dos"), *r = por(an, "Parche Roto"), *raiz = por(an, "Raíz del juego");
		CHECK(u && d && r && raiz, "cambiador: raíz + 3 modos detectados");
		CHECK(u && u->baseOk && u->parche == "Parche Uno" && u->filasBase == static_cast<int>(b.filas.size()), "cambiador: cada modo usa SU base y se llama como su carpeta");
		CHECK(d && d->baseOk && d->filasBase == static_cast<int>(otra.filas.size()), "cambiador: el otro modo usa otra base");
		CHECK(r && !r->baseOk && !r->error.empty(), "cambiador: el modo con la base dañada no se toca (" + (r ? r->error.substr(0, 60) : std::string()) + ")");
		CHECK(raiz && raiz->baseOk && raiz->parche == "Parche Dos", "cambiador: la raíz se identifica por version_actual.txt («Parche Dos»)");
		const Informe inf = aplicarAlJuego(j, kLista);
		CHECK(inf.ok && inf.escritos == 3 && fs::exists(pa(j / "ConmeGol Extras" / "Parche Uno" / "SiderAddons")) && !fs::exists(pa(j / "ConmeGol Extras" / "Parche Roto" / "SiderAddons")), "cambiador: escribe en los que tienen base y no en el roto");
		std::string crudo, err; std::vector<Fila> f1, f2;
		leerBin(leer(pa(j / "ConmeGol Extras" / "Parche Uno" / "SiderAddons")), crudo, err); leerAsignacion(crudo, f1, err);
		leerBin(leer(pa(j / "ConmeGol Extras" / "Parche Dos" / "SiderAddons")), crudo, err); leerAsignacion(crudo, f2, err);
		CHECK(f1.size() == b.filas.size() && f2.size() == otra.filas.size(), "cambiador: cada resultado conserva el tamaño de SU base (no se mezclan)");
		const std::string antes = leer(pa(j / "ConmeGol Extras" / "Parche Uno" / "SiderAddons"));
		aplicarAlJuego(j, kLista);
		CHECK(leer(pa(j / "ConmeGol Extras" / "Parche Uno" / "SiderAddons")) == antes, "cambiador: idempotente");
		quitarDelJuego(j);
		CHECK(!fs::exists(pa(j / "ConmeGol Extras" / "Parche Uno" / "SiderAddons")) && !fs::exists(pa(j / "SiderAddons")), "cambiador: quitar deja todo en la base original");
	}
	{   // 7) sin ninguna base
		const fs::path j = juegoNuevo(tmp, "d_nada", nombresIni);
		const auto an = analizar(j, kLista);
		CHECK(an.size() == 1 && !an[0].baseOk && an[0].error.find("No encontré PlayerAssignment.bin") != std::string::npos && !an[0].queHacer.empty(), "sin ninguna base: «qué detecté» y «qué hacer»");
		CHECK(aplicarAlJuego(j, kLista).escritos == 0 && !fs::exists(j / "SiderAddons" / "livecpk"), "sin base: no se crea nada");
	}
	{   // 8) lista por nombres sobre una base con Team.bin y Player.bin con nombres, dentro de un CPK
		const fs::path j = juegoNuevo(tmp, "d_nombres", nombresIni);
		std::map<uint32_t, std::string> nombres;
		for (uint32_t id : b.jugadores) nombres[id] = "Jugador " + std::to_string(id);
		nombres[5000] = "Lionel Messi"; nombres[5031] = "Lionel Messi";
		escribir(j / "download" / "CGP_database.cpk", construirCpk(archivosBase(b, nombres)));
		Lista l;
		std::string err;
		leerLista(R"({"version":2,"cambios":[{"jugador":"Lionel Messi","equipo_origen":"Equipo 1000","equipo_destino":"FC Barcelona"},{"jugador":"Lionel Messi","equipo_destino":"Real Madrid CF"}]})", l, err);
		const auto an = analizar(j, l);
		CHECK(an.size() == 1 && an[0].baseOk && an[0].nombresDisponibles && an[0].aplicados == 1 && an[0].saltados.size() == 1 && an[0].saltados[0].codigo == "jugador_ambiguo", "nombres sobre un CPK: 1 se aplica (con origen) y 1 se salta por ambiguo");
		const Informe inf = aplicarAlJuego(j, l);
		std::string crudo; std::vector<Fila> filas;
		CHECK(inf.ok && leerBin(leer(pa(j / "SiderAddons")), crudo, err) && leerAsignacion(crudo, filas, err) && fila(filas, 5000, 1002) && fila(filas, 5031, 1001), "nombres: el resultado tiene al Messi de «Equipo 1000» en el FC Barcelona (equipo 1002) y el otro intacto");
	}
}

static void pruebaSinBase(const fs::path& tmp) {
	std::cout << "[sin base reconocida: no se crea nada]\n";
	const fs::path juego = tmp / "juegoRaro";
	escribir(juego / "PES2021.exe", "MZ");
	escribir(juego / "SiderAddons" / "sider.ini", FACES + MODS);
	escribir(juego / "download" / "CGP_database.cpk", "esto no es un cpk");
	const Lista l = lista({ { 5000, 1003, 0, "" } });
	const auto an = analizar(juego, l);
	CHECK(an.size() == 1 && !an[0].baseOk && an[0].estado == "sin_base" && !an[0].error.empty(), "análisis: base no reconocida con motivo (" + an[0].error + ")");
	const Informe inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok && inf.escritos == 0 && !fs::exists(juego / "SiderAddons" / "livecpk"), "aplicar: no crea ninguna carpeta ni archivo");
	CHECK(leer(juego / "SiderAddons" / "sider.ini") == FACES + MODS, "aplicar: sider.ini intacto");
	const fs::path j2 = tmp / "juegoRoto";
	escribir(j2 / "PES2021.exe", "MZ");
	escribir(j2 / "SiderAddons" / "sider.ini", FACES + OLMOS + MODS);
	escribirBaseOlmos(j2 / "SiderAddons", baseFalsa());
	const fs::path pbin = j2 / "SiderAddons" / "olmosjr23" / "Database" / "common" / "etc" / "pesdb" / "PlayerAssignment.bin";
	std::string danado = leer(pbin);
	danado[200] ^= 0x40;
	escribir(pbin, danado);
	const Informe i2 = aplicarAlJuego(j2, l);
	CHECK(i2.ok && i2.escritos == 0 && !fs::exists(j2 / "SiderAddons" / "livecpk"), "base dañada: no se crea nada");
}

// ─── juego real: COPIAS de sus bases ─────────────────────────────────────────────────────────────
static void pruebaReal(const fs::path& tmp, const fs::path& real) {
	std::cout << "[copias de las bases reales: 26 (olmosjr23), 27, UEFAGOL 27, Clásicos]\n";
	const fs::path extras = real / "ConmeGol Extras";
	const fs::path juego = tmp / "juegoReal";
	std::error_code ec;
	escribir(juego / "PES2021.exe", "MZ");
	escribir(juego / "SiderAddons" / "sider.ini", leer(real / "SiderAddons" / "sider.ini"));
	fs::create_directories(juego / "download", ec);
	fs::copy_file(real / "download" / "CGP_database.cpk", juego / "download" / "CGP_database.cpk", fs::copy_options::overwrite_existing, ec);
	const std::vector<std::pair<std::string, bool>> modos = { { "ConmeGOL Patch 26", true }, { "ConmeGOL Patch 27", false }, { "UEFAGOL 27", false }, { "ConmeGOL Clasicos", false } };
	for (const auto& m : modos) {
		const fs::path o = extras / m.first, d = juego / "ConmeGol Extras" / m.first;
		if (!fs::is_regular_file(o / "download" / "CGP_database.cpk", ec)) { std::cout << "  (no está " << m.first << ": se salta)\n"; continue; }
		fs::create_directories(d / "download", ec); fs::create_directories(d / "SiderAddons" / "livecpk", ec);
		fs::copy_file(o / "download" / "CGP_database.cpk", d / "download" / "CGP_database.cpk", fs::copy_options::overwrite_existing, ec);
		if (m.second) {   // parche 26: su sider.ini y su base olmosjr23
			escribir(d / "SiderAddons" / "sider.ini", leer(o / "SiderAddons" / "sider.ini"));
			const fs::path ol = o / "SiderAddons" / "olmosjr23" / "Database" / "common" / "etc" / "pesdb";
			for (const char* n : { "PlayerAssignment.bin", "Player.bin" }) escribir(d / "SiderAddons" / "olmosjr23" / "Database" / "common" / "etc" / "pesdb" / n, leer(ol / n));
		}
	}
	const Lista l = lista({ { 208750, 139, 0, "" }, { 201930, 2725, 0, "" }, { 114506, 101, 0, "" }, { 999999, 139, 0, "" } });
	const auto an = analizar(juego, l);
	for (const Analisis& a : an) {
		std::cout << "  · " << a.objetivo.nombre << ": base " << (a.baseOk ? a.baseOrigen : "NO (" + a.error + ")") << ", filas " << a.filasBase << ", aplicados " << a.aplicados << ", saltados " << a.saltados.size() << "\n";
		for (const Salto& s : a.saltados) std::cout << "      salto " << s.cambio.jugador << " → " << s.cambio.equipo << ": " << s.codigo << "\n";
		CHECK(a.baseOk && a.filasBase > 10000, "la base de «" + a.objetivo.nombre + "» se reconoce");
	}
	const Informe inf = aplicarAlJuego(juego, l);
	CHECK(inf.ok, "aplicar sobre las copias: " + inf.mensaje);
	for (const Analisis& a : inf.objetivos) {
		if (a.aplicados == 0) continue;
		std::string crudo, err; std::vector<Fila> filas;
		const fs::path f = pa(a.objetivo.siderAddons);
		CHECK(leerBin(leer(f), crudo, err) && leerAsignacion(crudo, filas, err), "«" + a.objetivo.nombre + "»: el resultado carga (formato exacto, " + std::to_string(filas.size()) + " filas)");
		std::map<uint32_t, int> porEquipo; std::set<std::pair<uint32_t, uint32_t>> pares; bool dup = false;
		for (const Fila& x : filas) { porEquipo[x.equipo]++; if (!pares.insert({ x.jugador, x.equipo }).second) dup = true; }
		int maximo = 0; for (const auto& c : porEquipo) maximo = std::max(maximo, c.second);
		CHECK(!dup && maximo <= kMaxPorEquipo, "«" + a.objetivo.nombre + "»: sin duplicados y ≤ 40 por equipo (máximo " + std::to_string(maximo) + ")");
		const std::string antes = leer(f);
		aplicarAlJuego(juego, l);
		CHECK(leer(f) == antes, "«" + a.objetivo.nombre + "»: idempotente (mismos bytes)");
	}
	CHECK(contar(leer(juego / "SiderAddons" / "sider.ini"), "Phoenix-DB") == 1, "sider.ini de la raíz (el del 27): una sola línea Phoenix-DB");
	quitarDelJuego(juego);
	bool quedan = false; for (const Analisis& a : inf.objetivos) if (fs::exists(pa(a.objetivo.siderAddons))) quedan = true;
	CHECK(!quedan, "quitar: ningún PlayerAssignment.bin generado queda");
}

// Más pruebas con COPIAS de las bases reales: nombres del parche detectados, lista por nombres y data original (solo lectura).
static void pruebaRealExtra(const fs::path& tmp, const fs::path& real) {
	std::cout << "[bases reales: parche detectado, lista por nombres y data original]\n";
	const fs::path juego = tmp / "juegoReal";   // lo dejó pruebaReal (copias de CPK, sider.ini y olmosjr23)
	std::error_code ec;
	if (!fs::is_directory(juego, ec)) { std::cout << "  (no hay copia: se salta)\n"; return; }
	// la raíz también necesita volver a tener su CPK/base para esta pasada
	Lista l; std::string err;
	leerLista(R"({"version":2,"cambios":[{"jugador":"Cristiano Ronaldo","equipo_origen":"Al-Nassr FC","equipo_destino":"Real Madrid CF"},{"jugador":"Erling Haaland","equipo_destino":"FC Barcelona"}]})", l, err);
	const auto an = analizar(juego, l);
	for (const Analisis& a : an) {
		std::cout << "  · " << a.objetivo.nombre << ": parche «" << a.parche << "»" << (a.baseGenerica ? " (genérico)" : "") << ", base " << a.baseOrigen << ", nombres " << (a.nombresDisponibles ? "sí" : "no") << ", aplicados " << a.aplicados << ", saltados " << a.saltados.size() << "\n";
		for (const std::string& e : a.comoLoSupe) std::cout << "      - " << e << "\n";
		for (const Salto& s : a.saltados) std::cout << "      salto: " << s.codigo << ": " << s.motivo << "\n";
		CHECK(a.baseOk && a.nombresDisponibles, "«" + a.objetivo.nombre + "»: base reconocida y con nombres de jugadores y equipos");
	}
	const Analisis* p26 = por(an, "ConmeGOL Patch 26"); const Analisis* p27 = por(an, "ConmeGOL Patch 27"); const Analisis* u27 = por(an, "UEFAGOL 27"); const Analisis* cla = por(an, "ConmeGOL Clasicos");
	CHECK(p26 && p26->parche.find("olmosjr23") != std::string::npos && p26->baseOrigen.find("olmosjr23") != std::string::npos, "26: parche detectado por huella y base = olmosjr23\Database (por cpk.root)");
	CHECK(p27 && p27->parche == "ConmeGOL Patch 27" && p27->baseOrigen.find("CGP_database.cpk") != std::string::npos, "27: parche detectado por huella y base = su CGP_database.cpk");
	CHECK(u27 && u27->parche == "UEFAGOL 27", "UEFAGOL 27: detectado");
	CHECK(cla && cla->parche.find("Cl") != std::string::npos, "Clásicos: detectado");
	CHECK(p26 && p26->aplicados >= 1, "por nombres sobre la base del 26 (olmosjr23): Cristiano Ronaldo de «Al-Nassr FC» a «Real Madrid CF» se resuelve y se aplica");
	// data original: solo la carpeta Data real (junction de solo lectura): sin parche, la base es cifrada → no se crea nada
	const fs::path vj = tmp / "juegoVanilla";
	fs::create_directories(vj / "SiderAddons", ec);
	escribir(vj / "PES2021.exe", "MZ"); escribir(vj / "SiderAddons" / "sider.ini", FACES + MODS);
	const std::string cmd = "cmd /c mklink /J \"" + (vj / "Data").string() + "\" \"" + (real / "Data").string() + "\" >nul 2>&1";
	if (std::system(cmd.c_str()) == 0 && fs::is_directory(vj / "Data", ec)) {
		const auto av = analizar(vj, kLista);
		std::cout << "  · data original (Data\dt*.cpk, solo lectura): " << (av.empty() ? std::string("sin sitios") : av[0].parche + " · " + av[0].error.substr(0, 110)) << "\n";
		CHECK(av.size() == 1 && !av[0].baseOk, "data original de PES (cifrada o sin prioridad clara): no se reconoce y no se crea nada");
		CHECK(aplicarAlJuego(vj, kLista).escritos == 0 && !fs::exists(vj / "SiderAddons" / "livecpk"), "data original: aplicar no escribe nada");
		std::system(("cmd /c rmdir \"" + (vj / "Data").string() + "\" >nul 2>&1").c_str());   // quita solo el enlace, no el contenido real
	}
	else std::cout << "  (no se pudo crear el enlace a Data: se salta)\n";
}

int main(int argc, char** argv) {
	if (argc < 2) { std::cout << "uso: fichajes_test <carpeta temporal vacía> [carpeta del juego real]\n"; return 2; }
	const fs::path tmp = deU8(argv[1]);
	fs::create_directories(tmp);
	pruebaFormatos();
	pruebaLista();
	pruebaAplicar();
	pruebaJuegoFalso(tmp);
	pruebaNombres();
	pruebaCpk();
	pruebaDeteccion(tmp);
	pruebaSinBase(tmp);
	if (argc >= 3) { pruebaReal(tmp, deU8(argv[2])); pruebaRealExtra(tmp, deU8(argv[2])); }
	std::cout << "\n" << (total - fallos) << "/" << total << " comprobaciones bien" << (fallos ? " — HAY FALLOS" : " — TODO OK") << "\n";
	return fallos ? 1 : 0;
}

