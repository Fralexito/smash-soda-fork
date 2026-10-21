// Prueba de phoenix::mandosextra (phoenix_mandos_extra.dat): formato, escritura atómica y copia de seguridad.
// Trabaja en una carpeta temporal: no toca la carpeta real de PES.
// Uso: mandosextra_test <carpeta-temporal>
#include "phoenix/core/MandosExtra.h"

#include <Windows.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace phoenix::mandosextra;
using Bytes = std::vector<unsigned char>;

static int fallos = 0;
#define CHEQUEA(c) do { if (!(c)) { std::cerr << "FALLA (linea " << __LINE__ << "): " #c "\n"; fallos++; } } while (0)

static Bytes leer(const fs::path& f) {
	std::ifstream in(f, std::ios::binary);
	return Bytes(std::istreambuf_iterator<char>(in), {});
}
static Bytes guid(unsigned char semilla) { Bytes g(16); for (int i = 0; i < 16; i++) g[i] = static_cast<unsigned char>(semilla + i); return g; }
static uint32_t u32(const Bytes& v, size_t o) { return v[o] | (v[o + 1] << 8) | (v[o + 2] << 16) | (static_cast<uint32_t>(v[o + 3]) << 24); }

int main(int argc, char** argv) {
	if (argc < 2) { std::cerr << "uso: mandosextra_test <carpeta-temporal>\n"; return 2; }
	const fs::path dir = argv[1];
	std::error_code ec;
	fs::remove_all(dir, ec);
	fs::create_directories(dir);

	// ---- Formato ------------------------------------------------------------------
	Bytes mapa(kMapa); for (size_t i = 0; i < kMapa; i++) mapa[i] = static_cast<unsigned char>(0xA0 + i);
	const std::vector<Bytes> guids = { guid(0x10), Bytes(), guid(0x30) };   // puestos 9, 10 (desenchufado) y 11
	const Bytes a = construir(guids, mapa);
	CHEQUEA(a.size() == 12 + 3 * 0x40);
	CHEQUEA(std::memcmp(a.data(), "PHXM", 4) == 0);
	CHEQUEA(u32(a, 4) == 1);
	CHEQUEA(u32(a, 8) == 3);
	CHEQUEA(std::memcmp(&a[12], guids[0].data(), 16) == 0);                 // casilla 1 = puesto 9: GUID
	CHEQUEA(std::memcmp(&a[12 + 16], mapa.data(), kMapa) == 0);             // + 24 botones de 2 bytes
	for (int b = 0; b < 16; b++) CHEQUEA(a[12 + 0x40 + b] == 0);            // puesto desenchufado: GUID a cero
	CHEQUEA(std::memcmp(&a[12 + 2 * 0x40], guids[2].data(), 16) == 0);
	CHEQUEA(esValido(a) && cantidad(a) == 3);
	CHEQUEA(guidDe(a, 0) == guids[0] && guidDe(a, 1).empty() && guidDe(a, 2) == guids[2] && guidDe(a, 3).empty());
	CHEQUEA(esValido(construir({}, mapa)) && cantidad(construir({}, mapa)) == 0);   // N = 0 también vale

	// Archivos no válidos
	Bytes malo = a; malo[0] = 'X';              CHEQUEA(!esValido(malo));
	malo = a; malo[4] = 2;                      CHEQUEA(!esValido(malo));   // versión 2: no la entiende
	malo = a; malo.pop_back();                  CHEQUEA(!esValido(malo));   // tamaño que no cuadra con N
	malo = a; malo[8] = 9;                      CHEQUEA(!esValido(malo) && cantidad(malo) == 0);
	CHEQUEA(!esValido(Bytes()));

	// Mapa de botones = casilla 1 de settings.dat (612 bytes, «WECF»)
	Bytes settings(612, 0); std::memcpy(settings.data(), "WECF", 4);
	const Bytes defecto(kMapa, 0x77);
	CHEQUEA(mapaDeCasilla1(settings, defecto) == defecto);                  // casilla 1 sin mapa: el de fábrica
	std::memcpy(&settings[0x64 + 16], mapa.data(), kMapa);
	CHEQUEA(mapaDeCasilla1(settings, defecto) == mapa);
	CHEQUEA(mapaDeCasilla1(Bytes(100, 1), defecto) == defecto);             // no es settings.dat

	// ---- Escritura atómica y copia -------------------------------------------------
	const fs::path f = dir / "phoenix_mandos_extra.dat";
	std::string copia, error;
	CHEQUEA(escribir(f.wstring(), a, true, copia, error));                  // primera vez: no hay nada que copiar
	CHEQUEA(copia.empty());
	CHEQUEA(leer(f) == a);
	CHEQUEA(!fs::exists(dir / "phoenix_mandos_extra.dat.phoenix-tmp"));    // no queda el temporal

	const Bytes b = construir({ guid(0x50) }, mapa);
	CHEQUEA(escribir(f.wstring(), b, true, copia, error));                  // ya había archivo: copia antes
	CHEQUEA(!copia.empty() && copia.rfind("phoenix_mandos_extra.dat.phoenix-", 0) == 0);
	CHEQUEA(leer(dir / copia) == a);                                         // la copia es lo de antes
	CHEQUEA(leer(f) == b);

	std::string copia2;
	const Bytes c = construir({ guid(0x60), guid(0x70) }, mapa);
	CHEQUEA(escribir(f.wstring(), c, false, copia2, error));                // sin copia (ya se hizo esta sesión)
	CHEQUEA(copia2.empty() && leer(f) == c);
	int copias = 0;
	for (const auto& e : fs::directory_iterator(dir)) if (e.path().filename().wstring().find(L".phoenix-2") != std::wstring::npos) copias++;
	CHEQUEA(copias == 1);

	// Si no se puede escribir, el archivo de antes queda intacto
	const fs::path enUso = dir / "bloqueado.dat";
	{ std::ofstream o(enUso, std::ios::binary); o.write(reinterpret_cast<const char*>(c.data()), c.size()); }
	HANDLE h = CreateFileW(enUso.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	std::string c3, e3;
	CHEQUEA(!escribir(enUso.wstring(), a, false, c3, e3) && !e3.empty());
	CloseHandle(h);
	CHEQUEA(leer(enUso) == c);
	CHEQUEA(!fs::exists(dir / "bloqueado.dat.phoenix-tmp"));
	std::string c4, e4;
	CHEQUEA(!escribir(L"", a, false, c4, e4));                               // sin carpeta Documentos

	fs::remove_all(dir, ec);
	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
