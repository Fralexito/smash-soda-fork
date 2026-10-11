# 32 · Idea: un "mundo compartido" entre varias Ligas Máster conectadas

**Para FRALEX · 10 de octubre de 2026, 21:49 (Lima).**
Esto **no está construido todavía** — es una idea pensada en conversación, para dejarla anotada antes de olvidarla. No se tocó ningún archivo del juego ni se escribió código. Cuando se decida construirla, hay que volver a leer esta guía y planear la implementación paso a paso (y pedir el OK de FRALEX antes de tocar nada).

---

## 0. La pregunta que lo originó

FRALEX preguntó: si él tiene su Liga Máster con el Barcelona y un amigo tiene la suya con el Real Madrid (cada uno en su propia PC, cada uno jugando su propia carrera), ¿se puede hacer que un fichaje hecho en una de las dos Ligas Máster — incluso uno hecho por la CPU, no solo por la persona — se refleje también en la Liga Máster del otro? ¿Es posible simular que los dos "juegan el mismo online" aunque en realidad sean dos partidas separadas?

Respuesta corta: **sí es posible**, reusando piezas que ya existen (Phoenix Sync, la web, el mecanismo de "Mercado"), pero aparecen problemas nuevos que hay que resolver con reglas claras, no solo con más código.

---

## 1. La idea central: dos capas, no una

Ya existen, dentro del propio juego, dos capas distintas en cada Liga Máster:

- **Capa profunda:** la carrera real de cada persona — su dinero, sus contratos, el calendario, las decisiones de toda la temporada. Esto es privado de cada uno, vive dentro de su guardado (`MLxxxxxxxx`) y de su option file.
- **Capa de encima ("datos en vivo"):** el mecanismo que ya usa Phoenix-DB (`PlayerAssignment.bin` + el botón "Activar") para decir "este jugador se muestra en este equipo", sin tocar el dinero ni los contratos de nadie.

**La idea clave:** lo que se comparte entre Ligas Máster conectadas es **solo la capa de encima** — nunca la capa profunda. Esto no es una limitación resignada, es la solución a varios problemas a la vez (ver sección 3).

---

## 2. Cómo funcionaría, paso a paso

1. **La web guarda "la verdad" de un mundo compartido:** una lista en Supabase de "este jugador está en este equipo ahora", parecida a lo que ya hace el Mercado con los fichajes manuales.
2. **Cada Phoenix Sync conectado vigila su propia Liga Máster.** Compara "antes" contra "después" (la misma técnica que ya usamos para encontrar las barras de crecimiento) para notar cuándo la CPU local movió a un jugador de equipo.
3. **Cuando nota un cambio, intenta "anotarlo" en la web**, diciendo cuál era el equipo viejo que él tenía anotado (no solo el nuevo). Esto es igual que la regla de seguridad de `palanca_varias`: comprobar el valor viejo antes de escribir.
4. **La web acepta o rechaza ese cambio:**
   - Si el equipo viejo que mandó coincide con lo que la web ya tenía → se acepta, se actualiza la lista compartida.
   - Si no coincide (porque otra Liga Máster ya cambió a ese jugador primero) → se rechaza. Gana el que llegó primero.
5. **Cada Phoenix Sync, al revisar la web, aplica la versión más reciente** sobre la capa "de encima" de su propio juego — igual que ya hace con un fichaje normal del Mercado.

---

## 3. Los tres problemas que aparecen, y su solución

### 3.1 El dinero y los "efectos en cadena"

Si el fichaje de la CPU de alguien se rechaza, ¿qué pasa con el dinero que ya gastó, o los ajustes de plantilla que ya hizo pensando en ese jugador?

**Solución: no hay que corregir nada, porque nunca se toca.** Como solo se sincroniza la capa "de encima" (qué equipo se *ve*), la capa profunda de cada persona (su dinero, sus contratos) queda intacta y privada siempre. El fichaje de la CPU "pasó" de verdad dentro de su propio mundo — eso no se borra ni se corrige —, pero en la capa compartida ese jugador se ve en otro equipo. Es como llevar cada uno su propio cuaderno de cuentas, mientras en la pizarra del salón (lo que todos ven) solo se anota "quién juega dónde ahora".

### 3.2 Que se sienta raro ver cambiar a un jugador de la nada

**Solución: un aviso simple.** Cuando Phoenix Sync aplique un cambio que vino de otra Liga Máster, debería dejar una nota visible en Phoenix Link, por ejemplo: *"Mbappé cambió al Real Madrid — fichaje hecho en la Liga Máster de [nombre]."* Así se siente como una noticia del mundo compartido, no como un error.

### 3.3 Demasiados choques si se sincronizan todos los jugadores

**Solución elegida (FRALEX la prefiere): sincronizar solo una lista corta de "jugadores conectados".**
No se comparten los 500+ jugadores de todas las ligas — eso generaría choques por jugadores que a nadie le importan (un suplente cualquiera). En cambio:
- Se define una lista corta de jugadores "del mundo compartido" (por ejemplo, solo los de nivel alto, las estrellas que de verdad emocionan cuando cambian de equipo).
- Solo esos jugadores entran al mundo compartido y pueden generar un choque.
- Todos los demás jugadores siguen siendo 100% privados de cada Liga Máster — su CPU puede hacer lo que quiera con ellos sin que le importe a nadie más.

Esto reduce muchísimo la cantidad de choques posibles, porque ya no se vigilan cientos de jugadores, solo un puñado.

---

## 4. Cómo quedaría la regla completa, junta

1. Se define una lista corta de jugadores "del mundo compartido".
2. Solo esos jugadores se sincronizan entre Ligas Máster conectadas, usando la capa "de encima" (sin tocar el dinero ni los contratos de nadie).
3. Si hay un choque (dos CPUs mueven al mismo jugador casi al mismo tiempo), gana el que llegó primero a la web — regla de "comprobar el valor viejo antes de aceptar", igual que en `palanca_varias`.
4. El que pierde el choque recibe un aviso simple la próxima vez que sincronice ("este jugador cambió de equipo en otra Liga Máster"), y su capa "de encima" se corrige para coincidir con la web.
5. Todo lo demás (jugadores comunes, dinero, contratos, calendario) se queda privado en cada Liga Máster, sin tocar nunca.

---

## 5. Importante: no es tiempo real como jugar un partido juntos

No se va a ver el cambio en medio de un partido. Se va a ver la próxima vez que el juego revise los datos en vivo (por ejemplo, al entrar al menú o activar esa opción). Tampoco hace falta estar en la misma red de la casa — esto funciona por internet, a través de la web, igual que ya funciona el Mercado.

---

## 6. Pendiente (nada construido todavía)

1. Decidir qué tan larga es la lista de "jugadores del mundo compartido" (¿solo los más famosos? ¿un número fijo por equipo?).
2. Diseñar la tabla en Supabase para esta lista compartida (parecida a la del Mercado, pero con un "contador de versión" por jugador para la regla de choques).
3. Decidir cada cuánto tiempo revisa Phoenix Sync si hay cambios (¿automático? ¿con un botón?).
4. Diseñar el aviso/noticia que se le muestra a cada usuario cuando se aplica un cambio que vino de otra Liga Máster.
5. Probarlo primero con solo 2 personas conectadas (bajo riesgo de choques) antes de pensar en más.
