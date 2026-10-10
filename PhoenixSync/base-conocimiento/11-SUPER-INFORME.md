# 11 · Súper informe: todo lo descubierto, las posibilidades y mis consejos

**Para FRALEX · 9 de octubre de 2026.**
Este informe junta **en un solo lugar** lo que descubrimos sobre PES 2021, lo que eso permite hacer y lo que te recomiendo.
Si quieres el detalle de una parte, al final (sección 7) está la lista de documentos.

> **Multiparche.** Todo se descubrió y se probó sobre **ConmeGOL Patch 26**, pero casi todo lo pone **el juego**, no el parche, así que vale para cualquier parche de PES 2021. Donde algo depende del parche, se marca con 🧩 y se explica. La guía para llevarlo a otro parche (con sus riesgos) es el informe **12 · Guía multiparche**.

---

## 0. Cómo leer este informe

- Cada parte empieza con **la idea en simple** y después da los datos.
- Los símbolos significan:
  - ✅ = **probado en el juego** (lo viste en pantalla).
  - 🔎 = **visto en los archivos**, todavía sin prueba en pantalla.
  - ❌ = **probado y no funciona**.
  - 🟢 = ya se puede hacer.
  - 🟡 = se puede, pero falta investigar.
  - 🔴 = muy difícil; puede que no salga.
  - 🧩 = depende del parche.

### Palabras que se repiten

| Palabra | Qué significa, en simple |
|---|---|
| Option file (`EDIT00000000`) | El archivo de «Editar» del juego. Guarda **las plantillas** (qué jugador está en qué equipo), nombres de equipos y jugadores editados |
| Base de datos (`pesdb`) | Los archivos internos del juego: `Player.bin` (stats de cada jugador), `PlayerAssignment.bin` (plantillas «de fábrica»), `Team.bin`, `Country.bin`… |
| Sider | Programa que se pone delante del juego y le «entrega» archivos nuestros en lugar de los originales, sin tocar el juego |
| Phoenix-DB | Nuestra carpeta dentro de Sider. Lo que pongas ahí **gana** sobre la base del parche |
| Liga Máster (`ML0000000N`) | Cada carrera es un archivo propio, como un cuaderno de esa partida |
| Cifrado | El juego guarda sus archivos «revueltos». Hay que abrirlos con la llave y volver a cerrarlos |
| Ancla | Algo conocido que se busca para ubicarse dentro de un archivo (por ejemplo, tus 5 primeros jugadores) |
| Huella (sha256 / md5) | Código que identifica un archivo exacto. Si cambia un byte, cambia la huella |

---

## 1. El mapa general (las 4 piezas)

Piensa en un restaurante:

| Pieza | Papel en el restaurante | Qué es de verdad |
|---|---|---|
| **La web** (Phoenix Evolution, Supabase) | **La receta**: dice qué fichajes se aprobaron | Tu página. Publica los cambios de la liga **firmados** (con un sello que no se puede falsificar) |
| **Phoenix Sync** | **El cocinero**: prepara los archivos | Programa en C++ que lee la receta y fabrica el option file, la base o la carrera nueva |
| **Phoenix Link** | **El mesero**: lleva el plato y avisa | Tu app de PC. Coloca los archivos y avisa dentro del juego (buzón `avisos.txt`) |
| **El juego + Sider** | **El comensal** | PES 2021, que lee lo que le dejamos |

**La regla del restaurante:** el cocinero **nunca** cocina encima del plato anterior. Siempre hace uno nuevo, lo prueba (huella) y guarda el viejo.

---

## 2. Todo lo descubierto

### A. Abrir y cerrar los archivos del juego ✅

**En simple:** todos los guardados de PES 2021 van en el **mismo tipo de sobre** y con **la misma llave**. Esto vale **para todos los parches**.

- Herramienta: **libpesXcrypter**, una biblioteca libre de la comunidad de PES.
- El sobre tiene 5 piezas: cabecera · etiqueta (384 B) · logo (17.315 B) · **datos** · serial (44 B).
- Abrir y cerrar sin cambios da un archivo **idéntico** ✅.
- El juego **acepta archivos editados**: no comprueba la huella de la cabecera ✅.
- El menú *Cargar* muestra el **texto info** (desde el byte 128 de la etiqueta), no el nombre ✅.
- Se puede cerrar con **otro tamaño** de datos ✅ (la Liga Máster crece al fichar).
- Detalle: informe **10**.

