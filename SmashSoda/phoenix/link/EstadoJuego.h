#pragma once
// Estado del partido que escribe phoenix.lua (Sider) en <juego>\SiderAddons\content\phoenix\estado.json
// (cada ~1 s y solo durante el partido). Phoenix Link solo LEE este archivo: nunca lo crea ni lo borra.
// Si no existe, no se entiende o tiene mas de 5 s, se considera «sin datos del juego» y todo sigue en manual.
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::juego {

constexpr double kMaxEdadSeg = 5.0;     ///< mas viejo que esto = sin datos
constexpr size_t kMaxBytes = 8192;      ///< nunca se lee mas que esto (40 goles con goleador caben de sobra)

struct GolJuego {
	int minuto = 0;
	bool local = true;            ///< true = gol del equipo local
	std::string jugador;          ///< nombre de la camiseta del goleador (vacío = no se sabe: Sider 7.3.3)
	uint32_t idJugador = 0;       ///< id del jugador en el juego (0 = no se sabe)
	bool propia = false;          ///< lo marcó un jugador del otro equipo (en propia puerta)
};

struct EstadoPartidoJuego {
	bool valido = false;          ///< hay datos frescos y entendibles
	std::string fase;             ///< menu | en_juego | pausado | descanso | final
	int minuto = 0, periodo = 0;
	int segundo = 0, anadido = 0;
	bool relojCorre = true;       ///< el reloj del juego avanza (false = repetición, celebración, pausa...)
	int local = 0, visita = 0;    ///< IDs de equipo (0 = sin dato)
	std::string nombreLocal, nombreVisita;   ///< nombres del juego (vacío si no se saben)
	int golesLocal = -1, golesVisita = -1;   ///< -1 = sin dato
	int pkLocal = -1, pkVisita = -1;         ///< penales (-1 = sin dato)
	std::vector<GolJuego> goles;  ///< en orden, con el minuto del juego
	bool completo = false;        ///< fase final: el partido llegó al final (no se abandonó)
	long long seq = 0;
	double edadSeg = -1.0;        ///< antiguedad del archivo al leerlo (-1 = no existe)
};

// Entiende el texto del archivo. No toca `valido` ni `edadSeg`. false = formato desconocido.
bool parsear(const std::string& texto, EstadoPartidoJuego& e);
// Lee <carpetaBuzon>\estado.json con permisos compartidos (el juego puede reemplazarlo mientras tanto).
EstadoPartidoJuego leer(const std::filesystem::path& carpetaBuzon);

// Pantalla del juego (phoenix_referee.lua v1.2+, archivo pantalla.json, cada 2 s TAMBIÉN fuera del partido).
// Va aparte de estado.json a propósito: así el árbitro (modo competitivo, abandono…) sigue igual que antes.
// Solo sirve para mostrar en la web «eligiendo equipos», «saliendo al campo», etc.
constexpr double kMaxEdadPantallaSeg = 6.0;
struct PantallaJuego {
	bool valida = false;
	std::string pantalla;         ///< menu | equipos | ajustes | cargando | saliendo | en_juego | descanso | pausa | final
};
bool parsearPantalla(const std::string& texto, PantallaJuego& p);
PantallaJuego leerPantalla(const std::filesystem::path& carpetaBuzon);

} // namespace phoenix::juego
