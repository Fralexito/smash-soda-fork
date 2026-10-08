// =============================================================================
//  Phoenix Mercado · Pruebas del Core (sin internet, sin Windows)
//  Usa una web «falsa» (HttpFalso) para probar todas las respuestas posibles.
// =============================================================================

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <set>

#include "../core/ClienteMercado.h"
#include "../core/Copias.h"
#include "../core/Sha256.h"
#include "../core/OptionFile.h"
#include "../core/BaseDatosParche.h"
#include "../core/Catalogo.h"
#include "../core/Emparejamiento.h"
#include "../core/Integridad.h"
#include "../core/LigaMaster.h"
#include "../core/Firma.h"
#include "../core/Sincronizacion.h"
#include <nlohmann/json.hpp>
#include <cstdlib>

using namespace mercado;
namespace fs = std::filesystem;

static int fallos = 0, total = 0;
#define CHECK(cond) do { total++; if (!(cond)) { fallos++; std::printf("  FALLA %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

struct HttpFalso : ClienteHttp {
	RespuestaHttp siguiente;
	std::vector<std::string> ultimasCabeceras;
	std::string ultimaUrl, contenidoDescarga;
	RespuestaHttp peticion(const std::string&, const std::string& url, const std::string&, const std::vector<std::string>& cab) override {
		ultimaUrl = url; ultimasCabeceras = cab; return siguiente;
	}
	bool descargar(const std::string&, const std::string& ruta, long long, std::string&) override {
		std::ofstream(aRuta(ruta), std::ios::binary) << contenidoDescarga; return true;
	}
};

struct TokenFalso : FuenteToken {
	std::string t;
	std::string token() override { return t; }
	bool guardar(const std::string& x) override { t = x; return true; }
	void borrar() override { t.clear(); }
	std::string nombreModo() const override { return "falso"; }
};

