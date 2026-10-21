# 14 · Guía exacta: cómo se encontró el calendario de la Liga Máster

**Para FRALEX · 9 de octubre de 2026, 21:45 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §22. Diario: `PRUEBAS.md` (9 oct, 21:45).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

## 0. En simple

El calendario de una carrera es como **un cuaderno de partidos numerados**:

- Cada partido de la temporada tiene **un número** (en la carrera del City, del 0 al 5.758).
- Cada competición tiene **su tramo de números**. Por ejemplo, la Premier va del 5.531 al 5.720.
- Cada partido guarda: **local**, **visitante**, **competición**, **jornada** y su **número**.

Lo que **todavía no** encontramos es dónde se apuntan **los goles** (los resultados).

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Guardados del City ya abiertos en la nube: r0 (4/8), g18 (19/8), g19 (21/8), fD (22/9) | Comparar la carrera en distintas fechas |
| **Tu PC** (enlace de la app): `ML00000000` (ranura 1) y `ML00000001` (ranura 2), copiados a la nube sin tocar los originales | Tener una **segunda carrera** (Barça, marzo de 2026) para comprobar |
| `dec` | Abrir los guardados (con ida y vuelta idéntica ✅) |
| `info` | Leer la etiqueta del guardado («FC Barcelona / LaLiga EA Sports · 4/3/2026 · UEFA Champions League») |
| Python 3 + NumPy | Buscar, contar y comparar byte a byte |
| `prototipos/mlparse.py` | Nombres e IDs de los 700 equipos |

---

## 2. Paso a paso

### Paso 1 · Descartar el blob
- Del avance anterior (guía 13): el blob es **solo** la caja de carnets de jugador. El calendario tenía que estar **fuera**.

### Paso 2 · Usar el «ID interno de club» como pista
- **Qué es:** un número que junta el ID del option file y el número de bloque del equipo: `ID option × 16.384 + bloque`.
- **Cómo:** en una zona que cambiaba después de un partido aparecieron números raros. Al «desarmarlos» con esa fórmula salieron **clubes de verdad**: Blooming, César Vallejo, Ayacucho, Cantolao, Mannucci… (Liga 1 de Perú). ✅
- **Conclusión:** el juego usa ese ID en las tablas de la temporada.

### Paso 3 · Buscar ese ID en todo el guardado
- **Cómo:** se recorrió el archivo entero buscando cualquier número que sea un ID interno válido (27.287 apariciones) y se contó cuántas caen en cada zona.
- **Resultado:** una zona muy densa entre `0x1f0000` y `0x28c000`.

### Paso 4 · Encontrar los registros de partido
- **Cómo:** dentro de esa zona se buscaron **dos IDs de club seguidos** (local y visitante).
- **Resultado:** aparecieron **Sunderland – Everton**, **Arsenal – Brentford**, **Aston Villa – Brighton**… con un número detrás (5.683, 5.684, 5.685…).
- **Forma del registro** (cada 32 bytes):

| Parte | Qué guarda |
|---|---|
| Local | ID interno del club |
| Visitante | ID interno del club |
| Número de partido | Global (único en la carrera) |
| Competición | Número de la competición (Premier = 30 en la carrera del City) |
| Código | Jornada (6 bits) + orden dentro de la jornada |
| 8 bytes «del usuario» | Solo se rellenan en **tus** partidos |

### Paso 5 · Mapa completo de competiciones
- **Cómo:** se leyeron **todos** los registros (5.273 de liga) y se agruparon por competición.
- **Resultado (carrera del City):** cada competición ocupa un tramo seguido. Ejemplos:

| Competición (nº) | Partidos | Tramo | Qué parece |
|---|---|---|---|
| 17 | 435 | 0–434 | Liga de 30 equipos a una vuelta (Argentina) |
| 18, 19, 80, 82 | 380 cada una | — | Ligas de 20 equipos a dos vueltas |
| 30 | 190 | 5.531–5.720 | **Premier** (primera vuelta) |

- **Tus partidos de Premier (City):** jornada 1 **City – Crystal Palace**, 2 **Everton – City**, 3 **City – Brentford**, 4 **Brighton – City**, 5 **City – Newcastle**.

### Paso 6 · Comprobar en tu otra carrera (Barça)
- **Cómo:**
  1. Se copió `ML00000000` de tu PC a la nube (el original no se tocó).
  2. Se abrió con `dec`: **ida y vuelta idéntica** ✅.
  3. `info` mostró que es **otra carrera**: Barça, 4/3/2026.
  4. Se corrió la **misma búsqueda**.
- **Resultado:** LaLiga completa (**380** partidos, 38 jornadas, números 0–379), con el mismo formato. ✅

| Jornada | Partido |
|---|---|
| 1 | **Barça – Real Sociedad** |
| 2 | Deportivo – Barça |
| 3 | Barça – Celta |
| 10 | **Real Madrid – Barça** |

### Paso 7 · ¿Dónde van los goles?
- **Cómo:** se compararon los mismos registros al inicio (4/8), el 21/8 y el 22/9 de la carrera del City.
- **Resultado:** **no cambian** al jugarse. El marcador **no** va en el registro del partido.
- **Zonas descartadas como resultados:**
  - `0x66fa2c…`: registros de 16 B con jugador y algo parecido a una fecha (¿lesiones o sanciones?).
  - `0x10c0000…`: una **imagen** guardada dentro del archivo.
- **Estado:** ⏳ resultados sin localizar.

### Paso 8 · Copas
- En la misma zona están los **grupos** de las copas: 4 clubes por grupo y 6 números de partido por grupo. Entre el 4/8 y el 22/9 se rellenaron (el sorteo).

---

## 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| La forma del registro de partido (32 B) | ✅ La pone el juego |
| El ID interno de club | ✅ Lo pone el juego |
| Los **números de competición** (Premier 30, LaLiga 29…) | 🧩 Dependen del parche y de la carrera |
| Los **tramos** de números de partido | 🧩 Dependen de qué competiciones trae el parche |
| La zona donde está (`0x1f0000–0x28c000`) | ⚠️ 🧩 Medida en ConmeGOL. El programa debe **buscar la forma**, no la dirección |

---

## 4. Qué tienes que mirar en el juego (para pasar de 🔎 a ✅)

1. Carga la carrera del **Barça** (ranura 1), entra a **Calendario** de LaLiga y mira si:
   - jornada 1: Barça – Real Sociedad (en casa);
   - jornada 2: Deportivo – Barça (fuera);
   - jornada 10: Real Madrid – Barça (fuera).
2. **Para encontrar los resultados:** el día de un partido, guarda en una ranura **justo antes** de jugar, juega y guarda en **otra ranura** justo después (sin avanzar el día).

---

## 5. Qué abre este avance

- **Leer el calendario** de cualquier carrera y mandarlo a la web (próximos partidos de cada liga).
- 🟡 Con los resultados localizados: tablas, goleadores y resultados en la web, solos.
- 🔴 Más adelante: **cambiar** el calendario desde la web (por ejemplo, para una liga propia). Riesgo alto: no tocar sin pruebas.
