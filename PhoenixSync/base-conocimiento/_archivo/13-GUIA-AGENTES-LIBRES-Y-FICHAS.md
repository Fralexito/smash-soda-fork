# 13 · Guía exacta: agentes libres, fichas `0xdb65` y qué es el blob (Liga Máster)

**Para FRALEX · 9 de octubre de 2026, 21:00 (Lima).**
Esta guía cuenta **cada paso** del avance de hoy: qué se hizo, **cómo se comprobó** y **con qué herramienta**.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §21. Diario: `PRUEBAS.md` (entrada del 9 oct, 21:00).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

## 0. En simple

Cada jugador de una carrera tiene una **ficha de Liga Máster** (como un carnet) dentro de la zona comprimida del guardado (el «blob»).
Hoy aprendimos tres cosas:

1. **Cómo se reconoce a un agente libre** (jugador sin club).
2. **Dónde están los carnets** de unos 1.000 jugadores especiales que el programa no encontraba (y se arregló).
3. **Que el blob es solo la caja de carnets**: el calendario está en otra parte.

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| 7 guardados de tu carrera **ya abiertos** en la nube (copias; tu PC no se tocó) | r0 (4/8), g18 (19/8), g19 y «juego» (21/8), j9 (31/8, antes de despedir a Stones), jA (31/8, después), fD (22/9) |
| `prototipos/blob.py` | Descomprimir el blob de cada guardado |
| Python 3 (scripts de una sola vez) | Contar, comparar byte a byte y cruzar datos |
| `catalogo_ok.json` | Ponerle **nombre** a cada ID de jugador |
| `core/BlobLM.cpp` (C++) | El arreglo del programa |
| `pruebas/main.cpp` + CMake + g++ | Compilar y correr las pruebas automáticas con guardados reales |

**Cómo se supo la fecha de cada guardado:** leyendo la fecha de la partida, que está en el byte `0xacc61c` de los datos (año, mes y día).

---

## 2. Paso a paso

### Paso 1 · Buscar a Stones antes y después de despedirlo
- **Qué:** j9 (antes) y jA (después de que **el juego** lo despidiera, la «verdad del juego»).
- **Cómo:**
  1. Se leyó su número en la carrera desde la plantilla del City: **reg 4346**, ID **47785**.
  2. Se buscaron todas las veces que aparece la pareja `(4346, 47785)` en los dos archivos.
- **Resultado:** antes aparecía en **20** sitios; después, en **7**. Ya no está en el bloque del City, pero sí en listas de noticias, caja, historial y negociaciones.

### Paso 2 · Comparar su carnet (ficha de 156 B) antes y después
- **Cómo:** se descomprimió el blob de los dos guardados (`blob.py`) y se compararon los 156 bytes del carnet de Stones, uno por uno.
- **Resultado (solo cambiaron 21 bytes en todo el blob):**

| Parte del carnet | Antes | Después |
|---|---|---|
| Competiciones inscritas | 6 | 3 (se quitaron las del City) |
| Club anterior (+0x5e) | vacío | **City** |
| Fin de contrato (+0x66) | 31/8/2027 | **vacío** |
| Byte +0x75 | 0x05 | **0x25** (se encendió el bit 0x20) |

### Paso 3 · ¿Ese bit marca a TODOS los libres?
- **Cómo:** se recorrieron **todos los carnets** de la carrera y se cruzaron tres datos: ¿está en algún equipo?, ¿tiene el bit 0x20?, ¿tiene fin de contrato?
- **Resultado:**

| Guardado | Sin equipo + bit 0x20 + sin fin de contrato |
|---|---|
| j9 (antes) | **94** |
| jA (después) | **95** (los 94 + Stones) |

- **Con el catálogo** se vio quiénes son los 94: los agentes libres que trae Konami (L. Manas, J. Rekarte, E. Gois…), sin club anterior.
- **Conclusión:** agente libre = **bit 0x20 de +0x75** + **fin de contrato vacío** + **en ningún equipo**. ✅ (en archivos; en pantalla, ⏳)

