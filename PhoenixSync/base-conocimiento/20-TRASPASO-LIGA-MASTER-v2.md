# 20 · Traspaso para otra IA (versión 2): cómo continuar TODO el trabajo de Liga Máster (PES 2021)

**Para:** cualquier chat o IA que continúe el trabajo de FRALEX. **Fecha:** 9 de octubre de 2026, 23:55 (Lima).
**Versión 2.** Actualiza la guía de traspaso anterior (la que está en el proyecto de claude.ai como `claude/traspaso-liga-master-para-otra-ia.md`). La versión 1 **no se tocó**; esta la reemplaza en contenido.
**Si eres una IA nueva, lee este documento completo antes de tocar nada.**

---

## 0. Quién es el usuario y cómo hay que tratarlo

- Se llama **FRALEX** (no uses su nombre oficial). Habla español.
- Se marea con muchos números o símbolos juntos. Explica **simple, ordenado, espaciado y paso a paso**. Define cada palabra nueva desde cero y **repite lo importante**.
- Quiere **0 errores**: si aparece un error, **no avances** hasta resolverlo.
- Avisa cada cierto tiempo, con mensajes cortos, de qué estás haciendo.

---

## 1. Qué es el proyecto (en 6 líneas)

1. **Phoenix Evolution** = la web (repo Fralexito/phoenixevolution, Supabase).
2. **Phoenix Link** = la app de PC (fork de Smash Soda con Parsec). Carpeta `SmashSoda/`, rama `rediseño-phoenix-portal`. **No es de este frente.**
3. **Phoenix Sync** (antes «Phoenix Mercado») = programa en C++ que lee y edita los datos de PES 2021. Carpeta `PhoenixSync/` del repo Fralexito/smash-soda-fork, rama **`mercado-fase0`**.
4. Meta: un parche propio de PES 2021, conectado a la web, con liga online propia.
5. Esta guía cubre la **Liga Máster** (archivos `ML0000000N`) y su relación con la base (`Player.bin`).
6. Parche de trabajo: **ConmeGOL 26** (guardados en la carpeta Steam `239200`). Todo debe pensarse **multiparche** (sección 9).

---

## 2. Reglas de trabajo (obligatorias)

### Sobre los archivos
1. **Nunca** trabajes sobre el original de un guardado: siempre una **copia**.
2. **Siempre** haz la ida y vuelta (descifrar y volver a cifrar) antes de editar. Si no sale idéntico, **no sigas**.
3. **Siempre** cierra en un archivo **nuevo**. Jamás sobrescribas.
4. **Anota la huella** (md5/sha256) de cada archivo que entregues, y **compruébala en el PC** después de copiarlo. El juego autoguarda encima de la ranura cargada.
5. Los guardados y los `.bin` descifrados **no se suben a GitHub**: son datos del usuario.
6. No edites archivos del juego ni option files sin **respaldo** y aprobación de FRALEX. «Si algo se rompe, mejor no lo hagas».

### Sobre lo que se da por bueno
7. Un cambio solo es **«probado» (✅)** cuando FRALEX lo ve en el juego. Lo medido en archivos es **«observado» (🔎)**.

### Sobre los registros (decisión de FRALEX, 9 oct)
8. **Anota siempre**, sin que te lo pida: cada prueba en el juego o análisis va **al final** de `PhoenixSync/PRUEBAS.md`, y cada cambio, en **una línea** al final de `PhoenixSync/REGISTRO.md`.
9. **Nunca borres ni sobrescribas** nada en esos archivos ni en los documentos: solo **añadir al final**, o crear un archivo **nuevo** con otro nombre.
10. **GitHub:** FRALEX pidió mantenerlo **actualizado**. Haz commit y push de lo que añadas (documentos, código que compila, registros) en la rama `mercado-fase0`. Nada a `master` sin permiso.
11. **Drive:** después de **cada avance importante**, escribe una **guía exacta** (qué hiciste, cómo lo comprobaste, con qué recurso) en `PhoenixSync/base-conocimiento/NN-GUIA-….md` y súbela como **documento nuevo** a la carpeta de Drive `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI`. Fuera de eso, **no subas nada al Drive** si FRALEX no lo pide. Nunca reemplaces un documento del Drive.
12. Las guías siempre **multiparche**: separa lo que pone el juego de lo que pone el parche (🧩) y escribe los riesgos.

