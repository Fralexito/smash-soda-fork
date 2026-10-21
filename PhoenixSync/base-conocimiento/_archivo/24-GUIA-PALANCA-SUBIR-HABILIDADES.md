# 24 · Guía exacta: la «palanca» para subir habilidades en una Liga Máster ya empezada

**Para FRALEX · 10 de octubre de 2026, 01:15 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de la guía 23 (barras de crecimiento).
Diario: `PRUEBAS.md` (10 oct, 01:05 y 01:11). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

## 0. En simple

Cada habilidad de un jugador de tu equipo tiene una **barra de progreso** de 0 a 9.999.
Cuando la barra se llena, el juego sube esa habilidad **+1**.

**La palanca:** nosotros llenamos la barra **a mano**, casi hasta el tope (9.999).
Luego el juego, al avanzar, la termina de llenar y **él mismo** sube la habilidad.

**Probado en el juego:** Lamine, Conservación del balón **93 → 94**. ✅

Es como llenar casi entera la barra de experiencia de un personaje de videojuego: con un poquito más, **sube de nivel solo**.

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tu ranura 6 (Barça, 16/3/2026, huella `1feef3a4…`) | La carrera de prueba |
| Respaldo en `_PhoenixMercado_prueba\respaldos_ranuras\ML00000005_ranura6_original_16-3-2026` | Poder volver atrás |
| `herramientas/palanca/palanca.cpp` (nueva) | Abrir el guardado, cambiar **un** número y volver a cerrarlo |
| `SobrePes` (Phoenix Sync) | Descifrar y cifrar el guardado con su envoltura original |
| Tus 2 capturas de Lamine (al cargar y varios días después) | La verdad en pantalla |

---

## 2. Paso a paso

### Paso 1 · Respaldo (antes de tocar nada)
- Se copió tu ranura 6 original a `respaldos_ranuras` y se comprobó que la copia es **idéntica** (misma huella).

### Paso 2 · Cambiar un solo número
- En la ficha de 368 bytes de Lamine, la **barra n.º 6** (Conservación del balón) estaba en **551**.
- La herramienta **comprueba primero** que el valor es 551 (si no, no toca nada) y lo cambia a **9.999**.
- Texto en «Cargar»: «PRUEBA PALANCA (Barca) / 16/3/2026 / Lamine barra 6 = 9999».

### Paso 3 · Comprobar el archivo nuevo
- Se volvió a abrir y se comparó con el original: **solo 2 bytes distintos**, los de esa barra. ✅
- Se llevó a tu PC y se comprobó la huella (`f220841e…`) **antes y después** de copiarlo a la ranura 6. ✅

### Paso 4 · Tu prueba en el juego
| Momento | Conservación del balón |
|---|---|
| Al cargar | **93** (aún no cambia: falta que el juego avance) ✅ |
| Varios días después | **94** ✅ |

- **Conclusión:** la palanca **funciona**. El juego aceptó el cambio sin quejarse, y la carrera sigue normal (media 90, todo lo demás coherente).

### Paso 5 · Regalo extra: otras barras que estaban casi llenas
En esos días también subieron, por crecimiento normal:

| Habilidad | Antes → después | Barra probable (estaba casi llena) |
|---|---|---|
| Actitud ofensiva | 83 → 84 | **n.º 1** (9.969) 🔎 casi seguro |
| Salto | 71 → 72 | n.º 22 (9.940) o n.º 25 (9.690) 🔎 |
| Equilibrio | 87 → 88 | n.º 22 o n.º 25 🔎 |

Para confirmarlo hace falta el guardado **de después**, en una ranura nueva. ⏳

---

## 3. Qué abre este avance

- **Ya se puede subir +1** cualquier habilidad cuya barra conozcamos, en una carrera **ya empezada**, usando el sistema del propio juego. ✅
- **Es de +1 por vez:** la barra vuelve a empezar desde 0 al llenarse. Para subir +3 habría que repetir la palanca 3 veces, avanzando entre medias. ⏳ Sin probar si se puede más rápido.
- **Falta:** saber qué habilidad es cada una de las ~30 barras (hoy: n.º 6 seguro, n.º 1 casi seguro).
- **No sirve (todavía) para BAJAR** una habilidad ni para ponerla en un número exacto de golpe.

---

## 4. Riesgos y cuidados

- **Siempre respaldo antes**, y comprobar la huella del archivo en el PC.
- Cambiar **solo** la barra (un número de 0 a 9.999). Nunca poner 10.000 o más: no sabemos qué haría el juego.
- La herramienta **se niega** a cambiar si el valor viejo no es el esperado: así no se toca nada por error.
- Las barras solo existen para los jugadores de **tu equipo** (ficha de 368 bytes). Para los demás equipos, ⏳.

---

## 5. Multiparche 🧩

- Las barras y la forma de subir +1 son **del juego**: valen igual en ConmeGOL, Sudamerican y cualquier parche de PES 2021.
- Lo que cambia por parche es **dónde** cae la ficha de cada jugador dentro del guardado. Por eso la herramienta **no** usa posiciones fijas a ciegas: comprueba el valor antes de tocar. En otro parche, primero hay que buscar la ficha del jugador (como en la guía 23) y luego aplicar la palanca.
- Probado solo en **ConmeGOL 26**, carrera del Barça.

---

## 6. Próximos pasos

1. **Guardar el «después»** en una ranura nueva → confirmo qué barras son Actitud ofensiva, Salto y Equilibrio.
2. **Mapear todas las barras de una vez:** a cada jugador de tu equipo le lleno **una barra distinta** (al jugador A la n.º 2, al B la n.º 3, al C la n.º 4…). Avanzas, y en cada jugador miras qué habilidad subió: así cada jugador nos «dice» una barra, sin confundirlas.
3. Probar la palanca en **dos barras a la vez** y **dos veces seguidas** (+2).
