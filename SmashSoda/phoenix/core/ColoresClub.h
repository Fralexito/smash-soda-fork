#pragma once

// =============================================================================
//  Phoenix Link · colores de los equipos del Campo 2D — lógica pura (sin Windows).
//  Prueba: pruebas-link/colores_test.cpp.
// -----------------------------------------------------------------------------
//  Cada fichita del campo se pinta con el color referente de su equipo (camiseta principal), sacado del NOMBRE del
//  equipo que da el juego (phoenix_referee: «FC Barcelona», «Argentina»…). Si el nombre no está en la tabla se usa el
//  color de siempre (cian / violeta). Reglas para que se vea bien sobre el césped y se distingan los dos equipos:
//    · un color demasiado oscuro (negro, azul marino) se cambia por el secundario o, si tampoco sirve, por un gris azulado;
//    · si los dos equipos quedan casi iguales, el segundo usa su color secundario y, si sigue parecido, el violeta.
//  El equipo A (el de la izquierda en el primer saque) se da por el LOCAL del juego; si no fuera así los colores van
//  cruzados (es un aviso conocido: el juego no dice de qué lado sale cada uno; ver docs/CAMPO-2D-Y-ARBITRO.md («CAMPO-2D»)).
// =============================================================================

#include <string>

namespace phoenix::campo {

constexpr const char* kColorPorDefectoA = "#00e5ff";   ///< cian
constexpr const char* kColorPorDefectoB = "#b07bff";   ///< violeta

struct ColorEquipo {
	std::string principal, secundario;   ///< «#rrggbb»
};

/// Busca el equipo por nombre (sin mayúsculas ni tildes, por trozos: «FC Barcelona», «barca», «Barcelona SC»…).
/// false = no está en la tabla.
bool colorDeEquipo(const std::string& nombre, ColorEquipo& salida);

struct ColoresPartido {
	std::string a, b;        ///< colores finales «#rrggbb» (nunca vacíos)
	bool conocidoA = false;  ///< el nombre de A estaba en la tabla
	bool conocidoB = false;
};

/// Colores de los dos equipos con las reglas de contraste. Nombres vacíos o desconocidos → colores por defecto.
ColoresPartido coloresDelPartido(const std::string& nombreA, const std::string& nombreB);

/// Utilidades (públicas para la prueba).
std::string normalizarNombre(const std::string& s);                 ///< minúsculas, sin tildes ni signos
bool hexValido(const std::string& s);                               ///< «#rrggbb»
double luminancia(const std::string& hex);                          ///< 0 (negro) … 1 (blanco)
double distanciaColor(const std::string& a, const std::string& b);  ///< 0 … 441 (distancia RGB)

}
