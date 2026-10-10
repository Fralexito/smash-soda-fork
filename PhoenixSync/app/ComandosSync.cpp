// =============================================================================
//  Phoenix Sync · Comandos de «option file compartido» (consola, etapa 1)
// -----------------------------------------------------------------------------
//  PhoenixSync sync-grupo <id>             elige el grupo de esta PC
//  PhoenixSync sync-juego <carpeta>        carpeta del juego (para Player.bin y nombre del parche)
//  PhoenixSync sync-estado                 modo (lo decide el admin en la web), pendientes, historial
//  PhoenixSync sync-vigilar [minutos]      deja corriendo el motor (una vuelta cada 3 s)
//  PhoenixSync sync-traer                  «Traer cambios del grupo» ahora
//  PhoenixSync sync-aplicar [id]           «Aplicar» (todas las pendientes o una)
//  PhoenixSync sync-rechazar <id>
//  PhoenixSync sync-deshacer               «Deshacer último cambio del grupo»
//  PhoenixSync sync-publicar si|no         interruptor local «publicar mis fichajes»
//  PhoenixSync sync-pausa si|no            pausa todo en esta PC
//  PhoenixSync sync-respaldar              «Respaldar ahora»
//  PhoenixSync sync-respaldos              lista de respaldos
//  PhoenixSync sync-restaurar <nombre> [--aunque-juego-abierto]
// =============================================================================

#include <windows.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <thread>

#include "../core/RutasJuego.h"
#include "../core/SyncCompartido.h"
#include "../windows/Plataforma.h"

using namespace mercado;

namespace {
	std::string rutaJuegoTxt() { return windows::carpetaDatos() + "sync\\juego.txt"; }
	std::string leerJuego() {
		std::ifstream f(aRuta(rutaJuegoTxt()), std::ios::binary);
		std::string s; std::getline(f, s);
		while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
		return s;
	}
	void err(const Error& e) { std::printf("ERROR %s%s%s\n", e.codigo.c_str(), e.detalle.empty() ? "" : " · ", e.detalle.c_str()); }
	void avisos(sync::Motor& m) { for (const auto& a : m.tomarAvisos()) std::printf("· %s\n", a.c_str()); }
}

