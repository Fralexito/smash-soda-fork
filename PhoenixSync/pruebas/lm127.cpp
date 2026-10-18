// Phoenix Sync · Pruebas de la Liga Máster desde la web (127): CambiosLM + PartidasLM.
//  Sin archivos: formato, validaciones y estado.
//  Con PM_ML=<COPIA de una ranura ML0000000N> (cifrada, nunca la real): todos los tipos sobre la partida,
//  deshacer, idempotencia, filtro por partida y escritura segura en una carpeta temporal.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "core/CambiosLM.h"
#include "core/PartidasLM.h"
#include "core/Sha256.h"

using namespace mercado;
using json = nlohmann::json;
namespace fs = std::filesystem;

static int total = 0, fallos = 0;
#define CHECK(c) do { total++; if (!(c)) { fallos++; std::printf("  FALLO linea %d: %s\n", __LINE__, #c); } } while (0)

static json lista(json cambios, int64_t desde = 0) { return { { "formato", lmweb::kFormato }, { "desde", desde }, { "cambios", cambios } }; }
static lmweb::ListaLM parsearOk(const json& j) { auto l = lmweb::parsear(j.dump()); if (!l.ok()) std::printf("  parsear: %s\n", l.error.detalle.c_str()); return l.ok() ? *l.valor : lmweb::ListaLM{}; }
static const lmweb::ResultadoCambio* resultado(const lmweb::Informe& inf, int64_t v) { for (auto& r : inf.resultados) if (r.version == v) return &r; return nullptr; }
static std::string estadoDe(const lmweb::Informe& inf, int64_t v) { auto r = resultado(inf, v); return r ? r->estado : "(nada)"; }

static void sinArchivos() {
	std::printf("Formato y validaciones\n");
	CHECK(!lmweb::parsear("no es json").ok());
	CHECK(!lmweb::parsear(json{ { "cambios", json::array() } }.dump()).ok());                       // sin formato
	CHECK(lmweb::parsear(lista(json::array()).dump()).ok());
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "borrar_todo" } } }).dump()).ok());   // tipo desconocido
	CHECK(!lmweb::parsear(lista({ { { "version", 2 }, { "tipo", "valor" }, { "pes_id", 5 }, { "valor", 100 } }, { { "version", 2 }, { "tipo", "valor" }, { "pes_id", 5 }, { "valor", 100 } } }).dump()).ok());   // orden
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "ia_ia" }, { "pes_id", 5 } } }).dump()).ok());   // sin club destino
	CHECK(lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "ia_ia" }, { "pes_id", 5 }, { "club_hacia_pes", 9 } } }).dump()).ok());
	CHECK(lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "traspaso" }, { "pes_id", 5 }, { "club_desde_pes", 0 }, { "club_hacia_pes", 9 }, { "cambio_id", "x" } } }).dump()).ok());
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "ia_ia" }, { "pes_id", 5 }, { "club_hacia_pes", 9 }, { "dorsal", 120 } } }).dump()).ok());
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "dinero" } } }).dump()).ok());       // sin importes
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "dinero" }, { "presupuesto", 150 } } }).dump()).ok());   // no múltiplo de 100
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "dinero" }, { "presupuesto", 5e10 } } }).dump()).ok());  // > 40.000 M
	CHECK(lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "dinero" }, { "presupuesto", 1000000 }, { "tope_salarial", nullptr } } }).dump()).ok());
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "valor" }, { "pes_id", 5 } } }).dump()).ok());
	CHECK(!lmweb::parsear(lista({ { { "version", 3 }, { "tipo", "deshacer" }, { "deshace", 3 } } }).dump()).ok());   // tiene que ser anterior
	CHECK(lmweb::parsear(lista({ { { "version", 3 }, { "tipo", "deshacer" }, { "deshace", 2 } } }).dump()).ok());
	CHECK(!lmweb::parsear(lista({ { { "version", 1 }, { "tipo", "stat" }, { "partida", 7 }, { "pes_id", 5 } } }).dump()).ok());   // partida no es texto
	CHECK(lmweb::esRanuraLM("ML0000000B") && lmweb::esRanuraLM("ML00000001") && !lmweb::esRanuraLM("ML0000000B.zip") && !lmweb::esRanuraLM("EDIT00000000") && !lmweb::esRanuraLM("ML000000G1"));
	// Estado: ida y vuelta con pendientes y diario
	lmweb::EstadoPartida e; e.versionAplicada = 9; e.diario["4"] = { { "tipo", "valor" }, { "pes_id", 5 }, { "antes", { { "valor", 1000 } } } };
	auto l = parsearOk(lista({ { { "version", 7 }, { "tipo", "stat" }, { "pes_id", 5 }, { "campo", "velocidad" } } }));
	if (!l.cambios.empty()) e.pendientes.push_back(l.cambios[0]);
	auto e2 = lmweb::EstadoPartida::desdeJson(json::parse(e.aJson().dump()));
	CHECK(e2.versionAplicada == 9 && e2.diario == e.diario && e2.pendientes.size() == 1 && e2.pendientes[0].tipo == "stat" && e2.pendientes[0].version == 7);
	CHECK(lmweb::EstadoPartida::desdeJson(json("basura")).versionAplicada == 0);
}

