#include "core/SobrePes.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
using namespace mercado;
// uso: palanca <ML> <ML nuevo> <ofs hex> <valor u16> <reg esperado hex en ofs+4-?> "texto info"
int main(int argc, char** argv) {
	if (argc < 6) { std::printf("uso: palanca <ML> <nuevo> <ofsHex> <valorViejo> <valorNuevo> [texto]\n"); return 2; }
	auto s = SobrePes::abrir(argv[1]);
	if (!s.ok()) { std::printf("ERROR %s %s\n", s.error.codigo.c_str(), s.error.detalle.c_str()); return 1; }
	std::vector<uint8_t> d = s.valor->datos();
	size_t o = std::strtoul(argv[3], 0, 16);
	uint16_t viejo = uint16_t(std::atoi(argv[4])), nuevo = uint16_t(std::atoi(argv[5]));
	uint16_t act; std::memcpy(&act, d.data() + o, 2);
	if (act != viejo) { std::printf("ERROR valor actual %u != esperado %u\n", act, viejo); return 1; }
	std::memcpy(d.data() + o, &nuevo, 2);
	SobrePes sobre = *s.valor;
	if (argc > 6) sobre.ponerTextoInfo(argv[6]);
	auto g = sobre.guardarComo(argv[2], d);
	if (!g.ok()) { std::printf("ERROR %s %s\n", g.error.codigo.c_str(), g.error.detalle.c_str()); return 1; }
	std::printf("ok %u -> %u sha256 %s\n", act, nuevo, g.valor->c_str());
	return 0;
}
