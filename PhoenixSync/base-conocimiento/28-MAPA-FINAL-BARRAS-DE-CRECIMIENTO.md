# 28 · Mapa final y completo de las barras de crecimiento (28 de 30)

**Para FRALEX · 10 de octubre de 2026, 09:40 (Lima).**
Cierre de la investigación de las guías 23, 24, 25 y 26. Aquí está **todo resuelto**: ya no falta ninguna barra de portero ni de las lentas.
Diario paso a paso: `PRUEBAS.md` (10 oct, 09:30 a 09:37). Detalle técnico: `liga-master/ESTRUCTURA-ML.md` §25.

Símbolos: ✅ comprobado en pantalla y en el archivo · 🔎 comprobado solo en pantalla · 🧩 depende del parche.

---

## 0. En simple

Hace unas horas (guía 26) el mapa tenía **26 de 28 barras** conocidas. Faltaban dos: la 17 y la 18, que tenían que ser **Reflejos** y **Cobertura** (las dos habilidades de portero que quedaban sin barra), pero no sabíamos cuál era cuál.

Se hizo una prueba más (la «ronda 2»): a **Joan García** se le subió la barra 17 y se le bajó la 18; a **Szczęsny**, al revés (17 abajo, 18 arriba). Resultado en pantalla:

| Portero | Reflejos | Cobertura |
|---|---|---|
| Joan García | **92 → 93** (subió) | 91 → 91 (igual) |
| Szczęsny | **87 → 86** (bajó) | 87 → 87 (igual) |

Reflejos subió exactamente donde se plantó la barra 17 hacia arriba (Joan) **y** bajó exactamente donde se plantó la barra 17 hacia abajo (Szczęsny). Eso confirma, por partida doble: **barra 17 = Reflejos**.

Cobertura no se movió en esta prueba (probablemente porque esta vez se avanzó menos tiempo en el juego que en la ronda 1, y su barra no llegó a completar la vuelta). Pero como ya no queda ninguna otra habilidad de portero sin dueño, por descarte: **barra 18 = Cobertura**.

Con esto, **las 28 barras que el juego realmente usa ya tienen dueño conocido.**

---

## 1. La tabla final completa (28 de 30 barras)

| Barra | Habilidad | Cómo se confirmó |
|---|---|---|
| 1 | Actitud ofensiva | 🔎 código secreto (guía 25) |
| 2 | Actitud defensiva | ✅ código secreto |
| 3 | Actitud de portero | ✅ ronda 1 (guía 26) |
| 4 | Regate | ✅ código secreto |
| 5 | Control de balón | ✅ código secreto |
| 6 | Conservación del balón | ✅ palanca (guía 24) + código secreto |
| 7 | Finalización | ✅ código secreto |
| 8 | Pase raso | ✅ código secreto |
| 9 | Pase bombeado | ✅ código secreto |
| 10 | Cabeceo | ✅ código secreto |
| 11 | Recuperación de balón | ✅ código secreto |
| 12 | Agresividad | ✅ código secreto |
| 13 | Balón parado | ✅ código secreto |
| 14 | Efecto | ✅ código secreto |
| 15 | Atajar (portero) | ✅ ronda 1 (sube en Joan, baja en Szczęsny) |
| 16 | Despejar (portero) | ✅ ronda 1 |
| **17** | **Reflejos (portero)** | **✅ ronda 2 (sube en Joan, baja en Szczęsny) — NUEVO** |
| **18** | **Cobertura (portero)** | **🔎 ronda 2, por descarte (único candidato que quedaba) — NUEVO** |
| 19 | Precisión con el pie malo | ✅ ronda 1 |
| 20 | Velocidad | ✅ código secreto |
| 21 | Contacto físico | ✅ código secreto |
| 22 | Equilibrio | ✅ código secreto |
| 23 | Potencia de tiro | ✅ código secreto |
| 24 | Aceleración | ✅ código secreto |
| 25 | Salto | ✅ código secreto |
| 26 | Resistencia | ✅ código secreto |
| 27 | Regularidad | ✅ ronda 1 |
| 28 | Resistencia a lesiones | ✅ ronda 1 |
| 29 | *(sin usar, siempre 0)* | — |
| 30 | *(sin usar, siempre 0)* | — |

**Lo único que queda sin resolver de todo este tema:** la habilidad **«Uso de pie malo»** no tiene barra conocida — ya no quedan números libres para ella (solo la 29 y la 30, que siempre están en 0 en todos los jugadores probados). Lo más probable es que esta habilidad **no suba por barra**, sino por otro mecanismo del juego que no hemos buscado todavía (o que de verdad no suba nunca de forma automática). Para la gran mayoría de lo que Phoenix necesita (subir/bajar cualquier otra habilidad desde la web), esto no es un obstáculo: ya se puede hacer con 28 de las ~29 habilidades «de barra».

---

## 2. Cómo se hizo la ronda 2, paso a paso (para humano o IA)