int main() {
	std::printf("SHA-256\n");
	CHECK(sha256::deTexto("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	CHECK(sha256::deTexto("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	CHECK(sha256::deTexto(std::string(1000000, 'a')) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

	std::printf("Sobre de respuestas\n");
	HttpFalso http; TokenFalso tok; std::string logs;
	ClienteMercado api(http, tok, [&](const std::string&, const std::string& t) { logs += t + "\n"; });

	http.siguiente = { 200, R"({"ok":true,"version_api":"mercado-0.1.0","datos":{"ok":true,"version_api":"mercado-0.1.0","hora":"2026-10-08T01:37:30.830Z"}})", "" };
	auto e = api.eco();
	CHECK(e.ok() && e.valor->versionApi == "mercado-0.1.0");
	CHECK(http.ultimaUrl == std::string(kBaseUrl) + "/eco");
	bool sinAuth = true; for (auto& c : http.ultimasCabeceras) if (c.rfind("Authorization", 0) == 0) sinAuth = false;
	CHECK(sinAuth); // /eco va sin token

	auto y0 = api.yo();
	CHECK(!y0.ok() && y0.error.codigo == "NO_VINCULADO"); // sin token ni siquiera llama

	tok.t = "pml_SECRETO1234567890abcdef";
	http.siguiente = { 401, R"({"ok":false,"version_api":"mercado-0.1.0","error":{"codigo":"TOKEN_INVALIDO"}})", "" };
	auto y1 = api.yo();
	CHECK(!y1.ok() && y1.error.codigo == "TOKEN_INVALIDO" && y1.error.http == 401);
	CHECK(tok.t.empty()); // token rechazado → se borra
	CHECK(logs.find("SECRETO") == std::string::npos); // el token jamás llega al log

	http.siguiente = { 502, "<html>Bad gateway</html>", "" };
	CHECK(api.eco().error.codigo == "RESPUESTA_INVALIDA");
	http.siguiente = { 0, "", "timeout" };
	CHECK(api.eco().error.codigo == "RED");

	CHECK(limpiarSecretos("Authorization: Bearer pml_abc xyz") == "Authorization: Bearer *** xyz");

	std::printf("Option oficial + verificación\n");
	tok.t = "pml_otro";
	const std::string contenido = "contenido-de-prueba";
	http.siguiente = { 200, std::string(R"({"ok":true,"datos":{"version":"v1","sha256":")") + sha256::deTexto(contenido)
		+ R"(","tamano":19,"url":"https://x/y?token=abc","expira_en_seg":300,"notas":null}})", "" };
	auto o = api.optionActual();
	CHECK(o.ok() && o.valor->version == "v1" && o.valor->url.find("?token=") != std::string::npos);

	const fs::path tmp = fs::temp_directory_path() / "phoenix-mercado-pruebas";
	fs::remove_all(tmp); fs::create_directories(tmp);
	http.contenidoDescarga = contenido;
	CHECK(api.descargarOption(*o.valor, (tmp / "bueno").string()).ok());
	CHECK(api.descargarOption(*o.valor, (tmp / "bueno").string()).error.codigo == "DESTINO_OCUPADO"); // no sobrescribe
	http.contenidoDescarga = "alterado";
	auto malo = api.descargarOption(*o.valor, (tmp / "malo").string());
	CHECK(!malo.ok() && malo.error.codigo == "OPTION_HUELLA_DISTINTA" && !fs::exists(tmp / "malo"));

	std::printf("Copias de seguridad\n");
	const fs::path original = tmp / "EDIT00000000";
	std::ofstream(original, std::ios::binary) << "datos del option file";
	const std::string huellaAntes = sha256::deArchivo(original.string());
	auto c1 = copias::crear(original.string(), (tmp / "copias").string());
	auto c2 = copias::crear(original.string(), (tmp / "copias").string());
	CHECK(c1.ok() && c2.ok() && c1.valor->ruta != c2.valor->ruta); // dos copias, ninguna pisa a otra
	CHECK(c1.ok() && c1.valor->sha256 == huellaAntes);
	CHECK(sha256::deArchivo(original.string()) == huellaAntes);   // original intacto
	CHECK(copias::crear((tmp / "no-existe").string(), tmp.string()).error.codigo == "ARCHIVO_NO_EXISTE");
	fs::remove_all(tmp);



	std::printf("Nombres\n");
	CHECK(normalizarNombre("Luka Modrić") == "luka modric");
	CHECK(normalizarNombre("João  Simões-Pérez") == "joao simoes perez");
	CHECK(parecidoNombre("Gaby Torres", "Gabriel Torres") >= 0.7);
	CHECK(parecidoNombre("Richard Ortiz", "Celso Ortiz") < 0.6);      // otra persona
	CHECK(parecidoNombre("Dante", "Dante Bonfim") >= 0.8);
	{
		std::map<uint32_t, FichaJugador> base;
		base[10] = { 10, "Luis García", "", 151, 190, 26, 0, {} };   // homónimo con el mismo ID
		base[20] = { 20, "Luis García", "", 151, 177, 38, 8, {} };   // el verdadero
		base[30] = { 30, "Celso Ortiz", "", 150, 175, 36, 4, {} };
		Emparejador e(base);
		auto r1 = e.emparejar({ 1, 10, "Luis García", 151, 177, 37, 8 });
		CHECK(r1.idLocal != 10);                                       // no se deja engañar por el ID
		auto r2 = e.emparejar({ 2, 0, "Richard Ortiz", 150, 174, 35, 4 });
		CHECK(r2.estado != EstadoEmparejamiento::Automatico);          // nunca automático con otra persona
		auto r3 = e.emparejar({ 3, 30, "Celso Ortiz", 150, 175, 37, 4 });
		CHECK(r3.estado == EstadoEmparejamiento::Automatico && r3.idLocal == 30 && r3.metodo == "por_id");
	}

	std::printf("Huella de plantillas\n");
	{
		PlantillasPhoenix a{ {7, {3, 1, 2}}, {5, {9}} }, b{ {5, {9}}, {7, {2, 3, 1}} };
		CHECK(huellaPlantillas(a) == huellaPlantillas(b));            // el orden no importa
		CHECK(textoCanonico(a) == "5:9\n7:1,2,3\n");
		PlantillasPhoenix c{ {7, {1, 2}}, {5, {9, 3}} };               // jugador 3 movido al club 5
		CHECK(huellaPlantillas(a) != huellaPlantillas(c));
		auto d = diferencias(a, c);
		CHECK(d.size() == 1 && d[0].jugador == 3 && d[0].clubEsperado == 7 && d[0].clubActual == 5);
	}


	std::printf("Lotes de catálogo\n");
	{
		nlohmann::json c = { {"formato","x"}, {"parche","p"}, {"equipos", nlohmann::json::array({ {{"pes_team_id",1},{"nombre","A"}} })}, {"jugadores", nlohmann::json::array()} };
		for (int i = 0; i < 7001; i++) c["jugadores"].push_back({ {"pes_id", i + 1}, {"nombre", "J" + std::to_string(i)} });
		auto l = lotesCatalogo(c.dump(), 3000);
		CHECK(l.size() == 3);
		if (l.size() != 3) return 1;
		auto l0 = nlohmann::json::parse(l[0]), l2 = nlohmann::json::parse(l[2]);
		CHECK(l0["equipos"].size() == 1 && l0["jugadores"].size() == 3000);
		CHECK(l2["equipos"].empty() && l2["jugadores"].size() == 1001);
		nlohmann::json m = { {"equipos", nlohmann::json::array()}, {"jugadores", nlohmann::json::array({
			{{"pes_id",1},{"nombre","Ok"},{"dorsal",120},{"edad",30},{"fuente","desconocida"},{"nacionalidad",144}},
			{{"pes_id",2},{"fuente","desconocida"}} })} };
		auto lm = nlohmann::json::parse(lotesCatalogo(m.dump())[0]);
		CHECK(lm["jugadores"].size() == 1);                                   // sin nombre: no se envía
		CHECK(!lm["jugadores"][0].contains("dorsal") && lm["jugadores"][0]["edad"] == 30);   // fuera de rango: se quita
		CHECK(!lm["jugadores"][0].contains("fuente") && lm["jugadores"][0]["nacionalidad"] == "144");
	}

	std::printf("Bits\n");
	{ const uint8_t b[4] = { 0xB4, 0x01, 0, 0 };   // 0x01B4 = 436
	  CHECK(leerBits(b, 0, 16) == 436); CHECK(leerBits(b, 2, 3) == 5); }

	std::vector<uint8_t> dSint;   // la Liga Máster sintética, para reutilizarla en la sincronización
	std::printf("Liga Máster interna (equipo del usuario, datos sintéticos)\n");
	{
		using namespace mercado::lm;
		// Guardado ML falso: cabecera, 700 bloques, y después una zona con las 12 tablas alineadas, orden/roles y lista K.
		const size_t FIN = 0x50 + 1680 * 700;
		std::vector<uint8_t> d(FIN + 0x200000, 0);
		auto p32 = [&](size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = uint8_t(v >> (8 * i)); };
		auto p16 = [&](size_t o, uint16_t v) { d[o] = uint8_t(v); d[o + 1] = uint8_t(v >> 8); };
		p32(0, 10); p32(4, 0x50); p32(16, 700); p32(20, 700);
		auto plantilla = [&](int k, const std::vector<std::pair<uint32_t, uint32_t>>& j, const std::vector<uint16_t>& dor) {
			const size_t b = 0x50 + 1680 * size_t(k), s = b + 0x14c;
			for (int i = 0; i < 40; i++) {
				const bool hay = size_t(i) < j.size();
				p32(s + 8 * i, hay ? j[i].first : 65535); p32(s + 8 * i + 4, hay ? j[i].second : 0);
				p16(b + 0x14c + 4 + 0x14a + 2 * i, hay ? dor[i] : 0);
			}
			d[b + 0x14c + 4 + 0x2d6] = uint8_t(j.size());
		};
		// Usuario = equipo 5 con 26 jugadores (reg 1000+i, pid 5000+i); IA = equipo 7 con 3.
		std::vector<std::pair<uint32_t, uint32_t>> U, IA = { {1,9001}, {2,9002}, {3,9003} };
		std::vector<uint16_t> dU, dIA = { 1, 2, 3 };
		for (uint32_t i = 0; i < 26; i++) { U.push_back({ 1000 + i, 5000 + i }); dU.push_back(uint16_t(10 + i)); }
		plantilla(5, U, dU); plantilla(7, IA, dIA);
		{ std::vector<std::pair<uint32_t, uint32_t>> B; std::vector<uint16_t> dB; for (uint32_t i = 0; i < 12; i++) { B.push_back({ 3000 + i, 8000 + i }); dB.push_back(uint16_t(1 + i)); } plantilla(8, B, dB); }
		// Tablas: registro = [x][reg][pid][…]; el contenido lleva una marca por registro para ver que se mueve entero.
		// A2 va exactamente a 976 B de A (como en el juego). La de contratos (48) tiene el registro real 28 B antes de `reg`:
		// [índice][club][tipo][inicio][-1][-1][x][reg][pid][sueldo][cláusula][fin]; el jugador 3 tiene además una OFERTA (tipo 3).
		const uint32_t clubU = (2005u << 14) | 5u, clubOtro = (2300u << 14) | 300u;
		struct T { size_t stride; int dir; size_t reg0; };
		std::vector<T> tablas = { {24,+1,0}, {24,+1,0}, {24,-1,0}, {44,+1,0}, {368,+1,0}, {192,+1,0}, {52,+1,0}, {108,+1,0}, {108,+1,0}, {108,+1,0}, {5628,+1,0}, {48,+1,0}, {16,+1,0} };
		size_t cur = FIN + 0x1000;
		for (size_t ti = 0; ti < tablas.size(); ti++) {
			auto& t = tablas[ti];
			const size_t nreg = t.stride == 368 ? 60 : 30;   // la de 368 lleva 26 + juveniles
			if (ti == 1) cur = tablas[0].reg0 - 4 + 976;     // A2
			if (t.stride == 48) cur += 32;                   // sitio para la cabecera del registro 0
			size_t ini = cur + (t.dir < 0 ? t.stride * (nreg + 2) : 0);
			t.reg0 = ini + 4;
			for (size_t i = 0; i < nreg + 2; i++) {
				const long long r0 = (long long)ini + t.dir * (long long)(i * t.stride);
				const size_t nUsados = t.stride == 368 ? 26 + 30 : t.stride == 48 ? 27 : 26;
				const bool usado = i < nUsados;
				uint32_t reg = 0xffff, pid = 0;
				if (usado) { if (i < 26) { reg = 1000 + uint32_t(i); pid = 5000 + uint32_t(i); } else if (t.stride == 48) { reg = 1003; pid = 5003; } else { reg = 20000 + uint32_t(i); pid = 7000 + uint32_t(i); } }
				p32(size_t(r0), 0xAA000000 | uint32_t(i)); p32(size_t(r0) + 4, reg); p32(size_t(r0) + 8, pid);
				const size_t finMarca = t.stride == 48 ? 24 : t.stride;
				for (size_t b = 12; b < finMarca; b++) d[size_t(r0) + b] = uint8_t(usado ? (i + 1) : 0);
				if (t.stride == 48) { p32(size_t(r0) + 12, usado ? uint32_t(i + 1) : 0); p32(size_t(r0) + 16, 0); p32(size_t(r0) + 20, usado ? 0x1f0807eb : 0xffff); }   // sueldo (i+1)/100 €, cláusula 0, fin 31/8/2027
				if (t.stride == 48) {   // cabecera del registro (28 B antes de reg): índice, club, tipo (5 contrato / 3 oferta), inicio, -1, -1
					const size_t h = size_t(r0) - 24;
					p32(h, usado ? uint32_t(i) : 0xffff); p32(h + 4, !usado ? 0xffffffff : i == 26 ? clubOtro : clubU);
					p32(h + 8, !usado ? 0 : i == 26 ? 3 : 5); p32(h + 12, usado && i < 26 ? 0x010707e4 : 0xffff); p32(h + 16, 0xffffffff); p32(h + 20, 0xffffffff);
				}
				if (t.stride == 24 && ti == 1) p32(size_t(r0), 0x40a00000);   // A2: valoración (float 5.0) antes de reg
			}
			cur += t.stride * (nreg + 3) + 64;
		}
		// Lista de negociaciones abiertas (60 B): [estado][club que negocia][banderas][ffff][-1][-1][ffff][club del jugador][reg][pid][monto][monto][ffff][ffff][0]
		const size_t NEG = cur + 0x200;
		{
			auto negociacion = [&](size_t i, uint32_t reg, uint32_t pid, uint32_t monto) {
				const size_t r = NEG + 60 * i;
				p32(r, 0x1d); p32(r + 4, clubOtro); p32(r + 8, 1); p32(r + 12, 0xffff); p32(r + 16, 0xffffffff); p32(r + 20, 0xffffffff); p32(r + 24, 0xffff);
				p32(r + 28, clubU); p32(r + 32, reg); p32(r + 36, pid); p32(r + 40, monto); p32(r + 44, monto); p32(r + 48, 0xffff); p32(r + 52, 0xffff); p32(r + 56, 0);
			};
			negociacion(0, 1001, 5001, 2600); negociacion(1, 1003, 5003, 62400); negociacion(2, 1012, 5012, 100);
			for (size_t i = 3; i < 6; i++) {   // vacías
				const size_t r = NEG + 60 * i; const uint32_t v[15] = { 0xffff, 0xffffffff, 0, 0xffff, 0xffffffff, 0xffffffff, 0xffff, 0xffffffff, 0xffff, 0, 0, 0, 0xffff, 0xffff, 0 };
				for (int w = 0; w < 15; w++) p32(r + 4 * size_t(w), v[w]);
			}
			cur = NEG + 60 * 6 + 64;
		}
		// Orden de formación (permutación) y roles: en el bloque de alineación del usuario (627, más abajo); lista K en su espejo.
		std::vector<uint8_t> orden; for (int i = 25; i >= 0; i--) orden.push_back(uint8_t(i));   // 25,24,…,0
		const uint8_t roles[6] = { 3, 9, 0, 0, 0, 25 };
		// Un señuelo: la misma permutación suelta en otro sitio NO debe tomarse por el orden del usuario.
		const size_t SENUELO = FIN + 0x100;
		for (size_t i = 0; i < 40; i++) d[SENUELO + i] = i < 26 ? orden[i] : 0xff;
		for (int i = 0; i < 6; i++) d[SENUELO + 0x28 + i] = roles[i];
		const size_t K0 = cur + 0x100;
		for (size_t i = 0; i < 26; i++) { const size_t r = K0 + 16 * i; p32(r, i ? 0xc0 : 0); p32(r + 4, 1000 + orden[i]); p32(r + 8, 5000 + orden[i]); }
		p32(K0 + 16 * 26, 0xc1); p32(K0 + 16 * 26 + 4, 0xffff); p32(K0 + 16 * 27, 0xc7); p32(K0 + 16 * 27 + 4, 0xffff); p32(K0 + 16 * 28, 0xc7); p32(K0 + 16 * 28 + 4, 0xffff);

		// Alineaciones de la IA (ESTRUCTURA-ML.md §10): 629 bloques de 600 B como en el juego. ID option = 2000 + k;
		// el usuario (5) cambia de ID como en el juego (City: -11) y su bloque viejo queda con el ID original.
		for (int k = 0; k < 700; k++) p32(0x50 + 1680 * size_t(k) + 656, 2000 + uint32_t(k));
		p32(0x50 + 1680 * 5 + 656, 0xfffffff5);
		const size_t ALI = FIN + 0x100000;
		auto bloqueAli = [&](int k) { return ALI + 600 * size_t(k); };
		for (int k = 0; k < 629; k++) {
			p32(bloqueAli(k), uint32_t(k)); p32(bloqueAli(k) + 4, k < 627 ? 2000 + uint32_t(k) : 0xfffffff5);
			for (int i = 0; i < 40; i++) d[bloqueAli(k) + 0x220 + i] = 0xff;
		}
		auto ponerOrden = [&](int k, const std::vector<uint8_t>& o, std::array<uint8_t, 6> roles) {
			for (size_t i = 0; i < 40; i++) d[bloqueAli(k) + 0x220 + i] = i < o.size() ? o[i] : 0xff;
			for (size_t i = 0; i < 6; i++) d[bloqueAli(k) + 0x220 + 0x28 + i] = roles[i];
		};
		// El orden del USUARIO (equipo 5) va en el bloque 627 (el primero con su ID); el 628 queda como identidad (reserva del juego).
		ponerOrden(627, orden, { roles[0], roles[1], roles[2], roles[3], roles[4], roles[5] });
		{ std::vector<uint8_t> o; for (uint8_t i = 0; i < 26; i++) o.push_back(i); ponerOrden(628, o, { 0, 0, 0, 0, 0, 0 }); }
		const size_t LU = bloqueAli(627) + 0x220, RO = LU + 0x28;
		auto leerOrden = [&](const std::vector<uint8_t>& dd, int k) { return std::vector<uint8_t>(dd.begin() + long(bloqueAli(k) + 0x220), dd.begin() + long(bloqueAli(k) + 0x220 + 46)); };
		auto compacto = [](std::vector<uint8_t> o, std::array<uint8_t, 6> roles) { o.resize(40, 0xff); o.insert(o.end(), roles.begin(), roles.end()); return o; };
		auto equipoIA = [&](int k, uint32_t reg0, uint32_t pid0, int n) {
			std::vector<std::pair<uint32_t, uint32_t>> J; std::vector<uint16_t> dJ;
			for (uint32_t i = 0; i < uint32_t(n); i++) { J.push_back({ reg0 + i, pid0 + i }); dJ.push_back(uint16_t(1 + i)); }
			plantilla(k, J, dJ);
		};
		ponerOrden(7, { 2, 0, 1 }, { 0, 0, 1, 0, 0, 2 });
		ponerOrden(8, { 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, { 11, 1, 2, 0, 0, 0 });            // 12: XI = 11,0…9 · banca = 10
		equipoIA(9, 6000, 9100, 20);                                                            // formato «identidad de 40»
		{ std::vector<uint8_t> o; for (uint8_t i = 0; i < 40; i++) o.push_back(i); ponerOrden(9, o, { 0, 0, 0, 0, 0, 0 }); }
		equipoIA(10, 6100, 9200, 15);                                                           // formato «0…14, ff, 15…38»
		{ std::vector<uint8_t> o; for (uint8_t i = 0; i < 15; i++) o.push_back(i); o.push_back(0xff); for (uint8_t i = 15; i < 39; i++) o.push_back(i); ponerOrden(10, o, { 0, 0, 0, 0, 0, 0 }); }
		equipoIA(11, 6200, 9250, 13);                                                           // orden roto (repetido): no se toca
		{ std::vector<uint8_t> o; for (uint8_t i = 0; i < 13; i++) o.push_back(i); o[1] = 0; ponerOrden(11, o, { 0, 0, 0, 0, 0, 0 }); }
		equipoIA(12, 6300, 9300, 25);                                                           // 25 con posiciones, para los sustitutos
		{ std::vector<uint8_t> o; for (uint8_t i = 0; i < 25; i++) o.push_back(i); ponerOrden(12, o, { 3, 7, 7, 3, 3, 20 }); }
		// Posiciones del 12: XI 0–10 · banca 11–17 · reservas 18–24  (0 PT, 1 DC, 2 LI, 3 LD, 4 MCD, 5 MC, 6 II, 8 MP, 9 EI, 10 ED, 12 DC)
		const int pos12[25] = { 0, 1, 2, 3, 1, 4, 5, 8, 9, 12, 10,   0, 1, 3, 5, 8, 12, 9,   0, 1, 2, 5, 12, 10, 6 };
		auto posicionDe = [&](uint32_t pid) { return pid >= 9300 && pid < 9325 ? pos12[pid - 9300] : -1; };

		// Ficha del último partido (la crea el juego al jugar): registros de 16 B con SOLO los que jugaron, en orden de
		// plantilla. Empieza igual que una tabla (0,1,2,3,4…) pero no tiene a todos: no se debe tocar.
		const size_t FICHA = FIN + 0x180000;
		const uint32_t jugaron[12] = { 0, 1, 2, 3, 4, 6, 9, 10, 12, 20, 21, 22 };
		for (size_t i = 0; i < 17; i++) {
			const size_t r = FICHA + 16 * i;
			p32(r, i < 12 ? 0x60195a : 0); p32(r + 4, i < 12 ? 1000 + jugaron[i] : 0xffff); p32(r + 8, i < 12 ? 5000 + jugaron[i] : 0); p32(r + 12, 0);
		}
		const std::vector<uint8_t> fichaAntes(d.begin() + long(FICHA), d.begin() + long(FICHA + 16 * 17));

		// Dinero: a 0x97d38 de la tabla A (como en el juego): 163.240.600 € de presupuesto, 147.520.600 € de tope.
		const size_t FINZ = tablas[0].reg0 + 0x97d38;
		p32(FINZ, 1632406); p32(FINZ + 0x10, 1632406); p32(FINZ + 0x14, 1475206);

		// Carrera recién empezada (respaldo r0 del juego): A2 está VACÍA (0xffff). Tiene que funcionar igual, sin A2.
		{
			std::vector<uint8_t> d0 = d;
			for (size_t i = 0; i < 32; i++) { const size_t r = tablas[1].reg0 + 24 * i; for (size_t b = 0; b < 24; b++) d0[r - 4 + b] = 0; d0[r] = 0xff; d0[r + 1] = 0xff; }
			auto g0 = GuardadoLM::desdeDatos(d0);
			CHECK(g0.ok());
			if (g0.ok()) {
				CHECK(g0.valor->tablasDe(5).size() == 13);   // las 12 de la plantilla + la ficha del partido (sin A2)
				auto f0 = g0.valor->finanzas(5);
				CHECK(f0.ok() && f0.valor->presupuestoFichajes == 163240600 && f0.valor->sueldosActuales == 351 * 100);
				auto v0 = g0.valor->moverUsuarioAIA(5, 7, 5003, 3, 5020);
				if (!v0.ok()) std::printf("  sin A2: %s %s\n", v0.error.codigo.c_str(), v0.error.detalle.c_str());
				CHECK(v0.ok() && g0.valor->equipo(5).valor->plantilla.size() == 25 && g0.valor->equipo(7).valor->plantilla.size() == 4);
			}
		}

		dSint = d;   // copia para las pruebas de sincronización
		auto g = GuardadoLM::desdeDatos(d);
		CHECK(g.ok());
		if (g.ok()) {
			{   // Finanzas por ancla: sueldos = suma de los 26 contratos vigentes (cada uno lleva su marca i+1 en el sueldo: 1+2+…+26 = 351 ×100)
				auto f = g.valor->finanzas(5);
				if (!f.ok()) std::printf("  finanzas: %s %s\n", f.error.codigo.c_str(), f.error.detalle.c_str());
				CHECK(f.ok());
				if (f.ok()) {
					CHECK(f.valor->presupuestoFichajes == 163240600 && f.valor->topeSalarial == 147520600 && f.valor->presupuestoFichajesInicial == 163240600);
					CHECK(f.valor->sueldosActuales == 351 * 100);   // 1+2+…+26 contratos vigentes; la oferta (tipo 3) no cuenta
					CHECK(f.valor->presupuestoSalarial() == f.valor->topeSalarial - f.valor->sueldosActuales);
				}
				CHECK(g.valor->finanzas(7).error.codigo == "NO_ES_USUARIO");
				CHECK(g.valor->fijarFinanzas(5, 500000000, 0).ok());
				auto f2 = g.valor->finanzas(5);
				CHECK(f2.ok() && f2.valor->presupuestoFichajes == 500000000 && f2.valor->topeSalarial == 147520600);
				CHECK(g.valor->fijarFinanzas(5, 0, 1).error.codigo == "IMPORTE_INVALIDO");
				CHECK(g.valor->fijarFinanzas(5, 0, 100).error.codigo == "TOPE_INSUFICIENTE");
				CHECK(g.valor->fijarFinanzas(5, 0, 200000000).ok() && g.valor->finanzas(5).valor->presupuestoSalarial() == 200000000 - g.valor->finanzas(5).valor->sueldosActuales);
			}
			CHECK(g.valor->esEquipoUsuario(5) && !g.valor->esEquipoUsuario(7));
			CHECK(g.valor->tablasDe(5).size() == 14);   // las 12 de la plantilla + A2 + la ficha del partido (la venta la descarta)
			if (g.valor->tablasDe(5).size() != 14) for (auto& t : g.valor->tablasDe(5)) std::printf("  tabla paso %zu dir %d en 0x%zx\n", t.stride, t.dir, t.ofsReg0);
			auto a = g.valor->alineacionDe(5);
			CHECK(a.ok());
			if (a.ok()) { CHECK(a.valor->ofsOrden == LU && a.valor->ofsRoles == RO && a.valor->ofsK == K0 && a.valor->flagLibreK == 0xc1); }
			// Errores que no deben tocar nada
			const auto antes = g.valor->datos();
			CHECK(g.valor->moverUsuarioAIA(5, 7, 5009, 0, 0).error.codigo == "FALTA_SUSTITUTO");       // está en el XI (posición 16? no: orden invertido → pid 5009 = idx 9 → posición 16, banca)
			CHECK(g.valor->moverUsuarioAIA(5, 7, 5025, 0, 0).error.codigo == "FALTA_SUSTITUTO");       // idx 25 está en posición 0 (XI) y es rol
			CHECK(g.valor->moverUsuarioAIA(5, 7, 4242, 0, 0).error.codigo == "JUGADOR_NO_ESTA");
			CHECK(g.valor->moverUsuarioAIA(7, 5, 9001, 0, 0).error.codigo == "PLANTILLA_MINIMA");   // la IA de 3 jugadores no puede vender
			CHECK(g.valor->moverUsuarioAIA(8, 7, 8000, 0, 0).error.codigo == "NO_ES_USUARIO");     // equipo de la IA con 12: no tiene tablas
			CHECK(g.valor->moverUsuarioAIA(5, 7, 5003, 0, 5003).error.codigo == "SUSTITUTO_INVALIDO");
			CHECK(g.valor->datos() == antes);
			// Venta real: idx 3 (pid 5003, posición 22 → reserva, pero tiene rol) con sustituto idx 20 (pid 5020, posición 5).
			auto r = g.valor->moverUsuarioAIA(5, 7, 5003, 3, 5020);   // dorsal 3 está ocupado en el destino → el más alto libre (99)
			CHECK(r.ok());
			if (r.ok()) {
				CHECK(*r.valor == 99);
				auto u = g.valor->equipo(5), ia = g.valor->equipo(7);
				CHECK(u.valor->plantilla.size() == 25 && ia.valor->plantilla.size() == 4);
				CHECK(ia.valor->plantilla.back().pid == 5003 && ia.valor->plantilla.back().reg == 1003 && ia.valor->plantilla.back().dorsal == 99);
				CHECK(u.valor->plantilla[3].pid == 5004 && u.valor->plantilla[3].dorsal == 14);
				// Tablas, como lo hace el juego (§12): A B C D E F J M → el registro 3 queda VACÍO en su sitio (el 4 no se mueve);
				// G H (108) e I (48) → se compacta (el 4 ocupa el sitio del 3 con su contenido entero); A2 → no se toca.
				for (size_t ti = 0; ti < tablas.size(); ti++) {
					const auto& t = tablas[ti];
					const auto& dd = g.valor->datos();
					auto u32 = [&](size_t o) { return uint32_t(dd[o] | (dd[o + 1] << 8) | (dd[o + 2] << 16) | (uint32_t(dd[o + 3]) << 24)); };
					const long long r3 = (long long)t.reg0 + t.dir * 3 * (long long)t.stride, r4 = (long long)t.reg0 + t.dir * 4 * (long long)t.stride;
					const size_t nUs = t.stride == 368 ? 56 : t.stride == 48 ? 27 : 26;
					const long long rUlt = (long long)t.reg0 + t.dir * (long long)(nUs - 1) * (long long)t.stride;
					if (ti == 1) {   // A2 intacta
						CHECK(u32(size_t(r3)) == 1003 && u32(size_t(r3) + 4) == 5003 && u32(size_t(r3) - 4) == 0x40a00000);
					}
					else if (t.stride == 108 || t.stride == 48) {   // compactada
						CHECK(u32(size_t(r3)) == 1004 && u32(size_t(r3) + 4) == 5004 && dd[size_t(r3) + 8] == 5 && u32(size_t(r3) - 4) == (0xAA000004));
						CHECK(u32(size_t(rUlt)) == 0xffff && u32(size_t(rUlt) + 4) == 0 && dd[size_t(rUlt) + 8] == 0);
						if (t.stride == 48) {
							CHECK(u32(size_t(r3) - 28) == 4 && u32(size_t(r3) - 24) == clubU && u32(size_t(r3) - 20) == 5);   // la cabecera viaja con el registro
							const long long r25 = (long long)t.reg0 + 25 * 48, r24 = (long long)t.reg0 + 24 * 48;
							CHECK(u32(size_t(r25)) == 0xffff && u32(size_t(r24)) == 1025 && u32(size_t(r24) - 28) == 25);   // la oferta (registro 26) también se fue: 27 → 25
							bool queda5003 = false; for (int i = 0; i < 30; i++) queda5003 |= u32(size_t(t.reg0 + 48 * i) + 4) == 5003;
							CHECK(!queda5003);
						}
					}
					else {   // hueco en su sitio
						CHECK(u32(size_t(r3)) == 0xffff && u32(size_t(r3) + 4) == 0 && u32(size_t(r3) - 4) == (0xAA000003));
						CHECK(u32(size_t(r4)) == 1004 && u32(size_t(r4) + 4) == 5004 && dd[size_t(r4) + 8] == 5);
						CHECK(u32(size_t(rUlt)) == (t.stride == 368 ? 20055u : 1025u));
						size_t sucios = 0; for (size_t b = 8; b < t.stride - 4; b++) sucios += dd[size_t(r3) + b] != 0;
						if (t.stride == 24 && t.dir > 0) CHECK(sucios == 2 && dd[size_t(r3) + 16] == 0xff && dd[size_t(r3) + 17] == 0xff);   // A: fecha vacía
						else if (t.stride == 24 && t.dir < 0) CHECK(sucios == 2 && dd[size_t(r3) + 10] == 4 && dd[size_t(r3) + 11] == 4);     // B: conserva +10
						else CHECK(sucios == 0);
						if (t.stride == 368) { const long long r26 = (long long)t.reg0 + 26 * (long long)t.stride; CHECK(u32(size_t(r26)) == 20026 && u32(size_t(r26) + 4) == 7026); }
					}
				}
				// Negociaciones: la del 5003 desapareció y la lista se compactó (5001, 5012, vacía…).
				{
					const auto& dd = g.valor->datos();
					auto u32 = [&](size_t o) { return uint32_t(dd[o] | (dd[o + 1] << 8) | (dd[o + 2] << 16) | (uint32_t(dd[o + 3]) << 24)); };
					CHECK(u32(NEG + 36) == 5001 && u32(NEG + 60 + 36) == 5012 && u32(NEG + 60 + 40) == 100 && u32(NEG + 60 + 4) == clubOtro);
					CHECK(u32(NEG + 120 + 28) == 0xffffffff && u32(NEG + 120 + 32) == 0xffff && u32(NEG + 120 + 8) == 0 && u32(NEG + 180 + 28) == 0xffffffff);
					CHECK(g.valor->ultimoInforme().find("negociaciones abiertas quitadas: 1") != std::string::npos);
				}
				// Orden: 25,24,…,4,3,2,1,0 → el 3 (posición 22) lo ocupa el 20 (que estaba en la posición 5) y todo > 3 baja uno.
				auto a2 = g.valor->alineacionDe(5);
				CHECK(a2.ok());
				if (a2.ok()) {
					std::vector<uint8_t> esperado;
					for (int i = 25; i >= 0; i--) { if (i == 20) continue; esperado.push_back(uint8_t(i == 3 ? 20 - 1 : (i > 3 ? i - 1 : i))); }
					CHECK(a2.valor->orden == esperado);
					CHECK(a2.valor->roles[0] == 19 && a2.valor->roles[1] == 8 && a2.valor->roles[5] == 24 && a2.valor->roles[2] == 0);
					CHECK(a2.valor->flagLibreK == 0xc0 && a2.valor->flagsK.size() == 25 && a2.valor->flagsK[0] == 0 && a2.valor->flagsK[24] == 0xc0);
					CHECK(g.valor->datos()[LU + 25] == 0xff);
				}
			}
			// La ficha del último partido no se tocó (el vendido, idx 3, había jugado) y el informe lo explica.
			CHECK(std::vector<uint8_t>(g.valor->datos().begin() + long(FICHA), g.valor->datos().begin() + long(FICHA + 16 * 17)) == fichaAntes);
			CHECK(g.valor->ultimoInforme().find("descartada") != std::string::npos);
			// El destino de la IA (7) recibió al vendido como última reserva; sus roles no cambian.
			CHECK(leerOrden(g.valor->datos(), 7) == compacto({ 2, 0, 1, 3 }, { 0, 0, 1, 0, 0, 2 }));

			std::printf("Liga Máster interna (alineaciones de la IA)\n");
			// Sustitutos por posición (equipo 12: compacto, 25)
			auto s9 = g.valor->sugerirSustituto(12, 9309, posicionDe);   // DC titular → la reserva DC (idx 22)
			CHECK(s9.ok() && *s9.valor == 9322);
			auto s0 = g.valor->sugerirSustituto(12, 9300, posicionDe);   // portero → la reserva portero (idx 18), no el de la banca
			CHECK(s0.ok() && *s0.valor == 9318);
			auto s5 = g.valor->sugerirSustituto(12, 9305, posicionDe);   // MCD: ninguna reserva MCD → misma línea (MC, idx 21)
			CHECK(s5.ok() && *s5.valor == 9321);
			auto s12 = g.valor->sugerirSustituto(12, 9312, posicionDe);  // suplente sin rol → no hace falta nadie
			CHECK(s12.ok() && *s12.valor == 0);
			auto s20 = g.valor->sugerirSustituto(12, 9320, posicionDe);  // no titular con rol (LI) → otro lateral (LD, idx 13) antes que un DC
			CHECK(s20.ok() && *s20.valor == 9313);
			CHECK(g.valor->sugerirSustituto(12, 9300, nullptr).error.codigo == "SIN_SUSTITUTO");   // sin posiciones no se arriesga con el portero
			CHECK(g.valor->sugerirSustituto(12, 4242, posicionDe).error.codigo == "JUGADOR_NO_ESTA");
			auto su = g.valor->sugerirSustituto(5, 5004, posicionDe);    // equipo del usuario: va por su propia alineación
			CHECK(su.ok());

			// IA → IA: errores que no tocan nada
			const auto antesIA = g.valor->datos();
			CHECK(g.valor->moverEntreIA(8, 9, 8001, 0, 0).error.codigo == "FALTA_SUSTITUTO");       // titular sin sustituto
			CHECK(g.valor->moverEntreIA(8, 9, 8001, 0, 4242).error.codigo == "SUSTITUTO_NO_ESTA");
			CHECK(g.valor->moverEntreIA(11, 9, 9251, 0, 0).error.codigo == "ALINEACION_IA_INVALIDA");  // orden roto en el origen
			CHECK(g.valor->moverEntreIA(9, 11, 9101, 0, 0).error.codigo == "ALINEACION_IA_INVALIDA");  // orden roto en el destino
			CHECK(g.valor->moverEntreIA(5, 9, 5004, 0, 0).error.codigo == "EQUIPO_DEL_USUARIO");
			CHECK(g.valor->datos() == antesIA);

			// IA → IA con sustituto: 8001 (idx 1, titular en el puesto 2, con rol) sale; 8010 (banca) ocupa su puesto.
			auto m1 = g.valor->moverEntreIA(8, 9, 8001, 7, 8010);
			CHECK(m1.ok() && *m1.valor == 99);   // el 7 está ocupado en el 9 (dorsales 1…20) → el más alto libre
			CHECK(leerOrden(g.valor->datos(), 8) == compacto({ 10, 0, 9, 1, 2, 3, 4, 5, 6, 7, 8 }, { 10, 9, 1, 0, 0, 0 }));
			{ std::vector<uint8_t> o; for (uint8_t i = 0; i <= 20; i++) o.push_back(i); CHECK(leerOrden(g.valor->datos(), 9) == compacto(o, { 0, 0, 0, 0, 0, 0 })); }
			CHECK(g.valor->equipo(8).valor->plantilla.size() == 11 && g.valor->equipo(9).valor->plantilla.size() == 21);
			CHECK(g.valor->equipo(9).valor->plantilla.back().pid == 8001);

			// IA → IA sin sustituto (reserva sin rol): 9119 (idx 19) del 9 al 10 («0…14, ff, …» → compacto 0…15)
			auto m2 = g.valor->moverEntreIA(9, 10, 9119, 0, 0);
			CHECK(m2.ok());
			{ std::vector<uint8_t> o; for (uint8_t i = 0; i < 20; i++) o.push_back(i); CHECK(leerOrden(g.valor->datos(), 9) == compacto(o, { 0, 0, 0, 0, 0, 0 })); }
			{ std::vector<uint8_t> o; for (uint8_t i = 0; i <= 15; i++) o.push_back(i); CHECK(leerOrden(g.valor->datos(), 10) == compacto(o, { 0, 0, 0, 0, 0, 0 })); }
			CHECK(g.valor->equipo(9).valor->plantilla.back().pid == 8001 && g.valor->equipo(10).valor->plantilla.back().pid == 9119);

			// Usuario: un suplente sin rol se va SIN sustituto (los de atrás suben), igual que en la IA.
			// Tras la primera venta el orden del usuario es 24,23,22,21,20,18,…,4,3,19,2,1,0: en el puesto 12 está el índice 11 (5012).
			// Antes, se deja la lista K ATRASADA (la de antes de la primera venta, con 26 y el vendido), como la deja el juego
			// un tiempo tras un cambio de plantilla: hay que encontrarla igual, conservar los flags y reconstruirla.
			{
				std::vector<uint8_t> dd = g.valor->datos();
				std::copy(antes.begin() + long(K0), antes.begin() + long(K0 + 16 * 29), dd.begin() + long(K0));
				auto gk = GuardadoLM::desdeDatos(dd); CHECK(gk.ok());
				auto ak = gk.valor->alineacionDe(5);
				CHECK(ak.ok() && !ak.valor->kEspejo && ak.valor->orden.size() == 25 && ak.valor->flagsK.size() == 25 && ak.valor->flagsK[0] == 0 && ak.valor->flagsK[1] == 0xc0);
				auto vk = gk.valor->moverUsuarioAIA(5, 7, 5012, 0, 0);
				CHECK(vk.ok());
				auto ak2 = gk.valor->alineacionDe(5);
				CHECK(ak2.ok() && ak2.valor->kEspejo && ak2.valor->orden.size() == 24 && ak2.valor->flagLibreK == 0xc0);
			}
			auto a3 = g.valor->alineacionDe(5);
			CHECK(a3.ok() && a3.valor->kEspejo && a3.valor->orden.size() == 25 && a3.valor->orden[12] == 11);
			auto v2 = g.valor->moverUsuarioAIA(5, 7, 5012, 0, 0);
			if (!v2.ok()) std::printf("  v2: %s %s\n", v2.error.codigo.c_str(), v2.error.detalle.c_str());
			CHECK(v2.ok());
			auto a4 = g.valor->alineacionDe(5);
			CHECK(a4.ok() && a4.valor->orden.size() == 24 && a4.valor->flagLibreK == 0xc0);
			if (a3.ok() && a4.ok()) {
				std::vector<uint8_t> esperado;
				for (size_t i = 0; i < a3.valor->orden.size(); i++) { const uint8_t v = a3.valor->orden[i]; if (v == 11) continue; esperado.push_back(uint8_t(v > 11 ? v - 1 : v)); }
				CHECK(a4.valor->orden == esperado);
			}
			CHECK(leerOrden(g.valor->datos(), 7) == compacto({ 2, 0, 1, 3, 4 }, { 0, 0, 1, 0, 0, 2 }));
		}
	}

	std::printf("Firma Ed25519 y sobres de la web\n");
	{
		CHECK(aBase64({ 'M','a','n' }) == "TWFu" && aBase64({ 'M','a' }) == "TWE=" && aBase64({ 'M' }) == "TQ==");
		CHECK(desdeBase64("TWFu") == std::vector<uint8_t>({ 'M','a','n' }) && desdeBase64("TWE=") == std::vector<uint8_t>({ 'M','a' }) && desdeBase64("TQ==") == std::vector<uint8_t>({ 'M' }));
		CHECK(desdeBase64("T$==").empty());
		std::vector<uint8_t> semilla(32); for (int i = 0; i < 32; i++) semilla[i] = uint8_t(i);
		auto par = parClavesDesdeSemilla(semilla);
		// Misma clave pública que la biblioteca estándar (cryptography de Python) con esta semilla: la implementación es compatible.
		CHECK(aBase64(par.publica) == "A6EHv/POEL4dcN0Y50vAmWfk1jCbpQ1fHdyGZBJVMbg=");
		const std::string contenido = "{\"liga\":\"galaxy\",\"desde\":0,\"version_actual\":3,\"cambios\":[]}";
		const std::string firma = firmarTexto(par, contenido);
		// Firma que produjo Python para el mismo texto y la misma clave: idéntica byte a byte.
		CHECK(firma == "sYTfVmWe+AZPIXqVNx9dwkf306fDV/0cNV+JjrYs8oIOEgL5TxRFdyajDkjpprnPfmPlVeORm5DhxDeJRt++AQ==");
		ClavePublica clave; clave.claveId = "k1"; clave.publica = par.publica;
		CHECK(verificarFirma(clave, contenido, firma));
		CHECK(!verificarFirma(clave, contenido + " ", firma));
		nlohmann::json sobre = { {"contenido", contenido}, {"firma", firma}, {"clave_id", "k1"}, {"algoritmo", "Ed25519"} };
		auto ab = abrirSobreFirmado(sobre.dump(), clave);
		CHECK(ab.ok() && *ab.valor == contenido);
		sobre["clave_id"] = "k2"; CHECK(abrirSobreFirmado(sobre.dump(), clave).error.codigo == "CLAVE_DESCONOCIDA"); sobre["clave_id"] = "k1";
		sobre["contenido"] = contenido + " "; CHECK(abrirSobreFirmado(sobre.dump(), clave).error.codigo == "FIRMA_INVALIDA"); sobre["contenido"] = contenido;
		sobre["algoritmo"] = "RSA"; CHECK(abrirSobreFirmado(sobre.dump(), clave).error.codigo == "SOBRE_INVALIDO");
		CHECK(abrirSobreFirmado("no es json", clave).error.codigo == "SOBRE_INVALIDO");
		CHECK(abrirSobreFirmado("{}", clave).error.codigo == "SOBRE_INVALIDO");
	}

	std::printf("Sincronización con la web (cambios de liga)\n");
	{
		using namespace mercado::sinc;
		auto l = parsearCambios(R"({"liga":"galaxy","desde":2,"version_actual":5,"cambios":[
			{"version":3,"phoenix_id":10,"pes_id":9101,"club_desde":"Nueve","club_hacia":"Diez","club_desde_pes":2009,"club_hacia_pes":2010,"tipo":"traspaso","fecha":"2026-10-08"},
			{"version":4,"phoenix_id":11,"pes_id":5012,"club_desde":"Usuario","club_hacia":"Siete","club_desde_pes":2005,"club_hacia_pes":2007,"tipo":"traspaso"},
			{"version":5,"phoenix_id":12,"pes_id":null,"club_desde":"Siete","club_hacia":null,"club_desde_pes":2007,"club_hacia_pes":null,"tipo":"libre"}]})");
		CHECK(l.ok());
		if (l.ok()) {
			CHECK(l.valor->liga == "galaxy" && l.valor->desde == 2 && l.valor->versionActual == 5 && l.valor->cambios.size() == 3);
			CHECK(l.valor->cambios[0].pesId == 9101 && l.valor->cambios[0].clubDesdePes == 2009 && l.valor->cambios[0].clubHaciaPes == 2010);
			CHECK(l.valor->cambios[2].pesId == 0 && l.valor->cambios[2].clubHaciaPes == 0);
		}
		CHECK(parsearCambios(R"({"cambios":[{"version":2},{"version":2}]})").error.codigo == "CAMBIOS_INVALIDOS");
		CHECK(parsearCambios(R"({"cambios":[{"version":9},{"version":3}]})").error.codigo == "CAMBIOS_INVALIDOS");
		CHECK(parsearCambios("[]").error.codigo == "CAMBIOS_INVALIDOS");
		CHECK(cuerpoAplicado(7, "abc") == R"({"huella_plantillas":"abc","version":7})");

		// Liga Máster sintética (la misma de arriba, desde cero): usuario = 5 (ID real 2005), IA 7…12 (ID 2000 + k).
		{
			using namespace mercado::lm;
			auto gl = GuardadoLM::desdeDatos(dSint);
			CHECK(gl.ok());
			auto cambios = parsearCambios(R"({"liga":"galaxy","desde":0,"version_actual":8,"cambios":[
				{"version":1,"pes_id":9101,"club_desde":"Nueve","club_hacia":"Diez","club_desde_pes":2009,"club_hacia_pes":2010},
				{"version":2,"pes_id":5012,"club_desde":"Usuario","club_hacia":"Siete","club_desde_pes":2005,"club_hacia_pes":2007},
				{"version":3,"pes_id":8005,"club_desde":"Ocho","club_hacia":"Usuario","club_desde_pes":2008,"club_hacia_pes":2005},
				{"version":4,"pes_id":9001,"club_desde":"Siete","club_hacia":null,"club_desde_pes":2007,"club_hacia_pes":null},
				{"version":5,"pes_id":null,"club_desde":"Siete","club_hacia":"Nueve","club_desde_pes":2007,"club_hacia_pes":2009},
				{"version":6,"pes_id":9324,"club_desde":"Once","club_hacia":"Nueve","club_desde_pes":2011,"club_hacia_pes":2009},
				{"version":7,"pes_id":9102,"club_desde":"Nueve","club_hacia":"Fantasma","club_desde_pes":2009,"club_hacia_pes":99999},
				{"version":8,"pes_id":9103,"club_desde":"Nueve","club_hacia":"Nueve","club_desde_pes":2009,"club_hacia_pes":2009}]})");
			CHECK(cambios.ok());
			if (gl.ok() && cambios.ok()) {
				const auto antes = gl.valor->datos();
				// Sin option file ni LM en el alcance → error y nada cambia.
				CHECK(aplicarCambios(*cambios.valor, 0, { false, false }, nullptr, &*gl.valor, nullptr).error.codigo == "SIN_DESTINO");
				auto inf = aplicarCambios(*cambios.valor, 0, { true, true }, nullptr, &*gl.valor, nullptr);
				if (!inf.ok()) std::printf("  sincronización: %s %s\n", inf.error.codigo.c_str(), inf.error.detalle.c_str());
				CHECK(inf.ok());
				if (inf.ok()) {
					CHECK(inf.valor->versionAplicada == 8 && inf.valor->aplicadosOption == 0);
					CHECK(inf.valor->aplicadosLM == 4 && inf.valor->pendientesLM == 3);   // v1, v2, v6 y v8 (ya estaba); v3, v4, v7 pendientes
					CHECK(inf.valor->lineas.size() == 8);
					CHECK(inf.valor->lineas[0].ligaMaster && inf.valor->lineas[1].ligaMaster && inf.valor->lineas[5].ligaMaster && inf.valor->lineas[7].ligaMaster);
					CHECK(!inf.valor->lineas[2].pendienteLM.empty() && !inf.valor->lineas[3].pendienteLM.empty() && !inf.valor->lineas[6].pendienteLM.empty());
					CHECK(inf.valor->lineas[4].texto.find("sin pes_id") != std::string::npos);
					CHECK(inf.valor->lineas[5].texto.find("estaba en") != std::string::npos);   // la web decía club 2011; en la LM estaba en el 12
					CHECK(inf.valor->lineas[7].texto.find("ya estaba") != std::string::npos);
					CHECK(gl.valor->equipo(10).valor->plantilla.back().pid == 9101 && gl.valor->equipo(9).valor->plantilla.size() == 20);   // 20 - 9101 + 9324
					CHECK(gl.valor->equipo(7).valor->plantilla.back().pid == 5012 && gl.valor->equipo(5).valor->plantilla.size() == 25);
					CHECK(gl.valor->equipo(12).valor->plantilla.size() == 24);
					std::printf("%s", inf.valor->texto().c_str());
				}
				// Todo o nada: un cambio que falla (el equipo 11 tiene la alineación rota) deja la Liga Máster como estaba.
				auto g2 = GuardadoLM::desdeDatos(dSint);
				auto malos = parsearCambios(R"({"cambios":[
					{"version":1,"pes_id":9101,"club_desde_pes":2009,"club_hacia_pes":2010},
					{"version":2,"pes_id":9251,"club_desde_pes":2011,"club_hacia_pes":2009}]})");
				CHECK(malos.ok());
				auto r2 = aplicarCambios(*malos.valor, 0, { true, true }, nullptr, &*g2.valor, nullptr);
				CHECK(!r2.ok() && r2.error.codigo == "ALINEACION_IA_INVALIDA" && g2.valor->datos() == antes);
				// Las versiones ya aplicadas se saltan.
				auto g3 = GuardadoLM::desdeDatos(dSint);
				auto r3 = aplicarCambios(*cambios.valor, 7, { true, true }, nullptr, &*g3.valor, nullptr);
				CHECK(r3.ok() && r3.valor->lineas.size() == 1 && r3.valor->versionAplicada == 8 && g3.valor->datos() == antes);
			}
		}
	}

	// --- Pruebas con archivos reales (opcionales) -------------------------
	//  PM_EDIT = ruta a una COPIA de EDIT00000000 · PM_CPK = ruta a CGP_database.cpk
	const char* rEdit = std::getenv("PM_EDIT");
	const char* rCpk = std::getenv("PM_CPK");
	if (rEdit) {
		std::printf("Option file real\n");
		const std::string huellaAntes = sha256::deArchivo(rEdit);
		auto of = OptionFile::abrir(rEdit);
		CHECK(of.ok());
		if (of.ok()) {
			CHECK(of.valor->equipos().size() > 100);
			CHECK(of.valor->plantillas().size() == of.valor->equipos().size());
			// Catálogo de posiciones del parche (opcional, PM_CATALOGO = catálogo JSON): permite probar al portero.
			std::function<int(uint32_t)> posicionDe;
			if (const char* rCat = std::getenv("PM_CATALOGO")) {
				std::ifstream fc(rCat); auto cj = nlohmann::json::parse(fc);
				static std::map<uint32_t, int> posiciones;
				static const std::map<std::string, int> codigo = { {"GK",0},{"CB",1},{"LB",2},{"RB",3},{"DMF",4},{"CMF",5},{"LMF",6},{"RMF",7},{"AMF",8},{"LWF",9},{"RWF",10},{"SS",11},{"CF",12} };
				for (auto& j : cj["jugadores"]) { auto it = codigo.find(j.value("posicion", "")); if (it != codigo.end()) posiciones[j["pes_id"].get<uint32_t>()] = it->second; }
				posicionDe = [](uint32_t id) { auto it = posiciones.find(id); return it == posiciones.end() ? -1 : it->second; };
				std::printf("  catálogo de posiciones: %zu jugadores\n", posiciones.size());
			}
			// Elegir un jugador cualquiera (la última reserva) de un equipo y moverlo a otro. Con catálogo, el equipo de
			// origen es uno con alineación «normal» (un solo portero en el XI y otro de reserva), como un club jugable.
			uint32_t jug = 0, origen = 0, destino = 0;
			for (const auto& [eq, pl] : of.valor->plantillas()) {
				if (!jug && pl.size() >= 18 && pl.size() < 40) {
					if (posicionDe) {
						auto a = of.valor->alineacion(eq);
						if (!a.ok()) continue;
						int porterosXI = 0, porteros = 0;
						for (size_t i = 0; i < a.valor->orden.size(); i++) { const bool pt = posicionDe(pl[a.valor->orden[i]].jugador) == 0; porteros += pt; porterosXI += pt && i < 11; }
						if (porterosXI != 1 || porteros < 2) continue;
					}
					jug = pl.back().jugador; origen = eq;
				}
				else if (jug && !destino && eq != origen && !pl.empty() && pl.size() < 39) destino = eq;
			}
			std::printf("  origen %u → destino %u, jugador %u\n", origen, destino, jug);
			auto copia = *of.valor;
			CHECK(copia.mover(jug, destino, origen).ok());
			const std::string salida = (fs::temp_directory_path() / "pm-edit-prueba").string();
			fs::remove(salida);
			auto g = copia.guardarComo(salida);
			CHECK(g.ok());
			CHECK(copia.guardarComo(salida).error.codigo == "DESTINO_OCUPADO");
			auto re = OptionFile::abrir(salida);
			bool enDestino = false, enOrigen = false;
			if (re.ok()) {
				for (auto& p : re.valor->plantillas().at(destino)) enDestino |= p.jugador == jug;
				for (auto& p : re.valor->plantillas().at(origen)) enOrigen |= p.jugador == jug;
			}
			CHECK(enDestino && !enOrigen);
			CHECK(sha256::deArchivo(rEdit) == huellaAntes);   // el original no se tocó
			fs::remove(salida);

			// Alineación (bloque de tácticas): el que se va era la última reserva → el XI del origen no cambia y el que
			// llega es la última reserva del destino. Con un TITULAR hace falta sustituto y el XI conserva a los otros 10.
			{
				auto aO = of.valor->alineacion(origen), aD = of.valor->alineacion(destino);
				CHECK(aO.ok() && aD.ok());
				if (aO.ok() && aD.ok()) {
					const auto& plO = of.valor->plantillas().at(origen);
					const auto& plD = of.valor->plantillas().at(destino);
					auto xi = [](const std::vector<PlazaPlantilla>& pl, const AlineacionLocal& a) {
						std::vector<uint32_t> v; for (size_t i = 0; i < 11 && i < a.orden.size(); i++) v.push_back(pl[a.orden[i]].jugador); return v; };
					const auto xiO = xi(plO, *aO.valor), xiD = xi(plD, *aD.valor);
					auto a2O = copia.alineacion(origen), a2D = copia.alineacion(destino);
					CHECK(a2O.ok() && a2D.ok());
					if (a2O.ok() && a2D.ok()) {
						CHECK(xi(copia.plantillas().at(origen), *a2O.valor) == xiO);
						CHECK(xi(copia.plantillas().at(destino), *a2D.valor) == xiD);
						CHECK(a2D.valor->orden.size() == plD.size() + 1 && a2D.valor->orden.back() == plD.size());
						CHECK(a2O.valor->orden.size() == plO.size() - 1);
					}
					// Titular: sin sustituto se niega; con el sugerido, el XI conserva a los otros 10 y el que se va ya no está.
					// Sin catálogo de posiciones se prueba con un jugador de campo (puesto 5); con PM_CATALOGO (catálogo JSON
					// del parche) también con el portero (puesto 0), que solo puede cubrirlo otro portero.
					// Sincronización con la web sobre el option file real: el titular del puesto 5 cambia de club por una lista de
					// cambios firmada; el XI del origen conserva a los otros 10 y el que llega es la última reserva del destino.
					{
						const uint32_t titular = plO[aO.valor->orden[5]].jugador;
						nlohmann::json lista = { {"liga","galaxy"}, {"desde",0}, {"version_actual",1}, {"cambios", nlohmann::json::array({
							{ {"version",1}, {"pes_id",titular}, {"club_desde","A"}, {"club_hacia","B"}, {"club_desde_pes",origen}, {"club_hacia_pes",destino} } }) } };
						auto lc = mercado::sinc::parsearCambios(lista.dump());
						CHECK(lc.ok());
						auto c3 = *of.valor;
						auto inf = mercado::sinc::aplicarCambios(*lc.valor, 0, { true, false }, &c3, nullptr, posicionDe);
						if (!inf.ok()) std::printf("  sincronización option real: %s %s\n", inf.error.codigo.c_str(), inf.error.detalle.c_str());
						CHECK(inf.ok() && inf.valor->aplicadosOption == 1 && inf.valor->versionAplicada == 1);
						bool enB = false; for (auto& p : c3.plantillas().at(destino)) enB |= p.jugador == titular;
						bool enA = false; for (auto& p : c3.plantillas().at(origen)) enA |= p.jugador == titular;
						CHECK(enB && !enA);
						auto a3 = c3.alineacion(origen);
						CHECK(a3.ok());
						if (a3.ok()) { const auto xi3 = xi(c3.plantillas().at(origen), *a3.valor); for (size_t i = 0; i < 11; i++) if (i != 5) CHECK(xi3[i] == xiO[i]); CHECK(xi3[5] != titular); }
						auto aD3 = c3.alineacion(destino);
						CHECK(aD3.ok() && aD3.valor->orden.back() == plD.size() && c3.plantillas().at(destino).back().jugador == titular);
						// Y el camino de «agente libre» (club_hacia null): sale del club y no entra en ninguno.
						nlohmann::json libre = { {"cambios", nlohmann::json::array({ { {"version",1}, {"pes_id",titular}, {"club_desde_pes",origen}, {"club_hacia_pes",nullptr} } }) } };
						auto ll = mercado::sinc::parsearCambios(libre.dump());
						auto c4 = *of.valor;
						auto inf4 = mercado::sinc::aplicarCambios(*ll.valor, 0, { true, false }, &c4, nullptr, posicionDe);
						CHECK(inf4.ok() && inf4.valor->aplicadosOption == 1);
						// (puede seguir en su selección: solo se le saca del club)
						int equiposAntes = 0, equiposDespues = 0;
						for (auto& [eq, pl] : of.valor->plantillas()) for (auto& p : pl) equiposAntes += p.jugador == titular;
						for (auto& [eq, pl] : c4.plantillas()) for (auto& p : pl) equiposDespues += p.jugador == titular;
						bool enOrigen4 = false; for (auto& p : c4.plantillas().at(origen)) enOrigen4 |= p.jugador == titular;
						CHECK(!enOrigen4 && equiposDespues == equiposAntes - 1 && c4.plantillas().at(origen).size() == plO.size() - 1);
					}
					for (size_t puesto : { size_t(5), size_t(0) }) {
						if (puesto == 0 && !posicionDe) continue;
						const uint32_t titular = plO[aO.valor->orden[puesto]].jugador;
						auto c2 = *of.valor;
						CHECK(c2.mover(titular, destino, origen).error.codigo == "FALTA_SUSTITUTO");
						auto sug = of.valor->sugerirSustituto(origen, titular, posicionDe);
						if (!sug.ok()) std::printf("  sustituto puesto %zu: %s %s\n", puesto, sug.error.codigo.c_str(), sug.error.detalle.c_str());
						CHECK(sug.ok() && *sug.valor != 0 && *sug.valor != titular);
						if (sug.ok()) {
							if (posicionDe && puesto == 0) CHECK(posicionDe(*sug.valor) == 0);   // al portero lo cubre otro portero
							CHECK(c2.mover(titular, destino, origen, 0, *sug.valor).ok());
							auto a3 = c2.alineacion(origen);
							CHECK(a3.ok());
							if (a3.ok()) {
								const auto xi3 = xi(c2.plantillas().at(origen), *a3.valor);
								CHECK(xi3[puesto] == *sug.valor);
								for (size_t i = 0; i < 11; i++) if (i != puesto) CHECK(xi3[i] == xiO[i]);
								bool sigue = false; for (auto v : xi3) sigue |= v == titular; CHECK(!sigue);
								CHECK(a3.valor->colaIdentidad == aO.valor->colaIdentidad);
							}
						}
					}
				}
			}

			if (rCpk) {
				std::printf("Base de datos del parche + catálogo\n");
				auto base = leerBaseDatos(rCpk);
				CHECK(base.ok() && base.valor->size() > 1000);
				if (base.ok()) {
					auto cat = construirCatalogo(*of.valor, *base.valor, "prueba");
					std::printf("  catálogo: %d equipos, %d jugadores, %d sin datos\n", cat.equipos, cat.jugadores, cat.sinDatos);

					{   // nombres de club únicos (la web identifica clubes por nombre)
						auto cj = nlohmann::json::parse(cat.json); std::set<std::string> vistos; bool repetido = false;
						for (auto& e : cj["equipos"]) repetido |= !vistos.insert(e["nombre"].get<std::string>()).second;
						CHECK(!repetido);
					}
					CHECK(cat.jugadores > 1000 && cat.sinDatos * 50 < cat.jugadores);
					// Los jugadores editados deben coincidir en altura con la base (dato estable).
					int iguales = 0, revisados = 0;
					for (const auto& e : of.valor->editados()) {
						auto it = base.valor->find(e.id);
						if (it == base.valor->end()) continue;
						revisados++; iguales += it->second.altura == e.altura && it->second.nacionalidad == e.nacionalidad;
					}

					auto anom = revisarEstructura(*of.valor, *base.valor);
					for (auto& a : anom) std::printf("  [%s] %s %s\n", a.gravedad.c_str(), a.codigo.c_str(), a.detalle.c_str());
					CHECK(!hayBloqueo(anom));                         // option y base del mismo parche
					if (const char* otro = std::getenv("PM_CPK_OTRO")) {   // base de OTRO parche
						auto b2 = leerBaseDatos(otro);
						CHECK(b2.ok() && hayBloqueo(revisarEstructura(*of.valor, *b2.valor)));   // debe entrar en modo seguro
						if (b2.ok()) {
							std::vector<uint32_t> liga;
							for (auto& [eq, pl] : of.valor->plantillas()) for (auto& p : pl) if (liga.size() < 500) liga.push_back(p.jugador);
							auto inf = compararBases(*base.valor, *b2.valor, liga);
							std::printf("  cambios entre parches: %d nuevos, %d eliminados, %d modificados | liga: %zu eliminados, %zu otra identidad\n",
								inf.nuevos, inf.eliminados, inf.modificados, inf.ligaEliminados.size(), inf.ligaCambiaronIdentidad.size());
							CHECK(inf.nuevos > 0 && inf.eliminados > 0);
						}
					}
					std::printf("  editados que coinciden: %d/%d\n", iguales, revisados);
					CHECK(iguales == revisados);   // un save nuevo puede no tener editados
				}
			}
		}
	}

	std::printf("\n%d/%d pruebas OK\n", total - fallos, total);
	return fallos == 0 ? 0 : 1;
}