### B. El option file ✅

**En simple:** es la «libreta de plantillas» que el juego usa cuando juegas amistosos y modos normales.

| Qué guarda | Dónde | Estado |
|---|---|---|
| Plantillas: 284 B por equipo (40 jugadores + 40 dorsales); 🧩 cuántos equipos depende del parche (749 en ConmeGOL; el límite es 750) | desde `0x9D4648` | ✅ (fichajes probados: Lamine, Julián, Mbappé) |
| Tácticas: orden de formación y 6 roles por equipo (628 B) | desde `0xA09880` | ✅ (prueba Boca: Braida entró de lateral, sin porteros en el ataque) |
| Jugadores editados (312 B) | desde `0x7C` | ✅ |
| Equipos (588 B, nombre y abreviatura) | desde `0x8ED2FC` | ✅ |

- Phoenix Sync lo cambia con `OptionFile::mover` (con dorsal y **sustituto por posición**: un portero solo lo cubre otro portero).

### C. La base de datos (`Player.bin` y compañía) ✅

**En simple:** es la «ficha técnica» de cada jugador: velocidad, tiro, altura, posiciones…

- Cada jugador ocupa **312 B**. Ya están mapeadas: las 19 habilidades de campo, **potencia de tiro**, las **5 de portero**, pie malo, regularidad, resistencia a lesiones, país, altura, peso, edad y parte de las habilidades especiales ✅ (comprobado contra las pantallas con Lamine, Joan García y Livakovic).
- Formato del archivo: cabecera **WESYS** de 16 B + zlib. El juego acepta nuestra compresión ✅.
- **Phoenix-DB:** si nuestra carpeta va **antes** que la del parche en `sider.ini`, el juego usa **nuestro** archivo ✅.
  - 🧩 Tiene que estar en **cada** `sider.ini` que use tu parche. Ejemplos: en ConmeGOL, en la carpeta del juego **y** en la del modo (el switcher copia el modo encima); en Sudamerican hay **dos** Sider (normal y Mundial). Si un parche **no usa Sider**, hay que verlo aparte (informe 12).
- `PlayerAssignment.bin` (16 B por fila): jugador, equipo, dorsal, orden y banderas ✅.
- Límites de la base: **750 equipos, 40 jugadores por equipo, 30.000 jugadores**.
- 🧩 **Ojo:** cada parche guarda su base en un sitio distinto, y no siempre es el que parece. En ConmeGOL 26, la que de verdad usa el juego es la que sirve Sider desde `olmosjr23\Database` (no el `.cpk`). En Sudamerican está dentro de `SP_Subs.cpk`. **Phoenix-DB siempre se fabrica desde la base que el juego usa de verdad.**

### D. Cómo y cuándo lee el juego esos archivos ✅

**En simple:** el juego no mira los archivos todo el tiempo. Solo los relee en momentos concretos.

| Momento | ¿Relee la base y el option file? |
|---|---|
| Al arrancar el juego | ✅ todo |
| **Editar → Cargar** | ✅ Player, PlayerAssignment, Team, Coach, Competition… y el option file |
| Al armar un amistoso, abrir habilidades | ❌ no |
| Al entrar a la Liga Máster | ❌ no lee `Player.bin` |

**Las 3 reglas de oro** (son del juego: valen para todos los parches):
1. **Stats** → van por `Player.bin` (Phoenix-DB).
2. **Plantillas** → dependen de la opción **«Datos Actual. en vivo»** del juego:
   - **Activada:** el juego toma las plantillas de la **base** (`PlayerAssignment.bin`).
   - **Desactivada:** las toma del **option file**.
   - Por eso, para que un fichaje se vea **siempre**, se pone en **los dos** (así lo hicimos con Mbappé).
3. Si un jugador está entre los **editados** del option file, **mandan sus datos editados**, no los de `Player.bin`.

**El botón para aplicar todo:** **Editar → Cargar** (sin pulsar *Guardar*).

### E. La Liga Máster ✅

**En simple:** cada carrera es un cuaderno propio. Los cambios del option file o de la base **no llegan** a una carrera ya empezada: hay que escribir en su cuaderno.

