// Pruebas del lector de estado.json (phoenix_estadio.lua → Link), incluido el goleador de la v1.1.
// MSVC: cl /std:c++20 /EHsc /utf-8 /I SmashSoda /I external pruebas-link\estadojuego_test.cpp SmashSoda\phoenix\link\EstadoJuego.cpp
#include <cstdio>
#include <string>
#include <nlohmann/json.hpp>

#include "phoenix/link/EstadoJuego.h"

using phoenix::juego::EstadoPartidoJuego;
using phoenix::juego::parsear;

static int fallos = 0;
#define COMPROBAR(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

// ¿Se puede volcar a JSON (como hace Link al mandar a la web) sin lanzar por UTF-8 roto?
static bool vuelcaBien(const EstadoPartidoJuego& e) {
	try {
		nlohmann::json g = nlohmann::json::array();
		for (const auto& x : e.goles) g.push_back({ {"j", x.jugador} });
		const std::string s = nlohmann::json{ {"l", e.nombreLocal}, {"v", e.nombreVisita}, {"g", g} }.dump();
		return !s.empty();
	}
	catch (...) { return false; }
}

int main() {
	// 1) Formato v1.0 (sin goleador): sigue igual
	{
		EstadoPartidoJuego e;
		COMPROBAR(parsear(R"({"v":2,"fase":"en_juego","minuto":34,"goles_local":1,"goles_visita":0,"goles":[{"m":12,"l":"l"}]})", e));
		COMPROBAR(e.goles.size() == 1);
		COMPROBAR(e.goles[0].minuto == 12 && e.goles[0].local);
		COMPROBAR(e.goles[0].jugador.empty() && e.goles[0].idJugador == 0 && !e.goles[0].propia);
	}
	// 2) v1.1 con goleador, id y propia puerta
	{
		EstadoPartidoJuego e;
		COMPROBAR(parsear(R"({"fase":"en_juego","goles":[{"m":12,"l":"l","j":"MESSI","id":7511},{"m":80,"l":"v","j":"O'NEIL \"JR\"","id":12,"pp":true}]})", e));
		COMPROBAR(e.goles.size() == 2);
		COMPROBAR(e.goles[0].jugador == "MESSI" && e.goles[0].idJugador == 7511 && !e.goles[0].propia);
		COMPROBAR(!e.goles[1].local && e.goles[1].jugador == "O'NEIL \"JR\"" && e.goles[1].idJugador == 12 && e.goles[1].propia);
		COMPROBAR(vuelcaBien(e));
	}
	// 3) Nombres largos con tildes: se cortan sin dejar una letra a medias (el volcado no puede lanzar)
	{
		std::string largo;
		for (int i = 0; i < 40; i++) largo += "\xC3\xA1";   // «á» × 40 = 80 bytes
		const std::string texto = "{\"fase\":\"en_juego\",\"nombre_local\":\"" + largo + "\",\"nombre_visita\":\"x" + largo
			+ "\",\"goles\":[{\"m\":1,\"l\":\"l\",\"j\":\"y" + largo + "\"}]}";
		EstadoPartidoJuego e;
		COMPROBAR(parsear(texto, e));
		COMPROBAR(e.nombreLocal.size() <= 48 && e.nombreVisita.size() <= 48 && e.goles[0].jugador.size() <= 32);
		COMPROBAR(e.nombreLocal.size() == 48);          // 24 «á» exactas
		COMPROBAR(e.nombreVisita.size() == 47);         // «x» + 23 «á» (la 24.ª quedaba a medias)
		COMPROBAR(e.goles[0].jugador.size() == 31);     // «y» + 15 «á»
		COMPROBAR(vuelcaBien(e));
	}
	// 4) Valores raros: no rompen nada
	{
		EstadoPartidoJuego e;
		COMPROBAR(parsear(R"({"fase":"en_juego","goles":[{"m":1e300,"l":"v","j":123,"id":-5,"pp":"si"},{"m":-3,"id":99999999999},{"l":"l"}]})", e));
		COMPROBAR(e.goles.size() == 2);                 // el tercero no tiene minuto
		COMPROBAR(e.goles[0].minuto == 200 && e.goles[0].jugador.empty() && e.goles[0].idJugador == 0 && !e.goles[0].propia);
		COMPROBAR(e.goles[1].minuto == 0 && e.goles[1].idJugador == 0);
	}
	// 5) Controles dentro del nombre: fuera
	{
		EstadoPartidoJuego e;
		COMPROBAR(parsear(R"({"fase":"final","goles":[{"m":90,"l":"l","j":"A\nB\tC"}]})", e));
		COMPROBAR(e.goles[0].jugador == "ABC");
	}
	// 6) Más de 40 goles: se queda con 40
	{
		std::string g;
		for (int i = 0; i < 55; i++) g += std::string(i ? "," : "") + "{\"m\":" + std::to_string(i) + ",\"l\":\"l\",\"j\":\"JUGADOR NUMERO " + std::to_string(i) + "\",\"id\":4294967295}";
		EstadoPartidoJuego e;
		COMPROBAR(parsear("{\"fase\":\"en_juego\",\"goles\":[" + g + "]}", e));
		COMPROBAR(e.goles.size() == 40 && e.goles[39].idJugador == 4294967295u);
	}
	// 7) Lo que escribe phoenix_estadio v1.1 con 40 goles y nombres de 24 bytes cabe en kMaxBytes
	{
		std::string g;
		for (int i = 0; i < 40; i++) g += std::string(i ? "," : "") + "{\"m\":120,\"l\":\"v\",\"j\":\"ABCDEFGHIJKLMNOPQRSTUVWX\",\"id\":4294967295,\"pp\":true}";
		const std::string texto = "{\"v\":2,\"fase\":\"en_juego\",\"minuto\":120,\"segundo\":59,\"periodo\":4,\"anadido\":5,\"reloj_corre\":true,"
			"\"local\":2147483647,\"visita\":2147483647,\"nombre_local\":\"" + std::string(32, 'L') + "\",\"nombre_visita\":\"" + std::string(32, 'V') + "\","
			"\"goles_local\":99,\"goles_visita\":99,\"pk_local\":99,\"pk_visita\":99,\"goles\":[" + g + "],\"completo\":false,\"torneo\":65535,\"seq\":999999999}";
		COMPROBAR(texto.size() < phoenix::juego::kMaxBytes);
		std::printf("tamano maximo de estado.json: %zu de %zu bytes\n", texto.size(), phoenix::juego::kMaxBytes);
	}
	// 8) Formato desconocido: no se acepta
	{
		EstadoPartidoJuego e;
		COMPROBAR(!parsear("{\"fase\":\"otra\"}", e));
		COMPROBAR(!parsear("{roto", e));
	}
	// 9) pantalla.json (phoenix_referee v1.2)
	{
		using phoenix::juego::PantallaJuego;
		using phoenix::juego::parsearPantalla;
		PantallaJuego p;
		COMPROBAR(parsearPantalla(R"({"v":1,"pantalla":"equipos","t":1760140000})", p) && p.pantalla == "equipos");
		for (const char* v : { "menu", "ajustes", "cargando", "saliendo", "en_juego", "descanso", "pausa", "final" }) {
			PantallaJuego q;
			COMPROBAR(parsearPantalla(std::string("{\"pantalla\":\"") + v + "\"}", q) && q.pantalla == v);
		}
		PantallaJuego r;
		COMPROBAR(!parsearPantalla(R"({"pantalla":"otra"})", r));
		COMPROBAR(!parsearPantalla(R"({"pantalla":3})", r));
		COMPROBAR(!parsearPantalla("{roto", r));
		COMPROBAR(!parsearPantalla("[]", r));
		COMPROBAR(!phoenix::juego::leerPantalla("Z:/no/existe").valida);
	}
	std::printf(fallos == 0 ? "TODO BIEN\n" : "%d FALLOS\n", fallos);
	return fallos == 0 ? 0 : 1;
}
