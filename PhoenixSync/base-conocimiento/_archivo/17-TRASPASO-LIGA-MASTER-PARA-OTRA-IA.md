# 17 · Traspaso para otra IA: cómo replicar TODO el trabajo de Liga Máster (PES 2021)

**Para:** cualquier chat/IA que continúe el trabajo de FRALEX. **Fecha:** 9 oct 2026.
**Si eres una IA nueva, lee este documento completo antes de tocar nada.** Aquí está el método, el orden de lectura, los comandos exactos, las reglas y un prompt de arranque.

---

## 0. Quién es el usuario y cómo hay que tratarlo

- Se llama **FRALEX** (no uses su nombre oficial). Habla español.
- Se marea con muchos números/símbolos juntos. Explica **simple, ordenado, espaciado, paso a paso**, define cada palabra nueva desde cero y **repite lo importante**.
- Quiere **0 errores**: si aparece un error, **no avances** hasta resolverlo.
- Avisa cada cierto tiempo con mensajes cortos de qué estás haciendo.

## 1. Qué es el proyecto (en 6 líneas)

1. **Phoenix Evolution** = web (repo Fralexito/phoenixevolution, Supabase).
2. **Phoenix Link** = fork de Smash Soda con Parsec.
3. **Phoenix Sync** (antes «Phoenix Mercado») = programa en C++ que lee/edita datos de PES 2021. Carpeta `PhoenixSync/` en `C:\dev\smash-soda-fork`, rama `mercado-fase0`.
4. Meta: un parche propio de PES 2021 con juego online propio.
5. Esta guía cubre la **Liga Máster** (archivos `ML0000000N`) y la ficha de jugador (`Player.bin`).
6. Parche de trabajo: **ConmeGOL 26** (guardados en carpeta Steam `239200`). Otros parches (Sudamerican `292733975847239680`, Football Life 27, Evoweb, Gogosz) están **pendientes**.

## 2. Reglas de trabajo (obligatorias)

1. **Nunca** trabajes sobre el original del guardado: siempre una **copia**.
2. **Siempre** haz la ida y vuelta (descifrar y volver a cifrar) antes de editar. Si no sale idéntico, **no sigas**.
3. **Siempre** cierra en un archivo **nuevo**. Jamás sobrescribas.
4. **Anota la huella** (md5/sha256) de cada archivo que entregues. El juego autoguarda sobre la ranura cargada.
5. **Un cambio solo es «probado» cuando FRALEX lo ve en el juego.** Lo medido en archivos es «observado».
6. Mantén **PRUEBAS.md** y **REGISTRO.md** (en `PhoenixSync/`) al día. Mantén la carpeta de Drive `15TL096cZpM9A9bLPHtRtpNL6YbczyPuI` actualizada.
7. Los guardados y los `.bin` descifrados **no se suben a GitHub** (son datos del usuario).
8. Nunca pidas contraseñas ni secretos. Contenido de otros modders solo con permiso y crédito. Nunca distribuyas el juego.
9. Antes de trabajar en el repo: `git fetch` + `git pull --rebase` (hay **dos cuentas de Claude**). **No** hagas `git checkout` a otra rama en `C:\dev\smash-soda-fork`. **No** hagas commit/push si no te lo pide. Nada a `master` sin permiso.
10. La web/Supabase solo se toca con prompts para el chat WEB.
11. No edites archivos del juego ni option files sin respaldo y aprobación.
12. Si el git del puente da «Operation not permitted» al borrar, quedan candados `.git/index.lock`: **muévelos** (`mv`), no los fuerces.

## 3. Qué leer y en qué orden

Todo en `PhoenixSync/base-conocimiento/` (copia en la carpeta de Drive):

| Orden | Archivo | Para qué |
|---|---|---|
| 1 | `00-INDICE.md` | Mapa de todo |
| 2 | **este (17)** | Método y comandos |
| 3 | `10-COMO-SE-ABRIO-EL-GUARDADO.md` | Cómo se abre/cierra el guardado |
| 4 | `09-LIGA-MASTER-COMO-LO-HICIMOS.md` | Camino paso a paso (traspasos, venta, dinero, blob, fichar) |
| 5 | `../liga-master/ESTRUCTURA-ML.md` | Bytes exactos (§1–§23) |
| 6 | `13`, `14`, `15` | Agentes libres/fichas, calendario, tablas de posiciones |
| 7 | `01-MOTOR-PES2021.md` y `16-PLAYER-BIN-FICHA-COMPLETA.md` | Motor y ficha de jugador |
| 8 | `12-GUIA-MULTIPARCHE.md`, `parches/*.md` | Qué cambia entre parches |
| 9 | `11-SUPER-INFORME.md`, `PRUEBAS.md`, `REGISTRO.md` | Estado y diario |