### Sobre el repositorio
13. Antes de trabajar: `git fetch` + `git pull --rebase`. Hay **dos cuentas de Claude** trabajando en el mismo repo.
14. Si al subir hay conflicto en `PRUEBAS.md` o `REGISTRO.md`: **se quedan las dos partes** (primero la de la otra cuenta y luego la tuya). Nunca descartes la otra parte.
15. **No** hagas `git checkout` de otra rama en `C:\dev\smash-soda-fork` (Link compila en el worktree `_phoenix-link`). **No** toques `SmashSoda/`.
16. **Compila sin errores** antes de cada commit de código, y corre las pruebas automáticas.
17. La web y Supabase **solo** se tocan con prompts para el chat WEB.
18. Nunca pidas contraseñas ni secretos. Contenido de otros modders solo con permiso y crédito.
19. Si el git del puente da «Operation not permitted» al borrar y quedan candados `.git/index.lock`: **muévelos** (`mv`), no los fuerces.

### Numeración de guías
20. Antes de crear una guía, mira qué números existen: las dos cuentas numeran en la misma carpeta. Ya hay **dos «17»** (el traspaso v1 se pensó como 17, y en el repo está `17-OPCION-EN-VIVO-DONDE-ESTA.md`) y **dos «19»** (`19-GUIA-VERIFICACION-EN-PANTALLA.md` y `19-OPCION-EN-VIVO-PASO-A-PASO.md`). No se renombraron para no romper nada. Usa siempre el siguiente número libre.

---

## 3. Qué leer y en qué orden (todo en `PhoenixSync/base-conocimiento/`, copia en el Drive)

| Orden | Archivo | Para qué |
|---|---|---|
| 1 | `00-INDICE.md` | Mapa general |
| 2 | **este (20)** | Método, reglas, estado y comandos |
| 3 | `10-COMO-SE-ABRIO-EL-GUARDADO.md` | Cómo se abre y cierra un guardado |
| 4 | `09-LIGA-MASTER-COMO-LO-HICIMOS.md` | Traspasos, venta, dinero, blob, fichar (paso a paso) |
| 5 | `08-LIGA-MASTER-INFORME.md` | Resumen y guía de uso |
| 6 | `13` → `14` → `15` → `16` → `18` → `19-GUIA-VERIFICACION-EN-PANTALLA` | Agentes libres y fichas, calendario, tablas, goleadores y lector de temporada, stats en la LM, verificación en pantalla |
| 7 | `../liga-master/ESTRUCTURA-ML.md` (§1–§25) | Bytes exactos de todo |
| 8 | `01-MOTOR-PES2021.md` y `05-PUERTA-EN-VIVO.md` | Motor y base de datos (Player.bin, Editar → Cargar) |
| 9 | `12-GUIA-MULTIPARCHE.md` y `parches/*.md` | Qué cambia entre parches |
| 10 | `11-SUPER-INFORME.md`, `../PRUEBAS.md`, `../REGISTRO.md` | Estado general y diarios |

---

## 4. Entorno y acceso

