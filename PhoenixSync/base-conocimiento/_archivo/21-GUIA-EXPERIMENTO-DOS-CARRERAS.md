# 21 · Guía exacta: el experimento de las dos carreras (stats en la Liga Máster)

**Para FRALEX · 10 de octubre de 2026, 00:40 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de la guía 18.
Diario: `PRUEBAS.md` (9 oct 23:58 → 10 oct 00:40). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · ❌ comprobado que no · ⏳ falta · 🧩 depende del parche.

---

## 0. En simple

Imagina que cada carrera es un **álbum de fotos** de los jugadores:

- Cuando **creas** la carrera, el juego **saca una foto** de cada jugador tal como está en la base en ese momento.
- Desde ahí, la carrera usa **su foto**, no la base. Aunque cambies la base después, la foto no cambia.
- Con el tiempo, la carrera **retoca su foto**: el jugador crece o baja.

**Lo que ya sabemos hacer:** si pones las stats en Phoenix-DB **antes de crear** una carrera, la carrera nace con ellas. ✅
**Lo que falta:** encontrar **dónde** guarda la carrera su «foto», para poder cambiarla en carreras ya empezadas. ⏳

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| Tu juego y tus capturas de Lamine | La verdad en pantalla |
| Phoenix-DB en tu PC: `Player.bin` v99 (Lamine Velocidad 99) y v90 (original) | Las dos versiones de la base |
| Respaldo: `_PhoenixMercado_prueba\db\Player_v99.bin` y `Player_v90.bin` (huellas `2d1a854f…` y `de702dff…`) | Cambiar de versión con seguridad |
| `dec` (abrir guardados, con ida y vuelta) | Abrir las dos carreras nuevas |
| Python 3 + NumPy | Comparar las dos carreras byte a byte y bit a bit |

---

## 2. Paso a paso

### Paso 1 · Carrera nueva con la base v99
- **Tú:** creaste una carrera con el Barça y la guardaste en la **ranura 4** (`ML00000003`, huella `6cbe94ff…`).
- **Pantalla:** Lamine **Velocidad 99**, media 90. ✅

### Paso 2 · Cambiar la base a v90 (yo, con el juego cerrado)
- Copié la v90 original a las **dos** carpetas de Phoenix-DB (primero como `.tmp` y luego renombrando) y comprobé la huella: `de702dff…` en las dos. ✅
- La v99 quedó respaldada, idéntica, en `_PhoenixMercado_prueba\db\Player_v99.bin`.

### Paso 3 · La misma carrera nueva con la base v90
- **Tú:** creaste la misma carrera y la guardaste en la **ranura 5** (`ML00000004`, huella `006dd17a…`).
- **Pantalla:** Lamine **Velocidad 90**, media 87. ✅

### Paso 4 · La prueba que lo decide
- **Tú:** con la base todavía en **v90**, cargaste la **ranura 4**.
- **Pantalla:** Lamine **Velocidad 99**. ✅
- **Conclusión firme:** la carrera **guarda su propia copia** («su foto»), tomada **al crearse**.

| Carrera | Base cuando se creó | Base al mirarla | Velocidad |
|---|---|---|---|
| Ranura 4 | v99 | v99 | 99 |
| Ranura 5 | v90 | v90 | 90 |
| Ranura 4 | v99 | **v90** | **99** |
| Barça vieja (ranura 1) | anterior | v99 | **90** |

### Paso 5 · Buscar la «foto» comparando las ranuras 4 y 5
Se abrieron las dos (ida y vuelta idéntica) y se compararon. **Nada de esto encontró la velocidad:**

| Qué se probó | Resultado |
|---|---|
| Bytes con 99 o 59 en una y 90 o 50 en la otra | ❌ solo ruido |
| Como número decimal o de 2 bytes | ❌ |
| Empaquetado en trozos de 6, 7 y 8 bits, antes y después del blob | ❌ solo ruido |
| Grupos con sus 17 habilidades en cualquier orden | ❌ solo ruido |
| Trozos de su ficha original de `Player.bin` | ❌ no están |
| Su ficha gigante «M» | idéntica en las dos ❌ |
| Su carnet del blob | solo cambia el **sueldo** (es mejor jugador, cobra más) |

- **Lo que sí cambia entre las dos:** su **curva de medias** (84, 85, 86… frente a 82, 83, 84…) y unos números **al azar** que el juego sortea al crear la carrera.
- **Zona descartada:** desde `0x10c0000` hay una **imagen** (bloques de textura), no datos de jugadores.
- **Por qué es difícil:** al crear una carrera el juego sortea muchas cosas (juveniles, sorteos, semillas). Entre dos carreras nuevas cambian **1,77 millones** de bytes, y la velocidad de un solo jugador se pierde entre tanto ruido.

---

## 3. Qué significa para tu proyecto, hoy

1. **Para una liga o temporada nueva:** pon las stats en Phoenix-DB **antes de que cada jugador cree su carrera**. Así todas nacen con los mismos datos. ✅ Ya funciona.
2. **Para carreras ya empezadas:** todavía no se puede. ⏳
3. **Ojo:** si cambias la base **después**, las carreras viejas **no** se enteran. Las nuevas sí.

---

## 4. Multiparche 🧩
- Que la carrera «saque una foto» al crearse lo hace **el juego**: pasa igual en todos los parches.
- La base de la que saca la foto es la que el juego usa de verdad en cada parche (guía 12, §2.2).

---

## 5. Próximo plan para encontrar la «foto»

Necesitamos una comparación con **mucho menos ruido**. Dos ideas:

1. **Misma carrera, una semana después.** Guarda la ranura 4, avanza unos días **sin jugar partidos** y guarda en otra ranura. Entre esas dos habrá pocos cambios. Si a algún jugador le cambia una habilidad (crecimiento), esa diferencia señala dónde está la «foto».
2. **Un jugador que crece mucho.** Con un jovencito de tu equipo (por ejemplo, un juvenil), juega un partido y mira si sube alguna habilidad. Así sabemos qué número buscar.
