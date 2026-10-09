# 06 · La ruta del descubrimiento (noche del 8 al 9 de octubre de 2026)

**Qué es:** el relato **en orden**, paso a paso, de cómo se llegó a cambiar el juego en vivo: qué se intentó, por qué, qué salió bien, **qué salió mal** (incluidos los errores de Claude) y qué se aprendió en cada paso.
Sirve para **no repetir errores** y para entender **por qué** el método final es como es.
- Detalle técnico de cada prueba: `../PRUEBAS.md`.
- Método final: `05-PUERTA-EN-VIVO.md`.
- Resumen general: `../MANUAL-PARCHE-PHOENIX.md`.

**Participantes:** FRALEX (juega y prueba en su PC, toma las fotos y decide) y Claude (analiza, programa, instala con respaldo y documenta).

---

## Etapa 1 · Entender el terreno (00:10–00:30)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 1 | Se abrió por dentro el **switcher** de ConmeGOL (`PES2021 Start.exe`). Es un programa de Python empaquetado con PyInstaller; se extrajo y se desensambló con Python 3.14. | ✅ Hace robocopy de la carpeta del modo encima del juego, abre Sider, espera 2 s y abre el juego. **No usa internet.** |
| 2 | Revisión de lo que trae Sider: `sider.dll`, eventos, librerías y el módulo Real-Time-Match-Manager. | ✅ Sider tiene `io`, `memory`, `ffi` y eventos como `display_frame` y `livecpk_read`. **No tiene red.** Existe un precedente: un módulo que cambia la lluvia en vivo leyendo un `.ini`. |
| 3 | **Decisión de arquitectura:** web ⇄ Phoenix Link (internet) ⇄ archivos («buzón») ⇄ Sider. | ✅ El juego nunca toca internet. |

**Acierto:** abrir el switcher explicó por qué el primer intento de instalar `phoenix.lua` (días antes) nunca apareció: el switcher lo pisaba.

## Etapa 2 · El buzón: avisos en vivo (00:30–00:45)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 4 | Se instaló `phoenix.lua` v0.1 **en la carpeta del modo**, con respaldo de `sider.ini` y una sola línea nueva. | ❌ **No cargó.** `sider.log`: «attempt to call global 'pcall' (a nil value)». |
| 5 | **Error de Claude:** dio por hecho que el Lua de Sider traía `pcall`, como cualquier Lua. Lo había probado con Lua normal, no con el entorno real de Sider. | Lección: **probar siempre con los globales exactos de Sider**. El volcado de `env.lua` en el log los lista. |
| 6 | v0.2 sin `pcall` (errores contados con una «bandera»). FRALEX pulsó Shift+R. | ❌ «Reloaded modules: 0». **Error de Claude:** creyó que Shift+R cargaría un módulo que había fallado; solo recarga los que ya estaban activos. |
| 7 | Se reinició el juego. | ✅ «PHOENIX EVOLUTION · conectado». |
| 8 | Se cambió el aviso con el juego abierto. | ✅ **Apareció en 1–2 s** («hola causa»). Tildes, ñ, ⚡ y → se ven bien. |

## Etapa 3 · Nombre y organización (00:45–02:20)

- Se prepararon prompts para el chat WEB (buzón con ETag) y para el chat LINK (el «cartero»).
- **Phoenix Mercado pasó a llamarse Phoenix Sync** por decisión de FRALEX. Se renombraron la carpeta, los exe y los textos, pero se mantuvo lo que otros usan: la API `/mercado/v1`, el token y la rama. Compiló, con 295/295 pruebas OK.
- Más tarde, el chat LINK construyó el cartero y **el «hola» escrito en la web apareció en el juego (02:59)**: la cadena completa web → juego quedó confirmada.

## Etapa 4 · Buscar al jugador en la memoria (02:35–03:00)

