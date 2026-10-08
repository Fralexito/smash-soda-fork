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
		struct T { size_t stride; int dir; size_t reg0; };
		std::vector<T> tablas = { {24,+1,0}, {24,-1,0}, {44,+1,0}, {368,+1,0}, {192,+1,0}, {52,+1,0}, {108,+1,0}, {108,+1,0}, {108,+1,0}, {5628,+1,0}, {48,+1,0}, {16,+1,0} };
		size_t cur = FIN + 0x1000;
		for (auto& t : tablas) {
			const size_t nreg = t.stride == 368 ? 60 : 30;   // la de 368 lleva 26 + juveniles
			size_t ini = cur + (t.dir < 0 ? t.stride * (nreg + 2) : 0);
			t.reg0 = ini + 4;
			for (size_t i = 0; i < nreg + 2; i++) {
				const long long r0 = (long long)ini + t.dir * (long long)(i * t.stride);
				const bool usado = i < (t.stride == 368 ? 26 + 30 : 26);
				uint32_t reg = 0xffff, pid = 0;
				if (usado) { if (i < 26) { reg = 1000 + uint32_t(i); pid = 5000 + uint32_t(i); } else { reg = 20000 + uint32_t(i); pid = 7000 + uint32_t(i); } }
				p32(size_t(r0), 0xAA000000 | uint32_t(i)); p32(size_t(r0) + 4, reg); p32(size_t(r0) + 8, pid);
				for (size_t b = 12; b < t.stride; b++) d[size_t(r0) + b] = uint8_t(usado ? (i + 1) : 0);
			}
			cur += t.stride * (nreg + 3) + 64;
		}
		// Orden de formación (permutación), roles y lista K en su espejo.
		std::vector<uint8_t> orden; for (int i = 25; i >= 0; i--) orden.push_back(uint8_t(i));   // 25,24,…,0
		const size_t LU = FIN + 0x100, RO = LU + 0x28;
		for (size_t i = 0; i < 40; i++) d[LU + i] = i < 26 ? orden[i] : 0xff;
		const uint8_t roles[6] = { 3, 9, 0, 0, 0, 25 }; for (int i = 0; i < 6; i++) d[RO + i] = roles[i];
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

		auto g = GuardadoLM::desdeDatos(d);
		CHECK(g.ok());
		if (g.ok()) {
			CHECK(g.valor->esEquipoUsuario(5) && !g.valor->esEquipoUsuario(7));
			CHECK(g.valor->tablasDe(5).size() == 12);
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
				// Tablas: el registro 3 desapareció, el 4 ocupa su sitio con su contenido entero, el último usado quedó vacío.
				for (const auto& t : tablas) {
					const auto& dd = g.valor->datos();
					auto u32 = [&](size_t o) { return uint32_t(dd[o] | (dd[o + 1] << 8) | (dd[o + 2] << 16) | (uint32_t(dd[o + 3]) << 24)); };
					const long long r3 = (long long)t.reg0 + t.dir * 3 * (long long)t.stride;
					CHECK(u32(size_t(r3)) == 1004 && u32(size_t(r3) + 4) == 5004 && dd[size_t(r3) + 8] == 5 && u32(size_t(r3) - 4) == (0xAA000000 | 4));
					const size_t nUs = t.stride == 368 ? 56 : 26;
					const long long rUlt = (long long)t.reg0 + t.dir * (long long)(nUs - 1) * (long long)t.stride;
					CHECK(u32(size_t(rUlt)) == 0xffff && u32(size_t(rUlt) + 4) == 0 && dd[size_t(rUlt) + 8] == 0);
					if (t.stride == 368) { const long long r25 = (long long)t.reg0 + 25 * (long long)t.stride; CHECK(u32(size_t(r25)) == 20026 && u32(size_t(r25) + 4) == 7026); }
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
			auto s20 = g.valor->sugerirSustituto(12, 9320, posicionDe);  // reserva con rol (LI) → otro defensa de las reservas (DC, idx 19)
			CHECK(s20.ok() && *s20.valor == 9319);
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
			// Elegir un jugador cualquiera de la primera plantilla con gente y moverlo a otra.
			uint32_t jug = 0, origen = 0, destino = 0;
			for (const auto& [eq, pl] : of.valor->plantillas()) {
				if (!jug && !pl.empty() && pl.size() < 40) { jug = pl.back().jugador; origen = eq; }
				else if (jug && !destino && eq != origen && !pl.empty() && pl.size() < 39) destino = eq;
			}
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
