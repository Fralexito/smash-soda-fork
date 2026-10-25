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
	bool perdida = false;        ///< el .lua está pero la línea ya no: una actualización del parche la borró (hay copias phoenix de sider.ini o otro destino la tiene)
	bool sinBase = false;        ///< su sider.ini no tiene una base del parche reconocida: Phoenix-DB NO se pone (y no se exige)
};

/// Lo que se sabe del cambiador de parches (ConmeGOL Extras\<modo>\): quién usa qué sider.ini.
struct Parche {
	std::string modoActual;                  ///< version_actual.txt (último modo lanzado con el lanzador) o ""
	std::vector<std::string> modosRaiz;      ///< modos cuyo SiderAddons NO trae sider.ini (parche 27): usan el sider.ini de la raíz
	std::vector<std::string> modosPropios;   ///< modos con su propio sider.ini (parche 26): el lanzador lo copia sobre la raíz al iniciar
	std::vector<std::string> advertencias;   ///< cosas raras que el usuario debe saber (legible)
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
/// Modos del cambiador y modo actual (solo lectura). Con el lanzador del parche 27, «Iniciar» hace
/// robocopy "ConmeGol Extras\<modo>" sobre la raíz (/E /IS /IT, sin borrar): un modo sin sider.ini propio deja el de la raíz.
Parche infoParche(const fs::path& juego);
/// Procesos del juego que impiden tocar sider.ini / modules (PES2021.exe, sider.exe). Vacío si todo está cerrado.
std::vector<std::string> procesosAbiertos();
/// Estado de todos los módulos del paquete (y de phoenix.lua, solo lectura) en el juego.
std::vector<Modulo> estado(const fs::path& paquete, const fs::path& juego);
/// Instala o actualiza un módulo del paquete en todos los destinos. El que llama comprueba que PES está cerrado.
Resultado instalar(const fs::path& paquete, const fs::path& juego, const std::string& archivo);
/// Comenta su línea en sider.ini de todos los destinos (con copia previa). El .lua se queda.
Resultado quitar(const fs::path& paquete, const fs::path& juego, const std::string& archivo);
/// Desinstala: quita sus líneas de sider.ini (activas o comentadas; con phoenix.lua también la raíz Phoenix-DB)
/// en la raíz y en cada modo, con copia de sider.ini, y renombra el .lua a <archivo>.desinstalado-<fecha>.
/// No borra nada: se puede volver a instalar cuando se quiera.
Resultado desinstalar(const fs::path& paquete, const fs::path& juego, const std::string& archivo);
/// sider.ini sin las líneas lua.module de este archivo (activas o comentadas). `cambio` = si había alguna.
std::string quitarLineasModulo(const std::string& ini, const std::string& archivo, bool& cambio);
/// sider.ini sin la raíz cpk.root de Phoenix-DB (activa o comentada).
std::string quitarRaizPhoenixDB(const std::string& ini, bool& cambio);

// --- Lógica pura de sider.ini (se prueba sin Windows) --------------------------------------------
/// Devuelve el sider.ini con la línea del módulo activa. `cambio` = false si ya estaba. "" + error si no hay lista de módulos.
std::string activarLinea(const std::string& ini, const std::string& archivo, bool& cambio, std::string& error);
/// Devuelve el sider.ini con la línea del módulo comentada. `cambio` = false si no estaba activa.
std::string comentarLinea(const std::string& ini, const std::string& archivo, bool& cambio);
/// ¿Hay una línea activa / comentada para el módulo?
void buscarLinea(const std::string& ini, const std::string& archivo, bool& activa, bool& comentada);
/// Devuelve el sider.ini con  cpk.root = ".\livecpk\Phoenix-DB"  justo antes de la primera raíz activa para la que
/// tieneBase(ruta) es true (la que tiene common\etc\pesdb\Player.bin) o que es olmosjr23\Database; si no hay base, no se pone.
/// `cambio` = false si ya estaba activa. "" + error si no hay ninguna cpk.root.
std::string activarRaizPhoenixDB(const std::string& ini, const std::function<bool(const std::string&)>& tieneBase, bool& cambio, std::string& error);
/// ¿Este sider.ini tiene una base del parche reconocida (cpk.root activa con Player.bin u olmosjr23\Database)? Sin ella Phoenix-DB no va.
bool hayBaseParche(const std::string& ini, const std::function<bool(const std::string&)>& tieneBase);
/// ¿Hay una cpk.root activa a Phoenix-DB?
bool tieneRaizPhoenixDB(const std::string& ini);

/// Raíces cpk.root ACTIVAS del sider.ini, en orden (rutas tal cual, p. ej. ".\livecpk\Faces").
std::vector<std::string> raicesActivas(const std::string& ini);
/// Carpetas de los modos de un cambiador de parches (con o sin sider.ini propio): <juego>\<carpeta>\<modo>.
std::vector<fs::path> carpetasModos(const fs::path& juego);
/// Como activarRaizPhoenixDB, pero si el sider.ini no tiene base reconocida la pone antes de la primera cpk.root activa
/// (para los fichajes generados sobre la base del parche: docs/FICHAJES-Y-SYNC.md («FICHAJES-COMO-CAMBIOS»)). "" + error si no hay ninguna cpk.root.
std::string activarRaizPhoenixDBSiempre(const std::string& ini, const std::function<bool(const std::string&)>& tieneBase, bool& cambio, std::string& error);
/// Asegura cpk.root = ".\livecpk\Phoenix-DB" en el sider.ini de cada carpeta SiderAddons dada (una línea, con copia previa
/// sider.ini.phoenix-<sello> y vuelta atrás si algo falla). El que llama comprueba que PES está cerrado.
Resultado asegurarRaizPhoenixDB(const std::vector<fs::path>& carpetasSider);

/// Mandos 9–16 (phoenix_mandos_fase1.lua): luajit.ext.enabled = 1, overlay.enabled = 1, cpk.root PhoenixMandos la primera
/// y su lua.module el último. `cambio` = false si ya estaba todo.
std::string prepararIniMandos(const std::string& ini, bool& cambio);
/// Pone la línea lua.module activa de `archivo` detrás de todas las demás (si no lo estaba ya).
std::string moverModuloAlFinal(const std::string& ini, const std::string& archivo, bool& cambio);
/// Quita la cpk.root de PhoenixMandos (activa o comentada) / ¿está activa?
std::string quitarRaizMandos(const std::string& ini, bool& cambio);
bool tieneRaizMandos(const std::string& ini);

/// Versión declarada en el .lua (`version = "x"`) o "".
std::string versionDe(const std::string& lua);

} // namespace phoenix::modulos