FRALEX quería cambios **en tiempo real**. Primera idea: encontrar la ficha de Lamine Yamal en la memoria del juego.

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 9 | v0.3: tecla B. Recorre la memoria por regiones y busca los 58 bytes exactos de la ficha. | 🟡 Revisó 4,4 GB en 33 s **sin crash**, pero **0 coincidencias**: el juego cambia bits al cargar (forma, lesión). |
| 10 | FRALEX pulsó B por segunda vez. | ❌ **CRASH** (0xC0000005 dentro de sider.dll). **Error de Claude:** leía memoria **directo**, averiguando qué zonas existían en un cuadro y leyéndolas en los siguientes. Justo entonces ReShade recargó sus efectos y liberó memoria. **No se dañó nada** (solo lectura; los guardados tenían fecha anterior). Se reinstaló la versión segura de inmediato. |
| 11 | v0.4: **copia segura** con `ReadProcessMemory`, que si la zona ya no existe responde «falló» en vez de cerrar el juego. Solo memoria privada. Busca el ID y **verifica** 5 cualidades. | ✅ **Ficha encontrada sin crash** (Velocidad 90, Aceleración 93…). La versión de v0.4 se instaló solo **con el OK de FRALEX**, tras el crash. |
| 12 | v0.5: comparar copias. FRALEX estaba en la pantalla de habilidades de Lamine. | ✅ **Calibración gratis:** las 15 cualidades de la pantalla coinciden con el mapa, y **Potencia de tiro (358) queda confirmada**. Las «copias 1–5» eran falsas alarmas (listas con el nombre). Se filtraron exigiendo el ID. |

## Etapa 5 · Escribir en la memoria: el callejón sin salida (03:00–03:20)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 13 | v0.6: tecla V. Cambia **un byte** (Velocidad 99) con `WriteProcessMemory`, tras volver a comprobar la ficha. | ❌ La pantalla siguió en 90. Solo se escribió 1 de 4 copias. **Error de Claude:** exigía el nombre dentro de la ficha, y las copias que usa el juego no lo llevan. |
| 14 | v0.7: verificar por ID + bytes de cualidades. | ❌ Siguió en 90, aunque **todas** las copias ya tenían 99. |
| 15 | v0.8: buscar la ficha «desempaquetada» (6 formatos). | ❌ 0 coincidencias. |
| 16 | **Pista clave:** cada búsqueda encontraba **más copias**, y las nuevas nacían con 90 y con el formato exacto del archivo. Hipótesis: el juego **vuelve a leer el archivo**. | → Cambio de estrategia: **dejar la memoria e ir al archivo**. |

**Lección:** escribir en la memoria es frágil y no sirve, porque el juego rehace sus copias desde la base. **Mejor archivo que memoria.**

