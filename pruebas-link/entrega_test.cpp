// Pruebas del repartidor de datos (Entrega). Sin Windows: carpetas temporales.
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "phoenix/link/Entrega.h"

using namespace phoenix::entrega;
namespace fs = std::filesystem;
static int fallos = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << "FALLO linea " << __LINE__ << ": " #c "\n"; ++fallos; } } while (0)

static void escribir(const fs::path& p, const std::string& t) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << t; }
static std::string leer(const fs::path& p) { std::ifstream f(p, std::ios::binary); return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); }

static const std::string CAB = std::string("\xff\x10\x81\x57", 4) + "WESYS" + std::string(7, '\0');

static void pedido(const fs::path& ent, const std::string& id, const std::string& player, const std::string& edit, bool sha_mal = false) {
	nlohmann::json j = { {"version", 1}, {"id", id}, {"creado_en", "2026-10-09T10:00:00Z"}, {"resumen", "Lamine Yamal velocidad 99"}, {"archivos", nlohmann::json::array()} };
	if (!player.empty()) { escribir(ent / "Player.bin", player); j["archivos"].push_back({ {"nombre", "Player.bin"}, {"sha256", sha_mal ? std::string(64, 'a') : sha256Hex(player.data(), player.size())} }); }
	if (!edit.empty()) { escribir(ent / "EDIT00000000", edit); j["archivos"].push_back({ {"nombre", "EDIT00000000"}, {"sha256", sha256Hex(edit.data(), edit.size())} }); }
	escribir(ent / "entrega.json", j.dump());
}

