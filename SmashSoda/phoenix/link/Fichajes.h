#pragma once
// Fichajes de Phoenix como CAMBIOS (docs/FICHAJES-COMO-CAMBIOS.md).
// Link NO copia bases de nadie: detecta SOLO, por evidencia, la base REAL que ve el juego en cada sitio (raíz del juego y cada
// modo de un cambiador de parches), le aplica una lista corta de cambios «jugador → equipo» y deja el resultado en
// <SiderAddons>\livecpk\Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin. Solo cambia PlayerAssignment.bin; Player.bin queda
// el de la base. Todo o nada, con copia de seguridad, solo con el juego cerrado.
// La base se valida SIEMPRE por estructura (cabecera WESYS, zlib, filas de 16 bytes), no por nombres de parche:
// si no se lee o el formato es otro, no se crea nada.
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace phoenix::fichajes {

namespace fs = std::filesystem;

constexpr int kMaxPorEquipo = 40;          ///< ninguna base real tiene un equipo con más de 40 jugadores
constexpr int kVersionLista = 2;           ///< la 1 (ids) y la 2 (ids o nombres) se entienden

// ─── Formatos (lógica pura, se prueba sin el juego) ──────────────────────────────────────────────
/// Descomprime un flujo zlib (inflate propio, sin dependencias). `maximo` limita el tamaño de salida.
bool inflarZlib(const std::string& zlib, std::string& salida, size_t maximo, std::string& error);
/// zlib sin compresión (bloques «stored» + adler32): válido para cualquier lector zlib.
std::string zlibAlmacenado(const std::string& crudo);
/// .bin de PES (cabecera FF 10 81 'WESYS' + tamaño comprimido + tamaño real + zlib) → bytes reales.
bool leerBin(const std::string& archivo, std::string& crudo, std::string& error);
/// bytes reales → .bin de PES.
std::string escribirBin(const std::string& crudo);

/// Archivo guardado SIN comprimir dentro de un CPK (tabla @UTF enmascarada).
struct EntradaCpk { std::string carpeta, nombre; uint64_t desde = 0, tam = 0, tamReal = 0; };
using LectorCpk = std::function<bool(uint64_t desde, size_t cuanto, std::string& salida)>;
/// Lee solo la cabecera y la tabla del CPK (no el archivo entero) y devuelve las entradas de `carpeta`.
bool entradasCpk(const LectorCpk& leer, uint64_t tamArchivo, const std::string& carpeta, std::vector<EntradaCpk>& entradas, std::string& error);
/// Extrae un archivo SIN comprimir de un CPK ya en memoria. `carpeta` = "common/etc/pesdb".
bool extraerDeCpk(const std::string& cpk, const std::string& carpeta, const std::string& nombre, std::string& bytes, std::string& error);
/// DpFileList.bin de PES: nombre del CPK (en minúsculas) → prioridad (1 = la más alta).
std::map<std::string, int> leerDpFileList(const std::string& bytes);

/// Una fila de PlayerAssignment.bin (16 bytes, 4 × u32 little endian).
struct Fila {
	uint32_t id = 0;        ///< id de fila (editor)
	uint32_t jugador = 0;   ///< player_id
	uint32_t equipo = 0;    ///< team_id
	uint32_t empaq = 0;     ///< (puesto en la plantilla << 10) | número bajo (10 bits)
};
bool leerAsignacion(const std::string& crudo, std::vector<Fila>& filas, std::string& error);
std::string escribirAsignacion(const std::vector<Fila>& filas);
/// Player.bin: ids de jugador (cabecera de 8 bytes + huecos de 312 bytes; id = primeros 4 bytes; 0 = hueco).
/// `nombres` (opcional) recibe id → nombre completo plegado (ver plegarNombre).
bool leerJugadores(const std::string& crudo, std::set<uint32_t>& ids, std::string& error, std::map<uint32_t, std::string>* nombres = nullptr);
/// Team.bin: registros de 1532 bytes (id al principio, nombre a +368). id → nombre plegado.
bool leerEquipos(const std::string& crudo, std::map<uint32_t, std::string>& nombres, std::string& error);
/// Nombre sin mayúsculas, acentos, espacios ni signos (acepta UTF-8 o Latin-1): para comparar nombres de forma exacta pero tolerante.
std::string plegarNombre(const std::string& texto);

/// Nombres de la base de un PC (para las listas de la versión 2, que se refieren a jugadores y equipos por nombre).
struct Nombres {
	std::map<uint32_t, std::string> jugador;   ///< player_id → nombre plegado
	std::map<uint32_t, std::string> equipo;    ///< team_id → nombre plegado
};

// ─── Lista de cambios ────────────────────────────────────────────────────────────────────────────
struct Cambio {
	uint32_t jugador = 0;   ///< player_id (0 = se indica por nombre)
	uint32_t equipo = 0;    ///< team_id destino (0 = por nombre)
	uint32_t origen = 0;    ///< team_id donde se espera que esté (0 = no se indica)
	std::string jugadorNombre, equipoNombre, origenNombre;   ///< versión 2: nombres, resueltos contra la base de cada PC
	std::string nota;
};
struct Lista {
	int version = 1;
	bool estricta = false;  ///< false = «relajada»: gana el cambio de Fralex aunque el origen sea otro
	std::string nombre;
	std::vector<Cambio> cambios;
	std::string fuente;     ///< de dónde se leyó (para mostrar)
	std::string sha256;     ///< huella del texto leído
	bool usaNombres() const { for (const Cambio& c : cambios) if (c.jugador == 0 || c.equipo == 0 || (c.origen == 0 && !c.origenNombre.empty())) return true; return false; }
};
/// fichajes.json, versión 1: { "version":1, "regla":"relajada"|"estricta", "cambios":[{"player_id":N,"team_id":N,"origen":N?,"nota":"…"}] }
/// versión 2 (compatible): cada cambio puede ir por ids o por nombres:
///   {"jugador":"Nombre exacto","equipo_origen":"Nombre","equipo_destino":"Nombre"} (el origen es opcional; si el nombre del
///   jugador no es único, el equipo de origen lo desambigua; si sigue sin ser único, se salta).
bool leerLista(const std::string& json, Lista& lista, std::string& error);

// ─── Aplicar en memoria ──────────────────────────────────────────────────────────────────────────
struct Salto {
	Cambio cambio;
	std::string codigo;   ///< jugador_no_existe | equipo_no_existe | origen_cambiado | ambiguo | equipo_lleno | nombres_no_disponibles | jugador_no_encontrado | jugador_ambiguo | equipo_no_encontrado | equipo_ambiguo
	std::string motivo;   ///< legible
};
struct Hecho {
	Cambio cambio;
	std::string que;      ///< movido | alta | movido_origen_distinto
};
struct Resultado {
	std::vector<Fila> filas;
	std::vector<Hecho> aplicados;
	std::vector<Salto> saltados;
	int yaEstaban = 0;
};
/// Aplica la lista sobre las filas de la base. Un equipo es válido si la base le da al menos un jugador.
/// `nombres` solo hace falta si la lista usa nombres (versión 2).
Resultado aplicar(const std::vector<Fila>& base, const std::set<uint32_t>& jugadores, const Lista& lista, const Nombres* nombres = nullptr);
/// Comprueba el resultado completo (formato exacto): ids de fila únicos, sin pares (jugador, equipo) repetidos,
/// puesto único por equipo y como mucho kMaxPorEquipo por equipo en los equipos que cambiaron.
bool validar(const std::vector<Fila>& base, const std::vector<Fila>& nuevas, std::string& error);

// ─── Juego real ──────────────────────────────────────────────────────────────────────────────────
/// Un sitio donde puede ir Phoenix-DB: la raíz del juego o un modo del cambiador de parches.
struct Objetivo {
	std::string nombre;       ///< "Raíz del juego" o el nombre del modo
	fs::path siderAddons;     ///< <…>\SiderAddons
	fs::path cpk;             ///< su CGP_database.cpk (o el nombre habitual; se buscan todos los .cpk de su carpeta download)
	fs::path ini;             ///< sider.ini que manda ahí (el suyo o el de la raíz)
	bool esRaiz = false;
};
std::vector<Objetivo> objetivos(const fs::path& juego);

struct Analisis {
	Objetivo objetivo;
	bool baseOk = false;
	std::string parche;                    ///< «ConmeGOL Patch 27», «PES 2021 original», «parche desconocido»…
	bool baseGenerica = false;             ///< no se reconoce por nombre ni huella: se usa porque se lee y valida por estructura
	std::string baseOrigen;                ///< dónde está la base que ve el juego (carpeta o CPK)
	std::vector<std::string> comoLoSupe;   ///< la evidencia, en claro
	std::string error;                     ///< por qué la base no se reconoce («qué detecté»)
	std::string queHacer;                  ///< «qué hacer» cuando no hay base
	int filasBase = 0;
	bool nombresDisponibles = false;       ///< se pudieron leer los nombres de jugadores y equipos de la base
	int aplicados = 0, yaEstaban = 0;
	std::vector<Hecho> hechos;
	std::vector<Salto> saltados;
	std::string salida;       ///< PlayerAssignment.bin nuevo («» si no hay nada que escribir)
	std::string salidaSha;
	std::string baseSha;
	std::string estado;       ///< sin_base | nada | sin_aplicar | aplicado | desfasado | cambiado
	bool conPlayerBinAjeno = false;   ///< Phoenix-DB tiene un Player.bin distinto al de la base (taparía a los jugadores del parche)
};
/// Solo lee. Calcula qué pasaría en cada sitio.
std::vector<Analisis> analizar(const fs::path& juego, const Lista& lista);

/// Nombres de la base REAL de un modo (solo lectura), para poner nombre de jugador/equipo a un traspaso.
struct BaseNombres {
	bool ok = false;
	std::string parche, baseSha, error;
	Nombres plegados;                                  ///< para comparar (plegarNombre)
	std::map<uint32_t, std::string> jugadorLegible;    ///< player_id → nombre completo en UTF-8
	std::map<uint32_t, std::string> equipoLegible;     ///< team_id → nombre del equipo en UTF-8
};
/// `modo` = nombre del objetivo («Raíz del juego» o el modo); "" = el primer sitio con base reconocida.
BaseNombres leerNombresDeBase(const fs::path& juego, const std::string& modo);

struct Informe {
	bool ok = false;
	std::string mensaje;
	std::vector<Analisis> objetivos;
	std::vector<std::string> respaldos;
	int escritos = 0;
};
/// Genera Phoenix-DB en cada sitio con base reconocida (todo o nada) y asegura su raíz en sider.ini. Juego cerrado.
Informe aplicarAlJuego(const fs::path& juego, const Lista& lista);
/// Quita lo que generó Link (solo si no lo cambió otra herramienta). La copia de seguridad se queda.
Informe quitarDelJuego(const fs::path& juego);

/// Lista de cambios: primero la del usuario (%APPDATA%\Phoenix Mercado\fichajes\fichajes.json: punto de entrada para
/// Sync/web), si no, la que lleva Link (<exe>\fichajes\fichajes.json). Si ninguna existe: lista vacía.
bool cargarLista(const fs::path& usuario, const fs::path& paquete, Lista& lista, std::string& error);
fs::path carpetaListaUsuario();
fs::path carpetaListaPaquete();

} // namespace phoenix::fichajes