## Etapa 6 · El espía y el botón nativo (03:20–03:35)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 17 | v0.9: **espía** pasivo con `livecpk_read`, que cuenta cada lectura de `pesdb\*.bin` (técnica vista en `CommonLib.lua` de zlac). | ✅ Al arrancar se lee todo dos veces. Al abrir pantallas, **no**. |
| 18 | Idea de FRALEX: usar los **botones del propio juego** en vez de Sider. Primero se probó «Datos Actual. en vivo». | ❌ «Los servicios en línea finalizaron el 25/08/2022»: depende de Konami. |
| 19 | FRALEX entró y salió de **Editar** y de la Liga Máster. | ✅ **Entrar a Editar relee Player.bin** (lectura #3). La Liga Máster no. |

**Acierto de FRALEX:** buscar un botón nativo. Ese fue el camino ganador.

## Etapa 7 · Nuestro libro (03:35–03:55)

| Paso | Qué se hizo | Resultado |
|---|---|---|
| 20 | Se fabricó un **Player.bin propio**: la base de olmosjr23 descomprimida, **1 byte** cambiado (Velocidad 99), recomprimida WESYS + zlib y verificada de ida y vuelta. Se sirvió desde una raíz nueva, `livecpk\Phoenix-DB`, puesta **antes** de olmosjr23 (respaldo y una línea). | ✅ **Tras reiniciar: Velocidad 99.** El juego acepta nuestra base. |
| 21 | Se cambió a v95 con el juego abierto; FRALEX entró y salió de Editar. | ❓ Siguió 99. Además **se pulsó V** (la prueba vieja de escribir en memoria) y **ensució** el resultado. **Error de Claude:** dejar activa una tecla de una prueba ya descartada. Se quitó en v0.10. |
| 22 | v0.10: **huella** de cada lectura (suma cada 64 bytes del trozo que entrega Sider). Prueba limpia: arranque con v95. | ✅ Pantalla 95, y la huella coincide **exactamente** con v95. |
| 23 | Con el juego abierto: v90 → **solo Editar** (entrar y salir). | 🟡 La huella demuestra que leyó v90, **pero la pantalla siguió en 95**: Editar usa un borrador y lo descarta. |
| 24 | **Editar → Cargar** (sin guardar) → amistoso nuevo. | 🏆 **VELOCIDAD 90. Cambio en vivo sin reiniciar, con un botón nativo del juego.** |

## Etapa 8 · Cierre y calibración (03:55–04:25)

- Phoenix-DB quedó con **datos idénticos a los originales** (v90), en la raíz del juego y en la carpeta del modo.
- Se documentó todo: `05-PUERTA-EN-VIVO.md`, el **Manual**, el PDF de 47 páginas y Drive.
- **Calibración con fotos de FRALEX:**
  - Joan García y Livakovic ⇒ **portero 100 % mapeado** (269 Despejar, 300 Cobertura, 320 Reflejos, 326 Actitud, 364 Atajar);
  - **pie malo, regularidad y lesiones = valor guardado + 1**;
  - **496 = Patadón en largo**; 502/522 = Pase largo portero / Parapenaltis.

---

## Resumen de aciertos

1. **Abrir el switcher** antes de instalar nada (explicó el fallo antiguo).
2. **Buzón por archivo:** simple, seguro y rápido (1–2 s).
3. **Copia segura de memoria** tras el crash: nunca más se cerró el juego.
4. **El espía de lecturas:** convirtió suposiciones en datos.
5. **La huella:** prueba sin dudas de qué archivo leyó el juego.
6. **Cambiar de estrategia a tiempo:** de la memoria al archivo.
7. **El botón nativo** (idea de FRALEX) → Editar → Cargar.
8. **Cada cambio con respaldo, una línea y documentado.** Ningún archivo de FRALEX quedó dañado.

## Resumen de errores (y su lección)

| Error | Consecuencia | Lección / regla |
|---|---|---|
| Suponer que Sider tenía `pcall` | El módulo no cargó | Probar con los globales reales de Sider |
| Suponer que Shift+R cargaría un módulo fallido | Una vuelta de más | Shift+R solo recarga módulos activos |
| Leer memoria directo a lo largo de varios cuadros | **Crash del juego** (sin daños) | Siempre `ReadProcessMemory`; nunca memoria ajena directo |
| Buscar la ficha byte a byte | 0 resultados | Buscar por ID y verificar cualidades |
| Exigir el nombre para escribir | Se saltaron las copias reales | Verificar por lo que de verdad identifica (ID + cualidades) |
| Insistir en la memoria (v0.6–v0.8) | 3 intentos sin éxito | Si el juego rehace copias, el problema está en la **fuente** (el archivo) |
| Dejar activa la tecla V | Ensució una prueba | Desactivar las herramientas de pruebas descartadas |
| Creer que «entrar a Editar» bastaba | Una prueba a medias | Medir con huella; el paso correcto es **Cargar** |

## Cómo se trabajó (método que funcionó)

1. **Hipótesis pequeña → prueba mínima → medir → anotar.**
2. **Solo lectura primero; escribir después**, y solo con el OK de FRALEX.
3. **Simular antes de instalar:** LuaJIT con el entorno de Sider y memoria falsa, incluida una zona que desaparece.
4. **Instalar con respaldo** y una sola línea, comprobada con `diff`.
5. **Si algo falla, parar:** primero la versión segura y después el análisis («si hay un error no avanzamos»).
6. **Documentar en el momento:** PRUEBAS, REGISTRO, GitHub y Drive.