- La IA corre en la nube (Linux). El PC de FRALEX se alcanza por el **puente de la app de escritorio** (`device_bash`, `device_list_dir`, `device_stage_files`, `device_commit_files`). En la VM del puente, las carpetas están en `$HOME/mnt/`: `239200--save`, `Conmegol Patch`, `_PhoenixMercado_prueba`, `SP2026`… Si el puente no está, pide a FRALEX abrir el chat en la app de escritorio.
- **Guardados:** `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\` → `ML00000000` = ranura 1 del menú Cargar, `ML00000001` = ranura 2… (en hexadecimal: la ranura 11 es `ML0000000A`). `EDIT00000000` = option file. Respaldos en `save\_referencias_mercado\`.
- **Carpeta del juego/parche:** `D:\Frank\Games_\Conmegol Patch\`. Base que usa de verdad el juego: la que sirve Sider (`SiderAddons\olmosjr23\Database\…\Player.bin`), y **antes** la nuestra: `SiderAddons\livecpk\Phoenix-DB\…` (también en `ConmeGol Extras\ConmeGOL Patch 26\SiderAddons\…`).
- **Repo en el PC:** `C:\dev\smash-soda-fork\PhoenixSync\`.
- **Estado del PC al cerrar esta versión (9 oct, 23:55):** Phoenix-DB tiene el `Player.bin` **v99** de prueba (Lamine 162114, Velocidad 99) en las dos carpetas. Ranura 1 = carrera del **Barça** (4/3/2026, md5 7b800612…); ranuras 2 y 3 = carrera del **City** (21/8/2026).

---

## 5. Herramientas y comandos

**Cifrado** (biblioteca libre `terceros/pesxcrypter/`; herramientas en `liga-master/herramientas/`):
```
T=../../terceros/pesxcrypter
for t in dec info enc2; do gcc -O2 -o $t $t.c $T/crypt.c $T/masterkey.c $T/mt19937ar.c -I$T; done
dec  <ML0000000N> salida.bin reencriptado.bin     # descifra y comprueba la ida y vuelta
info <ML0000000N>                                  # cabecera, tamaños, etiqueta, serial
enc2 <ML original> <datos.bin> <ML nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'   # el 5.º = texto del menú Cargar
```
`dec` a veces termina con error **después** de escribir los archivos: comprueba la ida y vuelta, no el código de salida.

**Prototipos Python** (`liga-master/prototipos/`): `mlparse.py`, `mlsquad.py`, `build_c.py`, `build_v6.py`, `ref_v7_juego.py`, `diff_gt.py`, `blob.py`.

**Motor C++** (`core/`): `LigaMaster.*` (traspasos, venta, fichaje para el usuario, dinero), `Alineacion.h`, `BlobLM.*` (blob: lectura, reescritura, ficha por jugador), `SobrePes.*` (cifrar con cualquier tamaño, nunca sobrescribe), **`TemporadaLM.*`** (solo lectura: calendario, tablas, goleadores), `OptionFile.*`, `BaseDatosParche.*` (CPK y Player.bin), `Sincronizacion.*` (cambios firmados de la web).

**Compilar y probar:**
```
cmake -S PhoenixSync -B build-sync -A x64        (Windows)   ·   cmake -S PhoenixSync -B <carpeta> (Linux)
cmake --build build-sync --config Release
PhoenixSyncPruebas                                          → 211/211 sin archivos
PM_ML_DATOS=<ML descifrado> PM_CATALOGO=catalogo.json PhoenixSyncPruebas   → 225–234 según el guardado
```
Para Windows desde Linux: `x86_64-w64-mingw32-g++ -std=c++20` (necesita C++20).

**En el PC (VM del puente):** `~/phx/cpkls.py <texto> <archivos.cpk>` lista el índice de un CPK sin cargarlo entero.

---

## 6. El método (repítelo en cada parte nueva)

1. **Copiar** el guardado del PC a la nube (stage). El original no se toca.
2. **Descifrar** con `dec` y exigir ida y vuelta idéntica.
3. **Conseguir una «verdad del juego»:** FRALEX hace la acción real (fichar, despedir, jugar) y guarda en **otra ranura**.
4. **Comparar byte a byte** el antes y el después.
5. **Reproducir** lo mismo en Python y luego en C++, **buscando por la forma** (anclas), nunca por direcciones fijas, hasta que salga igual que el juego.
6. **Cifrar** con texto visible en el menú Cargar, **anotar la huella** y dejarlo en una ranura **libre** del PC.
7. **FRALEX lo prueba** en el juego. Anotar el resultado, bueno o malo.
8. Registrar (sección 2, reglas 8–11) y avisar a FRALEX en lenguaje simple.

**Para leer datos** (sin editar) vale también: leer del archivo, **escribir la predicción antes** y que FRALEX compare con la pantalla (así se probó la guía 19).

---

## 7. Estado de lo descubierto (9 oct, 23:55)

| Tema | Estado | Dónde |
|---|---|---|
| Abrir y cerrar el guardado (el juego no valida la huella de cabecera; menú Cargar = texto info) | ✅ | guía 10 |
| Traspaso entre dos equipos de la IA (plantilla + alineación de 600 B) | ✅ | guía 09, §3 y §10 |
| Vender de tu equipo a la IA (igual que el juego) | ✅ | guía 09, §17 |
| Fichar para tu equipo desde la IA (el contrato **no** se escribe: lo crea el juego) | ✅ | guía 09, §19–§20 |
| Presupuesto y tope salarial (por ancla a `0x97d38` de la tabla A) | ✅ | §13, §18 |
| Valor de mercado (ficha del blob) | ✅ | §15 |
| **Blob** = solo fichas de jugador: cabecera + **30.000** plazas de 156 B | ✅ (archivos) | guía 13, §21 |
| Blob localizado **por su forma** (ya no por la dirección fija 0x11403a8) | ✅ (pruebas) | §21 |
| Jugadores con **prefijo** en su número (**cambia en cada carrera**: 0xdb65 en el City, 0xdbdf en el Barça): ficha en `0x1e + 156·(reg & 0xffff)` | ✅ (pruebas) | §21, §24 |
| Agentes libres: bit 0x20 de +0x75 + fin de contrato vacío + sin equipo | 🔎 | guía 13 |
| **Calendario** (registro de partido de 32 B, número global, competición, jornada) | ✅ **en pantalla** | guías 14 y 19, §22 |
| **Tablas de posiciones** (fila de 20 B; actual + jornada anterior) | ✅ **en pantalla** | guías 15 y 19, §23 |
| **Goleadores** (fila de 20 B) | ✅ **en pantalla** | guías 16 y 19, §24 |
| Asistencias (2.ª lista) | 🔎 | §24 |
| Listas 3 y 4 de cada competición | ⏳ sin identificar | §24 |
| Lector `TemporadaLM` (calendario, tablas, goleadores; cruces 35/35, 152/152, 38/38) | ✅ (pruebas) | guía 16 |
| **Stats de Phoenix-DB en una carrera YA empezada** | ❌ **no llegan** (Lamine 90 en la carrera, 99 en amistoso) | guía 18, §25 |
| Dónde guarda la carrera su copia de las habilidades | ⏳ no encontrada | guía 18, §25 |
| `InstallVersionPlayer.bin` (lo lee el juego al entrar a la LM) = 7.426 «jugador → versión», sin habilidades | 🔎 | §25 |
| Resultados partido a partido | ⏳ no encontrados (se deducen de tabla + calendario en la última jornada) | §23 |
| Fichar un agente libre | ⏳ falta la verdad del juego | guía 13 |

---

## 8. Trampas conocidas

| Trampa | Qué hacer |
|---|---|
| Solo editar la lista del club deja al jugador vivo en Estrategia | Tocar todas las tablas de tu equipo y la lista K |
| Porteros en el ataque | Recalcular orden de formación y los 6 roles |
| Jugador en blanco «DC 0» | Editar la alineación de 600 B del club de la IA |
| Contrato duplicado | No escribir el contrato al fichar |
| El juego guarda encima de la ranura cargada | Anotar huellas y usar otra ranura |
| Direcciones fijas que dejan de servir | Buscar por la forma (anclas) |
| Creer que el calendario está en el blob | No está: el blob es solo fichas |
| Suponer que el prefijo de jugador es siempre 0xdb65 | Cambia en cada carrera |
| Creer que Phoenix-DB cambia una carrera empezada | No: solo amistosos y modos que leen la base |
| El panel derecho de la ficha de un jugador | Muestra el equipo que estás mirando (por ejemplo, tu próximo rival), no el club del jugador |
| Dos cuentas numerando guías | Mirar los números libres antes de crear una guía |

---

## 9. Multiparche (resumen de la guía 12)

- **Lo pone el juego (igual en todos):** el cifrado, el formato del option file, de `Player.bin` y de la Liga Máster, Editar → Cargar, el autoguardado, la regla de «Datos Actual. en vivo», que la Liga Máster tenga su propia copia de las habilidades.
- **Lo pone el parche (🧩):** la carpeta de guardados, dónde está la base de verdad, los IDs y la cantidad de equipos y jugadores, los números de competición, si usa Sider (y cuántos `sider.ini`), si trae exe modificado.
- **Estado por parche:** ConmeGOL 26 ✅ · Sudamerican 2026 🔎 (option file y emparejamiento) · ConmeGOL 27, Football Life 27, Evoweb, Gogosz ⏳.

---

## 10. Pendientes, en orden

1. **Experimento de stats en la Liga Máster** (guía 18, sección 4): FRALEX crea una carrera nueva con Phoenix-DB **v99** y guarda en la ranura 4; luego se pone la **v90** (con respaldo) y crea **la misma** carrera en la ranura 5; se comparan. Si en la primera Lamine sale con 99, las stats **sí entran al crear** una carrera.
2. **Resultados partido a partido:** guardar justo antes y justo después de un partido, el mismo día, en ranuras distintas.
3. **Fichar un agente libre:** FRALEX ficha uno en el juego y guarda en otra ranura.
4. Ver en pantalla las **asistencias** y averiguar las listas 3 y 4.
5. Repetir lo esencial en **Sudamerican** y en las versiones nuevas de los parches.
6. Llevar a la web lo que ya se lee (tablas, calendario, goleadores) y aplicar los traspasos aprobados en la web a la carrera elegida, con aviso de Phoenix Link.

---

## 11. Límite que NO se cruza

No se ayuda a modificar el ejecutable del juego ni su código en memoria para saltarse sus protecciones (por ejemplo, la puerta de inicio de sesión de «Datos Actual. en vivo»), ni a construir instaladores, conmutadores o documentación de ese camino. Todo lo de esta guía es **edición de archivos de datos y guardados** (modding normal). El camino recomendado para aplicar cambios es **Editar → Cargar**.

---

## 12. Prompt de arranque (pégalo al abrir un chat nuevo)

```
Eres un ingeniero de ingeniería inversa que continúa el proyecto Phoenix Evolution de FRALEX (PES 2021, parche ConmeGOL 26, rama mercado-fase0).
Antes de hacer nada: lee PhoenixSync/base-conocimiento/20-TRASPASO-LIGA-MASTER-v2.md completo y luego los archivos en el orden de su sección 3.
Habla en español, simple, ordenado, paso a paso; define cada palabra nueva; repite lo importante; avisa seguido con mensajes cortos. Meta: 0 errores; si hay un error, no avances.
Reglas: trabaja siempre sobre COPIAS; ida y vuelta antes de editar; cierra en archivo NUEVO; anota y comprueba huellas; un cambio es «probado» solo cuando FRALEX lo ve en el juego.
Registra siempre al FINAL de PRUEBAS.md y REGISTRO.md, sin borrar ni sobrescribir nada; mantén GitHub actualizado (commit y push en mercado-fase0; nada a master); tras cada avance importante, escribe una guía exacta nueva y súbela al Drive como documento nuevo; fuera de eso no subas nada al Drive sin que FRALEX lo pida.
Haz fetch + pull --rebase antes de trabajar; si hay conflicto en PRUEBAS/REGISTRO, conserva las dos partes. No hagas checkout de otra rama ni toques SmashSoda/. Compila y prueba antes de cada commit de código. No subas guardados ni .bin. Nunca pidas contraseñas. Guías siempre multiparche.
Primero confirma al usuario en 5 líneas qué entendiste y qué vas a hacer, y empieza por la tarea que él te pida. Si no sabes algo, dilo: no lo inventes.
```

---

## 13. Cómo registrar un hallazgo nuevo

1. Etiqueta: ✅ probado en el juego · 🔎 observado en archivos · ⏳ falta · ❌ comprobado que no · ⚠️ riesgo · 🧩 depende del parche.
2. Escribe: qué hiciste, cómo lo comprobaste, con qué herramienta, en qué guardado (con su huella) y el resultado.
3. Añádelo al final de `PRUEBAS.md` y `REGISTRO.md` y, si es un avance importante, a una guía nueva `NN-GUIA-….md` (copia en el Drive).
4. Avisa a FRALEX en lenguaje simple.
