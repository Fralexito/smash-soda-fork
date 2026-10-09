// Prueba: mueve un jugador en un option file con corrección de alineación; sustituto por posición desde el catálogo JSON.
#include "core/OptionFile.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <fstream>
#include <map>
using namespace mercado;
int main(int argc, char** argv) {
	if (argc < 6) { std::printf("uso: mover_of <EDIT> <EDIT nuevo> <catalogo.json> <jugador> <destino> [origen]\n"); return 2; }
	std::ifstream fc(argv[3]); auto cj = nlohmann::json::parse(fc);
	static std::map<uint32_t, int> pos; static std::map<uint32_t, std::string> nombre;
	static const std::map<std::string, int> cod = { {"GK",0},{"CB",1},{"LB",2},{"RB",3},{"DMF",4},{"CMF",5},{"LMF",6},{"RMF",7},{"AMF",8},{"LWF",9},{"RWF",10},{"SS",11},{"CF",12} };
	for (auto& j : cj["jugadores"]) { auto it = cod.find(j.value("posicion", "")); if (it != cod.end()) pos[j["pes_id"].get<uint32_t>()] = it->second; nombre[j["pes_id"].get<uint32_t>()] = j.value("nombre", "?"); }
	auto posicionDe = [](uint32_t id) { auto it = pos.find(id); return it == pos.end() ? -1 : it->second; };
	auto of = OptionFile::abrir(argv[1]);
	if (!of.ok()) { std::printf("ERROR %s %s\n", of.error.codigo.c_str(), of.error.detalle.c_str()); return 1; }
	const uint32_t jug = std::strtoul(argv[4], 0, 10), dest = std::strtoul(argv[5], 0, 10), orig = argc > 6 ? std::strtoul(argv[6], 0, 10) : 0;
	uint32_t origen = orig;
	if (!origen) for (auto& [eq, pl] : of.valor->plantillas()) for (auto& p : pl) if (p.jugador == jug && eq != dest && !origen) origen = eq;
	auto a = of.valor->alineacion(origen);
	const auto& pl = of.valor->plantillas().at(origen);
	std::printf("origen %u (%zu jugadores) XI:", origen, pl.size());
	for (size_t i = 0; i < 11 && a.ok(); i++) std::printf(" %s(%d)", nombre[pl[a.valor->orden[i]].jugador].c_str(), posicionDe(pl[a.valor->orden[i]].jugador));
	std::printf("\n");
	auto s = of.valor->sugerirSustituto(origen, jug, posicionDe);
	if (!s.ok()) { std::printf("ERROR %s %s\n", s.error.codigo.c_str(), s.error.detalle.c_str()); return 1; }
	std::printf("se va %s (pos %d) → sustituto %s (pos %d)\n", nombre[jug].c_str(), posicionDe(jug), *s.valor ? nombre[*s.valor].c_str() : "ninguno", *s.valor ? posicionDe(*s.valor) : -1);
	auto m = of.valor->mover(jug, dest, origen, (uint16_t)(argc > 7 ? std::strtoul(argv[7], 0, 10) : 0), *s.valor);
	if (!m.ok()) { std::printf("ERROR %s %s\n", m.error.codigo.c_str(), m.error.detalle.c_str()); return 1; }
	auto a2 = of.valor->alineacion(origen); const auto& pl2 = of.valor->plantillas().at(origen);
	std::printf("XI después:");
	for (size_t i = 0; i < 11 && a2.ok(); i++) std::printf(" %s", nombre[pl2[a2.valor->orden[i]].jugador].c_str());
	std::printf("\n");
	auto g = of.valor->guardarComo(argv[2]);
	if (!g.ok()) { std::printf("ERROR %s %s\n", g.error.codigo.c_str(), g.error.detalle.c_str()); return 1; }
	std::printf("ok sha256 %s\n", g.valor->c_str()); return 0;
}
