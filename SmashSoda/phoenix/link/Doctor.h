#pragma once
// Phoenix Doctor: ficha del juego del anfitrión (solo lectura, desde Phoenix Link, sin tocar el juego).
// MODO INFORMATIVO (decisión de Fralex, 11-oct-2026): aún no hay parche oficial, así que NO aprueba ni rechaza nada.
// Solo describe y deja una huella: qué parche y modos hay, qué módulos Lua están activos, qué raíces cpk.root sirven
// la base, y el sha256 de los archivos que deciden los datos del partido (Player.bin, PlayerAssignment.bin,
// option file, sider.ini, Luas de Phoenix). La huella viaja con el resultado oficial (detalle.doctor) para el staff.
// Cuando exista el parche oficial: comparar esta ficha con la de referencia que publique la web (fase 2).
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::doctor {

namespace fs = std::filesystem;

struct Archivo {
	std::string nombre;     ///< p. ej. "Phoenix-DB/Player.bin (Raíz del juego)"
	std::string sha256;     ///< "" si no existe
	long long bytes = -1;
};

struct Informe {
	bool juegoEncontrado = false;
	std::string juego;                    ///< carpeta (UTF-8)
	std::string parche;                   ///< nombre del parche (primer modo del cambiador o la carpeta)
	std::string exeHuella;                ///< sha256 del primer MB de PES2021.exe + tamaño (versión del exe)
	long long exeBytes = -1;
	std::vector<std::string> modos;       ///< carpetas SiderAddons (raíz + modos)
	std::vector<std::string> luaActivos;  ///< lua.module activos en sider.ini de la raíz, en orden
	std::vector<std::string> cpkRoots;    ///< cpk.root activos en sider.ini de la raíz, en orden
	std::vector<Archivo> archivos;
	std::string huella;                   ///< sha256 del resumen (la «huella del anfitrión»)
	std::string generado;                 ///< AAAA-MM-DD HH:MM (hora local)
	std::string modo = "informativo";     ///< hoy siempre «informativo»
	std::vector<std::string> avisos;      ///< cosas raras a la vista (no bloquean nada)
};

/// Revisa el juego de la carpeta dada (o el último conocido). Lee archivos; nunca escribe.
Informe revisar(const fs::path& carpetaJuego);
/// Resumen JSON compacto (para la interfaz y para el resultado oficial).
std::string comoJson(const Informe& i);

} // namespace phoenix::doctor
