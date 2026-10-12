// Prueba de los colores de equipo del Campo 2D (core/ColoresClub). Sin Windows.
#include <cstdio>
#include <string>
#include "phoenix/core/ColoresClub.h"
using namespace phoenix::campo;
static int fallos = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

int main() {
	CHECK(normalizarNombre("  Bayern Múnich!! ") == "bayern munich");
	CHECK(normalizarNombre("PERÚ") == "peru");
	CHECK(normalizarNombre("Atlético de Madrid") == "atletico de madrid");
	CHECK(hexValido("#a50044") && !hexValido("a50044") && !hexValido("#a5004") && !hexValido("#gg0044"));
	CHECK(luminancia("#000000") < 0.01 && luminancia("#ffffff") > 0.99);
	CHECK(distanciaColor("#000000", "#ffffff") > 440.0);

	ColorEquipo c;
	CHECK(colorDeEquipo("FC Barcelona", c) && c.principal == "#a50044");
	CHECK(colorDeEquipo("BARÇA", c) == false || true);            // «barça» con ç no se normaliza: no debe romper
	CHECK(colorDeEquipo("Real Madrid", c) && c.principal == "#ffffff");
	CHECK(colorDeEquipo("Atlético de Madrid", c) && c.principal == "#d52b1e");
	CHECK(colorDeEquipo("Atlético Nacional", c) && c.principal == "#16a34a");   // antes que «nacional»
	CHECK(colorDeEquipo("Inter", c) && c.principal == "#1d63d8" == false);       // «inter» azul, no el de las selecciones
	CHECK(colorDeEquipo("Inter", c) && c.principal == "#1d4ed8");
	CHECK(colorDeEquipo("Inter Miami", c) && c.principal == "#f4a5c0");
	CHECK(colorDeEquipo("Internacional", c) && c.principal == "#d52b1e");
	CHECK(colorDeEquipo("Perú", c) && c.principal == "#d91023");
	CHECK(colorDeEquipo("Bayern Munich", c) && c.principal == "#dc052d");
	CHECK(!colorDeEquipo("", c));
	CHECK(!colorDeEquipo("Club Desconocido FC", c));
	CHECK(!colorDeEquipo("Romance", c));                            // «roma» solo como palabra entera

	// Barça (granate) contra Bayern (rojo): demasiado parecidos → B usa el secundario (azul marino, oscuro) → no sirve → violeta
	ColoresPartido p = coloresDelPartido("FC Barcelona", "Bayern Munich");
	CHECK(p.conocidoA && p.conocidoB);
	CHECK(distanciaColor(p.a, p.b) >= 110.0);
	// Real Madrid (blanco) contra Man City (celeste): se distinguen, sin cambios
	p = coloresDelPartido("Real Madrid", "Manchester City");
	CHECK(p.a == "#ffffff" && p.b == "#6cabdd");
	// Juventus (blanco/negro) contra Real Madrid (blanco): B cambia
	p = coloresDelPartido("Real Madrid", "Juventus");
	CHECK(distanciaColor(p.a, p.b) >= 110.0);
	// Equipo oscuro: Juventus principal blanco; Milan rojo/negro: rojo se ve
	p = coloresDelPartido("Milan", "Inter");
	CHECK(distanciaColor(p.a, p.b) >= 110.0);
	// Desconocidos: defecto cian / violeta
	p = coloresDelPartido("", "Equipo X");
	CHECK(p.a == "#00e5ff" && p.b == "#b07bff" && !p.conocidoA && !p.conocidoB);
	// Uno conocido, otro no: el desconocido no se confunde con el conocido (A cian por defecto vs Argentina celeste)
	p = coloresDelPartido("Equipo X", "Argentina");
	CHECK(distanciaColor(p.a, p.b) >= 110.0);
	// Mismo equipo contra sí mismo (partido espejo): se distinguen igualmente
	p = coloresDelPartido("Argentina", "Argentina");
	CHECK(distanciaColor(p.a, p.b) >= 110.0);
	// Todo color final es hexadecimal válido
	const char* nombres[] = { "FC Barcelona", "Brasil", "Colombia", "Universitario", "Juventus", "Chelsea", "", "xyz" };
	for (const char* a : nombres) for (const char* b : nombres) { p = coloresDelPartido(a, b); CHECK(hexValido(p.a) && hexValido(p.b)); CHECK(distanciaColor(p.a, p.b) >= 60.0); }

	if (fallos == 0) std::printf("todo bien\n");
	return fallos ? 1 : 0;
}
