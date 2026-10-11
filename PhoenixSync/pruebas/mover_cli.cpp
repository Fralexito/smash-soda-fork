// =============================================================================
//  Phoenix Sync · Herramienta de línea de comandos: mover jugadores en el
//  OPTION FILE (amistosos / Editar → Cargar), encadenando varios movimientos
//  en un solo archivo de salida. Usa el motor ya probado (core/OptionFile.*).
//
//  Uso:
//    mover_cli <EDIT entrada> <EDIT salida> --mover <pid> <equipoDestino> <equipoOrigen> [posicion] [--mover ...]
//    mover_cli <EDIT entrada> --buscar <pid>          (solo informa en qué equipo está)
//
//  `equipoOrigen` es OBLIGATORIO y explícito (nunca se adivina): un jugador
//  suele estar en dos plantillas a la vez (club + selección), y adivinar cuál
//  de las dos es "el origen" puede sacarlo del equipo equivocado. Usa primero
//  --buscar para ver los equipos exactos y sus IDs antes de mover a nadie.
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
	struct Mov { uint32_t pid; uint32_t destino; uint32_t origen; int pos; };
	std::vector<Mov> movs;
	for (size_t i = 2; i < a.size(); ++i) {
		if (a[i] == "--mover" && i + 4 < a.size()) {
			movs.push_back({ (uint32_t)std::stoul(a[i+1]), (uint32_t)std::stoul(a[i+2]), (uint32_t)std::stoul(a[i+3]), std::stoi(a[i+4]) });
			i += 4;
		}
	}
	std::map<uint32_t, int> posiciones;
	for (auto& m : movs) if (m.pos >= 0) posiciones[m.pid] = m.pos;
	auto posicionDe = [&](uint32_t pid) -> int {
		auto it = posiciones.find(pid);
		return it != posiciones.end() ? it->second : -1;
	};

	for (auto& m : movs) {
		std::printf("Moviendo %u del equipo %u (%s) a %u (%s)...\n", m.pid, m.origen, nombreEquipo(of, m.origen).c_str(), m.destino, nombreEquipo(of, m.destino).c_str());
		const uint32_t equipoOrigen = m.origen;
		// Comprobación de seguridad: el jugador debe estar REALMENTE en ese equipo de origen.
		bool enOrigen = false;
		for (auto& [idEquipo, plantilla] : of.plantillas()) {
			if (idEquipo != equipoOrigen) continue;
			for (auto& plaza : plantilla) if (plaza.jugador == m.pid) enOrigen = true;
		}
		if (!enOrigen) {
			std::printf("ERROR: %u no está en la plantilla del equipo %u (%s). Nada escrito.\n", m.pid, equipoOrigen, nombreEquipo(of, equipoOrigen).c_str());
			return 4;
		}
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
