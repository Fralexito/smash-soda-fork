// Prueba MANUAL de phoenix::perfilespes::vivo::Conexion contra un PES 2021 ABIERTO. SOLO LEE (no escribe nada).
// Muestra el Usuario de cada mando y los 32 nombres de la tabla. Uso: usuarios_vivo_test.exe
#include "phoenix/core/UsuariosEnVivo.h"

#include <iostream>
#include <string>
#include <vector>

using namespace phoenix::perfilespes::vivo;

int main() {
	Conexion c;
	std::string err;
	if (!c.abrir(err)) { std::cerr << "NO CONECTA: " << err << "\n"; return 1; }
	std::cout << "Conectado. Tabla de Usuarios en 0x" << std::hex << c.direccionTabla() << std::dec << "\n";
	int m[kMandos];
	if (!c.leerMandos(m, err)) { std::cerr << "FALLA leerMandos: " << err << "\n"; return 1; }
	for (int i = 0; i < kMandos; i++) {
		std::vector<uint8_t> reg;
		if (!c.leerUsuario(m[i], reg, err)) { std::cerr << "FALLA leerUsuario: " << err << "\n"; return 1; }
		std::string nombre(reinterpret_cast<const char*>(&reg[4]));
		std::cout << "mando " << (i + 1) << " -> Usuario " << (m[i] + 1) << " [" << nombre << "]\n";
	}
	int conNombre = 0;
	for (int u = 0; u < 32; u++) {
		std::vector<uint8_t> reg;
		if (!c.leerUsuario(u, reg, err)) { std::cerr << "FALLA leerUsuario " << u << ": " << err << "\n"; return 1; }
		if (reg[4]) conNombre++;
	}
	std::cout << "Usuarios con nombre: " << conNombre << " de 32\nTODO OK (solo lectura)\n";
	return 0;
}