## 4. Entorno y acceso

- La IA corre en la nube (Linux). El PC de FRALEX se alcanza por el **puente de la app de escritorio** (`device_bash`, `device_list_dir`, `device_stage_files`, `device_commit_files`). Si el puente no está, pídele que abra el chat en la app de escritorio.
- Guardados: `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\` (`ML00000000`… = ranura N+1 del menú Cargar; `EDIT00000000` = option file). Respaldos en `save\_referencias_mercado\`.
- Carpeta del parche: `D:\Frank\Games_\Conmegol Patch\`. `Player.bin` vivo: `…\SiderAddons\olmosjr23\Database\common\etc\pesdb\Player.bin`.
- Repo: `C:\dev\smash-soda-fork\PhoenixSync\`. Pasa archivos del PC a la nube con **stage** y de vuelta con **commit** (copian, no mueven).

## 5. Herramientas y comandos exactos

Código en `PhoenixSync/liga-master/herramientas/` (`dec.c`, `info.c`, `enc2.c`) y la biblioteca pública en `terceros/pesxcrypter/` (`crypt.c`, `masterkey.c`, `mt19937ar.c`).

**Compilar** (gcc en Linux; MinGW en Windows), desde `liga-master/herramientas/`:

```
T=../../terceros/pesxcrypter
for t in dec info enc2; do gcc -O2 -o $t $t.c $T/crypt.c $T/masterkey.c $T/mt19937ar.c -I$T; done
```

**Usar:**

```
dec  <ML0000000N> salida.bin reencriptado.bin     # descifra y comprueba la ida y vuelta
info <ML0000000N>                                  # cabecera, tamaños, etiqueta, serial (si dudas de los argumentos, lee info.c)
enc2 <ML original> <datos_editados.bin> <ML_nuevo> "nombre" $'PRUEBA 4\n4/8/2026\nPremier League'
                                                   # cifra; el 5.º argumento es el texto info (3 líneas) que muestra el menú Cargar
