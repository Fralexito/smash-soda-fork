#pragma once
// Repartidor de datos: coloca los archivos que deja Phoenix Sync en %APPDATA%\Phoenix Mercado\entrega
// (Player.bin, PlayerAssignment.bin y/o EDIT00000000) en su sitio dentro del juego, de forma atomica y con respaldo.
// Player.bin y PlayerAssignment.bin van a cada carpeta Phoenix-DB\common\etc\pesdb (raiz + modos), todo o nada, con <nombre>.anterior.
// No fabrica nada: solo comprueba, coloca y avisa. Nunca crea carpetas del juego ni toca sider.ini.
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::entrega {

namespace fs = std::filesystem;

std::string aU8(const fs::path& p);
fs::path deU8(const std::string& s);

// SHA-256 en hexadecimal minusculas.
std::string sha256Hex(const void* datos, size_t n);
bool sha256Archivo(const fs::path& ruta, std::string& hex);

struct Archivo { std::string nombre, sha256; };
struct Pedido {
	int version = 0;
	std::string id, creadoEn, resumen;
	std::vector<Archivo> archivos;
};
bool leerPedido(const fs::path& entregaJson, Pedido& p, std::string& error);

struct Rutas {
	fs::path carpetaEntrega;     // %APPDATA%\Phoenix Mercado\entrega
	fs::path carpetaJuego;       // carpeta de PES2021.exe
	fs::path carpetaOptionFile;  // carpeta que contiene EDIT00000000 (vacia = no se encontro)
};

// Estado de la ultima entrega (ultima_link.json y deshacer_link.json dentro de la carpeta de entrega).
struct Ultima {
	std::string estado;   // "" (ninguna) | "colocada" | "rechazada" | "deshecha"
	std::string id, resumen, motivo, fecha;   // fecha: "AAAA-MM-DD HH:MM" (hora local de la PC)
	bool puedeDeshacer = false;
	std::string aviso;    // texto para el buzon del juego (solo al terminar una accion; no se guarda)
	std::vector<std::string> player;   // rutas de Player.bin colocados (UTF-8)
	std::string edit, editRespaldo;    // option file colocado y su respaldo
};

bool hayEntrega(const fs::path& carpetaEntrega);
Ultima leerUltima(const fs::path& carpetaEntrega);

// Procesa entrega.json si existe. Nunca lanza: devuelve la Ultima con estado colocada/rechazada.
Ultima entregar(const Rutas& r);
// Restaura el respaldo del option file y el Player.bin anterior de la ultima entrega colocada.
Ultima deshacer(const Rutas& r);

// Carpetas \pesdb donde colocar Player.bin, valido para cualquier parche:
// <juego>\SiderAddons y, si el parche trae un cambiador de modos (<juego>\<algo>\<modo>\SiderAddons, como ConmeGOL),
// cada modo que ya tenga Phoenix-DB. `omitidos` recibe los modos sin Phoenix-DB (se saltan). Nunca crea carpetas.
std::vector<fs::path> buscarCarpetasPesdb(const fs::path& carpetaJuego, std::string* omitidos = nullptr);
// Nombre legible del parche: el primer modo del cambiador (p. ej. "ConmeGOL Patch 26") o el nombre de la carpeta del juego.
std::string nombreParche(const fs::path& carpetaJuego);

// Busca <documentos>\KONAMI\eFootball PES 2021 SEASON UPDATE\<steamid>\save que tenga EDIT00000000
// (si hay varios, el modificado mas recientemente). Vacio si no hay.
fs::path buscarCarpetaOptionFile(const fs::path& documentos);

// Texto del buzon para cada caso (el resumen se recorta para que la instruccion no se corte).
std::string avisoColocada(const std::string& resumen);
std::string avisoRechazada(const std::string& motivo);
std::string avisoDeshecha();

} // namespace phoenix::entrega
