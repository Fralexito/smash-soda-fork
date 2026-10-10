# 19 · Guía exacta: verificación en pantalla del calendario, la tabla y los goleadores

**Para FRALEX · 9 de octubre de 2026, 23:45 (Lima).**
Cómo se comprobó en el juego que lo que Phoenix Sync lee del archivo es **lo mismo** que muestra la pantalla.
Viene de las guías 14 (calendario), 15 (tablas) y 16 (goleadores). Diario: `PRUEBAS.md` (9 oct, 23:45).

---

## 0. En simple

Antes, el calendario, las tablas y los goleadores estaban «vistos en archivos» 🔎.
Ahora tú los miraste en el juego y **coinciden uno por uno**. Pasan a **probado** ✅.

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tu carrera del **Barça** (ranura 1, 4/3/2026), copiada a la nube sin tocar el original | De ahí se leyeron los datos |
| El lector de temporada (`core/TemporadaLM`, guía 16) y los scripts de Python | Leer tabla, goleadores y calendario |
| Tus 4 capturas: goleadores de la Ligue 1, tabla de la Ligue 1, pantalla principal de la carrera y la ficha de Lamine | Comparar con la pantalla |

---

## 2. Paso a paso

### Paso 1 · Leer del archivo (antes de ver tus capturas)
Se dejó escrito en la guía 16 lo que el archivo decía, **antes** de que tú miraras el juego. Así la comparación es justa.

### Paso 2 · Comparar los goleadores
| Jugador | Club | Archivo | Pantalla |
|---|---|---|---|
| Hamza Igamane | Lille | 8 | 8 ✅ |
| Odsonne Édouard | Lens | 8 | 8 ✅ |
| Arthur Avom | Lorient | 8 | 8 ✅ |
| Sofiane Diop | Niza | 5 | 5 ✅ |
| Joaquín Panichelli | Estrasburgo | 5 | 5 ✅ |

### Paso 3 · Comparar la tabla
| Puesto | Club | Archivo (Pts · G-E-P · GF-GC) | Pantalla |
|---|---|---|---|
| 1 | Lille | 22 · 7-1-0 · 19-4 | ✅ igual |
| 2 | PSG | 20 · 6-2-0 · 17-4 | ✅ igual |
| 3 | Lens | 19 · 6-1-1 · 20-8 | ✅ igual |
| 4 | Lorient | 16 · 5-1-2 · 19-11 | ✅ igual |
| 5 | Marsella | 14 · 4-2-2 · 11-8 | ✅ igual |
| 6 | Mónaco | 14 · 4-2-2 · 9-6 | ✅ igual |

- **Las flechas también encajan:** el juego muestra que Marsella y Mónaco **suben** y que Lyon y Niza **bajan**. En la segunda tabla guardada (la de la jornada anterior), Niza era 5.º y Lyon 6.º. Eso confirma que esa segunda tabla es la **anterior**. ✅

### Paso 4 · Comparar LaLiga y el calendario
- La tabla de LaLiga en el juego está **toda en 0**, en el **mismo orden** que en el archivo (Barça, Real Sociedad, Deportivo, Celta…). ✅
- Tu próximo partido de LaLiga es **Barça – Real Sociedad, el 11/4/2026, en casa**: es la **jornada 1** que se encontró en el archivo. ✅

---

## 3. Resultado

| Qué | Antes | Ahora |
|---|---|---|
| Calendario (guía 14) | 🔎 | ✅ probado |
| Tablas de posiciones (guía 15) | 🔎 | ✅ probado |
| Goleadores (guía 16) | 🔎 | ✅ probado |
| Asistencias | 🔎 | 🔎 (falta mirar esa pantalla) |

---

## 4. Multiparche 🧩
- Probado en **ConmeGOL 26**, en dos carreras distintas (City y Barça). En otro parche hay que repetir esta misma comparación una vez (guía 12).

---

## 5. Qué abre
- Phoenix Sync puede mandar a tu web, **sin capturas**, la tabla, los goleadores y el calendario de cualquier liga de una carrera. ✅
