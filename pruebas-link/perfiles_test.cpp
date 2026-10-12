// Prueba de phoenix::perfilespes (los 32 «Datos personales» de PES 2021) con una COPIA de SYSTEM00000000.
// Uso: perfiles_test <carpeta temporal vacia> <copia de SYSTEM00000000>     (nunca toca el archivo original)
#include "phoenix/core/PerfilesPes.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace phoenix::perfilespes;
namespace fs = std::filesystem;

static int fallos = 0;
#define CHECK(c, msg) do { if (!(c)) { std::cerr << "FALLA: " << msg << "\n"; fallos++; } } while (0)

static std::vector<uint8_t> leer(const std::string& f) {
	std::ifstream in(fs::u8path(f), std::ios::binary);
	return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}
static void escribir(const std::string& f, const std::vector<uint8_t>& b) {
	std::ofstream o(fs::u8path(f), std::ios::binary | std::ios::trunc);
	o.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
}

int main(int argc, char** argv) {
	if (argc < 3) { std::cerr << "uso: perfiles_test <carpeta temporal> <copia de SYSTEM00000000>\n"; return 2; }
	const std::string tmp = argv[1];
	fs::create_directories(fs::u8path(tmp));
	const std::string orig = tmp + "/SYSTEM_original";
	fs::copy_file(fs::u8path(argv[2]), fs::u8path(orig), fs::copy_options::overwrite_existing);
	const std::vector<uint8_t> bytesOrig = leer(orig);
	std::string err;

	// 1) abrir y listar los 32
	Guardado g;
	if (!Guardado::abrir(orig, g, err)) { std::cerr << "FALLA: no abre el original: " << err << "\n"; return 1; }
	CHECK(g.datos().size() == kTamDatos, "tamaño de datos");
	const auto us = g.usuarios();
	CHECK(us.size() == 32, "32 usuarios");
	for (auto& u : us) std::cout << "Usuario " << (u.indice + 1) << ": [" << u.nombre << "]\n";

	// 2) cifrar sin cambios = idéntico al archivo original (el cifrado es el del juego)
	std::vector<uint8_t> cif;
	CHECK(g.cifrar(cif, err), "cifrar: " + err);
	CHECK(cif == bytesOrig, "cifrar sin cambios debe dar el MISMO archivo");

	// 3) exportar → JSON → importar a otro hueco → guardar → reabrir
	const int origen = 0, destino = 31;
	nlohmann::json perfil = aPerfil(g.usuario(origen));
	std::vector<uint8_t> reg; std::string nombre;
	CHECK(dePerfil(perfil, reg, nombre, err), "dePerfil: " + err);
	CHECK(nombre == g.usuario(origen).nombre, "nombre del perfil");
	Guardado h = g;
	CHECK(h.ponerUsuario(destino, reg, err), "ponerUsuario: " + err);
	CHECK(h.rotular(destino, "Amigo prueba", err), "rotular: " + err);
	const std::string nuevo = tmp + "/SYSTEM_nuevo";
	CHECK(h.guardarComo(nuevo, err), "guardarComo: " + err);
	CHECK(!h.guardarComo(nuevo, err), "guardarComo NO debe sobrescribir");
	Guardado r;
	CHECK(Guardado::abrir(nuevo, r, err), "reabrir nuevo: " + err);
	CHECK(r.usuario(destino).nombre == "Amigo prueba", "el nombre quedó en el hueco 32");
	for (int i = 0; i < 31; i++) CHECK(r.usuario(i).bytes == g.usuario(i).bytes, "el Usuario " + std::to_string(i + 1) + " no debe cambiar");
	// todo lo que NO es el hueco destino queda idéntico (datos antes y después de la tabla)
	const size_t ini = kInicioTabla + size_t(destino) * kTamUsuario;
	bool fuera = true;
	for (size_t i = 0; i < g.datos().size(); i++) if ((i < ini || i >= ini + kTamUsuario) && g.datos()[i] != r.datos()[i]) { fuera = false; break; }
	CHECK(fuera, "solo cambia el registro del hueco destino");
	// idempotencia: exportar/importar/exportar = igual
	Guardado r2 = r;
	nlohmann::json p2 = aPerfil(r.usuario(destino));
	std::vector<uint8_t> reg2; std::string n2;
	CHECK(dePerfil(p2, reg2, n2, err) && r2.ponerUsuario(destino, reg2, err), "reimportar");
	std::vector<uint8_t> c1, c2;
	CHECK(r.cifrar(c1, err) && r2.cifrar(c2, err) && c1 == c2, "exportar/importar/exportar debe dar el mismo archivo");

	// 4) reemplazar con copia con sello: el original queda como copia y el real cambia solo en el hueco
	const std::string real = tmp + "/SYSTEM00000000";
	fs::copy_file(fs::u8path(orig), fs::u8path(real), fs::copy_options::overwrite_existing);
	std::string copia;
	CHECK(h.reemplazar(real, "20261012-000000", err, &copia), "reemplazar: " + err);
	CHECK(leer(copia) == bytesOrig, "la copia con sello es idéntica al original");
	Guardado rr;
	CHECK(Guardado::abrir(real, rr, err) && rr.usuario(destino).nombre == "Amigo prueba", "el archivo reemplazado trae el hueco nuevo");
	CHECK(!h.reemplazar(real, "20261012-000000", err), "no debe repetir un sello ya usado");
	CHECK(leer(real) == leer(nuevo), "reemplazar y guardarComo dan el mismo archivo");
	CHECK(!fs::exists(fs::u8path(real + ".phoenix-tmp")), "no queda temporal");

	// 5) rechazos: hueco fuera de rango, registro corto/corrupto, archivo truncado o ajeno, perfil ajeno
	Guardado m = g;
	CHECK(!m.ponerUsuario(32, reg, err), "hueco 33 rechazado");
	CHECK(!m.ponerUsuario(-1, reg, err), "hueco -1 rechazado");
	std::vector<uint8_t> corto(reg.begin(), reg.begin() + 100);
	CHECK(!m.ponerUsuario(0, corto, err), "registro corto rechazado");
	std::vector<uint8_t> malo = reg; malo[0] = 7;
	CHECK(!m.ponerUsuario(0, malo, err), "registro que no empieza en 0 rechazado");
	CHECK(!m.rotular(0, std::string(40, 'x'), err), "nombre largo rechazado");
	CHECK(m.usuario(0).bytes == g.usuario(0).bytes, "los rechazos no cambian nada");
	escribir(tmp + "/truncado", std::vector<uint8_t>(bytesOrig.begin(), bytesOrig.begin() + bytesOrig.size() / 2));
	Guardado t;
	CHECK(!Guardado::abrir(tmp + "/truncado", t, err), "truncado rechazado");
	escribir(tmp + "/basura", std::vector<uint8_t>(50000, 0x41));
	CHECK(!Guardado::abrir(tmp + "/basura", t, err), "basura rechazada");
	CHECK(!Guardado::abrir(tmp + "/no_existe", t, err), "inexistente rechazado");
	CHECK(!dePerfil(nlohmann::json{ {"formato", 2} }, reg, nombre, err), "perfil de otro formato rechazado");
	CHECK(leer(orig) == bytesOrig, "el original no se tocó");

	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
