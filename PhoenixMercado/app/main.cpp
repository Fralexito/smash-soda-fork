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
#include <fstream>
#include "../windows/Plataforma.h"

using namespace mercado;

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
	if (a.empty()) { std::printf("Uso: eco | yo | vincular <codigo> | option | bajar <carpeta> | hash <archivo> | copia <archivo> <carpeta> | catalogo <EDIT> <cpk> <json> | mover <EDIT> <id> <equipo> <salida> [--manager]\n"); return 1; }

	try {
		windows::HttpWinHttp http;
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
		else { std::printf("Comando no reconocido.\n"); return 1; }
	}
	catch (const std::exception& e) { std::printf("ERROR inesperado: %s\n", e.what()); return 3; }
	return 0;
}