static void conPartida(const char* rutaMl) {
	std::printf("Partida real (copia): %s\n", rutaMl);
	auto g0 = lm::GuardadoLM::abrir(rutaMl);
	if (!g0.ok()) {
		// Otro juego / parche que no sabemos abrir: se deja tal cual y se avisa (regla 5c: si no se reconoce, no se toca).
		std::printf("  no se reconoce (%s: %s): se comprueba que se salta sin tocarla\n", g0.error.codigo.c_str(), g0.error.detalle.c_str());
		const fs::path tmp = fs::temp_directory_path() / "phoenix-lm127-desconocida";
		std::error_code ec; fs::remove_all(tmp, ec); fs::create_directories(tmp, ec);
		fs::copy_file(aRuta(rutaMl), tmp / "ML00000000", ec);
		const std::string sha0 = sha256::deArchivo(deRuta(tmp / "ML00000000"));
		std::vector<std::string> avisos;
		auto ps = lmweb::buscarPartidas({ deRuta(tmp) }, &avisos);
		CHECK(ps.empty() && avisos.size() == 1 && sha256::deArchivo(deRuta(tmp / "ML00000000")) == sha0);
		fs::remove_all(tmp, ec);
		return;
	}
	const lm::GuardadoLM& G = *g0.valor;
	const int kU = lmweb::equipoUsuario(G);
	CHECK(kU >= 0);
	if (kU < 0) return;
	const uint32_t idU = *G.idOptionDe(kU).valor;
	// Dos clubes de la IA con plantilla larga y un jugador de cada uno (la última reserva).
	int kA = -1, kB = -1;
	for (int k = 0; k < 528 && kB < 0; k++) {
		if (k == kU) continue;
		auto eq = G.equipo(k); auto id = G.idOptionDe(k);
		if (!eq.ok() || !id.ok() || eq.valor->plantilla.size() < 22) continue;
		(kA < 0 ? kA : kB) = k;
	}
	CHECK(kA >= 0 && kB >= 0);
	if (kB < 0) return;
	const uint32_t idA = *G.idOptionDe(kA).valor, idB = *G.idOptionDe(kB).valor;
	const uint32_t pA = G.equipo(kA).valor->plantilla.back().pid, pB = G.equipo(kB).valor->plantilla.back().pid;
	const std::string partida = lmweb::idPartida("ML0000000B", G);
	// Del usuario: el primero (desde las reservas) que el motor sabe vender en esta partida (un portero sin recambio no se puede).
	uint32_t pU = G.equipo(kU).valor->plantilla.back().pid;
	{
		const auto& pl = G.equipo(kU).valor->plantilla;
		for (size_t i = pl.size(); i-- > 0;) {
			lm::GuardadoLM prueba = G; lmweb::EstadoPartida ep;
			auto lp = parsearOk(lista({ { { "version", 1 }, { "tipo", "venta" }, { "pes_id", pl[i].pid }, { "club_hacia_pes", idA } } }));
			if (lmweb::aplicar(lp, partida, prueba, ep, {}).aplicados == 1) { pU = pl[i].pid; break; }
		}
	}
	std::printf("  usuario: bloque %d (club %u), partida %s; IA: %u (club %u) y %u (club %u); del usuario: %u\n", kU, idU, partida.c_str(), pA, idA, pB, idB, pU);
	auto fin0 = G.finanzas(kU); CHECK(fin0.ok());
	auto val0 = G.valorMercado(pA); CHECK(val0.ok() && *val0.valor > 0);
	if (!fin0.ok() || !val0.ok()) return;
	const uint64_t presu0 = fin0.valor->presupuestoFichajes, tope0 = fin0.valor->topeSalarial;
	std::printf("  dinero: fichajes %llu, tope %llu; valor de %u: %llu\n", (unsigned long long)presu0, (unsigned long long)tope0, pA, (unsigned long long)*val0.valor);

	lmweb::Contexto ctx;   // sin catálogo: el motor elige sustituto sin posición
	json cambios = json::array({
		{ { "version", 1 }, { "tipo", "ia_ia" }, { "pes_id", pA }, { "club_desde_pes", idA }, { "club_hacia_pes", idB } },
		{ { "version", 2 }, { "tipo", "fichaje" }, { "pes_id", pB }, { "club_desde_pes", idB }, { "club_hacia_pes", idU }, { "monto", 1000000 }, { "fin_contrato", "2028-06-30" } },
		{ { "version", 3 }, { "tipo", "venta" }, { "pes_id", pU }, { "club_desde_pes", idU }, { "club_hacia_pes", idA } },
		{ { "version", 4 }, { "tipo", "dinero" }, { "presupuesto", 250000000 } },
		{ { "version", 5 }, { "tipo", "valor" }, { "pes_id", pA }, { "valor", 77700000 } },
		{ { "version", 6 }, { "tipo", "stat" }, { "pes_id", pA }, { "campo", "velocidad" }, { "direccion", 1 } },
		{ { "version", 7 }, { "tipo", "valor" }, { "pes_id", pA }, { "valor", 1000 }, { "partida", "ML00000009:1" } },   // otra partida: no toca
		{ { "version", 8 }, { "tipo", "fichaje" }, { "pes_id", pA }, { "club_hacia_pes", idA } },                         // fichaje a un club que no es el tuyo
		{ { "version", 9 }, { "tipo", "ia_ia" }, { "pes_id", pA }, { "club_hacia_pes", idB } },                          // ya está allí
	});
	auto L = parsearOk(lista(cambios));
	CHECK(L.cambios.size() == 9);
	lm::GuardadoLM g = G;
	lmweb::EstadoPartida est;
	auto inf = lmweb::aplicar(L, partida, g, est, ctx);
	std::printf("%s", inf.texto().c_str());
	// Fichaje y venta dependen de la partida (alineación del usuario, porteros, tablas): pueden aplicarse o rechazarse,
	// pero lo que diga el informe tiene que ser verdad en la partida, y un rechazo no puede tocar nada.
	auto enClub = [](const lm::GuardadoLM& x, uint32_t pid) { for (int k : x.equiposDe(pid)) if (k < 528) return k; return -1; };
	const bool ap2 = estadoDe(inf, 2) == "aplicado", ap3 = estadoDe(inf, 3) == "aplicado";
	std::printf("  en esta partida: fichaje %s, venta %s\n", ap2 ? "aplicado" : "rechazado", ap3 ? "aplicada" : "rechazada");
	CHECK(estadoDe(inf, 1) == "aplicado");
	CHECK((ap2 || estadoDe(inf, 2) == "rechazado") && (ap3 || estadoDe(inf, 3) == "rechazado"));
	CHECK(estadoDe(inf, 4) == "aplicado" && estadoDe(inf, 5) == "aplicado");
	CHECK(estadoDe(inf, 6) == "pendiente");
	CHECK(resultado(inf, 7) == nullptr);
	CHECK(estadoDe(inf, 8) == "rechazado");
	CHECK(estadoDe(inf, 9) == "sin_efecto");
	CHECK(inf.modificado && est.versionAplicada == 9 && est.pendientes.size() == 1);
	// Lo que quedó en la partida
	CHECK(enClub(g, pA) == kB);
	CHECK(enClub(g, pB) == (ap2 ? kU : kB) && enClub(g, pU) == (ap3 ? kA : kU));
	CHECK(g.finanzas(kU).ok() && g.finanzas(kU).valor->presupuestoFichajes == 250000000);
	CHECK(g.valorMercado(pA).ok() && *g.valorMercado(pA).valor == 77700000);
	auto num = [](const json& j, const char* a, const char* b = nullptr) -> long long {
		if (!j.is_object() || !j.contains(a)) return -1;
		const json& x = b ? (j[a].is_object() && j[a].contains(b) ? j[a][b] : json()) : j[a];
		return x.is_number() ? x.get<long long>() : -1;
	};
	if (ap2) {
		CHECK(resultado(inf, 2) && num(resultado(inf, 2)->antes, "club_pes") == idB && num(resultado(inf, 2)->antes, "finanzas", "presupuesto") == (long long)presu0);
		CHECK(resultado(inf, 2) && num(resultado(inf, 2)->despues, "finanzas", "presupuesto") == (long long)presu0 - 1000000);
	}
	// El valor de otro jugador no cambió
	CHECK(g.valorMercado(pB).ok() && G.valorMercado(pB).ok() && *g.valorMercado(pB).valor == *G.valorMercado(pB).valor);

	// Idempotente: la misma lista otra vez no hace nada nuevo (solo reintenta el pendiente)
	lm::GuardadoLM g2 = g;
	auto inf2 = lmweb::aplicar(L, partida, g2, est, ctx);
	CHECK(!inf2.modificado && inf2.resultados.size() == 1 && inf2.resultados[0].version == 6 && g2.datos() == g.datos());

	// Motor de stats conectado (simulado): el pendiente se aplica en la siguiente vuelta
	lmweb::Contexto ctxS = ctx;
	int llamadas = 0;
	ctxS.aplicarStat = [&](lm::GuardadoLM&, const lmweb::CambioLM& c) { llamadas++; return Resultado<json>::bien({ { "antes", { { "velocidad", 80 } } }, { "despues", { { "velocidad", c.tipo == "deshacer" ? 80 : 81 } } } }); };
	auto inf3 = lmweb::aplicar(lmweb::ListaLM{}, partida, g2, est, ctxS);
	CHECK(estadoDe(inf3, 6) == "aplicado" && est.pendientes.empty() && llamadas == 1);

	// Deshacer: valor, dinero, venta, fichaje e IA↔IA (en orden inverso), uno que aquí nunca se aplicó, uno repetido y el stat
	auto D = parsearOk(lista({
		{ { "version", 10 }, { "tipo", "deshacer" }, { "deshace", 5 } },
		{ { "version", 11 }, { "tipo", "deshacer" }, { "deshace", 4 } },
		{ { "version", 12 }, { "tipo", "deshacer" }, { "deshace", 3 } },
		{ { "version", 13 }, { "tipo", "deshacer" }, { "deshace", 2 } },
		{ { "version", 14 }, { "tipo", "deshacer" }, { "deshace", 1 } },
		{ { "version", 15 }, { "tipo", "deshacer" }, { "deshace", 7 } },
		{ { "version", 16 }, { "tipo", "deshacer" }, { "deshace", 5 } },
		{ { "version", 17 }, { "tipo", "deshacer" }, { "deshace", 6 } },
	}, 9));
	auto inf4 = lmweb::aplicar(D, partida, g2, est, ctxS);
	std::printf("%s", inf4.texto().c_str());
	for (int v : { 10, 11, 14 }) CHECK(estadoDe(inf4, v) == "aplicado");
	// Deshacer lo que no se aplicó = sin efecto; deshacer lo aplicado = aplicado o, si el motor no puede, rechazado sin tocar nada.
	const std::string d12 = estadoDe(inf4, 12), d13 = estadoDe(inf4, 13);
	CHECK(ap3 ? (d12 == "aplicado" || d12 == "rechazado") : d12 == "sin_efecto");
	CHECK(ap2 ? (d13 == "aplicado" || d13 == "rechazado") : d13 == "sin_efecto");
	CHECK(estadoDe(inf4, 15) == "sin_efecto" && estadoDe(inf4, 16) == "sin_efecto" && estadoDe(inf4, 17) == "aplicado");
	CHECK(enClub(g2, pA) == kA);
	CHECK(enClub(g2, pB) == (d13 == "rechazado" ? kU : kB) && enClub(g2, pU) == (d12 == "rechazado" ? kA : kU));
	CHECK(*g2.valorMercado(pA).valor == *val0.valor);
	auto fin2 = g2.finanzas(kU);
	CHECK(fin2.ok() && fin2.valor->topeSalarial == tope0);
	CHECK(fin2.ok() && fin2.valor->presupuestoFichajes == (d13 == "rechazado" ? presu0 - 1000000 : presu0));
	if (fin2.ok()) std::printf("  dinero tras deshacer: %llu (antes %llu)\n", (unsigned long long)fin2.valor->presupuestoFichajes, (unsigned long long)presu0);

	// «traspaso» de la web: el motor decide el papel en cada partida (aquí: de un club de la IA al del usuario = fichaje)
	const int kAntes18 = enClub(g2, pA);
	auto T = parsearOk(lista({ { { "version", 18 }, { "tipo", "traspaso" }, { "pes_id", pA }, { "club_desde_pes", 0 }, { "club_hacia_pes", idU }, { "cambio_id", "uuid-de-prueba" } } }, 17));
	lm::GuardadoLM g3 = g2;
	auto inf5 = lmweb::aplicar(T, partida, g3, est, ctxS);
	const std::string e18 = estadoDe(inf5, 18);
	CHECK((e18 == "aplicado" && enClub(g3, pA) == kU) || (e18 == "rechazado" && enClub(g3, pA) == kAntes18 && g3.datos() == g2.datos()));

	// Escritura segura en una carpeta temporal (copia de la copia)
	const fs::path tmp = fs::temp_directory_path() / "phoenix-lm127-prueba";
	std::error_code ec; fs::remove_all(tmp, ec); fs::create_directories(tmp / "save", ec);
	const fs::path ranura = tmp / "save" / "ML0000000B";
	fs::copy_file(aRuta(rutaMl), ranura, ec);
	CHECK(!ec);
	std::vector<std::string> avisos;
	auto ps = lmweb::buscarPartidas({ deRuta(tmp / "save") }, &avisos);
	CHECK(ps.size() == 1 && ps[0].id == partida && ps[0].clubPes == idU);
	if (ps.size() != 1) return;
	std::printf("  partida encontrada: %s\n", ps[0].aJson().dump().c_str());
	lmweb::OpcionesEscritura op;
	op.carpetaEstado = deRuta(tmp / "estado"); op.raizRespaldos = deRuta(tmp / "respaldos"); op.versionSync = "prueba";
	const std::string sha0 = sha256::deArchivo(deRuta(ranura));
	// Con el juego abierto: nada cambia
	op.juegoAbierto = [] { return true; };
	auto rA = lmweb::aplicarEnPartida(ps[0], L, ctx, op);
	CHECK(rA.error.rfind("JUEGO_ABIERTO", 0) == 0 && sha256::deArchivo(deRuta(ranura)) == sha0 && !fs::exists(tmp / "estado"));
	// Con el juego cerrado: escribe, respalda y guarda el estado
	op.juegoAbierto = [] { return false; };
	auto rB = lmweb::aplicarEnPartida(ps[0], L, ctx, op);
	if (!rB.error.empty()) std::printf("  error: %s\n", rB.error.c_str());
	CHECK(rB.error.empty() && rB.escrito && rB.versionAplicada == 9 && !rB.respaldo.empty());
	CHECK(sha256::deArchivo(deRuta(ranura)) == rB.sha256Despues && rB.sha256Despues != sha0);
	CHECK(sha256::deArchivo(deRuta(ranura) + ".phoenix-antes") == sha0 && !fs::exists(deRuta(ranura) + ".phoenix-nuevo"));
	auto releida = lm::GuardadoLM::abrir(deRuta(ranura));
	CHECK(releida.ok() && releida.valor->datos() == g.datos());   // igual que en memoria
	CHECK(lmweb::leerEstado(op.carpetaEstado, ps[0]).versionAplicada == 9);
	// La partida sigue reconociéndose igual después de escribirla
	auto ps2 = lmweb::buscarPartidas({ deRuta(tmp / "save") });
	CHECK(ps2.size() == 1 && ps2[0].id == partida);
	// Otra vez: no reescribe
	auto rC = lmweb::aplicarEnPartida(ps[0], L, ctx, op);
	CHECK(rC.error.empty() && !rC.escrito && sha256::deArchivo(deRuta(ranura)) == rB.sha256Despues);
	// Informe para la web
	auto iw = lmweb::informeParaWeb({ rB });
	CHECK(iw["formato"] == "phoenix-lm/aplicado-1" && iw["partidas"].size() == 1 && iw["partidas"][0]["version_aplicada"] == 9 && iw["partidas"][0]["resultados"].size() == 8);
	// El estado guardado vuelve a leerse con el diario completo (para deshacer después de cerrar Sync)
	auto e3 = lmweb::leerEstado(op.carpetaEstado, ps[0]);
	CHECK(e3.diario.contains("1") && e3.diario.contains("4") && e3.diario.contains("5") && e3.pendientes.size() == 1);

	// Vuelta completa con la web simulada, en DOS páginas (como cuando hay más de 500 cambios), sobre otra copia limpia
	fs::create_directories(tmp / "save2", ec);
	fs::copy_file(aRuta(rutaMl), tmp / "save2" / "ML0000000B", ec);
	std::vector<int64_t> pedidos;
	auto web = [&](int64_t desde) -> Resultado<std::string> {
		pedidos.push_back(desde);
		json pag = json::array();
		for (const auto& c : cambios) if (c["version"].get<int64_t>() > desde && pag.size() < 5) pag.push_back(c);
		json j = lista(pag, desde); j["version_actual"] = 9;
		return Resultado<std::string>::bien(j.dump());
	};
	lmweb::OpcionesEscritura op2 = op; op2.carpetaEstado = deRuta(tmp / "estado2");
	auto vu = lmweb::vuelta({ deRuta(tmp / "save2") }, web, ctx, op2);
	CHECK(vu.ok() && vu.valor->partidas.size() == 1 && vu.valor->partidas[0].escrito && vu.valor->partidas[0].versionAplicada == 9);
	CHECK(pedidos.size() == 2 && pedidos[0] == 0 && pedidos[1] == 5);
	CHECK(vu.ok() && sha256::deArchivo(deRuta(tmp / "save2" / "ML0000000B")) == rB.sha256Despues);   // mismo resultado que de una vez
	pedidos.clear();
	auto vu2 = lmweb::vuelta({ deRuta(tmp / "save2") }, web, ctx, op2);   // segunda vuelta: pide desde 9 y no escribe
	CHECK(vu2.ok() && pedidos.size() == 1 && pedidos[0] == 9 && !vu2.valor->partidas[0].escrito);
	fs::remove_all(tmp, ec);
}

int main() {
	std::setvbuf(stdout, nullptr, _IONBF, 0);   // si algo revienta, que se vea hasta dónde llegó
	sinArchivos();
	if (const char* m = std::getenv("PM_ML")) conPartida(m);
	else std::printf("(PM_ML no definido: pruebas con partida saltadas)\n");
	std::printf("\n%d/%d pruebas OK\n", total - fallos, total);
	return fallos == 0 ? 0 : 1;
}