```

Otras piezas (en `PhoenixSync/`): `liga-master/prototipos/*.py` (`mlparse.py`, `mlsquad.py`, `build_c.py`, `build_v6.py`, `ref_v7_juego.py`, `diff_gt.py`, `blob.py`), y el motor C++ definitivo en `core/` (`LigaMaster.cpp`, `Alineacion.h`, `BlobLM.*`, `SobrePes.*`). Ejemplos:

```
python3 prototipos/build_c.py r0.bin v5.bin catalogo.json 154 66 133543 19
python3 prototipos/diff_gt.py base.bin guardado_por_el_juego.bin catalogo.json | head -80
```

**Compilar Phoenix Sync completo** (Windows, raíz del repo; o `COMPILAR_SYNC.bat`):

```
cmake -S PhoenixSync -B build-sync -A x64
cmake --build build-sync --config Release
build-sync\Release\PhoenixSyncPruebas.exe
```

Las pruebas automáticas deben terminar sin fallos (la última vez: 295 de 295).

## 6. El método (repítelo en cada parte nueva)

1. **Copiar** el guardado del PC a la nube (stage). El original no se toca.
2. **Descifrar** con `dec`; exigir ida y vuelta idéntica.
3. **Conseguir una «verdad del juego»:** FRALEX hace la acción real en el juego (fichar, despedir, jugar un partido) y guarda en **otra ranura**.
4. **Comparar byte a byte** el antes y el después; listar qué bytes cambian y dónde.
5. **Reproducir** lo mismo en Python y luego en C++ **por anclas** (no por direcciones fijas) y comprobar que sale el mismo archivo que el juego.
6. **Cifrar** (enc2/SobrePes) con texto visible en el menú Cargar, **anotar la huella**, devolverlo a una ranura **libre** del PC.
7. **FRALEX lo prueba** en el juego. Anotar resultado (bueno o malo) en `PRUEBAS.md`.
8. Añadir una línea a `REGISTRO.md` y, si hay un hallazgo nuevo, documentarlo y avisar a FRALEX.

## 7. Resumen de lo ya descubierto (detalle en los archivos citados)

- **Sobre cifrado:** 5 piezas (cabecera, descripción 384 B, logo 17.315 B, datos ~19,7 MB, serial 44 B). El juego **no** valida la huella de la cabecera. El menú Cargar muestra el **texto info** (desde el byte 128 de la descripción), no el nombre de 128 B.
- **Plantillas:** 700 bloques de 1.680 B desde `0x50` (40 pares `(reg, pid)`, dorsales, contador). City = bloque 154.
- **Alineación de la IA:** 629 bloques de 600 B (orden en `+0x220`, roles en `+0x248`).
- **Tu equipo:** tablas A–H, M, L y lista K; contratos en tabla I (48 B); negociaciones 60 B. ID interno de club = `(ID option << 14) | índice de bloque`.
- **Dinero:** `0xc7dd00` (presupuesto, inicial, tope). Presupuesto salarial = tope − sueldos. Se halla por ancla a `0x97d38` de la tabla A.
- **Blob:** zlib, 18 tramos de 256 KB, 30.000 fichas de 156 B (sueldo `+0x56`, valor `+0x5a`). Jugadores `0xdb65…` usan `reg & 0xffff`. Posición actual fija `0x11403a8` (riesgo en otros parches).
- **Agentes libres:** bit `0x20` en `+0x75` de la ficha + fin de contrato vacío + sin equipo.
- **Calendario:** registros de 32 B en `0x1f0000–0x28c000`. **Tablas:** filas de 20 B en `0xb00000–0xd40000` (actual y anterior). **Resultados partido a partido: no encontrados.**
- **Fichar para el usuario:** programa no escribe el contrato (el juego lo crea desde la ficha del blob).

## 8. Trampas conocidas

| Trampa | Qué hacer |
|---|---|
| Solo editar la lista del club deja al jugador vivo en Estrategia | Tocar todas las tablas del equipo y la lista K |
| Porteros en el ataque | Recalcular orden de formación y 6 roles |
| «DC 0» en blanco | Editar la alineación de 600 B del club de la IA |
| Contrato duplicado | No escribir el contrato al fichar |
| El juego guarda encima de la ranura | Anotar huellas; usar otra ranura |
| Direcciones fijas dejan de servir (el blob crece) | Buscar por anclas |
| Idea errónea: calendario en el blob | No está ahí (zona posterior al blob no cambia) |

## 9. Pendientes

1. Ver en pantalla: agentes libres, calendario, tablas (guías 13, 14, 15).
2. Resultados/goleadores: guardar justo antes y después de un partido el mismo día, en ranuras distintas.
3. Stats dentro de una carrera empezada: reiniciar el juego con un `Player.bin` cambiado.
4. Buscar la posición del blob por ancla; repetir todo en Sudamerican y otros parches.
5. Escribir **un** valor de **un** jugador en una copia de `Player.bin` (con respaldo y aprobación).
6. Aplicar un traspaso aprobado en la web solo a la carrera elegida, con aviso de Phoenix Link.

## 10. Prompt de arranque (pégalo al abrir un chat nuevo)

```
Eres un ingeniero de ingeniería inversa que continúa el proyecto Phoenix Evolution de FRALEX (PES 2021, parche ConmeGOL 26).
Antes de hacer nada: lee PhoenixSync/base-conocimiento/17-TRASPASO-LIGA-MASTER-PARA-OTRA-IA.md completo y luego los archivos en el orden de su sección 3.
Habla en español, simple, ordenado, paso a paso, define cada palabra nueva, repite lo importante, avisa seguido con mensajes cortos. Meta: 0 errores; si hay un error, no avances.
Reglas: trabaja siempre sobre COPIAS, haz ida y vuelta antes de editar, cierra en archivo NUEVO, anota huellas, un cambio es «probado» solo cuando FRALEX lo ve en el juego, actualiza PRUEBAS.md y REGISTRO.md, no hagas commit/push ni checkout de otra rama sin permiso, haz fetch + pull --rebase antes de trabajar en el repo, no subas guardados ni .bin, nunca pidas contraseñas.
Primero confirma al usuario en 5 líneas qué entendiste y qué vas a hacer, y empieza por la tarea que él te pida. Si no sabes algo, dilo: no lo inventes.
```

## 11. Cómo registrar un hallazgo nuevo

1. Etiqueta: ✅ probado en el juego · 🔎 observado en archivos · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.
2. Escribe: qué hiciste, cómo lo comprobaste, con qué herramienta, en qué guardado, resultado.
3. Añádelo a `PRUEBAS.md`, `REGISTRO.md` y, si procede, a un `NN-GUIA-….md`; copia el doc a la carpeta de Drive.
4. Avisa a FRALEX en lenguaje simple.