| Qué controlamos | Estado |
|---|---|
| Traspasos entre dos equipos de la IA (plantilla + alineación) | ✅ |
| Vender un jugador de **tu** equipo a la IA (igual que lo hace el juego) | ✅ |
| Fichar **para** tu equipo desde la IA | ✅ (Sommer jugó de titular, un solo contrato) |
| Presupuesto de fichajes y tope salarial | ✅ (500 M en pantalla) |
| Valor de mercado de un jugador | ✅ (Sommer 77,7 M) |
| Que todo aguante un partido y un guardado | ✅ |
| Stats dentro de una carrera ya empezada | ❌ todavía no (falta la prueba de reiniciar) |
| Agentes libres | 🟡 pendiente |
| Calendario de partidos | 🟡 pista encontrada (10 números de partido por jornada) |

Lo que hay dentro del cuaderno:
- **Los equipos** (plantillas y dorsales) y **las alineaciones** de la IA, guardadas aparte. 🧩 En ConmeGOL son 700 y 629; en otro parche pueden cambiar. Phoenix Sync los **busca por anclas** y no supone números fijos.
- **Tu equipo**: 12 tablas por jugador, contratos, negociaciones, orden de formación y Estrategia.
- **Dinero**: solo lo tiene tu club. **La IA no tiene dinero guardado**: ficha por reglas.
- **El «blob»**: 18 trozos comprimidos con una **ficha por jugador** (sueldo, valor, contrato, competiciones) y resultados de la temporada.
- **Competiciones**: listas de equipos con espacio de sobra (ya hay una liga de 30 equipos).
- 🧩 Probado solo en ConmeGOL 26. En otro parche hay que repetir las pruebas en una carrera nueva (informe 12).
- Detalle: informes **08** y **09**.

### F. Lo que el juego hace por su cuenta

- **Autoguarda encima** de la ranura cargada al terminar un partido → siempre se trabaja en otra ranura y se anota la huella.
- Hace sus propios fichajes de la IA (cientos el último día de mercado).
- Crea jugadores nuevos (los «regens»).
- **Crea el contrato** de un fichaje a partir de la ficha del blob → nosotros **no** lo escribimos (si no, queda duplicado).
- **Relee el guardado** al cargar la ranura → se puede cambiar con el juego abierto, si esa ranura no está cargada.

### G. La conexión con la web ✅

- La web publica los cambios de la liga **firmados (Ed25519)**. Phoenix Sync comprueba la firma con la clave pública incrustada: si alguien cambia algo por el camino, se rechaza ✅.
- `PhoenixSync sincronizar` aplica los cambios al option file **y/o** a la Liga Máster, **todo o nada**, y siempre en archivos nuevos.
- Probado en tu PC con `PROBAR2.bat`: conexión OK, firma OK, **295 de 295** pruebas automáticas ✅.
- Phoenix Link ya tiene el **buzón de avisos** (`avisos.txt` → mensaje dentro del juego) ✅.

### H. Qué vale para todos los parches y qué no 🧩

| Lo pone el juego (igual en todos los parches) | Cambia en cada parche |
|---|---|
| El sobre y la llave de los guardados | La **carpeta de guardados** (número de Steam: ConmeGOL 239200, Sudamerican 292733975847239680) |
| El formato del option file y de `Player.bin` | **Dónde está la base** (Sider o CPK) y su orden de carga |
| La estructura de la Liga Máster | Los **IDs** de equipos y jugadores, y cuántos hay |
| Editar → Cargar, la regla de «Datos Actual. en vivo», los editados mandan | Si usa **Sider**, cuántos `sider.ini` tiene y si un switcher o lanzador los pisa |
| El autoguardado y el contrato que crea el juego | Cuántos jugadores trae **editados** en su option file |
| — | Si trae un **exe modificado** (por ejemplo, Football Life) |

Detalle y guía paso a paso: informe **12**.

### I. Lo que NO funciona (para no repetirlo)

