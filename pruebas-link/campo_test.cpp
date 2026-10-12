// Prueba de la lógica pura del campo 2D (core/CampoVivo). Con la variable PHX_CAMPO_RAW apuntando a un volcado
// «RAW <dir> <160 palabras en hex>» (salida de tools/radar/entes.exe raw) también prueba con objetos reales del juego.
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include "phoenix/core/CampoVivo.h"

using namespace phoenix::campo;

static int fallos = 0;
#define CHEQUEA(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

static void poner32(std::vector<uint8_t>& o, uint32_t off, uint32_t v) { std::memcpy(o.data() + off, &v, 4); }
static void ponerF(std::vector<uint8_t>& o, uint32_t off, float v) { std::memcpy(o.data() + off, &v, 4); }

static std::vector<uint8_t> actor(uint32_t equipoIA, float x, float y, float z) {
	std::vector<uint8_t> o(kBytesObjeto, 0);
	poner32(o, kOffTipo, 0x18); poner32(o, kOffLado, 1); poner32(o, kOffMarca, 0x80001);
	poner32(o, kOffCodigo, (equipoIA << 16) | 3);
	poner32(o, kOffReg0, 0x10); poner32(o, kOffReg3, 0xDEAD);
	ponerF(o, kOffPosicion, x); ponerF(o, kOffPosicion + 4, y); ponerF(o, kOffPosicion + 8, z); ponerF(o, kOffPosicion + 12, 1.0f);
	return o;
}

int main() {
	// --- firma sintética ---
	Actor a;
	CHEQUEA(leerActor(actor(44, 10.5f, 0.9f, -3.2f).data(), a));
	CHEQUEA(a.codigo == 44 && a.x == 10.5f && a.z == -3.2f);
	{ auto o = actor(44, 1, 0.9f, 1); poner32(o, kOffTipo, 0x8); CHEQUEA(!leerActor(o.data(), a)); }          // otro tipo
	{ auto o = actor(44, 1, 0.9f, 1); poner32(o, kOffMarca, 0x80002); CHEQUEA(!leerActor(o.data(), a)); }      // marca no cuadra
	{ auto o = actor(44, 1, 0.9f, 1); poner32(o, kOffReg3, 0x55EF); CHEQUEA(!leerActor(o.data(), a)); }        // actor obsoleto
	{ auto o = actor(44, 1, 0.9f, 1); ponerF(o, kOffPosicion + 12, 0.0f); CHEQUEA(!leerActor(o.data(), a)); }  // w != 1
	{ auto o = actor(44, 99.0f, 0.9f, 1); CHEQUEA(!leerActor(o.data(), a)); }                                   // fuera del campo
	{ auto o = actor(44, 1, 0.9f, 1); ponerF(o, kOffPosicion, std::numeric_limits<float>::quiet_NaN()); CHEQUEA(!leerActor(o.data(), a)); } // NaN
	{ auto o = actor(44, 1, 0.0f, 1); CHEQUEA(!leerActor(o.data(), a)); }                                       // y = 0 (cerebro de IA)
	float x, y, z;
	{ auto o = actor(44, -20.0f, 1.1f, 7.0f); CHEQUEA(leerPosicion(o.data() + kOffPosicion, x, y, z) && x == -20.0f && z == 7.0f); }
	CHEQUEA(!leerPosicion(std::vector<uint8_t>(16, 0).data(), x, y, z));

	// --- elegir lista ---
	std::vector<Actor> todos;
	for (int i = 0; i < 22; i++) { Actor t; t.dir = 1000 + i * 0x500; t.codigo = 44; t.x = float(i); t.y = 0.9f; todos.push_back(t); }
	for (int i = 0; i < 22; i++) { Actor t; t.dir = 1000 + i * 0x500 + 0x280; t.codigo = 67; t.x = float(i); t.y = 0.9f; todos.push_back(t); }
	for (int i = 0; i < 2; i++) { Actor t; t.dir = 9999 + i; t.codigo = 1718 + i * 20; t.y = 0.9f; todos.push_back(t); }
	auto lista = elegirLista(todos);
	CHEQUEA(lista.size() == 22 && lista[0].codigo == 44);
	CHEQUEA(elegirLista({}).empty());
	{ std::vector<Actor> pocos(todos.begin(), todos.begin() + 10); CHEQUEA(elegirLista(pocos).empty()); }       // lista incompleta: se apaga
	Actor asis; asis.z = -35.2f; CHEQUEA(pareceAsistente(asis)); asis.z = 20.0f; CHEQUEA(!pareceAsistente(asis));
	CHEQUEA(exeConocido(458806784ull, "5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A"));
	CHEQUEA(!exeConocido(458806785ull, "5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A"));
	CHEQUEA(!exeConocido(458806784ull, "AAAA"));

	// --- objetos reales (opcional) ---
	if (const char* ruta = std::getenv("PHX_CAMPO_RAW")) {
		std::ifstream f(ruta);
		std::string linea; std::vector<Actor> reales; int objetos = 0;
		while (std::getline(f, linea)) {
			if (linea.rfind("RAW ", 0) != 0) continue;
			std::istringstream ss(linea); std::string raw, dir; ss >> raw >> dir;
			std::vector<uint8_t> o(kBytesObjeto, 0);
			for (int w = 0; w < 160; w++) { std::string h; ss >> h; uint32_t v = (uint32_t)std::strtoul(h.c_str(), nullptr, 16); std::memcpy(o.data() + w * 4, &v, 4); }
			objetos++;
			Actor r;
			if (leerActor(o.data(), r)) { r.dir = std::strtoull(dir.c_str(), nullptr, 16); reales.push_back(r); }
		}
		std::printf("reales: %d objetos, %zu actores validos\n", objetos, reales.size());
		CHEQUEA(reales.size() >= 40);
		auto l = elegirLista(reales);
		CHEQUEA(l.size() == 22);
		int asistentes = 0; for (auto& r : l) if (pareceAsistente(r)) asistentes++;
		std::printf("lista de %zu, asistentes por pegar a la banda: %d\n", l.size(), asistentes);
	}
	std::printf(fallos ? "campo_test: %d FALLOS\n" : "campo_test: todo bien\n", fallos);
	return fallos ? 1 : 0;
}
