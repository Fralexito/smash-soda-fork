// Igual que palanca.cpp pero con varios cambios a la vez. Todo o nada: si un valor viejo no coincide, no escribe nada.
// uso: palanca_varias <ML> <ML nuevo> "texto info" ofsHex:viejo:nuevo [ofsHex:viejo:nuevo ...]
#include "core/SobrePes.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
using namespace mercado;
int main(int argc, char** argv) {
	if (argc < 5) { std::printf("uso: palanca_varias <ML> <nuevo> \"texto\" ofsHex:viejo:nuevo ...\n"); return 2; }
	auto s = SobrePes::abrir(argv[1]);
	if (!s.ok()) { std::printf("ERROR %s %s\n", s.error.codigo.c_str(), s.error.detalle.c_str()); return 1; }
	std::vector<uint8_t> d = s.valor->datos();
	for (int a = 4; a < argc; a++) {
		std::string t = argv[a];
		size_t p1 = t.find(':'), p2 = t.find(':', p1 + 1);
		if (p1 == std::string::npos || p2 == std::string::npos) { std::printf("ERROR formato %s\n", argv[a]); return 1; }
		size_t o = std::strtoul(t.substr(0, p1).c_str(), 0, 16);
		uint16_t viejo = uint16_t(std::atoi(t.substr(p1 + 1, p2 - p1 - 1).c_str()));
		uint16_t nuevo = uint16_t(std::atoi(t.substr(p2 + 1).c_str()));
		if (o + 2 > d.size() || nuevo > 9999) { std::printf("ERROR fuera de rango %s\n", argv[a]); return 1; }
		uint16_t act; std::memcpy(&act, d.data() + o, 2);
		if (act != viejo) { std::printf("ERROR en %zx: valor %u, esperado %u (no se escribe nada)\n", o, act, viejo); return 1; }
		std::memcpy(d.data() + o, &nuevo, 2);
	}
	SobrePes sobre = *s.valor;
	sobre.ponerTextoInfo(argv[3]);
	auto g = sobre.guardarComo(argv[2], d);
	if (!g.ok()) { std::printf("ERROR %s %s\n", g.error.codigo.c_str(), g.error.detalle.c_str()); return 1; }
	std::printf("ok %d cambios sha256 %s\n", argc - 4, g.valor->c_str());
	return 0;
}
