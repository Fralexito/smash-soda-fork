// =============================================================================
//  Phoenix Sync · Herramienta de línea de comandos: mover jugadores en el
//  OPTION FILE (amistosos / Editar → Cargar), encadenando varios movimientos
//  en un solo archivo de salida. Usa el motor ya probado (core/OptionFile.*).
//
//  Uso:
//    mover_cli <EDIT entrada> <EDIT salida> --mover <pid> <equipoDestino> [posicion] [--mover ...]
//    mover_cli <EDIT entrada> --buscar <pid>          (solo informa en qué equipo está)
//
//  `posicion` (opcional, 0-12, ver LigaMaster.h). Si no se da, -1.
//  Nunca sobrescribe: falla si <EDIT salida> ya existe.
// =============================================================================

#include "../core/OptionFile.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

using namespace mercado;

static void imprimirError(const char* etapa, const Error& e) {
	std::printf("ERROR [%s] %s%s%s\n", etapa, e.codigo.c_str(), e.detalle.empty() ? "" : " · ", e.detalle.c_str());
}

static std::string nombreEquipo(const OptionFile& of, uint32_t id) {
	for (auto& e : of.equipos()) if (e.id == id) return e.nombre;
	return "(desconocido)";
}

int main(int argc, char** argv) {
	std::vector<std::string> a(argv + 1, argv + argc);
	if (a.size() < 2) {
		std::printf("uso: mover_cli <EDIT entrada> <EDIT salida> --mover <pid> <equipoDestino> <pos> [...]\n");
		std::printf("     mover_cli <EDIT entrada> --buscar <pid> [<pid> ...]\n");
		return 1;
	}

	auto rAbrir = OptionFile::abrir(a[0]);
	if (!rAbrir.ok()) { imprimirError("abrir", rAbrir.error); return 2; }
	OptionFile of = std::move(*rAbrir.valor);

	if (a.size() >= 2 && a[1] == "--buscar") {
		for (size_t i = 2; i < a.size(); ++i) {
			uint32_t pid = (uint32_t)std::stoul(a[i]);
			std::printf("pid %u está en: ", pid);
			bool alguno = false;
			for (auto& [idEquipo, plantilla] : of.plantillas()) {
				for (auto& plaza : plantilla) {
					if (plaza.jugador == pid) {
						std::printf("equipo %u (%s, dorsal %u)  ", idEquipo, nombreEquipo(of, idEquipo).c_str(), plaza.dorsal);
						alguno = true;
					}
				}
			}
			if (!alguno) std::printf("(no está en ninguna plantilla)");
			std::printf("\n");
		}
		return 0;
	}

	const std::string rutaSalida = a[1];
	struct Mov { uint32_t pid; uint32_t destino; int pos; };
	std::vector<Mov> movs;
	for (size_t i = 2; i < a.size(); ++i) {
		if (a[i] == "--mover" && i + 3 < a.size()) {
			movs.push_back({ (uint32_t)std::stoul(a[i+1]), (uint32_t)std::stoul(a[i+2]), std::stoi(a[i+3]) });
			i += 3;
		}
	}
	std::map<uint32_t, int> posiciones;
	for (auto& m : movs) if (m.pos >= 0) posiciones[m.pid] = m.pos;
	auto posicionDe = [&](uint32_t pid) -> int {
		auto it = posiciones.find(pid);
		return it != posiciones.end() ? it->second : -1;
	};

	for (auto& m : movs) {
		std::printf("Moviendo %u a %u (%s)...\n", m.pid, m.destino, nombreEquipo(of, m.destino).c_str());
		// sugerirSustituto necesita el equipo de ORIGEN; lo buscamos primero.
		uint32_t equipoOrigen = 0;
		for (auto& [idEquipo, plantilla] : of.plantillas()) {
			if (idEquipo == m.destino) continue;
			for (auto& plaza : plantilla) if (plaza.jugador == m.pid) { equipoOrigen = idEquipo; break; }
			if (equipoOrigen) break;
		}
		if (!equipoOrigen) {
			std::printf("ERROR: %u no está en ninguna plantilla distinta de %u.\n", m.pid, m.destino);
			return 4;
		}
		std::printf("  (equipo de origen detectado: %u, %s)\n", equipoOrigen, nombreEquipo(of, equipoOrigen).c_str());
		auto rSust2 = of.sugerirSustituto(equipoOrigen, m.pid, posicionDe);
		uint32_t sustituto = rSust2.ok() ? *rSust2.valor : 0;
		if (!rSust2.ok()) std::printf("  Aviso: sugerirSustituto: %s (se intenta sin sustituto)\n", rSust2.error.detalle.c_str());
		else std::printf("  Sustituto propuesto: %u\n", sustituto);

		auto rMover = of.mover(m.pid, m.destino, equipoOrigen, 0, sustituto);
		if (!rMover.ok()) { imprimirError("mover", rMover.error); return 5; }
		std::printf("  OK.\n");
	}

	auto rGuardar = of.guardarComo(rutaSalida);
	if (!rGuardar.ok()) { imprimirError("guardarComo", rGuardar.error); return 7; }
	std::printf("Guardado: %s\n", rGuardar.valor->c_str());
	return 0;
}
