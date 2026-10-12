// Herramienta (sin interfaz) para ver o aplicar el paquete de Luas de Phoenix en el juego REAL con el mismo código que el
// instalador de Link («Módulos del juego»): copia de sider.ini con sello antes de cada cambio, una línea por módulo,
// nada más del ini, y se niega a actuar si PES2021.exe o sider.exe están abiertos. Pensada para el parche 27 (2026-10-12),
// cuando el lanzador sustituyó el sider.ini de la raíz y se perdieron las líneas de Phoenix.
// OJO: el .exe NO puede llevar «instal» en el nombre (Windows pide elevación).
// Uso: phoenix_en_juego_real <carpeta del juego> <carpeta sider del paquete> estado|aplicar
#include "phoenix/link/ModulosJuego.h"
#include "phoenix/link/Entrega.h"

#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace phoenix::modulos;

static void mostrar(const fs::path& paquete, const fs::path& juego) {
	const Parche p = infoParche(juego);
	std::cout << "Modo actual (version_actual.txt): " << p.modoActual << "\n";
	for (const auto& m : p.modosRaiz) std::cout << "  usa el sider.ini de la raiz: " << m << "\n";
	for (const auto& m : p.modosPropios) std::cout << "  con sider.ini propio:       " << m << "\n";
	for (const auto& w : p.advertencias) std::cout << "  ADVERTENCIA: " << w << "\n";
	for (const auto& m : estado(paquete, juego)) {
		std::cout << m.archivo << "  v" << m.version << "  estado=" << m.estado << "  carga=" << (m.carga.empty() ? "?" : m.carga) << "\n";
		for (const auto& d : m.destinos)
			std::cout << "   - " << d.nombre << ": archivo=" << d.archivo << " igual=" << d.igual << " linea=" << d.linea << " comentada=" << d.comentada
				<< " perdida=" << d.perdida << " sinBase=" << d.sinBase << "\n";
	}
}

int main(int argc, char** argv) {
	if (argc < 4) { std::cerr << "uso: phoenix_en_juego_real <juego> <paquete> estado|aplicar\n"; return 2; }
	const fs::path juego = phoenix::entrega::deU8(argv[1]), paquete = phoenix::entrega::deU8(argv[2]);
	const std::string que = argv[3];
	mostrar(paquete, juego);
	if (que != "aplicar") return 0;
	const auto abiertos = procesosAbiertos();
	if (!abiertos.empty()) { std::cerr << "NO se toca nada: sigue abierto " << abiertos.front() << "\n"; return 3; }
	int fallos = 0;
	for (const char* a : { "phoenix_referee.lua", "phoenix.lua" }) {
		const Resultado r = instalar(paquete, juego, a);
		std::cout << "\n[" << a << "] " << (r.ok ? "OK" : "FALLO") << ": " << r.mensaje << "\n";
		for (const auto& c : r.respaldos) std::cout << "   copia: " << c << "\n";
		if (!r.ok) fallos++;
	}
	std::cout << "\n--- despues ---\n";
	mostrar(paquete, juego);
	return fallos ? 1 : 0;
}
