# 25 · Guía exacta: el mapa de las barras (qué barra sube qué habilidad)

**Para FRALEX · 10 de octubre de 2026, 01:55 (Lima).**
Qué se hizo, **cómo se comprobó** y **con qué recurso**. Viene de las guías 23 (barras) y 24 (palanca).
Diario: `PRUEBAS.md` (10 oct, 01:35 → 01:52). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado · 🔎 visto en archivos · ⏳ falta · 🧩 depende del parche.

---

## 0. En simple

Cada jugador de tu equipo tiene unas 30 **barras de progreso**. Ya sabíamos que, si llenamos una barra a mano, el juego sube **+1** esa habilidad (guía 24).

Faltaba saber **qué barra es de qué habilidad**. Ahora ya lo sabemos para **las 20 habilidades de jugador de campo**. ✅

---

## 1. El mapa

| Barra | Habilidad | | Barra | Habilidad |
|---|---|---|---|---|
| 1 | Actitud ofensiva | | 13 | Balón parado |
| 2 | Actitud defensiva | | 14 | Efecto |
| 4 | Regate | | 20 | Velocidad |
| 5 | Control de balón | | 21 | Contacto físico |
| 6 | Conservación del balón | | 22 | Equilibrio |
| 7 | Finalización | | 23 | Potencia de tiro |
| 8 | Pase raso | | 24 | Aceleración |
| 9 | Pase bombeado | | 25 | Salto |
| 10 | Cabeceo | | 26 | Resistencia |
| 11 | Recuperación de balón | | | |
| 12 | Agresividad | | | |

**Sin saber todavía:**
- Barras 3, 15, 16, 17, 18: solo se usan en **porteros** (seguramente las 5 de portero). ⏳
- Barras 19, 27, 28: crecen muy poquito (decenas al mes). ¿Pie malo, regularidad, lesiones? ⏳
- Barras 29 y 30: siempre en 0.

---

## 2. Recursos usados

| Recurso | Para qué |
|---|---|
| Ranura 7 (11/4/2026) | Punto de partida |
| Ranura 8 = copia de la 7 con 60 barras en 9.999 (`herramientas/palanca/palanca_varias.cpp`) | La prueba |
| Ranura 9 (22/4/2026), guardada por ti después de avanzar | Ver en el archivo qué barras se llenaron |
| 28 capturas tuyas (7 jugadores × páginas 2 y 3 × antes y después) | La verdad en pantalla |
| Respaldos en `_PhoenixMercado_prueba\respaldos_ranuras` | Poder volver atrás |
| Python | Comparar barras y decodificar |

---

## 3. Paso a paso

### Paso 1 · El truco del «código secreto»
- En vez de probar una barra cada vez (20 pruebas), se hizo **una sola prueba**.
- 7 jugadores jóvenes: Bardghji, Caicedo, Gavi, Fermín, Lamine, Bernal, Cubarsí.
- Cada barra se llenó en **3 de esos 7**, con un trío **distinto** para cada barra.
- Así, el trío de jugadores en los que sube una habilidad es como su **huella digital**: dice qué barra es.

### Paso 2 · Preparar la ranura 8
- 60 barras cambiadas a 9.999. La herramienta comprueba cada valor viejo antes de tocarlo (si uno no cuadra, no escribe nada).
- Comprobado: **120 bytes** distintos (60 números), todos donde se planeó. Respaldo de la ranura 7 antes.

### Paso 3 · Tu prueba
- Cargaste la ranura 8, sacaste capturas (antes), avanzaste ~11 días, sacaste capturas (después) y guardaste en la ranura 9.

### Paso 4 · Comprobar en el archivo
- En la ranura 9, **las 60 barras se llenaron** (dieron la vuelta). ✅
- Además se llenaron 4 solas, por crecimiento normal: Lamine (barras 4, 8, 24) y Bernal (barra 25).

### Paso 5 · Comprobar en la pantalla
- A cada jugador le subieron **exactamente** tantas habilidades como barras llenas tenía: **64 de 64**. ✅
- Con el trío de cada habilidad se sacó su barra. Dos parejas quedaron empatadas (8/12 y 13/24) y se desempataron con las capturas de Bernal: a él le subieron **Pase raso** y **Aceleración**, pero **no** Agresividad ni Balón parado.

---

## 4. Qué abre este avance

- Ya se puede **subir +1 cualquier habilidad de campo** de un jugador de tu equipo, eligiendo la barra correcta. ✅
- Phoenix Sync podrá hacerlo **desde la web**: «+1 Velocidad a X» → barra 20 en 9.999.
- Sigue siendo de **+1 por vez**, y solo para jugadores **de tu equipo**.

---

## 5. Riesgos y cuidados

- Respaldo **siempre** antes, y comprobar la huella del archivo en el PC.
- Nunca poner más de 9.999 (la herramienta no lo permite).
- Si la habilidad ya está en **99**, la barra no se mueve (lo vimos con la Velocidad de Lamine): no sirve llenarla.
- Al llenar muchas barras a la vez, el jugador sube varias habilidades de golpe y su **media** puede subir (Fermín 86→88, Bernal 78→80). En una liga real hay que decidir cuántas subidas se permiten.

---

## 6. Multiparche 🧩

- El mapa (qué barra es qué habilidad) es **del juego**, no del parche: vale igual en ConmeGOL, Sudamerican y cualquier parche de PES 2021.
- Lo que cambia por parche y por carrera es **dónde** está la ficha de cada jugador en el guardado. La herramienta no adivina: comprueba el valor viejo antes de tocar.
- Probado en **ConmeGOL 26**, carrera del Barça.

---

## 7. Próximos pasos

1. **Porteros:** misma prueba con los porteros (barras 3, 15, 16, 17, 18).
2. **Barras lentas** (19, 27, 28): llenarlas en 9.999 y mirar la página 3 (pie malo, regularidad, lesiones).
3. Probar si una barra que **baja de 0** resta −1 (veteranos).
4. Meterlo en Phoenix Sync como función «subir habilidad».
