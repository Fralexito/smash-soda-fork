#pragma once
// ESTADÍSTICAS E INFORMACIÓN de jugadores como CAMBIOS sobre el Player.bin de la base (migración web 126).
// Igual que los fichajes (link/Fichajes.h): Link NO copia bases de nadie. Lee el Player.bin REAL que ve el juego en cada sitio
// (raíz y cada modo del cambiador), le aplica una lista corta «jugador · campo · valor» y deja el resultado en
// <SiderAddons>\livecpk\Phoenix-DB\common\etc\pesdb\Player.bin. Como siempre se parte de la base, quitar un cambio de la lista
// = el jugador vuelve a su valor original (eso es el «deshacer»). Todo o nada, copia de seguridad antes, juego cerrado.
// Formato del registro (motor de PES 2021, vale para cualquier parche): base-conocimiento/16-PLAYER-BIN-FICHA-COMPLETA.md y
// 01-MOTOR §J de PhoenixSync; el editor es el de PhoenixSync/core/FichasBase portado a Link (mismos bits, mismas claves).
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "Fichajes.h"

namespace phoenix::fichas {

namespace fs = std::filesystem;

constexpr int kVersionLista = 1;

// ─── Campos (lógica pura) ────────────────────────────────────────────────────────────────────────
struct Campo {
	const char* clave;    ///< estable: la usan la web y la lista (igual que PhoenixSync)
	const char* texto;    ///< como se ve en el juego
	const char* grupo;    ///< campo | portero | pie_malo | estado | posicion | basico
	int bit, bits;        ///< posición y ancho en el registro de 312 B (contando desde 8 bytes antes del id)
	int sumar;            ///< valor en pantalla = guardado + sumar
	int minimo, maximo;   ///< rango permitido (valores de pantalla)
};
const std::vector<Campo>& campos();
const Campo* campo(const std::string& clave);   ///< nullptr si no existe

/// Player.bin en memoria (bytes reales, ya descomprimidos): leer y cambiar campos. Solo cambian los bits del campo.
class Ficha {
public:
	/// Acepta los dos tamaños reales de PES 2021 (último registro entero o recortado 8 bytes).
	bool cargar(const std::string& crudo, std::string& error);
	bool existe(uint32_t jugador) const { return _indice.count(jugador) > 0; }
	size_t jugadores() const { return _indice.size(); }
	/// Valor en pantalla. false si el jugador o el campo no existen.
	bool leer(uint32_t jugador, const std::string& clave, int& valor) const;
	/// codigo: "" (bien) | campo_desconocido | fuera_de_rango | jugador_no_existe
	std::string poner(uint32_t jugador, const std::string& clave, int valor);
	/// Nombre plegado (fichajes::plegarNombre) del jugador, "" si no hay.
	std::string nombrePlegado(uint32_t jugador) const;
	/// Ids cuyo nombre plegado es exactamente `plegado`.
	std::vector<uint32_t> buscarNombre(const std::string& plegado) const;
	const std::string& crudo() const { return _crudo; }
	/// Ids con algún byte distinto respecto de otra ficha del mismo tamaño.
	std::vector<uint32_t> distintos(const Ficha& otra) const;
private:
	std::string _crudo;
	std::map<uint32_t, size_t> _indice;   // id → inicio del registro (8 bytes antes del id)
	std::map<std::string, std::vector<uint32_t>> _porNombre;
};

// ─── Lista de cambios ────────────────────────────────────────────────────────────────────────────
struct Cambio {
	std::string id;          ///< id del cambio en la web (para el informe), puede ir vacío
	uint32_t jugador = 0;    ///< player_id (0 = por nombre)
	std::string nombre;      ///< nombre del jugador: si va junto al id, se COMPRUEBA (otro parche puede reusar el id); si va solo, se busca
	std::string campo;       ///< clave de campos()
	int valor = 0;           ///< valor en pantalla
};
struct Lista {
	int version = kVersionLista;
	std::vector<Cambio> cambios;
	std::string fuente, sha256;
};
/// fichas.json: { "version":1, "cambios":[ {"id":"…","player_id":162114,"jugador":"Lamine Yamal","campo":"velocidad","valor":95} ] }
/// Si hay dos cambios del mismo jugador y campo, gana el último.
bool leerLista(const std::string& json, Lista& lista, std::string& error);

struct Hecho {
	Cambio cambio;
	uint32_t jugador = 0;   ///< id resuelto en esta base
	int antes = 0;          ///< valor de la base (para el historial de la web)
	bool igual = false;     ///< la base ya tenía ese valor
};
struct Salto {
	Cambio cambio;
	std::string codigo;     ///< campo_desconocido | fuera_de_rango | jugador_no_existe | jugador_distinto | jugador_no_encontrado | jugador_ambiguo
	std::string motivo;
};
struct Resultado {
	std::string crudo;       ///< Player.bin resultante (bytes reales)
	std::vector<Hecho> aplicados;
	std::vector<Salto> saltados;
	int cambiados = 0;       ///< aplicados que cambian algo respecto de la base
	std::string error;       ///< la comprobación final falló (no se debe escribir nada)
};
/// Aplica la lista sobre una ficha de la base. Comprueba al final que solo cambiaron los jugadores pedidos.
Resultado aplicar(const Ficha& base, const Lista& lista);

// ─── Juego real ──────────────────────────────────────────────────────────────────────────────────
struct Analisis {
	fichajes::Objetivo objetivo;
	bool baseOk = false;
	std::string parche, baseOrigen, baseSha, error, queHacer;
	std::vector<Hecho> hechos;
	std::vector<Salto> saltados;
	int cambiados = 0;
	std::string salida;      ///< Player.bin nuevo (.bin de PES) — «» si no hay nada que escribir
	std::string salidaSha;   ///< sha256 de los bytes reales del resultado
	std::string estado;      ///< sin_base | nada | sin_aplicar | aplicado | desfasado | cambiado
};
std::vector<Analisis> analizar(const fs::path& juego, const Lista& lista);

struct Informe {
	bool ok = false;
	std::string mensaje;
	std::vector<Analisis> objetivos;
	std::vector<std::string> respaldos;
	int escritos = 0, retirados = 0;
};
/// Genera Player.bin en Phoenix-DB de cada sitio con base (todo o nada), con copia antes; si la lista ya no cambia nada, retira
/// lo que generó Link. Asegura la raíz Phoenix-DB en sider.ini. Juego cerrado.
Informe aplicarAlJuego(const fs::path& juego, const Lista& lista);
/// Quita el Player.bin que generó Link (solo si nadie lo cambió): el juego vuelve a las stats de la base. La copia se queda.
Informe quitarDelJuego(const fs::path& juego);

/// %APPDATA%\Phoenix Mercado\fichas (fichas.json, lo escribe el servicio que baja los cambios de la web).
fs::path carpetaListaUsuario();
bool cargarLista(const fs::path& carpeta, Lista& lista, std::string& error);

} // namespace phoenix::fichas
