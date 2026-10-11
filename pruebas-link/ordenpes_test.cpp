// Prueba de phoenix::ordenpes::reordenar con un settings.dat real.
// Uso: ordenpes_test <settings.dat> <esperado.dat> <guid1> ... <guid8>   (GUID {..} o "-" para casilla vacía)
#include "phoenix/core/OrdenPes.h"

#include <Windows.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static std::vector<unsigned char> leer(const char* f) {
	std::ifstream in(f, std::ios::binary);
	return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

int main(int argc, char** argv) {
	if (argc < 11) { std::cerr << "uso: ordenpes_test <settings> <esperado> g1..g8\n"; return 2; }
	const std::vector<unsigned char> s = leer(argv[1]), esperado = leer(argv[2]);
	int fallos = 0;
	if (!phoenix::ordenpes::esSettingsValido(s)) { std::cerr << "settings no valido\n"; return 1; }
	std::vector<std::vector<unsigned char>> guids(8), deLink;
	for (int i = 0; i < 8; i++) {
		const std::string g = argv[3 + i];
		if (g == "-") continue;
		GUID x{};
		const std::wstring w(g.begin(), g.end());
		if (FAILED(CLSIDFromString(w.c_str(), &x))) { std::cerr << "GUID malo: " << g << "\n"; return 2; }
		guids[i].assign(reinterpret_cast<unsigned char*>(&x), reinterpret_cast<unsigned char*>(&x) + 16);
		deLink.push_back(guids[i]);
	}
	const std::vector<unsigned char> r = phoenix::ordenpes::reordenar(s, guids, deLink);
	if (r != esperado) { std::cerr << "FALLA: el resultado no coincide con el esperado\n"; fallos++; }
	// Casilla vacía: GUID a cero
	std::vector<std::vector<unsigned char>> conVacia = guids;
	conVacia[5].clear();
	const std::vector<unsigned char> r2 = phoenix::ordenpes::reordenar(s, conVacia, deLink);
	for (int b = 0; b < 16; b++) if (r2[0x64 + 5 * 64 + b] != 0) { std::cerr << "FALLA: casilla 6 no quedo vacia\n"; fallos++; break; }
	// Un archivo no válido no se toca
	std::vector<unsigned char> malo(100, 1);
	if (phoenix::ordenpes::reordenar(malo, guids, deLink) != malo) { std::cerr << "FALLA: toco un archivo no valido\n"; fallos++; }
	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
