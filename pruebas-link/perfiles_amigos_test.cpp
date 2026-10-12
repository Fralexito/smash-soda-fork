// Prueba de phoenix::perfilespes::Base / traerDePes / prepararSala con una COPIA de SYSTEM00000000.
// Uso: perfiles_amigos_test <carpeta temporal> <copia de SYSTEM00000000>     (nunca toca el archivo original)
#include "phoenix/core/PerfilesAmigos.h"
#include "phoenix/core/PerfilesPes.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

using namespace phoenix::perfilespes;
namespace fs = std::filesystem;

static int fallos = 0;
#define CHECK(c, msg) do { if (!(c)) { std::cerr << "FALLA: " << (msg) << "\n"; fallos++; } } while (0)

static std::vector<uint8_t> leer(const std::string& f) {
	std::ifstream in(fs::u8path(f), std::ios::binary);
	return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

int main(int argc, char** argv) {
	if (argc < 3) { std::cerr << "uso: perfiles_amigos_test <carpeta temporal> <copia de SYSTEM00000000>\n"; return 2; }
	const std::string tmp = argv[1];
	fs::create_directories(fs::u8path(tmp));
	const std::string orig = tmp + "/SYSTEM_original", real = tmp + "/SYSTEM00000000", dirBase = tmp + "/base";
	fs::copy_file(fs::u8path(argv[2]), fs::u8path(orig), fs::copy_options::overwrite_existing);
	fs::copy_file(fs::u8path(orig), fs::u8path(real), fs::copy_options::overwrite_existing);
	const auto bytesOrig = leer(orig);
	std::string err;
	int sello = 0;
	auto s = [&]() { char b[32]; std::snprintf(b, sizeof b, "20261012-1400%02d", ++sello); return std::string(b); };

	Guardado g0;
	CHECK(Guardado::abrir(real, g0, err), "abrir: " + err);
	std::set<std::string> nombresPes; int sinNombre = 0;
	for (auto& u : g0.usuarios()) { if (u.nombre.empty()) sinNombre++; else nombresPes.insert(claveNombre(u.nombre)); }

	// 1) traer: PES -> Link. Solo lee PES.
	Base b(dirBase);
	CHECK(b.cargar(err), "cargar base vacia: " + err);
	Informe inf;
	CHECK(traerDePes(real, b, s(), inf, err), "traer: " + err);
	CHECK(inf.nuevos == static_cast<int>(nombresPes.size()) + sinNombre, "traer crea un perfil por nombre y por Usuario sin nombre (" + std::to_string(inf.nuevos) + ")");
	CHECK(leer(real) == bytesOrig, "traer NO toca el guardado de PES");
	std::cout << "perfiles tras traer: " << b.datos.perfiles.size() << " (con nombre " << nombresPes.size() << ", sin nombre " << sinNombre << ")\n";
	Informe inf2;
	CHECK(traerDePes(real, b, s(), inf2, err) && inf2.nuevos == 0 && inf2.actualizados == 0, "traer dos veces no cambia nada");
	CHECK(b.guardar(s(), err), "guardar base: " + err);
	Base b2(dirBase);
	CHECK(b2.cargar(err) && b2.aJson() == b.aJson(), "guardar y cargar da lo mismo: " + err);
	CHECK(fs::exists(fs::u8path(dirBase + "/respaldos")), "hay carpeta de respaldos");

	// 2) 40 amigos mas (sinteticos) para forzar la rotacion
	const std::string plantilla = aHex(g0.usuario(0).bytes);
	std::vector<std::string> idsSint;
	for (int i = 0; i < 40; i++) {
		std::vector<uint8_t> reg; deHex(plantilla, reg);
		const std::string nombre = "AMIGO" + std::to_string(i + 1);
		std::memset(reg.data() + 4, 0, kTamNombre); std::memcpy(reg.data() + 4, nombre.data(), nombre.size());
		reg[100] = static_cast<uint8_t>(i + 1);   // algo propio de cada uno
		PerfilAmigo p; p.id = "p" + std::to_string(1000 + i); p.nombre = nombre; p.registro = aHex(reg); p.actualizado = "x";
		b.datos.perfiles.push_back(p); idsSint.push_back(p.id);
	}
	b.proteger(1, true); b.proteger(25, true);
	CHECK(b.protegido(1) && b.protegido(25) && !b.protegido(2), "proteger");
	Guardado gA; CHECK(Guardado::abrir(real, gA, err), "reabrir A: " + err);

	// 3) preparar sala: 10 amigos nuevos
	std::vector<std::string> sel(idsSint.begin(), idsSint.begin() + 10);
	Informe ip;
	CHECK(prepararSala(real, b, sel, s(), ip, err), "preparar: " + err);
	CHECK(ip.escritos == 10 && ip.puestos.size() == 10, "escribio 10 huecos");
	CHECK(fs::exists(fs::u8path(ip.copia)) && leer(ip.copia) == bytesOrig, "copia con sello identica al original");
	Guardado gB; CHECK(Guardado::abrir(real, gB, err), "reabrir tras preparar: " + err);
	std::set<int> escritos;
	for (auto& a : ip.puestos) {
		escritos.insert(a.hueco);
		CHECK(gB.usuario(a.hueco - 1).nombre == b.porId(a.perfilId)->nombre, "hueco " + std::to_string(a.hueco) + " tiene el nombre elegido");
		CHECK(!b.protegido(a.hueco), "no pisa protegidos");
	}
	CHECK(*escritos.rbegin() == 32, "empieza por el ultimo hueco (32) y va hacia delante");
	for (int i = 0; i < kUsuarios; i++) if (!escritos.count(i + 1)) CHECK(gB.usuario(i).bytes == gA.usuario(i).bytes, "el hueco " + std::to_string(i + 1) + " no cambio");
	CHECK(gB.usuario(0).bytes == gA.usuario(0).bytes && gB.usuario(24).bytes == gA.usuario(24).bytes, "protegidos 1 y 25 intactos");
	for (int h : escritos) CHECK(std::any_of(b.datos.perfiles.begin(), b.datos.perfiles.end(), [&](const PerfilAmigo& p) { return p.registro == aHex(gA.usuario(h - 1).bytes); }), "el contenido anterior del hueco " + std::to_string(h) + " sigue en un perfil");
	bool fuera = true;
	for (size_t i = 0; i < gA.datos().size(); i++) { const size_t t0 = kInicioTabla, t1 = kInicioTabla + kUsuarios * kTamUsuario; if ((i < t0 || i >= t1) && gA.datos()[i] != gB.datos()[i]) { fuera = false; break; } }
	CHECK(fuera, "solo cambia la tabla de Usuarios");
	CHECK(b.guardar(s(), err), "guardar base 2: " + err);

	// 4) idempotente: otra vez lo mismo no escribe nada
	Informe ip2;
	CHECK(prepararSala(real, b, sel, s(), ip2, err) && ip2.escritos == 0 && ip2.copia.empty(), "repetir no escribe ni hace copia");
	const auto bytesB = leer(real);

	// 5) rotar: otros 10 distintos; los anteriores siguen en Link
	std::vector<std::string> sel2(idsSint.begin() + 10, idsSint.begin() + 20);
	Informe ip3;
	CHECK(prepararSala(real, b, sel2, s(), ip3, err) && ip3.escritos == 10, "rotar a otros 10: " + err);
	CHECK(leer(ip3.copia) == bytesB, "la copia de la rotacion es el estado anterior");
	for (auto& id : sel) CHECK(b.porId(id) != nullptr, "los amigos anteriores siguen en la base");

	// 6) no caben: 31 amigos con 2 huecos protegidos
	std::vector<std::string> demasiados(idsSint.begin(), idsSint.begin() + 31);
	Informe ip4;
	const auto antesCaben = leer(real);
	CHECK(!prepararSala(real, b, demasiados, s(), ip4, err), "31 con 2 protegidos no caben");
	CHECK(leer(real) == antesCaben, "al fallar no se toca el guardado");
	CHECK(!prepararSala(real, b, {}, s(), ip4, err), "elegir 0 se rechaza");

	// 7) PES cambio la config de un amigo: traer lo actualiza y guarda la anterior
	{
		Guardado gc; Guardado::abrir(real, gc, err);
		auto u = gc.usuario(30); u.bytes[100] = 0x7E;
		CHECK(gc.ponerUsuario(30, u.bytes, err) && gc.reemplazar(real, s(), err), "simular edicion en PES: " + err);
		Informe ia;
		CHECK(traerDePes(real, b, s(), ia, err) && ia.actualizados == 1 && ia.nuevos == 0, "traer detecta 1 cambio");
		const PerfilAmigo* p = b.porNombre(u.nombre);
		CHECK(p && !p->anterior.empty() && p->anterior != p->registro, "el perfil guarda la configuracion anterior");
	}

	// 8) nombre repetido en dos huecos: avisa y no pisa ese hueco
	{
		Guardado gc; Guardado::abrir(real, gc, err);
		CHECK(gc.rotular(3, "REPETIDO", err) && gc.rotular(4, "REPETIDO", err), "preparar duplicado");
		CHECK(gc.reemplazar(real, s(), err), "escribir duplicado");
		Informe id;
		CHECK(traerDePes(real, b, s(), id, err) && !id.avisos.empty(), "avisa del nombre repetido");
	}

	// 9) base danada: no se pisa; id borrado se avisa
	{
		const std::string d2 = tmp + "/base_mala";
		fs::create_directories(fs::u8path(d2));
		{ std::ofstream o(fs::u8path(d2 + "/perfiles-amigos.json")); o << "{ esto no es json"; }
		Base m(d2);
		CHECK(!m.cargar(err), "base danada se rechaza");
		std::ifstream in(fs::u8path(d2 + "/perfiles-amigos.json")); std::string txt((std::istreambuf_iterator<char>(in)), {});
		CHECK(txt == "{ esto no es json", "la base danada no se pisa");
		Informe ib;
		CHECK(b.borrar(idsSint[39]) && !b.borrar(idsSint[39]), "borrar un perfil");
		std::vector<std::string> con = { idsSint[39], idsSint[20] };
		CHECK(prepararSala(real, b, con, s(), ib, err) && !ib.avisos.empty(), "un id que ya no existe se avisa y se sigue");
	}
	CHECK(leer(orig) == bytesOrig, "el original no se toco");
	std::cout << (fallos ? "FALLAS: " + std::to_string(fallos) : std::string("TODO OK")) << "\n";
	return fallos ? 1 : 0;
}
