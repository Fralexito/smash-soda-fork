// Prueba de la lógica pura del campo 2D (core/CampoVivo). Con la variable PHX_CAMPO_RAW apuntando a un volcado
// «RAW <dir> <160 palabras en hex>» (salida de tools/radar/entes.exe raw) también prueba con objetos reales del juego.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

#include "phoenix/core/CampoVivo.h"

using namespace phoenix::campo;

static int fallos = 0;
#define CHEQUEA(c) do { if (!(c)) { std::printf("FALLO linea %d: %s\n", __LINE__, #c); fallos++; } } while (0)

static void poner32(std::vector<uint8_t>& o, uint32_t off, uint32_t v) { std::memcpy(o.data() + off, &v, 4); }
static void ponerF(std::vector<uint8_t>& o, uint32_t off, float v) { std::memcpy(o.data() + off, &v, 4); }

static std::vector<uint8_t> base(uint32_t codigo, uint32_t tipo, uint32_t lado, uint32_t marca, float x, float y, float z) {
	std::vector<uint8_t> o(kBytesObjeto, 0);
	poner32(o, kOffTipo, tipo); poner32(o, kOffLado, lado); poner32(o, kOffMarca, marca);
	poner32(o, kOffCodigo, (codigo << 16) | 3);
	poner32(o, kOffReg0, 0x10); poner32(o, kOffReg3, 0xDEAD);
	ponerF(o, kOffPosicion, x); ponerF(o, kOffPosicion + 4, y); ponerF(o, kOffPosicion + 8, z); ponerF(o, kOffPosicion + 12, 1.0f);
	return o;
}
static std::vector<uint8_t> actor(uint32_t equipoIA, float x, float y, float z) { return base(equipoIA, 0x18, 2, 0x80002, x, y, z); }
static std::vector<uint8_t> portero(float x) { return base(431, 0x18, 1, 0x80001, x, 1.1f, 0.2f); }
static std::vector<uint8_t> balon(float x, float y, float z) { return base(280, 0, 1, 0x10001, x, y, z); }

static Actor A(uint64_t dir, float x, float z, Clase c = Clase::Campo) { Actor a; a.dir = dir; a.x = x; a.y = 0.9f; a.z = z; a.clase = c; return a; }

