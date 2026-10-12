# Perfiles de amigos: más de 32 «Datos personales» en PES 2021

12 de octubre de 2026 · Frente LINK · Estado: **FASE 1 hecha (módulo y pruebas con copias); Fase 2 y 3 sin hacer; NADA probado todavía en el juego real.**

## 1. El problema

PES 2021 guarda hasta **32 «Datos personales»** (Usuario 1…32). Cada uno lleva el nombre y la configuración de un amigo (botones, cursor, ayudas…). Fralex tiene más de 32 amigos, y el nombre le sirve para saber quién controla a cada jugador.

## 2. Qué se descubrió (con la prueba ZZTEST, 12 oct)

- ✅ Viven en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\<SteamID>\save\SYSTEM00000000` (en la PC de Fralex: `D:\Users\Alexander\Documents\...\239200\save`). No en `settings.dat`.
- ✅ **PES no escribe ese archivo al editar el Usuario.** Lo escribe al **terminar un partido** (la primera prueba, sin partido, no cambió nada; la segunda, con partido, sí).
- ✅ El archivo está cifrado igual que EDIT/ML. Link ya sabe abrirlo (mismo cifrado). Descifrado: 219.568 bytes.
- ✅ Tabla **fija de 32 registros de 2.432 bytes desde el byte 0x50**; justo detrás (0x13050) siguen otros datos. Un registro: marca (0/1), **nombre de 48 bytes**, y desde +0x34 la configuración (mapa de botones, cursor, ayudas, mapas «Personalizado»…).
- ✅ El nombre «ZZTEST» apareció en el hueco 22 y cambiaron 4 bytes de la configuración (el botón que se cambió).
- ✅ **Ampliar el 32 de verdad exigiría cambiar el formato y el ejecutable** (la tabla está pegada a los demás datos). Descartado.
- ❓ Sin probar: que PES acepte un `SYSTEM00000000` escrito por Link (para EDIT y ML el juego no valida la cabecera; para SYSTEM falta probarlo). También si lee el archivo en vivo o solo al arrancar.

## 3. Plan (opción B, aprobada por Fralex)

Link guarda un **perfil por amigo** (nombre + configuración) en su propia base y lo vuelca a uno de los 32 huecos **antes de abrir PES**; al terminar, recoge los cambios que el amigo hizo.

| Fase | Qué | Estado |
|---|---|---|
| 0 | Localizar el archivo (ZZTEST) | ✅ |
| 1 | Módulo de Link `core/PerfilesPes`: leer los 32, exportar/importar un Usuario, escritura segura | ✅ con COPIAS (`pruebas-link/correr_perfiles.bat`) |
| 2 | Base de perfiles por Parsec ID, huecos protegidos/rotativos, «volcar» y «recoger» | pendiente |
| 3 | Interfaz «Perfiles de amigos» en Link | pendiente |
| 4 | Pruebas en el juego (con permiso de Fralex) y guía | pendiente |

## 4. Fase 1: qué hace el módulo (`SmashSoda/phoenix/core/PerfilesPes.*`)

- `Guardado::abrir` lee y valida el `SYSTEM00000000` (cabecera del sobre, tipo SYSTEM, PES 2021, 219.568 B, los 32 registros con forma de Usuario). Si algo no cuadra, **no hace nada**.
- `usuario(i)` / `usuarios()` leen nombre y registro; `aPerfil` / `dePerfil` exportan e importan un perfil en **JSON propio de Phoenix** (`{formato:1, juego:"PES2021", nombre, tam, registro:"<hex>"}`).
- `ponerUsuario` / `rotular` cambian un hueco en memoria. Solo cambia ese registro; el resto del archivo queda idéntico (probado byte a byte).
- `guardarComo` escribe un archivo nuevo (nunca sobrescribe) y lo relee. `reemplazar` hace **copia `SYSTEM00000000.phoenix-<sello>`** al lado, escribe un temporal, lo relee y valida, y solo entonces reemplaza (todo o nada).
- Pruebas (con una COPIA real): abrir y listar 32; cifrar sin cambios da **exactamente el mismo archivo**; exportar → importar → exportar es idempotente; los otros 31 Usuarios y los datos de después de la tabla no cambian; rechaza hueco 33, registro corto o corrupto, archivo truncado, basura, archivo inexistente, perfil de otro formato; no deja temporales; el original no se toca.

## 5. Reglas para las fases siguientes

- **Solo con PES cerrado** (salvo que se pruebe que PES relee el archivo en vivo).
- Nunca escribir en los guardados reales de Fralex sin su permiso explícito y copia previa.
- Huecos **protegidos** (los que Fralex quiera conservar tal cual) y huecos **rotativos** (p. ej. los últimos 8–16).
- Si dos amigos piden el mismo hueco, gana el primero y el otro va a otro hueco libre; si no hay, se avisa (máximo 32 jugando a la vez).
- **Nunca se pierde una configuración**: antes de pisar un hueco rotativo, su contenido se recoge a su perfil o se guarda copia.
- El nombre de los amigos **no** se sube a ningún sitio ni se escribe en esta documentación.

## 6. Cómo reproducir las pruebas

```
pruebas-link\correr_perfiles.bat "<copia de SYSTEM00000000>"
```
Trabaja solo sobre una copia en una carpeta temporal. Resultado esperado: 32 líneas «Usuario N» y `TODO OK`.