### Paso 4 · Encontrar el segundo grupo de carnets
- **Qué pasó:** al mirar justo después de los 16.422 carnets conocidos, apareció **otro grupo de 9.834 carnets** del mismo tamaño, con números de jugador que empiezan por `0xdb65`.
- **Cómo se comprobó:**
  1. Los números van **seguidos**, del `0xdb654026` al `0xdb65668f`.
  2. `0x4026` = **16.422**: el segundo grupo empieza **justo** donde acaba el primero.
  3. Se tomaron **todos** los jugadores `0xdb65` que están en alguna plantilla (788 en g19) y se buscó su carnet con la regla «posición = parte baja del número». **Los 788 coincidieron** con su ID.
- **Conclusión:** es **la misma lista**; para esos jugadores se usan los 16 bits bajos de su número. ✅

### Paso 5 · Arreglar el programa
- **Problema:** `BlobLM::fichaDe` usaba el número completo (`0xdb65…`), calculaba una posición enorme y respondía «no está». Con eso **no se podía fichar** a esos jugadores para tu equipo.
- **Arreglo:** si el número empieza por `0xdb65`, se usa su parte baja (`reg & 0xffff`). Es **una línea**.
- **Prueba nueva** (`pruebas/main.cpp`): «todo jugador de toda plantilla tiene su carnet en el blob».
- **Cómo se comprobó:**
  1. Se compiló con CMake + g++ **sin errores**.
  2. Pruebas sin archivos: **211/211** ✅.
  3. Pruebas con 4 guardados reales:

| Guardado | Con carnet | Sin carnet | Jugadores `0xdb65` |
|---|---|---|---|
| r0 (4/8) | 17.121 | **0** | 378 |
| g19 (21/8) | 17.551 | **0** | 793 |
| jA (31/8) | 17.900 | **0** | 1.127 |
| fD (22/9) | 17.947 | **0** | 1.174 |

  Antes del arreglo, los de la última columna habrían salido «sin carnet».

### Paso 6 · Descubrir que el blob es solo la caja de carnets
- **Cómo:**
  1. Se midió el blob: cabecera de 30 B + espacio para **30.000 carnets** de 156 B (= el **límite de jugadores** del juego). Ocupados: 26.255.
  2. Se comparó la zona **después** de los carnets entre el 19/8 y el 21/8, y entre el 4/8 y el 22/9 (casi dos meses de temporada).
- **Resultado:** esa zona **no cambia nunca** (solo carnets vacíos).
- **Conclusión:** el blob **no** guarda el calendario. Lo que cambiaba con cada jornada eran las **estadísticas de temporada** dentro de los carnets. ✅ Esto **corrige** una idea anterior (ESTRUCTURA-ML §14).

### Paso 7 · Primera mirada fuera del blob
- **Cómo:** se comparó el resto del guardado entre el 19/8 y el 21/8 (con un partido tuyo por medio).
- **Resultado:** además de lo conocido (plantillas, alineaciones, tus tablas), cambió una zona de registros de 20 B con jugador e ID (cerca de `0xccdbf0`). 🔎 Parece ser «jugadores destacados» o valoraciones de partido, **no** el calendario.

---

## 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| El bit 0x20 de agente libre y el fin de contrato vacío | ✅ Lo pone el juego |
| La regla `reg & 0xffff` para los `0xdb65` | ✅ Lo pone el juego |
| Que el segundo grupo empiece en **16.422** | 🧩 Depende de cuántos jugadores trae el parche. La regla no cambia |
| El blob = 30.000 carnets | ✅ Límite del juego |
| La **posición** del blob en el archivo (`0x11403a8`) | ⚠️ 🧩 Está **fija** en el programa (medida en ConmeGOL 26). En un parche con otro número de equipos o jugadores podría moverse. **Pendiente:** buscarla por ancla |

**Riesgo:** nada de esto se probó todavía en otro parche ni en la pantalla del juego.

---

## 4. Lo que falta (pruebas tuyas en el juego, en ranuras de prueba)

1. **Fichar a un agente libre** y guardar en otra ranura → para programar el fichaje de libres.
2. **Guardar justo antes y justo después de un partido, el mismo día** → para encontrar el calendario y los resultados.
3. **Reiniciar el juego con un `Player.bin` cambiado** y cargar una carrera → stats dentro de la Liga Máster.