int main() {
	// --- firmas sintéticas ---
	Actor a;
	CHEQUEA(leerActor(actor(44, 10.5f, 0.9f, -3.2f).data(), a));
	CHEQUEA(a.codigo == 44 && a.x == 10.5f && a.z == -3.2f && a.clase == Clase::Campo);
	{ auto o = actor(44, 1, 0.9f, 1); poner32(o, kOffTipo, 0x8); CHEQUEA(!leerActor(o.data(), a)); }          // otro tipo
	{ auto o = actor(44, 1, 0.9f, 1); poner32(o, kOffMarca, 0x80001); CHEQUEA(!leerActor(o.data(), a)); }      // marca de portero, no de campo
	{ auto o = actor(44, 1, 0.9f, 1); poner32(o, kOffReg3, 0x55EF); CHEQUEA(!leerActor(o.data(), a)); }        // actor obsoleto
	{ auto o = actor(44, 1, 0.9f, 1); ponerF(o, kOffPosicion + 12, 0.0f); CHEQUEA(!leerActor(o.data(), a)); }  // w != 1
	{ auto o = actor(44, 99.0f, 0.9f, 1); CHEQUEA(!leerActor(o.data(), a)); }                                   // fuera del campo
	{ auto o = actor(44, 1, 0.9f, 1); ponerF(o, kOffPosicion, std::numeric_limits<float>::quiet_NaN()); CHEQUEA(!leerActor(o.data(), a)); } // NaN
	{ auto o = actor(44, 1, 0.0f, 1); CHEQUEA(!leerActor(o.data(), a)); }                                       // y = 0 (cerebro de IA)
	CHEQUEA(leerPortero(portero(-47.0f).data(), a) && a.clase == Clase::Portero && a.x == -47.0f);
	{ auto p2 = base(1011, 0x18, 1, 0x10002, -42.5f, 1.1f, 5.4f); CHEQUEA(leerPortero(p2.data(), a) && a.clase == Clase::Portero && a.x == -42.5f); }   // portero con la marca 0x10002 (partido real, 14-oct)
	{ auto p3 = base(1011, 0x18, 1, 0x10003, -42.5f, 1.1f, 5.4f); CHEQUEA(!leerPortero(p3.data(), a)); }                                       // otra marca: no
	CHEQUEA(!leerPortero(actor(44, 1, 0.9f, 1).data(), a));                                                      // un jugador de campo no es portero
	CHEQUEA(!leerActor(portero(-47.0f).data(), a));                                                               // ni al revés
	CHEQUEA(leerBalon(balon(0, 0.11f, 0).data(), a) && a.clase == Clase::Balon);
	CHEQUEA(leerBalon(balon(0, 0.11f, 100.0f).data(), a));                                                        // aparcado en la repetición: sigue siendo candidato
	CHEQUEA(leerBalon(balon(10, 6.5f, 3).data(), a));                                                             // en el aire
	CHEQUEA(!leerBalon(balon(0, 0.11f, 60.0f).data(), a));                                                        // ni en el campo ni aparcado
	CHEQUEA(!leerBalon(actor(44, 0, 0.9f, 0).data(), a));
	float x, y, z;
	{ auto o = actor(44, -20.0f, 1.1f, 7.0f); CHEQUEA(leerPosicion(o.data() + kOffPosicion, x, y, z) && x == -20.0f && z == 7.0f); }
	CHEQUEA(!leerPosicion(std::vector<uint8_t>(16, 0).data(), x, y, z));
	{ auto o = balon(0, 0.11f, 100.0f); CHEQUEA(leerPosicionBalon(o.data() + kOffPosicion, x, y, z) && z == 100.0f); }

	// --- elegir lista y porteros ---
	std::vector<Actor> todos;
	for (int i = 0; i < 22; i++) { Actor t = A(1000 + i * 0x500, float(i), 0); t.codigo = 44; todos.push_back(t); }
	for (int i = 0; i < 22; i++) { Actor t = A(1000 + i * 0x500 + 0x280, float(i), 0); t.codigo = 67; todos.push_back(t); }
	for (int i = 0; i < 2; i++) { Actor t = A(9999 + i, 0, 0); t.codigo = 1718 + i * 20; todos.push_back(t); }
	auto lista = elegirLista(todos);
	CHEQUEA(lista.size() == 22 && lista[0].codigo == 44);
	CHEQUEA(elegirLista({}).empty());
	{ std::vector<Actor> pocos(todos.begin(), todos.begin() + 10); CHEQUEA(elegirLista(pocos).empty()); }       // lista incompleta: se apaga
	{
		std::vector<Actor> gk = { A(10, -47.0f, -0.4f, Clase::Portero), A(11, -47.1f, 0.1f, Clase::Portero), A(12, 46.9f, 0.3f, Clase::Portero), A(13, 46.9f, -0.3f, Clase::Portero) };
		auto p = elegirPorteros(gk);
		CHEQUEA(p.size() == 2 && p[0].x < 0 && p[1].x > 0);                                                   // una copia por lado
	}
	Actor asis; asis.z = -35.2f; CHEQUEA(pareceAsistente(asis)); asis.z = 20.0f; CHEQUEA(!pareceAsistente(asis));
	CHEQUEA(exeConocido(458806784ull, "5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A"));
	CHEQUEA(!exeConocido(458806785ull, "5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A"));
	CHEQUEA(!exeConocido(458806784ull, "AAAA"));

	// --- saque de centro real (Barça–City, 11 oct 2026, 0:00: saca el Barça, que ocupa la izquierda) ---
	// Lista de IA 44 tal cual la leyó entes.exe (22 actores: faltaba un jugador del Barça en ese barrido) + los dos porteros.
	const float saque[][2] = {
		{ -19.2f, -35.4f }, { -4.8f, -10.0f }, { -5.5f, -25.2f }, { -9.7f, 8.2f }, { 4.0f, -9.3f }, { 1.1f, 10.0f }, { 15.7f, -22.7f }, { 19.2f, 35.4f },
		{ 11.0f, 0.3f }, { -15.6f, 22.8f }, { -19.6f, -6.3f }, { 15.4f, 23.1f }, { -5.7f, 23.8f }, { -19.5f, 5.9f }, { -9.1f, -8.7f }, { 1.8f, -24.3f },
		{ 9.2f, 8.6f }, { -15.2f, -23.0f }, { 19.3f, 6.2f }, { 19.4f, -5.7f }, { 0.5f, 0.0f }, { 1.4f, 24.4f },
	};
	std::vector<Actor> ko;
	for (size_t i = 0; i < sizeof(saque) / sizeof(saque[0]); i++) ko.push_back(A(100 + i, saque[i][0], saque[i][1]));
	ko.push_back(A(500, -47.0f, -0.4f, Clase::Portero)); ko.push_back(A(501, 46.9f, 0.3f, Clase::Portero));
	CHEQUEA(esSaqueDeCentro(ko, 0.0f));
	CHEQUEA(!esSaqueDeCentro(ko, 0.3f));                                                                        // alguien se mueve: no es el saque
	{ std::vector<Actor> corro = ko; for (auto& c : corro) if (c.clase == Clase::Campo && !pareceAsistente(c)) c.z = 0; CHEQUEA(!esSaqueDeCentro(corro, 0.0f)); } // todos en una línea: no es formación
	auto eq = repartirEquipos(ko);
	CHEQUEA(eq.size() == ko.size());
	CHEQUEA(eq[0] == kEqArbitro && eq[7] == kEqArbitro);                                                      // asistentes
	CHEQUEA(eq[1] == kEqArbitro);                                                                              // árbitro en (-4.8, -10)
	CHEQUEA(eq[20] == kEqA);                                                                                   // el que saca, en el círculo, es del Barça (izquierda)
	CHEQUEA(eq[2] == kEqA && eq[3] == kEqA && eq[9] == kEqA);
	CHEQUEA(eq[4] == kEqB && eq[8] == kEqB && eq[21] == kEqB);
	CHEQUEA(eq[22] == kEqA && eq[23] == kEqB);                                                                 // porteros por su lado
	int nA = 0, nB = 0; for (size_t i = 0; i < eq.size(); i++) if (eq[i] == kEqA) nA++; else if (eq[i] == kEqB) nB++;
	CHEQUEA(nA == 10 && nB == 11);                                                                             // 9 + portero (faltaba uno) y 10 + portero
	{ std::vector<Actor> espejo = ko; for (auto& c : espejo) c.x = -c.x; auto e2 = repartirEquipos(espejo); CHEQUEA(e2[20] == kEqB && e2[1] == kEqArbitro && e2[22] == kEqB); } // saca el de la derecha


	// --- tanda de penales: un penal real muestreado (balón, pateador, portero), cada 0,2 s ---
	{
		const float pen[][7] = { // bx, by, bz, kx, kz, gx, gz
			{ -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -52.4f, -0.8f }, { -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -52.5f, -0.4f }, { -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -52.5f, -0.4f },
			{ -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -52.4f, -0.5f }, { -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -52.4f, -0.5f }, { -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -52.3f, -0.4f },
			{ -45.6f, 0.56f, -0.9f, -43.2f, -1.5f, -51.4f, 1.2f }, { -50.4f, 0.58f, -2.1f, -43.2f, -1.5f, -52.0f, 2.0f }, { -54.7f, 0.12f, -3.1f, -43.2f, -1.5f, -51.8f, 3.0f },
			{ -54.7f, 1.09f, -2.8f, -43.2f, -1.5f, -52.3f, 3.6f }, { -53.4f, 0.46f, -2.2f, -43.2f, -1.5f, -51.6f, 4.0f }, { -52.3f, 0.23f, -1.6f, -43.2f, -1.5f, -52.1f, 4.9f },
			{ -51.5f, 0.11f, -1.1f, -43.2f, -1.5f, -52.1f, 4.9f }, { -51.2f, 0.11f, -0.9f, -43.2f, -1.5f, -51.5f, 4.5f }, { -41.6f, 0.11f, 0.0f, -43.2f, -1.5f, -51.7f, -20.6f },
		};
		std::vector<Actor> l; std::vector<int> eqs;
		for (int i = 0; i < 14; i++) { l.push_back(A(700 + i, (i % 2 ? 3.0f : -3.0f), float(i - 7))); eqs.push_back(i % 2 ? kEqB : kEqA); } // los demás esperan en el círculo
		l.push_back(A(800, -43.2f, -1.5f)); eqs.push_back(kEqA);                                       // pateador (equipo A)
		l.push_back(A(801, -52.4f, -0.8f, Clase::Portero)); eqs.push_back(kEqB);
		Tanda tanda; double t = 100.0;
		for (size_t s = 0; s < sizeof(pen) / sizeof(pen[0]); s++) {
			l[14].x = pen[s][3]; l[14].z = pen[s][4]; l[15].x = pen[s][5]; l[15].z = pen[s][6];
			tanda.alimentar(true, pen[s][0], pen[s][1], pen[s][2], l, eqs, t); t += 0.2;
			if (s == 4) { CHEQUEA(tanda.estado().activa && tanda.estado().listo && tanda.estado().turnoEq == kEqA); }
			if (s == 8) { CHEQUEA(tanda.estado().enVuelo && !tanda.estado().listo); }
		}
		for (int i = 0; i < 8; i++) { tanda.alimentar(true, -41.6f, 0.11f, 0.0f, l, eqs, t); t += 0.2; } // de vuelta en el punto
		CHEQUEA(tanda.estado().lanzamientos.size() == 1 && tanda.estado().lanzamientos[0].gol && tanda.estado().lanzamientos[0].eq == kEqA);
		CHEQUEA(tanda.estado().listo && tanda.estado().activa);
		// un fallo: el balón sale por fuera del palo y se aparca
		for (int i = 0; i < 4; i++) { tanda.alimentar(true, -48.0f - i * 2, 0.5f, -4.5f - i * 0.3f, l, eqs, t); t += 0.2; }
		tanda.alimentar(true, -56.0f, 0.5f, -5.8f, l, eqs, t); t += 0.2;
		tanda.alimentar(true, 0.0f, 0.11f, 100.0f, l, eqs, t); t += 0.2;
		CHEQUEA(tanda.estado().lanzamientos.size() == 2 && !tanda.estado().lanzamientos[1].gol);
		// un penal en pleno partido (nadie espera en el círculo) no cuenta como tanda
		Tanda t2; std::vector<Actor> l2; std::vector<int> e2;
		for (int i = 0; i < 16; i++) { l2.push_back(A(900 + i, -30.0f - (i % 5), float(i * 2 - 15))); e2.push_back(i % 2); }
		for (int i = 0; i < 10; i++) t2.alimentar(true, -41.6f, 0.11f, 0.0f, l2, e2, 200.0 + i * 0.2);
		CHEQUEA(!t2.estado().activa);
	}

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
		CHEQUEA(l.size() >= 21);
		int asistentes = 0; for (auto& r : l) if (pareceAsistente(r)) asistentes++;
		std::printf("lista de %zu, asistentes por pegar a la banda: %d\n", l.size(), asistentes);
	}
	// ---- Poseedor del balón ----
	{
		auto jug = [](std::initializer_list<std::pair<float, float>> ps) {
			std::vector<PuntoCampo> v; for (auto& p : ps) { PuntoCampo c; c.x = p.first; c.z = p.second; v.push_back(c); } return v; };
		// 1) Regate: A corre a 6 m/s con el balón 0,8 m por delante; B lo persigue a 3 m. Nunca parpadea.
		{
			Poseedor p; int cambios = 0, ult = -2; bool sinPos = false;
			for (int i = 0; i < 80; i++) {
				const double t = 1.0 + i * 0.05; const float ax = static_cast<float>(i * 0.3);
				auto j = jug({ { ax, 0.f }, { ax - 3.f, 1.f }, { 30.f, 20.f } });
				const int r = p.actualizar(j, true, ax + 0.8f, 0.1f, 0.f, t);
				if (r != ult) { cambios++; ult = r; }
				if (i > 4 && r != 0) sinPos = true;
			}
			CHEQUEA(!sinPos); CHEQUEA(cambios <= 2);   // -1 → 0 una sola vez
		}
		// 2) Pase de A a B (20 m): libre en el aire y B lo recibe; nadie más lo «lleva» por el camino
		{
			Poseedor p; std::vector<int> seq;
			auto j = jug({ { 0.f, 0.f }, { 20.f, 0.f }, { 10.f, 15.f } });
			double t = 1.0;
			for (int i = 0; i < 6; i++, t += 0.05) p.actualizar(j, true, 0.6f, 0.1f, 0.f, t);
			CHEQUEA(p.actual() == 0);
			float bx = 0.6f;
			for (int i = 0; i < 20; i++, t += 0.05) { bx += 0.9f; seq.push_back(p.actualizar(j, true, bx, 0.3f, 0.f, t)); }   // 18 m/s
			for (int i = 0; i < 12; i++, t += 0.05) { seq.push_back(p.actualizar(j, true, 20.5f, 0.1f, 0.f, t)); }
			CHEQUEA(seq.back() == 1);
			for (int v : seq) CHEQUEA(v != 2);
			bool huboLibre = false; for (int v : seq) if (v == -1) huboLibre = true;
			CHEQUEA(huboLibre);
		}
		// 3) Balón alto que pasa por encima de C: C no lo lleva nunca
		{
			Poseedor p; auto j = jug({ { 5.f, 0.f } });
			bool lo_llevo = false;
			for (int i = 0; i < 30; i++) if (p.actualizar(j, true, 5.2f, 5.0f, 0.f, 1.0 + i * 0.05) == 0) lo_llevo = true;
			CHEQUEA(!lo_llevo);
		}
		// 4) Balón libre lejos de todos: -1 · cerca pero sin tiempo mínimo no se adquiere de golpe
		{
			Poseedor p; auto j = jug({ { 10.f, 10.f }, { -10.f, -10.f } });
			CHEQUEA(p.actualizar(j, true, 0.f, 0.1f, 0.f, 1.0) == -1);
			CHEQUEA(p.actualizar(j, true, 9.5f, 0.1f, 10.f, 1.05) == -1);
			CHEQUEA(p.actualizar(j, true, 9.5f, 0.1f, 10.f, 1.10) == -1);
			CHEQUEA(p.actualizar(j, true, 9.5f, 0.1f, 10.f, 1.20) == 0);
		}
		// 5) Disputa y sin parpadeo: A (1,0 m) y B (1,1 m) casi empatados → se queda A; B solo gana cuando A ya está lejos
		{
			Poseedor p; auto j = jug({ { 0.f, 0.f }, { 0.f, 1.4f } });
			for (int i = 0; i < 6; i++) p.actualizar(j, true, 0.f, 0.1f, 0.5f, 1.0 + i * 0.05);   // el balón está a 0,5 de A
			CHEQUEA(p.actual() == 0);
			for (int i = 0; i < 20; i++) {
				auto k = jug({ { 0.f, 0.f }, { 0.f, (i % 2) ? 0.9f : 1.1f } });   // B se pega, alterna 0,9/1,1 m del balón (en z=0,5+…)
				p.actualizar(k, true, 0.f, 0.1f, 1.f, 1.3 + i * 0.05);
			}
			CHEQUEA(p.actual() == 0);                                              // A a 1,0 m: lo conserva
			// A queda atrás a 2,0 m y B a 0,8 m del balón: cambia
			auto m = jug({ { 0.f, 0.f }, { 0.f, 2.8f } });
			int r = -9; for (int i = 0; i < 4; i++) r = p.actualizar(m, true, 0.f, 0.1f, 2.0f, 2.5 + i * 0.05);
			CHEQUEA(r == 1);
		}
		// 6) Repetición (balón no visible): se conserva 350 ms y luego libre
		{
			Poseedor p; auto j = jug({ { 0.f, 0.f } });
			for (int i = 0; i < 6; i++) p.actualizar(j, true, 0.5f, 0.1f, 0.f, 1.0 + i * 0.05);
			CHEQUEA(p.actual() == 0);
			CHEQUEA(p.actualizar(j, false, 0, 0, 0, 1.40) == 0);
			CHEQUEA(p.actualizar(j, false, 0, 0, 0, 1.80) == -1);
		}
		// 7) Árbitros y asistentes no son elegibles
		{
			Poseedor p; std::vector<PuntoCampo> j(2); j[0].x = 0.3f; j[0].z = 0; j[0].elegible = false; j[1].x = 3.f; j[1].z = 3.f;
			for (int i = 0; i < 6; i++) p.actualizar(j, true, 0.f, 0.1f, 0.f, 1.0 + i * 0.05);
			CHEQUEA(p.actual() == -1);
		}
		// 8) Índice fuera de rango (menos fichas que antes): sin caída
		{
			Poseedor p; auto j = jug({ { 0.f, 0.f }, { 1.f, 1.f } });
			for (int i = 0; i < 6; i++) p.actualizar(j, true, 1.f, 0.1f, 1.f, 1.0 + i * 0.05);
			auto k = jug({ { 50.f, 0.f } });
			CHEQUEA(p.actualizar(k, true, 1.f, 0.1f, 1.f, 2.0) == -1);
		}
	}
	// ---- Estadísticas derivadas ----
	{
		auto fichas = [](int n) { std::vector<PuntoCampo> v(static_cast<size_t>(n)); return v; };
		const std::vector<int> eq = { 0, 0, 1, 1 };   // piezas 0,1 = A; 2,3 = B
		// 1) posesión por tiempo: A 5 s, B 3 s
		{
			Estadisticas e; double t = 1.0;
			for (int i = 0; i < 100; i++, t += 0.05) e.alimentar(t, true, 0.f, 0.1f, 0.f, 0, eq, fichas(4), -1);
			for (int i = 0; i < 60; i++, t += 0.05) e.alimentar(t, true, 0.f, 0.1f, 0.f, 2, eq, fichas(4), -1);
			const auto& s = e.estado();
			const double a = s.tPosesion[0], b = s.tPosesion[1];
			CHEQUEA(a > 4.8 && a < 5.2); CHEQUEA(b > 2.8 && b < 3.2);
		}
		// 2) pase completado A→A en 1 s; pérdida A→B; pase tras >3 s libre no cuenta
		{
			Estadisticas e; double t = 1.0;
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 0.f, 0.1f, 0.f, 0, eq, fichas(4), -1);
			for (int i = 0; i < 20; i++, t += 0.05) e.alimentar(t, true, 5.f, 0.1f, 0.f, -1, eq, fichas(4), -1);   // en el aire 1 s
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 10.f, 0.1f, 0.f, 1, eq, fichas(4), -1);
			CHEQUEA(e.estado().pases[0] == 1);
			for (int i = 0; i < 20; i++, t += 0.05) e.alimentar(t, true, 12.f, 0.1f, 0.f, -1, eq, fichas(4), -1);
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 14.f, 0.1f, 0.f, 2, eq, fichas(4), -1);   // lo coge B
			CHEQUEA(e.estado().perdidas[0] == 1); CHEQUEA(e.estado().recuperaciones[1] == 1);
			for (int i = 0; i < 80; i++, t += 0.05) e.alimentar(t, true, 20.f, 0.1f, 0.f, -1, eq, fichas(4), -1);   // 4 s libre
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 20.f, 0.1f, 0.f, 3, eq, fichas(4), -1);
			CHEQUEA(e.estado().pases[1] == 0 && e.estado().recuperaciones[0] == 0);   // no cuenta: demasiado tiempo libre
		}
		// 3) tiro a puerta de A (A defiende x<0, ataca hacia +x) y tiro desviado
		{
			Estadisticas e; double t = 1.0;
			for (int i = 0; i < 8; i++, t += 0.05) e.alimentar(t, true, 30.f, 0.1f, 1.f, 0, eq, fichas(4), -1);
			float bx = 30.f;
			for (int i = 0; i < 12; i++, t += 0.05) { bx += 1.1f; e.alimentar(t, true, bx, 0.5f, 1.f, i < 3 ? 0 : -1, eq, fichas(4), -1); }   // 22 m/s hacia la portería derecha
			CHEQUEA(e.estado().tiros[0] == 1); CHEQUEA(e.estado().tirosPuerta[0] == 1);
			t += 4.0;
			for (int i = 0; i < 8; i++, t += 0.05) e.alimentar(t, true, 30.f, 0.1f, 8.f, 0, eq, fichas(4), -1);
			bx = 30.f; float bz = 8.f;
			for (int i = 0; i < 12; i++, t += 0.05) { bx += 1.1f; bz += 0.15f; e.alimentar(t, true, bx, 0.5f, bz, i < 3 ? 0 : -1, eq, fichas(4), -1); }   // se va a +11 m: no es tiro ni a puerta
			CHEQUEA(e.estado().tiros[0] == 1);
		}
		// 4) córner / saque de puerta / banda / gol
		{
			Estadisticas e; double t = 1.0;
			// A defiende x<0; B defiende x>0. Lo toca B y sale por el fondo derecho (z=15): córner para A
			for (int i = 0; i < 8; i++, t += 0.05) e.alimentar(t, true, 50.f, 0.1f, 15.f, 2, eq, fichas(4), -1);
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 53.5f, 0.1f, 15.f, -1, eq, fichas(4), -1);
			CHEQUEA(e.estado().corners[0] == 1);
			for (int i = 0; i < 30; i++, t += 0.05) e.alimentar(t, true, 45.f, 0.1f, 5.f, -1, eq, fichas(4), -1);   // vuelve al campo
			// Lo toca A y sale por el mismo fondo: saque de puerta para B
			for (int i = 0; i < 8; i++, t += 0.05) e.alimentar(t, true, 50.f, 0.1f, 15.f, 0, eq, fichas(4), -1);
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 53.5f, 0.1f, 15.f, -1, eq, fichas(4), -1);
			CHEQUEA(e.estado().saquesMeta[1] == 1);
			for (int i = 0; i < 30; i++, t += 0.05) e.alimentar(t, true, 45.f, 0.1f, 5.f, -1, eq, fichas(4), -1);
			// Banda: lo tocó A y sale por z=35: saque de banda para B
			for (int i = 0; i < 8; i++, t += 0.05) e.alimentar(t, true, 10.f, 0.1f, 33.f, 0, eq, fichas(4), -1);
			for (int i = 0; i < 10; i++, t += 0.05) e.alimentar(t, true, 10.f, 0.1f, 35.f, -1, eq, fichas(4), -1);
			CHEQUEA(e.estado().saquesBanda[1] == 1);
			for (int i = 0; i < 30; i++, t += 0.05) e.alimentar(t, true, 10.f, 0.1f, 20.f, -1, eq, fichas(4), -1);
			// Gol (red de la portería derecha): no es córner ni saque
			const auto antes = e.estado();
			for (int i = 0; i < 8; i++, t += 0.05) e.alimentar(t, true, 50.f, 0.1f, 1.f, 0, eq, fichas(4), -1);
			for (int i = 0; i < 6; i++, t += 0.05) e.alimentar(t, true, 53.f, 1.0f, 0.5f, -1, eq, fichas(4), -1);
			CHEQUEA(e.estado().corners[0] == antes.corners[0] && e.estado().saquesMeta[1] == antes.saquesMeta[1]);
		}
		// 5) sin saber el lado de defensa: no inventa tiros ni córners, pero sí posesión
		{
			Estadisticas e; double t = 1.0;
			for (int i = 0; i < 20; i++, t += 0.05) e.alimentar(t, true, 53.5f, 0.1f, 15.f, i < 5 ? 2 : -1, eq, fichas(4), 0);
			CHEQUEA(e.estado().corners[0] + e.estado().corners[1] + e.estado().saquesMeta[0] + e.estado().saquesMeta[1] == 0);
			CHEQUEA(e.estado().tPosesion[1] > 0);
		}
	}
	std::printf(fallos ? "campo_test: %d FALLOS\n" : "campo_test: todo bien\n", fallos);
	return fallos ? 1 : 0;
}