/// Devuelve -1 si `a[0]` no es un comando sync-…; si no, el código de salida.
int comandoSync(const std::vector<std::string>& a, ClienteHttp& http, FuenteToken& token) {
	if (a.empty() || a[0].rfind("sync-", 0) != 0) return -1;
	const std::string& c = a[0];

	if (c == "sync-juego" && a.size() >= 2) {
		std::error_code ec;
		if (!std::filesystem::is_directory(aRuta(a[1]), ec)) { std::printf("No existe la carpeta: %s\n", a[1].c_str()); return 1; }
		std::filesystem::create_directories(aRuta(windows::carpetaDatos() + "sync"), ec);
		std::ofstream(aRuta(rutaJuegoTxt()), std::ios::binary | std::ios::trunc) << a[1];
		std::printf("Carpeta del juego guardada. Parche detectado: %s\n", rutas::nombreParche(a[1]).c_str());
		return 0;
	}

	sync::ClienteSync web(http, token, [](const std::string& n, const std::string& t) {
		if (n != "info") std::fprintf(stderr, "[%s] %s\n", n.c_str(), t.c_str());
	});
	sync::Entorno en;
	en.documentos = [] { return windows::carpetasDocumentos(); };
	en.carpetaJuego = [] { return leerJuego(); };
	en.carpetaDatos = windows::carpetaDatos();
	en.juegoAbierto = [] { return windows::juegoAbierto(); };
	en.nombrePc = windows::nombrePc();
	sync::AccesoOptionReal option;
	sync::Motor m(web, option, en, [](const std::string& n, const std::string& t) { std::fprintf(stderr, "[%s] %s\n", n.c_str(), t.c_str()); });
	if (auto r = m.cargar(); !r.ok()) { err(r.error); return 2; }

	if (c == "sync-grupo" && a.size() >= 2) { m.elegirGrupo(a[1]); m.guardar(); std::printf("Grupo: %s\n", a[1].c_str()); }
	else if (c == "sync-estado") {
		const auto s = rutas::saveActivo(en.documentos());
		std::printf("Option file activo: %s\n", s ? s->ruta.c_str() : "(no encontrado)");
		std::printf("Carpeta del juego: %s\n", leerJuego().empty() ? "(sin elegir: usa sync-juego)" : leerJuego().c_str());
		std::printf("%s\n", m.resumen().dump(2).c_str());
	}
	else if (c == "sync-vigilar") {
		const int minutos = a.size() >= 2 ? std::max(1, std::atoi(a[1].c_str())) : 60;
		std::printf("Vigilando %d min (Ctrl+C para salir)…\n", minutos);
		const auto fin = std::chrono::steady_clock::now() + std::chrono::minutes(minutos);
		while (std::chrono::steady_clock::now() < fin) {
			m.ciclo();
			avisos(m);
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}
	}
	else if (c == "sync-traer") {
		m.ciclo();
		auto r = m.traerAhora();
		avisos(m);
		if (!r.ok() && r.error.codigo != "ESPERA") { err(r.error); return 2; }
		std::printf("%s\n", m.resumen()["entrantes"].dump(2).c_str());
	}
	else if (c == "sync-aplicar") {
		std::vector<std::string> ids; if (a.size() >= 2) ids.push_back(a[1]);
		auto r = m.aplicar(ids);
		avisos(m);
		if (!r.ok()) { err(r.error); return 2; }
		std::printf("Aplicados: %d (Phoenix Link los coloca y avisa en el juego)\n", *r.valor);
	}
	else if (c == "sync-rechazar" && a.size() >= 2) { auto r = m.rechazar(a[1]); if (!r.ok()) { err(r.error); return 2; } std::printf("Rechazado.\n"); }
	else if (c == "sync-deshacer") { auto r = m.deshacerUltimo(); if (!r.ok()) { err(r.error); return 2; } std::printf("Listo: Phoenix Link vuelve a poner el option file anterior.\n"); }
	else if ((c == "sync-publicar" || c == "sync-pausa") && a.size() >= 2) {
		const bool si = a[1] == "si" || a[1] == "sí" || a[1] == "1";
		if (c == "sync-publicar") m.ponerPublicar(si); else m.ponerPausa(si);
		m.guardar();
		std::printf("%s: %s\n", c == "sync-publicar" ? "Publicar mis fichajes" : "Pausa", si ? "sí" : "no");
	}
	else if (c == "sync-respaldar") {
		auto r = m.respaldarAhora("manual");
		if (!r.ok()) { err(r.error); return 2; }
		std::printf("Respaldo %s · %zu archivos · %lld bytes\n", r.valor->nombre.c_str(), r.valor->archivos.size(), r.valor->bytes);
		for (const auto& o : r.valor->omitidos) std::printf("  (saltado) %s\n", o.c_str());
	}
	else if (c == "sync-respaldos") {
		for (const auto& r : m.respaldos()) std::printf("%s · %s · %zu archivos · %lld bytes\n", r.nombre.c_str(), r.motivo.c_str(), r.archivos.size(), r.bytes);
	}
	else if (c == "sync-restaurar" && a.size() >= 2) {
		const bool forzar = a.size() >= 3 && a[2] == "--aunque-juego-abierto";
		auto r = m.restaurar(a[1], forzar);
		if (!r.ok()) { err(r.error); return 2; }
		std::printf("Restaurado (%d archivos). Antes se guardó %s.\nEn el juego: Partido → Datos Actual. en vivo → Activar (o cierra y abre el juego).\n",
			r.valor->colocados, r.valor->respaldoPrevio.nombre.c_str());
		for (const auto& s : r.valor->saltados) std::printf("  (saltado) %s\n", s.c_str());
	}
	else { std::printf("Comando sync no reconocido.\n"); return 1; }
	return 0;
}