int main() {
	// SHA-256 conocido
	CHECK(sha256Hex("abc", 3) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	CHECK(sha256Hex("", 0) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

	const fs::path T = fs::temp_directory_path() / "phx_entrega_test";
	fs::remove_all(T);
	const fs::path juego = T / "juego", ent = T / "entrega", opt = T / "docs" / "save";
	const std::string pesdb = "SiderAddons/livecpk/Phoenix-DB/common/etc/pesdb";
	const std::string pesdb2 = "ConmeGol Extras/ConmeGOL Patch 26/SiderAddons/livecpk/Phoenix-DB/common/etc/pesdb";
	fs::create_directories(juego / pesdb); fs::create_directories(juego / pesdb2); fs::create_directories(ent);
	escribir(juego / pesdb / "Player.bin", "VIEJO1"); escribir(juego / pesdb2 / "Player.bin", "VIEJO2");
	escribir(opt / "EDIT00000000", "AAAAAAAAAA");
	Rutas r{ ent, juego, opt };

	// 1) entrega buena (Player + Edit)
	pedido(ent, "id-1", CAB + "NUEVO", "BBBBBBBBBB");
	CHECK(hayEntrega(ent));
	Ultima u = entregar(r);
	CHECK(u.estado == "colocada");
	CHECK(leer(juego / pesdb / "Player.bin") == CAB + "NUEVO");
	CHECK(leer(juego / pesdb2 / "Player.bin") == CAB + "NUEVO");
	CHECK(leer(juego / pesdb / "Player.bin.anterior") == "VIEJO1");
	CHECK(leer(opt / "EDIT00000000") == "BBBBBBBBBB");
	CHECK(leer(u.editRespaldo.empty() ? fs::path() : deU8(u.editRespaldo)) == "AAAAAAAAAA");
	CHECK(!hayEntrega(ent));
	CHECK(fs::exists(ent / "entregados" / "id-1" / "entrega.json"));
	CHECK(fs::exists(ent / "historial.log"));
	CHECK(u.aviso.find("Datos nuevos de Phoenix") != std::string::npos && u.aviso.find("Activar") != std::string::npos);
	CHECK(leerUltima(ent).puedeDeshacer);

	// 2) deshacer
	Ultima d = deshacer(r);
	CHECK(d.estado == "deshecha");
	CHECK(leer(juego / pesdb / "Player.bin") == "VIEJO1");
	CHECK(leer(juego / pesdb2 / "Player.bin") == "VIEJO2");
	CHECK(leer(opt / "EDIT00000000") == "AAAAAAAAAA");
	CHECK(!leerUltima(ent).puedeDeshacer);
	CHECK(deshacer(r).estado == "deshecha");   // segunda vez: no hace nada

	// 3) sha mal -> nada se toca
	pedido(ent, "id-2", CAB + "X", "", true);
	u = entregar(r);
	CHECK(u.estado == "rechazada" && u.motivo.find("sha256") != std::string::npos);
	CHECK(leer(juego / pesdb / "Player.bin") == "VIEJO1");
	CHECK(!hayEntrega(ent));
	CHECK(u.aviso.find("entrega rechazada") != std::string::npos);

	// 4) cabecera mala
	pedido(ent, "id-3", "ESTO ES SOLO TEXTO, NADA MAS QUE TEXTO WESYS", "");
	CHECK(entregar(r).motivo.find("WESYS") != std::string::npos);

	// 5) option file de otro tamano
	pedido(ent, "id-4", "", "CORTO");
	u = entregar(r);
	CHECK(u.estado == "rechazada" && leer(opt / "EDIT00000000") == "AAAAAAAAAA");

	// 6) Phoenix-DB no instalado en ninguna parte -> rechaza y no crea nada
	fs::remove_all(juego / "SiderAddons"); fs::remove_all(juego / "ConmeGol Extras");
	pedido(ent, "id-5", CAB + "Z", "");
	CHECK(entregar(r).motivo == "Phoenix-DB no instalado");
	CHECK(!fs::exists(juego / "SiderAddons") && !fs::exists(juego / "ConmeGol Extras"));
	fs::create_directories(juego / pesdb); escribir(juego / pesdb / "Player.bin", "VIEJO1");

	// 7) multiparche: un modo del cambiador SIN Phoenix-DB se salta; el principal si se coloca
	fs::create_directories(juego / "ConmeGol Extras/ConmeGOL Patch 26/SiderAddons");
	pedido(ent, "id-6", CAB + "Z", "");
	CHECK(entregar(r).estado == "colocada");
	CHECK(leer(juego / pesdb / "Player.bin") == CAB + "Z");
	CHECK(!fs::exists(juego / pesdb2));   // no creo carpetas
	CHECK(leer(ent / "historial.log").find("sin Phoenix-DB en ConmeGOL Patch 26") != std::string::npos);
	CHECK(deshacer(r).estado == "deshecha" && leer(juego / pesdb / "Player.bin") == "VIEJO1");
	// 7b) otro parche con dos modos con Phoenix-DB y SIN SiderAddons principal: se colocan en ambos modos
	fs::remove_all(juego / "SiderAddons"); fs::remove_all(juego / "ConmeGol Extras");
	const std::string modoA = "Cambiador/Modo A/SiderAddons/livecpk/Phoenix-DB/common/etc/pesdb", modoB = "Cambiador/Modo B/SiderAddons/livecpk/Phoenix-DB/common/etc/pesdb";
	fs::create_directories(juego / modoA); fs::create_directories(juego / modoB);
	pedido(ent, "id-6b", CAB + "M", "");
	CHECK(entregar(r).estado == "colocada");
	CHECK(leer(juego / modoA / "Player.bin") == CAB + "M" && leer(juego / modoB / "Player.bin") == CAB + "M");
	CHECK(nombreParche(juego) == "Modo A");
	CHECK(buscarCarpetasPesdb(juego).size() == 2);
	fs::remove_all(juego / "Cambiador");
	fs::create_directories(juego / pesdb); escribir(juego / pesdb / "Player.bin", "VIEJO1");
	CHECK(nombreParche(juego) == "juego");

	// 8) archivo no permitido / id raro (no sale de la carpeta)
	{
		nlohmann::json j = { {"version", 1}, {"id", "../mal"}, {"resumen", "x"}, {"archivos", nlohmann::json::array()} };
		escribir(ent / "entrega.json", j.dump());
		CHECK(entregar(r).estado == "rechazada");
		j = { {"version", 1}, {"id", "ok-7"}, {"resumen", "x"}, {"archivos", { { {"nombre", "..\\sider.ini"}, {"sha256", "00"} } } } };
		escribir(ent / "entrega.json", j.dump());
		CHECK(entregar(r).motivo.find("no permitido") != std::string::npos);
	}
	escribir(ent / "entrega.json", "{ esto no es json");
	CHECK(entregar(r).motivo.find("ilegible") != std::string::npos);

	// 9) un rechazo no borra la posibilidad de deshacer una entrega anterior
	pedido(ent, "id-8", CAB + "OK8", "");
	CHECK(entregar(r).estado == "colocada");
	pedido(ent, "id-9", CAB + "X", "", true);
	CHECK(entregar(r).estado == "rechazada");
	CHECK(leerUltima(ent).estado == "rechazada" && leerUltima(ent).puedeDeshacer);
	CHECK(deshacer(r).estado == "deshecha");
	CHECK(leer(juego / pesdb / "Player.bin") == "VIEJO1");

	// 10) respaldos: solo los 5 ultimos
	for (int i = 0; i < 8; ++i) {
		std::string n = "EDIT00000000.phoenix-2020010" + std::to_string(i) + "-000000";
		escribir(opt / n, "r");
	}
	pedido(ent, "id-10", "", "CCCCCCCCCC");
	CHECK(entregar(r).estado == "colocada");
	int cuantos = 0;
	for (auto& e : fs::directory_iterator(opt)) if (e.path().filename().string().rfind("EDIT00000000.phoenix-", 0) == 0) ++cuantos;
	CHECK(cuantos == 5);

	// 11) buscar option file
	fs::create_directories(T / "d2" / "KONAMI" / "eFootball PES 2021 SEASON UPDATE" / "239200" / "save");
	escribir(T / "d2" / "KONAMI" / "eFootball PES 2021 SEASON UPDATE" / "239200" / "save" / "EDIT00000000", "x");
	CHECK(buscarCarpetaOptionFile(T / "d2").filename() == "save");
	escribir(T / "d3" / "KONAMI" / "eFootball PES 2021" / "111" / "save" / "EDIT00000000", "x");   // otra edicion
	CHECK(buscarCarpetaOptionFile(T / "d3").filename() == "save");
	CHECK(buscarCarpetaOptionFile(T / "nada").empty());

	// 12) aviso corto: la instruccion nunca se corta
	std::string av = avisoColocada(std::string(300, 'a'));
	CHECK(av.size() + 8 <= 110 + 8);
	CHECK(av.find("Activar") != std::string::npos);

	// 13) PlayerAssignment.bin (pedido de Sync, PROMPT-LINK-playerassignment.md)
	{
		const fs::path J = T / "pa", E = T / "pa-ent";
		const std::string db1 = "SiderAddons/livecpk/Phoenix-DB/common/etc/pesdb";
		const std::string db2 = "ConmeGol Extras/ConmeGOL Patch 26/SiderAddons/livecpk/Phoenix-DB/common/etc/pesdb";
		fs::create_directories(J / db1); fs::create_directories(J / db2); fs::create_directories(E);
		escribir(J / db1 / "Player.bin", "P1"); escribir(J / db2 / "Player.bin", "P2");
		escribir(J / db1 / "PlayerAssignment.bin.anterior", "VIEJO-DE-OTRA-ENTREGA");   // .anterior suelto: no debe volver al deshacer
		Rutas rp{ E, J, opt };
		auto pedidoPA = [&](const std::string& id, const std::string& asig, const std::string& player, bool shaMal) {
			nlohmann::json j = { {"version", 1}, {"id", id}, {"creado_en", "2026-10-10T10:00:00Z"}, {"resumen", "Fichaje: Lamine al Madrid"}, {"archivos", nlohmann::json::array()} };
			escribir(E / "PlayerAssignment.bin", asig);
			j["archivos"].push_back({ {"nombre", "PlayerAssignment.bin"}, {"sha256", shaMal ? std::string(64, 'b') : sha256Hex(asig.data(), asig.size())} });
			if (!player.empty()) { escribir(E / "Player.bin", player); j["archivos"].push_back({ {"nombre", "Player.bin"}, {"sha256", sha256Hex(player.data(), player.size())} }); }
			escribir(E / "entrega.json", j.dump());
		};
		// a) los dos archivos: los dos colocados en las dos carpetas, Player.bin.anterior guardado
		pedidoPA("pa-1", CAB + "ASIG", CAB + "PLAY", false);
		Ultima a = entregar(rp);
		CHECK(a.estado == "colocada");
		CHECK(leer(J / db1 / "PlayerAssignment.bin") == CAB + "ASIG" && leer(J / db2 / "PlayerAssignment.bin") == CAB + "ASIG");
		CHECK(leer(J / db1 / "Player.bin") == CAB + "PLAY" && leer(J / db1 / "Player.bin.anterior") == "P1");
		CHECK(a.player.size() == 4);
		CHECK(!fs::exists(J / db1 / "PlayerAssignment.bin.anterior") && leer(J / db1 / "PlayerAssignment.bin.anterior.viejo") == "VIEJO-DE-OTRA-ENTREGA");
		CHECK(fs::exists(E / "entregados" / "pa-1" / "PlayerAssignment.bin"));
		// b) deshacer: Player.bin vuelve; el PlayerAssignment.bin nuestro se aparta (no había uno antes)
		Ultima d = deshacer(rp);
		CHECK(d.estado == "deshecha");
		CHECK(leer(J / db1 / "Player.bin") == "P1" && leer(J / db2 / "Player.bin") == "P2");
		CHECK(!fs::exists(J / db1 / "PlayerAssignment.bin") && leer(J / db1 / "PlayerAssignment.bin.deshecho") == CAB + "ASIG");
		// c) solo PlayerAssignment, con uno ya colocado: se guarda .anterior y el deshacer lo devuelve
		escribir(J / db1 / "PlayerAssignment.bin", "PA-PREVIO"); escribir(J / db2 / "PlayerAssignment.bin", "PA-PREVIO2");
		pedidoPA("pa-2", CAB + "ASIG2", "", false);
		CHECK(entregar(rp).estado == "colocada");
		CHECK(leer(J / db1 / "PlayerAssignment.bin.anterior") == "PA-PREVIO" && leer(J / db1 / "Player.bin") == "P1");
		CHECK(deshacer(rp).estado == "deshecha" && leer(J / db1 / "PlayerAssignment.bin") == "PA-PREVIO" && leer(J / db2 / "PlayerAssignment.bin") == "PA-PREVIO2");
		// d) sha mal: no se coloca nada
		pedidoPA("pa-3", CAB + "ASIG3", CAB + "PLAY3", true);
		Ultima m = entregar(rp);
		CHECK(m.estado == "rechazada" && m.motivo.find("sha256") != std::string::npos);
		CHECK(leer(J / db1 / "PlayerAssignment.bin") == "PA-PREVIO" && leer(J / db1 / "Player.bin") == "P1");
		// e) sin cabecera WESYS: rechazado
		pedidoPA("pa-4", "SIN-CABECERA-NI-NADA", "", false);
		CHECK(entregar(rp).motivo.find("WESYS") != std::string::npos);
		// f) sin pesdb: rechazo claro y no se crean carpetas
		const fs::path J2 = T / "pa-sin";
		fs::create_directories(J2);
		Rutas rs{ E, J2, opt };
		pedidoPA("pa-5", CAB + "X", "", false);
		CHECK(entregar(rs).motivo == "Phoenix-DB no instalado");
		CHECK(!fs::exists(J2 / "SiderAddons"));
	}

	fs::remove_all(T);
	if (fallos == 0) std::cout << "entrega_test: TODO BIEN\n";
	return fallos == 0 ? 0 : 1;
}
