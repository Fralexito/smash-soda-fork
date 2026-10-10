# 23 · Guía exacta: las «barras de crecimiento» de la Liga Máster

**Para FRALEX · 10 de octubre de 2026, 00:55 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de las guías 18 y 21.
Diario: `PRUEBAS.md` (10 oct, 00:50 y 00:55). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

## 0. En simple

Cada jugador de **tu equipo** tiene, dentro de la carrera, unas **30 barras de progreso**, como las de un videojuego de rol:

- Cada barra va de **0 a 9.999**.
- Con los días y los partidos, las barras **se van llenando**.
- Cuando una barra **llega a 10.000**, vuelve a empezar y **esa habilidad sube +1**.

**Probado en el juego:** entre el 1/1 y el 16/3 de tu carrera, **solo una** barra de Lamine se llenó, y en pantalla **solo una** habilidad subió: **Conservación del balón, de 92 a 93**. ✅

---

## 1. Recursos usados

| Recurso | Para qué |
|---|---|
| La misma carrera en dos fechas: ranura 4 (1/1/2026, huella `6cbe94ff…`) y ranura 6 (16/3/2026, huella `1feef3a4…`) | Comparar con **poco ruido** |
| Tus capturas de Lamine en las ranuras 4 y 6 | La verdad en pantalla |
| `dec` (abrir, con ida y vuelta) y Python 3 | Comparar byte a byte |

---

## 2. Paso a paso

### Paso 1 · Comparar la misma carrera en dos fechas
- **Por qué:** entre dos carreras **nuevas** cambian 1,77 millones de bytes (todo lo que el juego sortea). Entre la **misma** carrera en dos fechas, solo **191.549**: unas 10 veces menos ruido.

### Paso 2 · Mirar las fichas de Lamine
- En su ficha de **368 bytes** (la de tu equipo que llamamos C/D) hay unos **30 números de 0 a 9.999** que, entre el 1/1 y el 16/3, **subieron casi todos**.
- **Uno de ellos** (el n.º 6) pasó de **9.765 a 551**: subió, llegó a 10.000 y volvió a empezar.

### Paso 3 · La predicción (antes de ver la pantalla)
- «Si son barras de progreso, en la ranura 6 Lamine tendrá **exactamente una habilidad con +1**».

### Paso 4 · Tu captura de la ranura 6
- **Conservación del balón: 92 → 93.** Todo lo demás igual. ✅
- **Conclusión:** son barras de crecimiento, y la **barra n.º 6 = Conservación del balón**.

### Paso 5 · Otro hallazgo de la comparación: fichas de partido
- Las fichas de **596 bytes** (5.759) no son de jugadores: son de **partidos**. Cada una lleva el número del partido, la competición, la fecha y la lista de jugadores que jugaron. 5.759 = los partidos de la temporada. 🔎 (Sirve para los resultados partido a partido.)

### Paso 6 · Lo que todavía no aparece
- El **número de la habilidad** en sí (el 92 o el 93, el 99 de velocidad) **no está guardado como número normal**: se buscó tal cual, empaquetado en 6, 7 y 8 bits y cruzando las dos pruebas. ⏳
- Tampoco está en su carnet del blob, ni en sus fichas L y F (esas guardan datos de estado).

---

## 3. Qué abre este avance

- **Ya se puede LEER** cómo va creciendo cada jugador de tu equipo (cuánto le falta a cada barra).
- **Posible palanca para CAMBIAR stats en carreras empezadas (por probar, ⏳):** dejar una barra en 9.999 para que la habilidad suba **+1** en el siguiente avance del juego. Es lento (+1 cada vez), pero usa el **propio** sistema del juego.
- **Falta:** saber qué habilidad es cada una de las otras ~29 barras. Se aprende igual: comparar dos fechas y mirar qué habilidad subió.

---

## 4. Multiparche 🧩
- Las barras de crecimiento son del **juego**: iguales en todos los parches.
- Qué jugadores crecen y cuánto depende de la base de cada parche (edad, potencial).

---

## 5. Próximos pasos

1. **Mapear las barras:** cada vez que juegues o avances días en una carrera, guarda **antes y después** en ranuras distintas y saca captura del jugador. Cada habilidad que suba me dice qué barra es.
2. **Probar la palanca (con tu permiso y en una ranura de prueba):** poner la barra de una habilidad en 9.999, avanzar un día y mirar si sube +1.
3. Seguir buscando dónde está el número de la habilidad.
