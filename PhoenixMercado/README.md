# Phoenix Mercado

Módulo de la Liga Máster web para PES 2021. Vive en este repo y está **separado en código** de Phoenix Soda:
la build de Phoenix Soda (`SmashSoda/`) no lo incluye todavía, así que **no la puede romper**.

| Carpeta | Qué es |
|---|---|
| `core/` | Toda la lógica, sin Windows ni interfaz: API `/mercado/v1`, SHA-256, copias, option file (leer/mover/guardar), base de datos del parche (.cpk → Player.bin), catálogo |
| `core/Emparejamiento.*` | Phoenix ID ↔ ID local: por ID (si los datos coinciden), por datos, o a revisión manual. Nunca automático ante la duda |
| `core/Integridad.*` | Huella de plantillas (antitrampa entre parches), modo seguro, informe de cambios tras una actualización |
| `terceros/` | libpesXcrypter (cifrado del option file) y miniz (zlib) |
| `windows/` | HTTPS (WinHTTP) y token cifrado (DPAPI) |
| `app/` | App de consola para probar |
| `pruebas/` | Pruebas del Core con una web falsa (sin internet) |

## Compilar (Windows, desde la raíz del repo)
```
cmake -S PhoenixMercado -B build-mercado -A x64
cmake --build build-mercado --config Release
build-mercado\Release\PhoenixMercadoPruebas.exe
build-mercado\Release\PhoenixMercado.exe eco
```

## Dos modos de token
| Modo | Cómo | Estado |
|---|---|---|
| **compartido** (por defecto) | Usa el token de Phoenix Link (`phoenix-token.dat`). **Solo lo lee**: nunca lo escribe ni lo borra | Necesita que la web acepte ese token en `/mercado` |
| **codigoManager** (`--manager`) | Segundo código de 8 caracteres solo para el modo manager; token propio en `%APPDATA%\Phoenix Mercado\` | Funciona hoy con `/mercado/v1/vincular` |

## Reglas
- Nunca toca el original: las copias llevan fecha y hora, nunca se sobrescriben y se verifican por SHA-256.
- El option oficial se descarga, se verifica la huella, y si no coincide se descarta.
- El token nunca va al log (`limpiarSecretos` lo tacha por si acaso).

## Comandos de prueba
```
PhoenixMercado catalogo <copia de EDIT00000000> <CGP_database.cpk> catalogo.json "ConmeGOL Patch 26"
PhoenixMercado mover <copia de EDIT00000000> <pes_id> <equipo_destino> <EDIT nuevo>
```
```
PhoenixMercado verificar <EDIT> <cpk>          → modo seguro: ¿se puede escribir?
PhoenixMercado emparejar <EDIT ref> <cpk ref> <EDIT local> <cpk local> informe.json
```
`mover` nunca sobrescribe: guarda en un archivo nuevo y lo relee para verificarlo.

## Pruebas con tus archivos reales
```
set PM_EDIT=C:\ruta\copia\EDIT00000000
set PM_CPK=C:\ruta\CGP_database.cpk
build-mercado\Release\PhoenixMercadoPruebas.exe
```
