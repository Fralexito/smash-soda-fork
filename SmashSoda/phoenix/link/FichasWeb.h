#pragma once
// Cambios «stats» e «info» de la web (docs/CONTRATO-WEB-PES.md §2 y §3c) → lista de link/Fichas, y resultados para el informe del PC (§4).
// Lógica pura (sin red ni disco): la usa ServicioFichas y se prueba en pruebas-link/fichas_test.cpp.
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "Fichas.h"

namespace phoenix::fichas::web {

constexpr long long kIdSintetico = 90000000;   // ids ≥ esto = «no es un id real, aplícalo por nombre» (contrato §2)

struct CambioWeb {
	long long seq = 0;
	std::string id, tipo, destino, estado;
	nlohmann::json datos;    ///< { jugador_id?, jugador, valores:{campo: n|null}, antes?, deshace?, lm? }
};

/// Lo que el PC recuerda de la web (se guarda en %APPDATA%\Phoenix Mercado\fichas\cambios-web.json).
struct Almacen {
	long long ultimoSeq = 0;
	std::map<long long, CambioWeb> cambios;   ///< solo stats/info activos con destino base o ambos
};

/// Una página de GET /v1/cambios → añade al almacén los stats/info para «base». Devuelve cuántos llegaron en la página (todos los tipos).
/// error ≠ "" si la respuesta no es la esperada (no se toca el almacén).
int leerPagina(const std::string& respuesta, Almacen& a, std::string& error);
std::string guardar(const Almacen& a);
bool leer(const std::string& texto, Almacen& a);

/// Lista efectiva: en orden de seq, el último valor de cada (jugador, campo) gana; null = «vuelve al de la base» (se quita).
Lista listaDesde(const Almacen& a);

/// Un resultado por cambio de la web (contrato §4): aplicado | ya_estaba | parcial | saltado | reemplazado | pendiente.
struct ResultadoCambio { std::string cambioId, resultado, motivo; int jugadores = 0; };
/// `analisis` = lo que dejó aplicarAlJuego / analizar. `pendiente` = aún no se pudo escribir (PES abierto, sin juego…).
std::vector<ResultadoCambio> resultados(const Almacen& a, const std::vector<Analisis>& analisis, bool pendiente, const std::string& motivoPendiente = "");

} // namespace phoenix::fichas::web
