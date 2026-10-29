// Phoenix Core · «Online Phoenix»: redirige los nombres *.konami.net a nuestro servidor SIN tocar el archivo hosts.
// Cambia, en la memoria de PES2021.exe, los punteros a getaddrinfo / GetAddrInfoW / gethostbyname / WinHttpConnect
// (el juego los pide en ejecución) y GetProcAddress (para el getaddrinfo que se resuelve más tarde, WspiapiLoad).
// Opción de phoenix_core.json, APAGADA por defecto:   "online_phoenix": {"activo": false, "ip": "127.0.0.1"}
#pragma once
#include <string>

namespace phx_online {

using LogFn = void (*)(const char* linea);

// Lee phoenix_core.json de `carpeta` y, si «activo» es true (y no es modo seguro), instala los desvíos.
void iniciar(const std::string& carpeta, bool modoSeguro, LogFn log);

// Restaura los punteros originales (se llama al descargar la DLL).
void detener();

}  // namespace phx_online