1. **Base:** la ranura 11 (`ML0000000A`, guardado del 2/5/2026, ya confirmado byte a byte contra la ronda 1).
2. **Respaldo primero**, con huella sha256 comprobada igual al original, en `_PhoenixMercado_prueba/respaldos_ranuras/`.
3. **Cambios con `palanca_varias`** (verifica cada valor viejo antes de escribir cualquier cosa; si uno no coincide, no se toca nada):
   - Joan García: barra 17 (offset `0xc5086e`) de 7307 → **9999**; barra 18 (offset `0xc50870`) de 5220 → **0**.
   - Szczęsny: barra 17 (offset `0xc5183e`) de 5311 → **0**; barra 18 (offset `0xc51840`) de 7717 → **9999**.
   - Se comprobó que **solo 8 bytes** del archivo cambiaron (los 4 números, de 2 bytes cada uno). Nada más se tocó.
4. **Entrega al PC:** por el puente de archivos, pero **comprimido en `.zip`** (ver aviso de la sección 3) y guardado en la ranura 12 (`ML0000000B`). Huella sha256 comprobada igual en los dos lados.
5. **FRALEX:** cargó la ranura 12, miró la página 3/4 de Joan García y Szczęsny (capturas «antes»), avanzó, y volvió a mirar la misma página (capturas «después»).
6. **Lectura del resultado:** la habilidad que sube en el jugador donde se plantó la barra hacia arriba **y** baja en el jugador donde se plantó hacia abajo, es esa barra. Si ninguna de las dos se mueve pero ya no queda otro candidato, se asigna por descarte.

**Para que una IA repita esto con otro juego de barras desconocidas:** el truco de «subir en un jugador, bajar en otro, al mismo tiempo» es más fuerte que solo subir o solo bajar, porque confirma la barra **dos veces en el mismo paso** (sube donde debía subir, baja donde debía bajar) — y si por algún motivo una de las dos direcciones no llega a completarse a tiempo, la otra dirección (o el descarte, si ya no quedan candidatos) igual resuelve el misterio.

---

## 3. ⚠️ Aviso importante para cualquiera que use el puente de archivos hacia el PC

Al mandar `ML0000000B` (un archivo de guardado, **sin extensión** en el nombre) directo por el puente, **llegó dañado**: en el contenedor pesaba 19.831.781 bytes y en el PC llegó con solo 19.813.994 (le faltaban 17.787 bytes), sin que la herramienta avisara de ningún error.

**Causa probable:** un archivo sin extensión (como todos los `MLxxxxxxxx`) puede ser tratado como texto por el puente, y le quita bytes que por casualidad se parecen a saltos de línea de Windows, dentro de los datos cifrados.

**Arreglo que sí funciona:** comprimir el archivo en `.zip` antes de mandarlo (eso viaja byte por byte sin tocarlo) y descomprimirlo ya en el PC. Se comprobó con huella sha256 en cada paso.

**Regla para todo lo que venga:** nunca mandar un `MLxxxxxxxx` (o cualquier archivo sin extensión) directo por el puente. Siempre comprimir primero, y siempre comprobar tamaño y huella en el PC después — no basta con que la herramienta no muestre error.

---

## 4. Qué abre esto para Phoenix Sync

- Ya se puede, en una carrera **ya empezada**, subir o bajar **en +1/−1** casi cualquier habilidad de un jugador del equipo del usuario (de las 28 de 30 barras), de forma controlada y verificada.
- Es la pieza que faltaba para que la web pueda pedir, por ejemplo, «sube la Conservación del balón de este jugador» y que Phoenix Sync lo traduzca en el cambio correcto dentro del guardado.
- Sigue pendiente (no es parte de este cierre): diseñar cómo se conecta esto con la web (qué pide la web, qué responde Phoenix Sync, cómo se entrega el archivo al jugador) y decidir qué pasa con «Uso de pie malo».

---

## 5. Multiparche 🧩

- El mecanismo de las barras (0 a 9.999, +1 al completar hacia arriba, −1 al completar hacia abajo) es del motor del juego: vale igual en cualquier parche de PES 2021.
- Lo que cambia por parche es **dónde** cae la ficha de cada jugador dentro del guardado — por eso todo esto se hace siempre verificando el valor antes de tocar, nunca con posiciones fijas a ciegas.
- Probado solo en **ConmeGOL 26**, carrera del Barça de FRALEX.

---

## 6. Pendiente

1. Averiguar si «Uso de pie malo» tiene alguna forma de subir (otra barra en otro lugar del archivo, o un mecanismo distinto).
2. Confirmar la ronda 2 también a nivel de archivo (ranura 13, cuando FRALEX guarde el «después»).
3. Probar si se puede subir **más de +1** de una vez, o hacerlo más rápido que una barra a la vez.
4. Diseñar la función real «subir/bajar habilidad» dentro de Phoenix Sync, conectada a la web.
