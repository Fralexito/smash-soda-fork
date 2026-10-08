// =============================================================================
//  Phoenix Mercado · App de prueba (consola)
// -----------------------------------------------------------------------------
//  PhoenixMercado eco
//  PhoenixMercado yo                     [--manager]
//  PhoenixMercado vincular ABCD2345      (solo modo manager)
//  PhoenixMercado option                 [--manager]   muestra versión/huella oficial
//  PhoenixMercado bajar <carpeta>        [--manager]   baja y verifica el oficial
//  PhoenixMercado hash <archivo>
//  PhoenixMercado copia <archivo> <carpetaCopias>
//  PhoenixMercado catalogo <EDIT> <base.cpk> <salida.json> [nombreParche]
//  PhoenixMercado verificar <EDIT> <base.cpk>            (modo seguro: ¿se puede escribir?)
//  PhoenixMercado emparejar <EDIT ref> <cpk ref> <EDIT local> <cpk local> <informe.json>
//  PhoenixMercado subir-catalogo <EDIT> <cpk> <nombreParche>                    (staff, por lotes)
//  PhoenixMercado subir-equivalencias <EDIT ref> <cpk ref> <EDIT local> <cpk local> <perfil> [--referencia]
//  PhoenixMercado mover <EDIT> <pes_id> <equipoDestino> <salidaNueva>   (nunca sobrescribe)
//
//  Modo de token: por defecto «compartido» (el de Phoenix Link).
//  --manager usa el token propio de Mercado (segundo código).
// =============================================================================

#include <windows.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "../core/ClienteMercado.h"
#include "../core/Copias.h"
#include "../core/Sha256.h"
#include "../core/OptionFile.h"
#include "../core/BaseDatosParche.h"
#include "../core/Catalogo.h"
#include "../core/Emparejamiento.h"
#include "../core/Integridad.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>
#include "../windows/Plataforma.h"

using namespace mercado;

static std::string cmd0(const std::vector<std::string>& a) { return a.empty() ? std::string() : a[0]; }

static void imprimirError(const Error& e) {
	std::printf("ERROR %s%s%s\n", e.codigo.c_str(), e.detalle.empty() ? "" : " · ", e.detalle.c_str());
	if (e.codigo == "TOKEN_INVALIDO")
		std::printf("Pista: vuelve a vincular esta PC.\n");
	if (e.codigo == "CODIGO_MANAGER_REQUERIDO")
		std::printf("Tu cuenta exige el código manager: usa --manager y «vincular».\n");
}

