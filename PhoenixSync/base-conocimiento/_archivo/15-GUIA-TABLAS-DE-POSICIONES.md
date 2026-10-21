# 15 · Guía exacta: cómo se descifraron las tablas de posiciones de la Liga Máster

**Para FRALEX · 9 de octubre de 2026, 22:10 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §23. Diario: `PRUEBAS.md` (9 oct, 22:10). Viene de la guía 14 (calendario).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

## 0. En simple

Cada liga de la carrera guarda **su tabla de posiciones**, como la que ves en el juego:

- puesto, puntos, ganados, empatados, perdidos;
- goles a favor, goles en contra, partidos jugados;
- goles y victorias **de visitante**.

Y guarda **dos tablas**: la de **ahora** y la de **la jornada anterior** (para saber quién subió o bajó).

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Carrera del City (fD, 22/9) ya abierta en la nube | La Premier con 1 jornada jugada |
| Carrera del Barça (ranura 1 de tu PC, 4/3/2026), copiada sin tocar el original | Ligas con muchas jornadas (Ligue 1 con 8) |
| El calendario descifrado (guía 14) | Para **cruzar** cada partido con la tabla |
| Python 3 | Buscar, leer y comprobar |
| `prototipos/mlparse.py` | Nombres de los equipos |

---

## 2. Paso a paso

### Paso 1 · Buscar al City en las zonas de la temporada
- **Cómo:** se buscó el **ID interno del City** (`0x2b409a`) en la zona que cambia con la temporada (`0xb00000–0xd40000`).
- **Resultado:** apareció en una lista de **20 clubes de la Premier**, uno cada 20 bytes, **ordenados** (Hull, City, Brentford, Aston Villa…), cada uno con un número de puesto (1, 2, 2, 4, 5, 6, 6, 8…). Los empates comparten puesto, igual que en el juego.

### Paso 2 · Descifrar los números de cada fila
- Cada fila tiene **20 bytes**: club, puesto y tres números «empaquetados» (varios datos dentro de un mismo número).
- **Cómo:** se probaron cortes de bits hasta que todo tuvo sentido:

| Número | Qué guarda dentro |
|---|---|
| C | puntos · ganados · perdidos · empatados · ganados de visitante |
| A | goles a favor · goles en contra · partidos jugados |
| B | goles de visitante |

### Paso 3 · Primera comprobación: los goles tienen que cuadrar
- **Idea:** en una liga, todos los goles que alguien marca los recibe otro. Así que **suma de goles a favor = suma de goles en contra**.
- **Resultado:** cuadra en **todas** las tablas (por ejemplo, 195 = 195, 171 = 171, 212 = 212, 220 = 220, 64 = 64). ✅

### Paso 4 · Segunda comprobación: cruzar con el calendario
- **Cómo:** se tomaron los 10 partidos de la **jornada 1** de la Premier (del calendario, guía 14). Para cada partido se comparó: goles a favor del local = goles en contra del visitante, y al revés.
- **Resultado: 10 de 10 cuadran** ✅:

| Partido | Resultado |
|---|---|
| City – Crystal Palace | 3–0 |
| Ipswich – Everton | 2–2 |
| Bournemouth – Brentford | 0–3 |
| Liverpool – Brighton | 0–0 |
| Nottingham Forest – Newcastle | 0–1 |
| Sunderland – Leeds | 0–2 |
| Arsenal – Chelsea | 0–1 |
| Aston Villa – Coventry | 3–1 |
| Fulham – Tottenham | 2–2 |
| Hull – Manchester United | 4–1 |

- Además, los que ganaron **de visitante** (Brentford, Leeds, Chelsea, Newcastle) tienen marcado «ganado de visitante», y los goles de visitante coinciden. ✅

### Paso 5 · Las dos tablas
- **Cómo:** en la carrera del Barça, la Ligue 1 aparece dos veces seguidas: una con **8** partidos jugados y otra con **7**.
- **Conclusión:** la primera es la **actual** y la segunda la de la **jornada anterior**. ✅ (en archivos)

### Paso 6 · Tabla de la Ligue 1 en tu carrera del Barça
| Puesto | Club | PJ | G | E | P | GF | GC | Pts |
|---|---|---|---|---|---|---|---|---|
| 1 | Lille | 8 | 7 | 1 | 0 | 19 | 4 | 22 |
| 2 | PSG | 8 | 6 | 2 | 0 | 17 | 4 | 20 |
| 3 | Lens | 8 | 6 | 1 | 1 | 20 | 8 | 19 |
| 4 | Lorient | 8 | 5 | 1 | 2 | 19 | 11 | 16 |
| 5 | Marsella | 8 | 4 | 2 | 2 | 11 | 8 | 14 |
| 6 | Mónaco | 8 | 4 | 2 | 2 | 9 | 6 | 14 |

- En esa carrera, **LaLiga todavía no empezó** (su tabla está vacía).

---

## 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| La forma de la fila (20 B) y cómo se empaquetan los números | ✅ La pone el juego |
| Las dos tablas (actual y anterior) | ✅ Lo hace el juego |
| Dónde está la zona y en qué orden van las ligas | 🧩 Depende del parche → el programa debe buscar por la forma |

---

## 4. Qué tienes que mirar en el juego (para pasar de 🔎 a ✅)

En tu carrera del **Barça** (ranura 1), abre la **tabla de la liga francesa** y mira si coincide con la tabla del paso 6.

---

## 5. Qué abre este avance

- **Tablas de todas las ligas de una carrera, directo a tu web**, sin capturas de pantalla.
- Con el calendario (guía 14): **próximos partidos** y, comparando dos guardados, **resultados**.
- ⏳ Falta: dónde se guarda cada resultado partido a partido (si es que se guarda), goleadores (hay una pista: registros con jugador + club + número) y las tablas de 40 filas que todavía no entendemos.
