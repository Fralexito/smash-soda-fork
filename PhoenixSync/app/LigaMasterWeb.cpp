// =============================================================================
//  Phoenix Sync · Liga Máster desde la web (127) — una vuelta en este PC
// -----------------------------------------------------------------------------
//  PhoenixSync lm-sincronizar [--saves <carpeta save>] [--cambios <sobre.json>] [--sin-informe]
//    1. busca las partidas de Liga Máster (ML0000000N) en las carpetas save,
//    2. baja GET /liga/cambios?ambito=pc (FIRMADO: sin firma válida no se escribe nada),
//    3. con PES CERRADO aplica en cada partida con respaldo (core/PartidasLM.h),
//    4. informa a la web: POST /liga/aplicado-pc (historial por partida + registro de PCs, destino «lm»).
//       Si la web no contesta, el informe queda en lm\informe-pendiente.json y se manda en la vuelta siguiente.
//  sync-vigilar la llama sola cada 2 minutos.
// =============================================================================

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "../core/ClienteMercado.h"
#include "../core/ClienteSync.h"
#include "../core/Firma.h"
#include "../core/PartidasLM.h"
#include "../core/RutasJuego.h"
#include "../core/Sincronizacion.h"
#include "../windows/Plataforma.h"

using namespace mercado;

namespace {
	std::string leerTodo(const std::string& ruta) {
		std::ifstream f(aRuta(ruta), std::ios::binary);
		return f ? std::string(std::istreambuf_iterator<char>(f), {}) : std::string();
	}
}

int vueltaLigaMaster(ClienteMercado& api, const std::vector<std::string>& savesPedidos, const std::string& rutaSobre, bool informar, bool callado) {
	std::vector<std::string> saves = savesPedidos;
	if (saves.empty()) for (const auto& s : rutas::buscarSaves(windows::carpetasDocumentos())) saves.push_back(s.ruta);
	if (saves.empty()) { if (!callado) std::printf("No se encontró ninguna carpeta save de PES 2021.\n"); return 0; }

	const std::string carpetaLm = windows::carpetaDatos() + "lm";
	const std::string pendiente = carpetaLm + "\\informe-pendiente.json";
	std::error_code ec;
	std::filesystem::create_directories(aRuta(carpetaLm), ec);

	// Lo que no se pudo informar la otra vez, primero.
	if (informar && std::filesystem::exists(aRuta(pendiente), ec)) {
		if (api.ligaAplicadoPc(leerTodo(pendiente)).ok()) std::filesystem::remove(aRuta(pendiente), ec);
	}

	auto contenidoDesde = [&](int64_t desde) -> Resultado<std::string> {
		std::string sobre;
		if (!rutaSobre.empty()) {
			sobre = leerTodo(rutaSobre);
			if (sobre.empty()) return Resultado<std::string>::mal("ARCHIVO_ERROR", "No se pudo leer " + rutaSobre);
		}
		else {
			auto r = api.ligaCambiosPcFirmados(desde);
			if (!r.ok()) return r;
			sobre = *r.valor;
		}
		return abrirSobreFirmado(sobre, sinc::claveWeb());   // sin firma válida, no hay contenido
	};

	lmweb::Contexto ctx;   // stats de LM: quedan pendientes hasta que se conecte el motor de estadísticas (126)
	lmweb::OpcionesEscritura op;
	op.carpetaEstado = carpetaLm;
	op.raizRespaldos = windows::carpetaDatos() + "respaldos";
	op.versionSync = sync::kVersionSync;
	op.juegoAbierto = [] { return windows::juegoAbierto(); };

	auto v = lmweb::vuelta(saves, contenidoDesde, ctx, op);
	if (!v.ok()) { std::printf("Liga Máster: ERROR %s · %s\n", v.error.codigo.c_str(), v.error.detalle.c_str()); return 2; }
	for (const auto& a : v.valor->avisos) if (!callado) std::printf("  (no se toca) %s\n", a.c_str());
	bool hayNovedad = false;
	for (const auto& p : v.valor->partidas) {
		const bool algo = !p.informe.resultados.empty() || !p.error.empty();
		hayNovedad = hayNovedad || algo;
		if (callado && !algo) continue;
		std::printf("Liga Máster %s · %s: %s", p.partida.ranura.c_str(), p.partida.clubNombre.c_str(), p.informe.texto().c_str());
		if (!p.error.empty()) std::printf("  ⚠ %s\n", p.error.c_str());
		if (p.escrito) std::printf("  Guardada (respaldo %s). Ábrela en el juego para verla.\n", p.respaldo.c_str());
	}
	if (!callado && v.valor->partidas.empty()) std::printf("No hay partidas de Liga Máster en este PC.\n");
	if (informar && !v.valor->partidas.empty() && (!callado || hayNovedad)) {
		const std::string cuerpo = v.valor->informe.dump();
		auto r = api.ligaAplicadoPc(cuerpo);
		if (!r.ok()) {
			// Se suma a lo que ya estaba pendiente (la web ignora los resultados repetidos). Máx. 20 partidas por informe.
			nlohmann::json j = v.valor->informe;
			auto antes = nlohmann::json::parse(leerTodo(pendiente), nullptr, false);
			if (antes.is_object() && antes.contains("partidas") && antes["partidas"].is_array())
				for (const auto& p : antes["partidas"]) if (j["partidas"].size() < 20) j["partidas"].push_back(p);
			std::ofstream(aRuta(pendiente), std::ios::binary | std::ios::trunc) << j.dump();
			std::printf("No se pudo informar a la web (%s): se manda en la próxima vuelta.\n", r.error.codigo.c_str());
		}
	}
	return 0;
}
