#pragma once
// Fichajes de Phoenix como CAMBIOS (docs/FICHAJES-COMO-CAMBIOS.md).
// Link NO copia bases de nadie: toma la base REAL de ese PC (la que usa el modo: olmosjr23\Database o el
// CGP_database.cpk del modo), le aplica una lista corta de cambios «jugador → equipo» y deja el resultado en
// <SiderAddons>\livecpk\Phoenix-DB\common\etc\pesdb\PlayerAssignment.bin. Solo cambia PlayerAssignment.bin;
// Player.bin queda el de la base. Todo o nada, con copia de seguridad, solo con el juego cerrado.
// Si la base no se reconoce (no se lee, formato raro) no se crea nada.
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace phoenix::fichajes {

namespace fs = std::filesystem;

constexpr int kMaxPorEquipo = 40;          ///< ninguna base real tiene un equipo con más de 40 jugadores
constexpr int kVersionLista = 1;

// ─── Formatos (lógica pura, se prueba sin el juego) ──────────────────────────────────────────────
/// Descomprime un flujo zlib (inflate propio, sin dependencias). `maximo` limita el tamaño de salida.
bool inflarZlib(const std::string& zlib, std::string& salida, size_t maximo, std::string& error);
/// zlib sin compresión (bloques «stored» + adler32): válido para cualquier lector zlib.
std::string zlibAlmacenado(const std::string& crudo);
/// .bin de PES (cabecera FF 10 81 'WESYS' + tamaño comprimido + tamaño real + zlib) → bytes reales.
bool leerBin(const std::string& archivo, std::string& crudo, std::string& error);
/// bytes reales → .bin de PES.
std::string escribirBin(const std::string& crudo);
/// Extrae un archivo SIN comprimir de un CPK (tabla @UTF enmascarada). `carpeta` = "common/etc/pesdb".
bool extraerDeCpk(const std::string& cpk, const std::string& carpeta, const std::string& nombre, std::string& bytes, std::string& error);

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
bool leerJugadores(const std::string& crudo, std::set<uint32_t>& ids, std::string& error);

// ─── Lista de cambios ────────────────────────────────────────────────────────────────────────────
struct Cambio {
	uint32_t jugador = 0;   ///< player_id
	uint32_t equipo = 0;    ///< team_id destino
	uint32_t origen = 0;    ///< team_id donde se espera que esté (0 = no se indica)
	std::string nota;
};
struct Lista {
	int version = kVersionLista;
	bool estricta = false;  ///< false = «relajada»: gana el cambio de Fralex aunque el origen sea otro
	std::string nombre;
	std::vector<Cambio> cambios;
	std::string fuente;     ///< de dónde se leyó (para mostrar)
	std::string sha256;     ///< huella del texto leído
};
/// fichajes.json: { "version":1, "regla":"relajada"|"estricta", "cambios":[{"player_id":N,"team_id":N,"origen":N?,"nota":"…"}] }
bool leerLista(const std::string& json, Lista& lista, std::string& error);

// ─── Aplicar en memoria ──────────────────────────────────────────────────────────────────────────
struct Salto {
	Cambio cambio;
	std::string codigo;   ///< jugador_no_existe | equipo_no_existe | origen_cambiado | ambiguo | equipo_lleno
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
Resultado aplicar(const std::vector<Fila>& base, const std::set<uint32_t>& jugadores, const Lista& lista);
/// Comprueba el resultado completo (formato exacto): ids de fila únicos, sin pares (jugador, equipo) repetidos,
/// puesto único por equipo y como mucho kMaxPorEquipo por equipo en los equipos que cambiaron.
bool validar(const std::vector<Fila>& base, const std::vector<Fila>& nuevas, std::string& error);

// ─── Juego real ──────────────────────────────────────────────────────────────────────────────────
/// Un sitio donde puede ir Phoenix-DB: la raíz del juego o un modo del cambiador de parches.
struct Objetivo {
	std::string nombre;       ///< "Raíz del juego" o el nombre del modo
	fs::path siderAddons;     ///< <…>\SiderAddons
	fs::path cpk;             ///< su CGP_database.cpk
	fs::path ini;             ///< sider.ini que manda ahí (el suyo o el de la raíz)
	bool esRaiz = false;
};
std::vector<Objetivo> objetivos(const fs::path& juego);

struct Analisis {
	Objetivo objetivo;
	bool baseOk = false;
	std::string baseOrigen;   ///< «olmosjr23\Database» | «CGP_database.cpk del modo» | …
	std::string error;        ///< por qué la base no se reconoce
	int filasBase = 0;
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