| Intento | Por qué no |
|---|---|
| Fichajes solo con `PlayerAssignment.bin` y la opción desactivada | Con la opción desactivada mandan las plantillas del option file |
| Cambiar el option file o la base para una carrera ya empezada | La carrera tiene su propio cuaderno |
| Cambiar stats escribiendo en la memoria del juego | La pantalla lee otra copia; además, leer memoria a lo bruto cerraba el juego |
| La actualización en vivo oficial de Konami | Sus servidores cerraron en 2022 |
| Cambiar el número de equipos de una liga sin más | El calendario depende de ese tamaño (en foros, la Liga Máster se colgaba en la temporada 2) |
| Escribir el contrato al fichar en la Liga Máster | El juego crea el suyo y queda duplicado |

---

## 3. Las posibilidades

### 🟢 Ya se puede (con lo que ya está probado)

1. **Mercado de fichajes conectado a la web.** Un fichaje aprobado en tu página → Phoenix Sync lo prepara → Phoenix Link lo coloca y avisa → tú pulsas **Editar → Cargar**.
2. **Liga Máster «viva».** La web mete fichajes, ventas, presupuesto o valor de mercado en una carrera entre partido y partido.
3. **Phoenix Weekly.** Stats ajustadas cada semana (por ejemplo, la forma real del jugador) con `Player.bin`, siempre con tu aprobación.
4. **Liga online justa.** Todos con el mismo option file y la misma base, comprobados por huella antes de jugar (verificador del anfitrión).
5. **Option file por sala o por liga.** Phoenix Link carga el que toca según la sala.
6. **Avisos dentro del juego.** «Fichaje aplicado», «nueva jornada», «tu partido es hoy».
7. **Resultado automático.** Un módulo de Sider deja el marcador final en un archivo y Phoenix Link lo sube a la web.
8. **Phoenix Doctor.** Revisa `sider.ini` y `sider.log` y te dice qué está mal antes de jugar.
9. **Eventos.** Jugador de la semana, castigos por lesión, homenajes, todo con `Player.bin`.

### 🟡 Se puede, pero falta investigar

1. **Stats dentro de la Liga Máster** (primero, la prueba de reiniciar el juego).
2. **Agentes libres** en la Liga Máster.
3. **Calendario de la Liga Máster** desde la web.
4. **Crear jugadores que no existen** (juveniles, jugadores reales que faltan): registros nuevos en `Player.bin` + su fila de plantilla + su cara.
5. **Competiciones propias** (Liga Phoenix, copas de la web): descifrar los archivos `Competition*` comparando antes y después, como hicimos con la Liga Máster.
6. **Estadísticas completas a la web** (goleadores, tarjetas, posesión).
7. **Fabricar nuestros propios CPK** (paquetes del juego) y **actualizaciones por partes** (solo lo que cambió). También abriría la puerta a parches que no usan Sider.
8. **Cambiar más archivos de la base en vivo:** entrenadores (`Coach.bin`), clubes (`Team.bin`), estadios, botines.
9. **Phoenix en todos los parches:** probar todo en Sudamerican, ConmeGOL 27, Football Life y otros (guía 12).

### 🔴 La frontera (lo que nadie ha hecho)

1. **Liga Máster online asíncrona.** Varios jugadores, cada uno con su club en su propia carrera, compartiendo **un mismo mercado y una misma tabla** en la web. Es la idea **más original** y, con lo que ya sabemos de la Liga Máster, **la más cercana**.
2. **El mundo real dentro del juego:** lesiones y forma de la Liga 1 real aplicadas cada semana.
3. **Espectadores en vivo:** marcador del partido en la web mientras se juega.
4. **Servidor propio** estilo PESBUL: meses o años de trabajo.

---

## 4. Mis consejos (en orden)

0. **Piensa siempre en multiparche.** Antes de cada guía o herramienta, pregúntate: ¿esto lo pone el juego o el parche? Lo del parche va en su **ficha** (`parches/<parche>.md`), nunca escrito a fuego en el programa.
1. **Primero, estabilidad.** Antes de sumar cosas nuevas, deja el sistema **limpio y probado**:
   - vuelve `phoenix.lua` a la versión **v02** (solo el buzón de avisos) en las dos carpetas `modules`;
   - deja Phoenix-DB con datos **originales** salvo lo que estés probando;
   - guarda la huella de cada archivo que funciona.
