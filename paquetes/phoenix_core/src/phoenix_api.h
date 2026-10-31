// Phoenix Core · API en C (la usan el cargador de Sider con LuaJIT ffi y, más adelante, Phoenix Link).
// Regla: hacia fuera nunca salen direcciones crudas del juego; solo funciones con nombre.
#pragma once
#include <stdint.h>

#ifdef PHOENIX_CORE_EXPORTS
#define PHX_API extern "C" __declspec(dllexport)
#else
#define PHX_API extern "C" __declspec(dllimport)
#endif

#define PHX_CORE_API_VERSION 1

// Versión de Core en texto ("0.1.0") y de la API (sube si cambia una firma).
PHX_API const char* phx_core_version(void);
PHX_API int         phx_core_api_version(void);

// Arranque. `carpeta` = carpeta de Core (la que tiene phoenix_core.json), con barra final o sin ella.
// Devuelve: 1 = arrancó normal, 2 = MODO SEGURO (todas las funciones apagadas), 0 = apagado por PHOENIX_CORE_OFF,
// -1 = error. Llamar UNA vez (las siguientes no hacen nada y devuelven el mismo código).
PHX_API int  phx_core_iniciar(const char* carpeta);

// Llamar cada fotograma (evento display_frame de Sider). Barato: solo mira el reloj.
// A los 60 s estables borra arrancando.flag (protector de bucle de crash).
PHX_API void phx_core_tick(void);

// Enciende (1) o apaga (0) una función por su id. Devuelve el estado nuevo (0/1) o -1 si no existe o no se pudo.
PHX_API int  phx_core_funcion(const char* id, int activar);

// Estado en JSON (versión, exe, modo, funciones) en `buf` (n bytes). Devuelve los bytes escritos sin el 0 final,
// o el tamaño necesario si no cabe.
PHX_API int  phx_core_estado(char* buf, int n);
