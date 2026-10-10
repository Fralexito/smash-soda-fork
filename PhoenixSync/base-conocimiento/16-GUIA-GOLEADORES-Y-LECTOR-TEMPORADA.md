# 16 · Guía exacta: goleadores, el «lector de temporada» y el error del prefijo

**Para FRALEX · 9 de octubre de 2026, 23:20 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §24. Diario: `PRUEBAS.md` (9 oct, 23:20). Viene de las guías 14 (calendario) y 15 (tablas).

Símbolos: ✅ comprobado · 🔎 visto en archivos (sin prueba en pantalla) · ⏳ falta · ⚠️ riesgo · 🧩 depende del parche.

---

## 0. En simple

1. **Goleadores:** cada competición guarda listas de jugadores con su puesto y una cantidad. La primera lista son los **goleadores**; la segunda parece ser de **asistencias**.
2. **Lector de temporada:** el programa Phoenix Sync ya sabe **leer solo** el calendario, las tablas y los goleadores de cualquier carrera. **Solo lee**: no cambia nada.
3. **Un error encontrado y arreglado:** en tu carrera del Barça, el programa no encontraba la ficha de 1.156 jugadores. Ya está corregido.

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| 6 guardados reales: r0 (4/8), g19 (21/8), jA (31/8), fD (22/9) de la carrera del City, y las ranuras 1 (Barça) y 2 (City) de tu PC | Probar con carreras y fechas distintas |
| `catalogo_ok.json` | Poner nombre a cada jugador |
| Python 3 | Encontrar y entender las listas |
| C++ (`core/TemporadaLM`, `core/BlobLM`, `core/LigaMaster`) | El lector nuevo y el arreglo |
| `pruebas/main.cpp` + CMake + g++ | Pruebas automáticas con los 6 guardados |
| MinGW | Comprobar que compila para Windows |
| Tu PC (solo mirar) | Ver qué `Player.bin` hay en Phoenix-DB (v99) y cuándo arrancó el juego |

---

## 2. Paso a paso

### Paso 1 · Encontrar listas de jugadores
- **Cómo:** se buscaron filas de 20 bytes con la forma `[número del jugador][ID][club][puesto][cantidad]`, una detrás de otra.
- **Resultado:** 74 listas en la carrera del City (22/9), **varias por competición**.

### Paso 2 · Saber cuál es la de goleadores
- **Cómo:** se miró la Premier de esa carrera, que tenía **una sola jornada jugada** (27 goles en total, según la tabla de la guía 15).
- **Resultado:** la primera lista suma **27**, y **club por club** coincide con los goles de la tabla:

| Club | Goles en la tabla | Goleadores en la lista |
|---|---|---|
| City | 3 | Foden 2 + Semenyo 1 |
| Hull | 4 | 4 jugadores con 1 |
| Aston Villa | 3 | Rogers, Buendía y otro |
| Brentford | 3 | Schade 2 + Milambo 1 |

- **Conclusión:** la primera lista = **goleadores** ✅ (en archivos).
- La segunda suma 15 y parece de **asistencias** 🔎. La tercera (41) y la cuarta (21) todavía no se sabe qué son (¿tarjetas?) ⏳.
- **Detalles que encajan:** en la Community Shield, City 5 – Arsenal 1 (Marmoush 3). En Brasil, **Haaland en el Santos con 3 goles**: el traspaso que hicimos nosotros.

### Paso 3 · Pasarlo al programa (solo lectura)
- **Qué:** un módulo nuevo, `core/TemporadaLM`, que lee **partidos**, **tablas** y **listas**.
- **Cómo evita equivocarse:** solo acepta una tabla si sus números **cuadran entre sí**:
  - puntos = 3 × ganados + empatados;
  - jugados = ganados + empatados + perdidos;
  - goles a favor de todos = goles en contra de todos.
- **Multiparche:** todo se busca **por la forma**, nunca por una dirección fija.

### Paso 4 · Probarlo con tus 6 guardados
- **Prueba:** en cada liga con **una sola jornada jugada**, cada partido de la jornada 1 tiene que cuadrar con la tabla (goles del local = goles en contra del visitante, y al revés).
- **Resultado:**

| Guardado | Partidos cruzados | Cuadran |
|---|---|---|
| g19 (21/8) | 35 | **35** ✅ |
| jA (31/8) | 152 | **152** ✅ |
| fD (22/9) | 38 | **38** ✅ |

- Y siempre hay una lista de goleadores que suma **exactamente** los goles de esas tablas. ✅

### Paso 5 · El error de la carrera del Barça
- **Qué pasó:** la prueba «todo jugador tiene su ficha» **falló** en la carrera del Barça: **1.156** jugadores sin ficha.
- **Por la regla «si hay un error, no avanzamos»**, se investigó antes de seguir.
- **Causa:** los jugadores añadidos por el juego llevan un **prefijo** en su número. En la carrera del City es `0xdb65`; en la del Barça es **`0xdbdf`**. **El prefijo cambia en cada carrera.** El programa solo conocía el primero.
- **Comprobación:** el jugador `0xdbdf5827` tiene su ficha en el lugar **22.567**, y `0x5827` = 22.567. La regla de siempre (los 16 bits bajos) sirve igual.
- **Arreglo:** el programa acepta **cualquier** prefijo y siempre comprueba que número e ID coincidan.
- **Resultado:** **0 jugadores sin ficha** en los 6 guardados. Todas las pruebas pasan: 211/211 sin archivos y entre 225 y 234 con cada guardado. Compila también para Windows. ✅

---

## 3. Multiparche 🧩

| Qué | ¿Vale para todos los parches? |
|---|---|
| Forma de las listas de goleadores (20 B) | ✅ La pone el juego |
| El prefijo de los jugadores añadidos | 🧩 Cambia en **cada carrera**: el programa no supone ninguno |
| Qué competición es cada lista | 🧩 Depende del parche (se deduce por los clubes) |

---

## 4. Qué tienes que mirar en el juego

1. **Stats en la Liga Máster (sin cambiar nada):** en tu PC, Phoenix-DB tiene ahora el `Player.bin` **v99** (Lamine Yamal con **Velocidad 99**; su valor normal es **90**). Si abriste el juego después de las 5:51 de esta mañana, entra a la carrera del **Barça** (ranura 1) y mira la **Velocidad de Lamine**:
   - si ves **99** → las stats de Phoenix-DB **sí llegan** a la Liga Máster al reiniciar el juego;
   - si ves **90** (u otro número) → la carrera guarda **sus propias** stats.
2. **Goleadores:** en la carrera del Barça, abre las estadísticas de la **liga francesa**. Según el archivo deberías ver:
   - **Goleadores:** Hamza Igamane (Lille), Odsonne Édouard (Lens) y Arthur Avom (Lorient), los tres con **8 goles**; luego Sofiane Diop (Niza) y Joaquín Panichelli (Estrasburgo) con 5.
   - **Asistencias (🔎):** João Neves (PSG) **6**, Désiré Doué (PSG) 4.

---

## 5. Qué abre este avance

- **Tablas, calendario y goleadores de toda una carrera, leídos solos** por Phoenix Sync, listos para mandarlos a tu web.
- ⏳ Falta: resultados partido a partido, qué son las listas 3 y 4, y fichar agentes libres (hace falta tu prueba en el juego).