2. **Termina el «repartidor» de Phoenix Link** (el prompt ya está hecho): que coloque los archivos, haga respaldo y avise. Es la pieza que une todo.
3. **Usa siempre Editar → Cargar** como el botón oficial de tu sistema. Es nativo, estable y existe en todos los parches.
4. **Fichajes en los dos sitios** (option file + `PlayerAssignment.bin`). Así se ven con la opción en vivo activada o desactivada.
5. **Haz la prueba de stats en la Liga Máster** (reiniciar el juego con un `Player.bin` nuevo). Es la duda más importante que queda en la Liga Máster.
6. **Una base por parche, y sin mezclar.** Cada parche tiene su propia base, su catálogo y su option file (en ConmeGOL hoy es la de `olmosjr23`). Si el parche se actualiza, o cambias de parche, **regenera** Phoenix-DB desde esa base **antes** de jugar (si no, servirías datos viejos o de otro parche).
7. **Una prueba a la vez.** Cambia una sola cosa, pruébala, anótala en `PRUEBAS.md`. Si algo falla, vuelves al respaldo sin perder nada.
8. **Nunca pulses «Guardar» en Editar** durante las pruebas, y **juega la Liga Máster en otra ranura** que la de prueba.
9. **Pide permiso y da crédito** por el contenido de otros (caras, kits, base de Olmos). Lo tuyo (herramientas, web, conexión) es 100 % tuyo.
10. **Apunta a la frontera, pero paso a paso.** La Liga Máster online asíncrona es tu gran carta: empieza por «un jugador, una carrera, conectada a la web», y después suma más.

> Sobre los experimentos de parches en memoria del ejecutable (el «botón nativo»): **no forman parte de este informe**. No los recomiendo para tu sistema ni les doy soporte. El camino recomendado es **Editar → Cargar**.

---

## 5. Cómo trabajamos (el método que funcionó)

1. **Copia** del archivo original (nunca se toca el original).
2. **Abrir** y comprobar la ida y vuelta.
3. **Verdad del juego:** tú haces la acción real en el juego y guardas en otra ranura.
4. **Comparar** el antes y el después, byte a byte.
5. **Programar** lo mismo y comparar con lo del juego hasta que salga igual.
6. **Probar en el juego** y anotar el resultado, salga bien o mal.

**Un cambio solo se da por bueno cuando lo ves en el juego.** Y en cada parche nuevo, se vuelve a probar.

---

## 6. Las reglas que nunca se rompen

1. Siempre respaldo; siempre archivo nuevo; nunca sobrescribir.
2. Siempre comprobar la huella del archivo entregado **en tu PC**.
3. Si hay un error, **no se avanza**.
4. Si algo puede romperse, **mejor no se hace**.
5. Los guardados y los datos abiertos **no se suben a GitHub**.
6. Los cambios de la web y de Supabase se piden por prompt al chat WEB.
7. Toda guía es **multiparche**: separa lo del juego de lo del parche y dice sus riesgos.

---

## 7. Dónde está cada cosa (carpeta `PhoenixSync/base-conocimiento/`)

| Documento | De qué trata |
|---|---|
| 01 · Motor de PES 2021 | Todos los formatos: option file, Liga Máster, `Player.bin` |
| 02 · Parche Phoenix | De qué está hecho un parche y qué falta para el tuyo |
| 03 · Investigación de parches | Causas de crash y reglas Phoenix |
| 04 · Estrategia Phoenix | ConmeGOL y Sudamerican por dentro, y cómo superarlos |
| 05 · Puerta en vivo | Cambiar la base sin reiniciar (Editar → Cargar) |
| 06 · Ruta del descubrimiento | La noche del 8 al 9 de octubre, etapa por etapa |
| 08 · Liga Máster: informe y guía | Qué controlamos y cómo se usa |
| 09 · Liga Máster: cómo lo hicimos | Paso a paso, con herramientas |
| 10 · Cómo se abrió el guardado | El sobre, la llave y cómo se cierra |
| 11 · Este súper informe | Todo junto + posibilidades + consejos |
| 12 · Guía multiparche | Cómo llevar todo a cualquier parche, con riesgos |

Además: `PhoenixSync/PRUEBAS.md` (diario de pruebas), `PhoenixSync/REGISTRO.md` (bitácora) y `liga-master/ESTRUCTURA-ML.md` (detalle de bytes).
