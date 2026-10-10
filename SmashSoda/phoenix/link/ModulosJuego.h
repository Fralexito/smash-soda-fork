#pragma once
// Instalador de módulos Lua de Sider (el «paquete de Luas» de Phoenix).
// Link lleva los módulos en <exe>\sider\*.lua y los instala en el juego SOLO con PES cerrado:
//   · copia el .lua a SiderAddons\modules\ en la raíz del juego y en cada modo de un cambiador (ConmeGOL Extras\<modo>\SiderAddons),
//     de forma atómica, y comprueba el sha256;
//   · guarda una copia de sider.ini (sider.ini.phoenix-AAAAMMDD-HHMMSS) y añade UNA línea  lua.module = "<archivo>"
//     debajo del último lua.module activo (o quita el ; si ya estaba comentada). Nada más del sider.ini cambia;
//   · «Quitar» comenta esa línea (;lua.module = …) con copia previa: el .lua se queda en modules\ (no se borra nada).
// Nunca crea carpetas, salvo para phoenix.lua (decisión de Fralex, 2026-10-10, para instalar Phoenix en la PC de un amigo):
//   crea content\phoenix\ (buzón de avisos) y livecpk\Phoenix-DB\common\etc\pesdb\ (vacías), añade además
//   cpk.root = ".\livecpk\Phoenix-DB" antes de la primera raíz que tenga la base del parche (informe 27 de Sync)
//   y guarda phoenix.lua.antes-<fecha> si había otra versión. Los módulos que Link no lleva solo se muestran.
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace phoenix::modulos {

namespace fs = std::filesystem;

struct Destino {
	std::string nombre;          ///< "Raíz del juego" o el nombre del modo (p. ej. "ConmeGOL Patch 26")
	std::string carpeta;         ///< ...\SiderAddons (UTF-8)
	bool archivo = false;        ///< el .lua está en modules\ de este destino
	bool igual = false;          ///< y es idéntico al que lleva Link (sha256)
	std::string version;         ///< versión del .lua instalado ("" si no se sabe)
	bool linea = false;          ///< lua.module activo en sider.ini
	bool comentada = false;      ///< hay una línea ;lua.module comentada
};

struct Modulo {
	std::string archivo;         ///< "phoenix_estadio.lua"
	std::string descripcion;
	bool gestionable = false;    ///< Link lo lleva y lo puede instalar o quitar
	std::string version;         ///< versión que lleva Link ("" si no lo lleva)
	std::string estado;          ///< no_instalado | instalado | desactualizado | apagado | a_medias | solo_lectura
	std::string carga;           ///< según el último sider.log: "" (no se sabe) | cargado | error
	std::vector<Destino> destinos;
};

struct Resultado {
	bool ok = false;
	std::string mensaje;         ///< legible, para la interfaz
	std::vector<std::string> respaldos;   ///< copias de sider.ini creadas (UTF-8)
};

/// Carpeta <exe>\sider con los módulos que lleva Link.
fs::path carpetaPaquete();
/// Carpetas SiderAddons con sider.ini: la raíz del juego y cada modo de un cambiador de parches.
std::vector<std::pair<std::string, fs::path>> destinos(const fs::path& juego);
/// Estado de todos los módulos del paquete (y de phoenix.lua, solo lectura) en el juego.
std::vector<Modulo> estado(const fs::path& paquete, const fs::path& juego);
/// Instala o actualiza un módulo del paquete en todos los destinos. El que llama comprueba que PES está cerrado.
Resultado instalar(const fs::path& paquete, const fs::path& juego, const std::string& archivo);
/// Comenta su línea en sider.ini de todos los destinos (con copia previa). El .lua se queda.
Resultado quitar(const fs::path& paquete, const fs::path& juego, const std::string& archivo);

// --- Lógica pura de sider.ini (se prueba sin Windows) --------------------------------------------
/// Devuelve el sider.ini con la línea del módulo activa. `cambio` = false si ya estaba. "" + error si no hay lista de módulos.
std::string activarLinea(const std::string& ini, const std::string& archivo, bool& cambio, std::string& error);
/// Devuelve el sider.ini con la línea del módulo comentada. `cambio` = false si no estaba activa.
std::string comentarLinea(const std::string& ini, const std::string& archivo, bool& cambio);
/// ¿Hay una línea activa / comentada para el módulo?
void buscarLinea(const std::string& ini, const std::string& archivo, bool& activa, bool& comentada);
/// Devuelve el sider.ini con  cpk.root = ".\livecpk\Phoenix-DB"  justo antes de la primera raíz activa para la que
/// tieneBase(ruta) es true (la que tiene common\etc\pesdb\Player.bin); si ninguna, antes de la primera cpk.root.
/// `cambio` = false si ya estaba activa. "" + error si no hay ninguna cpk.root.
std::string activarRaizPhoenixDB(const std::string& ini, const std::function<bool(const std::string&)>& tieneBase, bool& cambio, std::string& error);
/// ¿Hay una cpk.root activa a Phoenix-DB?
bool tieneRaizPhoenixDB(const std::string& ini);

/// Versión declarada en el .lua (`version = "x"`) o "".
std::string versionDe(const std::string& lua);

} // namespace phoenix::modulos