int main(int argc, char** argv) {
	SetConsoleOutputCP(CP_UTF8);
	std::vector<std::string> a(argv + 1, argv + argc);
	bool manager = false;
	for (auto it = a.begin(); it != a.end();) {
		if (*it == "--manager") { manager = true; it = a.erase(it); } else ++it;
	}
	if (a.empty()) { std::printf("Uso: eco | yo | vincular <codigo> | option | bajar <carpeta> | hash <archivo> | copia <archivo> <carpeta> | catalogo <EDIT> <cpk> <json> | mover <EDIT> <id> <equipo> <salida> | verificar <EDIT> <cpk> | emparejar <EDITref> <cpkRef> <EDIT> <cpk> <json> [--manager]\n"); return 1; }

	try {
		const bool lento = cmd0(a) == "subir-catalogo" || cmd0(a) == "subir-equivalencias";
		windows::HttpWinHttp http(lento ? 180000 : 8000);
		windows::TokenCompartido tCompartido;
		windows::TokenManager tManager;
		FuenteToken& token = manager ? static_cast<FuenteToken&>(tManager) : tCompartido;
		ClienteMercado api(http, token, [](const std::string& n, const std::string& t) {
			std::fprintf(stderr, "[%s] %s\n", n.c_str(), t.c_str());
		});

		const std::string& cmd = a[0];
		if (cmd == "eco") {
			auto r = api.eco();
			if (!r.ok()) { imprimirError(r.error); return 2; }
			std::printf("OK · API %s · hora del servidor %s\n", r.valor->versionApi.c_str(), r.valor->hora.c_str());
		}
		else if (cmd == "vincular" && a.size() >= 2) {
			if (!manager) { std::printf("«vincular» es del modo manager: añade --manager.\n"); return 1; }
			auto r = api.vincular(a[1], windows::nombrePc());
			if (!r.ok()) { imprimirError(r.error); return 2; }
			std::printf("PC vinculada a %s (token guardado cifrado).\n", r.valor->nombre.c_str());
		}
		else if (cmd == "yo") {
			auto r = api.yo();
			if (!r.ok()) { imprimirError(r.error); return 2; }
			std::printf("Conectado como %s (modo %s)\n", r.valor->nombre.c_str(), token.nombreModo().c_str());
		}
		else if (cmd == "option" || (cmd == "bajar" && a.size() >= 2)) {
			auto r = api.optionActual();
			if (!r.ok()) { imprimirError(r.error); return 2; }
			std::printf("Option oficial %s · %lld bytes · sha256 %s\n", r.valor->version.c_str(), r.valor->tamano, r.valor->sha256.c_str());
			if (cmd == "bajar") {
				const std::string destino = deRuta(aRuta(a[1]) / ("EDIT00000000.oficial-" + r.valor->version));
				auto d = api.descargarOption(*r.valor, destino);
				if (!d.ok()) { imprimirError(d.error); return 2; }
				std::printf("Descargado y verificado: %s\n", destino.c_str());
			}
		}
		else if (cmd == "hash" && a.size() >= 2) {
			std::string err;
			const std::string h = sha256::deArchivo(a[1], &err);
			if (h.empty()) { std::printf("ERROR %s\n", err.c_str()); return 2; }
			std::printf("%s  %s\n", h.c_str(), a[1].c_str());
		}
		else if (cmd == "copia" && a.size() >= 3) {
			auto r = copias::crear(a[1], a[2]);
			if (!r.ok()) { imprimirError(r.error); return 2; }
			std::printf("Copia creada y verificada: %s\n", r.valor->ruta.c_str());
		}
		else if (cmd == "catalogo" && a.size() >= 4) {
			auto of = OptionFile::abrir(a[1]);
			if (!of.ok()) { imprimirError(of.error); return 2; }
			auto base = leerBaseDatos(a[2]);
			if (!base.ok()) { imprimirError(base.error); return 2; }
			auto cat = construirCatalogo(*of.valor, *base.valor, a.size() >= 5 ? a[4] : "sin nombre");
			if (std::filesystem::exists(aRuta(a[3]))) { std::printf("ERROR DESTINO_OCUPADO %s\n", a[3].c_str()); return 2; }
			std::ofstream(aRuta(a[3]), std::ios::binary) << cat.json;
			std::printf("Catálogo: %d equipos, %d jugadores (%d sin datos) → %s\n", cat.equipos, cat.jugadores, cat.sinDatos, a[3].c_str());
		}
		else if (cmd == "mover" && a.size() >= 5) {
			auto of = OptionFile::abrir(a[1]);
			if (!of.ok()) { imprimirError(of.error); return 2; }
			auto m = of.valor->mover(static_cast<uint32_t>(std::stoul(a[2])), static_cast<uint32_t>(std::stoul(a[3])));
			if (!m.ok()) { imprimirError(m.error); return 2; }
			auto g = of.valor->guardarComo(a[4]);
			if (!g.ok()) { imprimirError(g.error); return 2; }
			std::printf("Guardado y verificado: %s (sha256 %s)\n", a[4].c_str(), g.valor->c_str());
		}
		else if (cmd == "verificar" && a.size() >= 3) {
			auto of = OptionFile::abrir(a[1]);
			if (!of.ok()) { imprimirError(of.error); return 2; }
			auto base = leerBaseDatos(a[2]);
			if (!base.ok()) { imprimirError(base.error); return 2; }
			auto an = revisarEstructura(*of.valor, *base.valor);
			for (auto& x : an) std::printf("[%s] %s %s\n", x.gravedad.c_str(), x.codigo.c_str(), x.detalle.c_str());
			std::printf(hayBloqueo(an) ? "MODO SEGURO: no se escribirá nada en este option file.\n" : "OK: se puede trabajar con este option file.\n");
			return hayBloqueo(an) ? 3 : 0;
		}
		else if (cmd == "emparejar" && a.size() >= 6) {
			auto ofR = OptionFile::abrir(a[1]); auto bR = leerBaseDatos(a[2]);
			auto ofL = OptionFile::abrir(a[3]); auto bL = leerBaseDatos(a[4]);
			for (const Error* e : { &ofR.error, &bR.error, &ofL.error, &bL.error })
				if (!e->codigo.empty()) { imprimirError(*e); return 2; }
			if (hayBloqueo(revisarEstructura(*ofL.valor, *bL.valor))) { std::printf("MODO SEGURO: el parche local no pasa la verificación.\n"); return 3; }
			std::vector<JugadorReferencia> refs; std::vector<ClubReferencia> clubes;
			for (const auto& t : ofR.valor->equipos()) {
				const auto& pl = ofR.valor->plantillas().at(t.id);
				if (pl.empty()) continue;
				ClubReferencia c{ t.id, t.id, t.nombre, {} };
				for (const auto& p : pl) {
					c.jugadores.push_back(p.jugador);
					auto f = bR.valor->find(p.jugador);
					if (f != bR.valor->end())
						refs.push_back({ p.jugador, p.jugador, f->second.nombre, f->second.nacionalidad, f->second.altura, f->second.edad, f->second.posicion });
				}
				clubes.push_back(c);
			}
			std::sort(refs.begin(), refs.end(), [](auto& x, auto& y) { return x.phoenixId < y.phoenixId; });
			refs.erase(std::unique(refs.begin(), refs.end(), [](auto& x, auto& y) { return x.phoenixId == y.phoenixId; }), refs.end());
			Emparejador emp(*bL.valor);
			auto rj = emp.emparejarTodos(refs);
			auto rc = emp.emparejarClubes(clubes, rj, *ofL.valor);
			if (std::filesystem::exists(aRuta(a[5]))) { std::printf("ERROR DESTINO_OCUPADO %s\n", a[5].c_str()); return 2; }
			std::ofstream(aRuta(a[5]), std::ios::binary) << informeJson(rj, rc, "local");
			int n[3] = {}, m[3] = {};
			for (auto& r : rj) n[int(r.estado)]++;
			for (auto& r : rc) m[int(r.estado)]++;
			std::printf("Jugadores: %d automáticos, %d a revisar, %d sin candidato\nClubes: %d automáticos, %d a revisar, %d sin candidato\nInforme: %s\n",
				n[0], n[1], n[2], m[0], m[1], m[2], a[5].c_str());
		}
		else if (cmd == "subir-catalogo" && a.size() >= 4) {
			auto of = OptionFile::abrir(a[1]); auto base = leerBaseDatos(a[2]);
			if (!of.ok()) { imprimirError(of.error); return 2; }
			if (!base.ok()) { imprimirError(base.error); return 2; }
			if (hayBloqueo(revisarEstructura(*of.valor, *base.valor))) { std::printf("MODO SEGURO: option y base no coinciden; no se sube nada.\n"); return 3; }
			auto cat = construirCatalogo(*of.valor, *base.valor, a[3]);
			auto lotes = lotesCatalogo(cat.json, 3000);
			std::printf("Catálogo: %d equipos, %d jugadores en %zu lotes\n", cat.equipos, cat.jugadores, lotes.size());
			for (size_t i = 0; i < lotes.size(); i++) {
				auto r = api.subirCatalogo(lotes[i]);
				if (!r.ok()) { std::printf("Lote %zu/%zu: ", i + 1, lotes.size()); imprimirError(r.error); return 2; }
				std::printf("Lote %zu/%zu OK: %s\n", i + 1, lotes.size(), r.valor->c_str());
			}
		}
		else if (cmd == "subir-equivalencias" && a.size() >= 6) {
			const bool referencia = std::find(a.begin(), a.end(), std::string("--referencia")) != a.end();
			const std::string perfil = a[5];
			auto ofR = OptionFile::abrir(a[1]); auto bR = leerBaseDatos(a[2]);
			auto ofL = OptionFile::abrir(a[3]); auto bL = leerBaseDatos(a[4]);
			for (const Error* e : { &ofR.error, &bR.error, &ofL.error, &bL.error })
				if (!e->codigo.empty()) { imprimirError(*e); return 2; }
			if (hayBloqueo(revisarEstructura(*ofL.valor, *bL.valor))) { std::printf("MODO SEGURO: el parche local no pasa la verificación.\n"); return 3; }
			auto pl = api.plantillas();
			if (!pl.ok()) { imprimirError(pl.error); return 2; }
			const auto contenido = nlohmann::json::parse(*pl.valor, nullptr, false);
			if (contenido.is_discarded()) { std::printf("ERROR plantillas ilegibles\n"); return 2; }
			std::map<uint32_t, const JugadorEditado*> editR;
			for (const auto& e : ofR.valor->editados()) editR[e.id] = &e;
			std::vector<JugadorReferencia> refs; int sinFicha = 0;
			for (const auto& c : contenido.value("clubes", nlohmann::json::array()))
				for (const auto& j : c.value("jugadores", nlohmann::json::array())) {
					const int64_t phx = j.value("phoenix_id", 0LL); const uint32_t pes = j.value("pes_id", 0u);
					if (!phx || !pes) continue;
					if (auto e = editR.find(pes); e != editR.end())
						refs.push_back({ phx, pes, e->second->nombre, e->second->nacionalidad, e->second->altura, e->second->edad, e->second->posicion });
					else if (auto f = bR.valor->find(pes); f != bR.valor->end())
						refs.push_back({ phx, pes, f->second.nombre, f->second.nacionalidad, f->second.altura, f->second.edad, f->second.posicion });
					else sinFicha++;
				}
			std::sort(refs.begin(), refs.end(), [](auto& x, auto& y) { return x.phoenixId < y.phoenixId; });
			refs.erase(std::unique(refs.begin(), refs.end(), [](auto& x, auto& y) { return x.phoenixId == y.phoenixId; }), refs.end());
			std::vector<ResultadoJugador> res;
			if (referencia) {
				// Parche de referencia: el ID local ES el pes_id del catálogo (se comprueba que exista).
				for (const auto& r : refs) {
					ResultadoJugador x; x.phoenixId = r.phoenixId;
					if (bL.valor->count(r.pesIdReferencia)) { x.estado = EstadoEmparejamiento::Automatico; x.idLocal = r.pesIdReferencia; x.metodo = "referencia"; x.motivo = "Parche de referencia del catálogo"; x.candidatos.push_back({ r.pesIdReferencia, 100, r.nombre }); }
					else { x.estado = EstadoEmparejamiento::SinCandidato; x.motivo = "No está en la base local"; }
					res.push_back(x);
				}
			}
			else res = Emparejador(*bL.valor).emparejarTodos(refs);
			int n[3] = {};
			for (auto& r : res) n[int(r.estado)]++;
			std::printf("Phoenix IDs con ficha: %zu (sin ficha en la referencia: %d) · automáticos %d, revisar %d, sin candidato %d\n",
				refs.size(), sinFicha, n[0], n[1], n[2]);
			for (size_t i = 0; i < res.size(); i += 3000) {
				std::vector<ResultadoJugador> lote(res.begin() + i, res.begin() + std::min(res.size(), i + 3000));
				auto r = api.subirEquivalencias(informeJson(lote, {}, perfil));
				if (!r.ok()) { std::printf("Lote %zu: ", i / 3000 + 1); imprimirError(r.error); return 2; }
				std::printf("Lote %zu OK: %s\n", i / 3000 + 1, r.valor->c_str());
			}
		}
		else { std::printf("Comando no reconocido.\n"); return 1; }
	}
	catch (const std::exception& e) { std::printf("ERROR inesperado: %s\n", e.what()); return 3; }
	return 0;
}
