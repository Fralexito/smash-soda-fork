#pragma once
// Buscar la carpeta de PES 2021 sin pedirle nada al usuario (boton «Buscar el juego» de SYNC › Preparar todo).
// Solo LEE nombres de carpetas (nunca abre ni escribe archivos del juego). Una carpeta cuenta si tiene PES2021.exe;
// «conSider» si ademas tiene SiderAddons\sider.ini (sin Sider Phoenix no puede ayudar). Tiene tope de profundidad y de tiempo.
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace phoenix::buscar {

namespace fs = std::filesystem;

struct Candidato {
	fs::path carpeta;
	bool conSider = false;
};

/// Recorre `raices` (en orden) hasta `profundidad` niveles y devuelve las carpetas con PES2021.exe (las que traen Sider primero).
/// Se detiene a los `limiteMs` o si `cancelar()` dice true. Salta carpetas del sistema y de programacion.
std::vector<Candidato> buscarJuegos(const std::vector<fs::path>& raices, int profundidad, int limiteMs, const std::function<bool()>& cancelar = nullptr);

/// Sitios donde mirar en ESTE PC: las bibliotecas de Steam (steamapps\common) y luego las unidades fijas. Solo Windows; en otros sistemas, vacio.
std::vector<fs::path> raicesDeBusqueda();

/// Lee las rutas «path» de un libraryfolders.vdf de Steam (texto de Valve). Logica pura, probada.
std::vector<std::string> rutasDeBibliotecasSteam(const std::string& vdf);

/// Busqueda en segundo plano (un solo hilo): iniciar() y luego estado() cada poco. Nunca bloquea la interfaz.
struct EstadoBusqueda {
	bool buscando = false;
	bool terminada = false;
	std::vector<Candidato> candidatos;
};
void iniciarBusqueda();
EstadoBusqueda estadoBusqueda();

} // namespace phoenix::buscar
