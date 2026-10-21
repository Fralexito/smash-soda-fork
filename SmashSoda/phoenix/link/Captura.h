#pragma once
// Traspasos en vivo · FASE 1 · CAPTURA (solo lectura). Docs: docs/FICHAJES-Y-SYNC.md («TRASPASOS-EN-VIVO»)
// Lee el option file de PES 2021 (EDIT00000000, la base "Editar"), saca quien esta en la plantilla de cada equipo y compara
// dos fotos: un jugador que SALE de un equipo y ENTRA en otro = traspaso. Todo lo demas (altas, bajas, convocatorias,
// dorsales, tacticas, Liga Master) se cuenta pero se IGNORA.
// El descifrado usa libpesXcrypter (dominio publico) y la distribucion del option file que verifico Phoenix Sync
// (PhoenixSync/core/OptionFile.cpp, posiciones de 4ccEditor y comprobadas con un option file real del ConmeGOL 26).
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace phoenix::traspasos {

namespace fs = std::filesystem;

/// Quien esta en cada plantilla en un momento dado (lo que dice el option file).
struct Foto {
	std::map<uint32_t, std::vector<uint32_t>> plantillas;   ///< team_id -> player_id (sin huecos)
	std::map<uint32_t, std::string> equipos;                 ///< team_id -> nombre que lleva el option file
	std::string sha256;                                      ///< huella del archivo leido
};

/// Descifra y lee. No modifica nada. `error` explica por que no (archivo ilegible, otro juego, formato desconocido...).
bool leerFoto(const std::string& bytesCifrados, Foto& foto, std::string& error);
bool leerFotoArchivo(const fs::path& archivo, Foto& foto, std::string& error);

struct Movimiento {
	uint32_t jugador = 0, desde = 0, hasta = 0;
	std::string desdeNombre, hastaNombre;   ///< nombres del option file (el de la base real se anade despues)
};
struct Diferencia {
	std::vector<Movimiento> traspasos;   ///< sale de un equipo y entra en otro
	int altas = 0;                       ///< entra en un equipo sin salir de otro (convocatoria, jugador libre...): se ignora
	int bajas = 0;                       ///< sale de un equipo y no entra en otro: se ignora
	int raros = 0;                       ///< varios equipos a la vez: se ignora (no se adivina)
	int equiposComparados = 0;
};
/// Compara dos fotos (solo equipos que estan en las dos). Orden estable: por jugador.
Diferencia comparar(const Foto& antes, const Foto& despues);

} // namespace phoenix::traspasos
