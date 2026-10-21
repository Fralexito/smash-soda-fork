# 18 · Guía exacta: ¿llegan las stats de Phoenix-DB a una Liga Máster? (prueba y búsqueda)

**Para FRALEX · 9 de octubre de 2026, 23:50 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**, paso a paso.
Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25. Diario: `PRUEBAS.md` (9 oct, 23:50).

Símbolos: ✅ comprobado · ❌ comprobado que no · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

## 0. En simple

- En **amistosos**, lo que ponemos en Phoenix-DB **sí** se ve: Lamine con **Velocidad 99**. ✅
- En una **Liga Máster ya empezada**, **no** se ve: Lamine sigue con **Velocidad 90**, aunque reinicies el juego. ❌
- Cada carrera tiene **su propia copia** de las habilidades de los jugadores, y además la **va cambiando** con el tiempo (Pase raso 83 en vez de 82, Contacto 77 en vez de 76).
- Todavía **no** encontramos dónde guarda la carrera esa copia. Para encontrarla hace falta un experimento sencillo (sección 4).

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tus 2 capturas (Lamine en Liga Máster y en amistoso) | El resultado de la prueba |
| Tu PC (solo leer): Phoenix-DB y `sider.log` | Ver qué `Player.bin` había (v99) y qué leyó el juego, y cuándo |
| El «espía» de Sider (lo que ya registraba `phoenix.lua`) | Saber qué archivos de la base lee el juego al entrar a la Liga Máster |
| `~/phx/cpkls.py` (nuevo, en tu PC) | Leer **solo el índice** de los paquetes de Konami, sin cargar los gigas |
| Python 3 + NumPy (en la nube) | Buscar la copia de las habilidades en tus guardados |

---

## 2. Paso a paso

### Paso 1 · Comprobar qué había en Phoenix-DB
- **Cómo:** se calculó la huella del `Player.bin` de Phoenix-DB en las **dos** carpetas (juego y modo) y se comparó con las versiones de prueba.
- **Resultado:** las dos eran la **v99** (Lamine con Velocidad 99), colocada a las 5:51 de la mañana. ✅

### Paso 2 · Comprobar que el juego la leyó
- **Cómo:** en `sider.log` se vio que el juego arrancó a las **23:00** y leyó `player.bin` desde la raíz **Phoenix-DB**.
- **Resultado:** el juego tenía la v99 cargada. ✅

### Paso 3 · Tu prueba en el juego
| Dónde | Velocidad de Lamine | Media |
|---|---|---|
| Amistoso | **99** ✅ | 90 |
| Liga Máster (Lamine en el Barça) | **90** ❌ | 88 |

- **Conclusión:** la Liga Máster **no** usa el `Player.bin` de Phoenix-DB para los jugadores de una carrera ya empezada.

### Paso 4 · ¿Qué lee el juego al entrar a la Liga Máster?
- **Cómo:** el registro del espía mostró que al entrar a la Liga Máster (23:17 y 23:21) el juego leyó **`installversionplayer.bin`**, y no `player.bin`.
- Ese archivo **no** viene en ConmeGOL. Con `cpkls.py` se encontró en los paquetes de Konami: el más nuevo es `download\dt80_700E_x64.cpk`, de 13.223 bytes, el **mismo tamaño** que leyó el juego.
- Se copió solo ese trocito y se abrió (WESYS + zlib).
- **Resultado:** son **7.426 parejas «jugador → versión»** (por ejemplo, versión 100). **No** trae habilidades y **Lamine no está**. 🔎
- **Conclusión:** no es la copia que buscamos. Parece una lista de «qué versión de datos usa cada jugador».

### Paso 5 · Buscar la copia dentro del guardado
Se buscaron los valores de tu captura (83, 89, 93, 92, 83, 84, 81, 62…) de muchas formas. **Ninguna dio resultado:**

| Dónde se buscó | Cómo | Resultado |
|---|---|---|
| Todo el guardado | Trozos de la ficha original de Lamine de `Player.bin` | ❌ no hay copia literal |
| Las 12 fichas de tu equipo (incluida la gigante «M») | Valores «empaquetados» de 5 a 8 bits, en el orden de la pantalla, con margen de ±1 | ❌ |
| Su ficha de 596 bytes | Igual | ❌ |
| Su carnet del blob (156 bytes) | Ya se había buscado antes | ❌ |

- Solo apareció su **curva de medias** (81, 82, 83, 84…), que ya conocíamos: no son las habilidades.

---

## 3. Multiparche 🧩
- Que la Liga Máster use **su propia copia** lo hace **el juego**: pasará igual en todos los parches.
- `InstallVersionPlayer.bin` viene de **Konami**, no del parche: es igual en todos los parches que usen los mismos paquetes de Konami.

---

## 4. El experimento que falta (para encontrar la copia)

**Idea:** hacer **dos carreras nuevas iguales**. En una, Lamine tiene Velocidad 99; en la otra, 90. Lo único distinto entre los dos guardados será **dónde está esa velocidad**.

1. **Con el juego como está ahora (v99):** crea una **carrera nueva** (por ejemplo, con el Barça y los ajustes por defecto). Mira la Velocidad de Lamine y **guarda en una ranura libre** (la 4).
2. **Cierra el juego y avísame.** Yo pongo en Phoenix-DB la versión original (**v90**), con respaldo.
3. **Abre el juego** y crea **la misma carrera** (mismo club, mismos ajustes). Mira a Lamine y **guarda en otra ranura libre** (la 5).
4. Yo comparo las dos carreras byte a byte.

**Lo que nos dice también:** si en la carrera nueva del paso 1 Lamine sale con **99**, entonces la carrera **copia** las stats de Phoenix-DB **al crearse**. Eso ya es muy útil: con una carrera nueva tus cambios sí entran.

---

## 5. Qué significa para tu proyecto, hoy

- Stats nuevas en **amistosos, salas y modos normales:** ✅ funcionan (Phoenix-DB + Editar → Cargar o reiniciar).
- Stats nuevas en una **carrera ya empezada:** ❌ todavía no. Hace falta encontrar la copia (sección 4) y escribir en ella.
