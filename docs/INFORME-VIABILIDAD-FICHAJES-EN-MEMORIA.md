# Informe de viabilidad · ¿Puede un módulo Lua mover jugadores entre clubes en memoria en PES 2021?
30-oct-2026 · Solo investigación: no se tocó código ni el juego.

Fuentes: documentos de Phoenix Sync (rama `mercado-fase0`: `PhoenixSync/MANUAL-SYNC.md`, `base-conocimiento/DATOS-EN-VIVO-Y-RECARGA.md`), README del mapa de memoria de xAranaktu (GitHub), ficha del Live Editor de Ruls (PES Modding), notas de versión y guía de Touchline.

---

## 1. Veredicto corto
**Hoy no es viable de forma segura, y no hace falta.**
- Nadie en la comunidad mueve jugadores entre clubes directo en la memoria.
- Phoenix ya lo intentó con stats y no funcionó.
- El método de Phoenix (cambiar archivos + recargar sin cerrar el juego) **sí funciona**. Lo que falta es que funcione siempre.

---

## 2. Qué hacen los demás

| Herramienta | ¿Cambia el club de un jugador en memoria? | Detalle |
|---|---|---|
| **Mapa de memoria de xAranaktu** (la base de todos) | **No** | Edita edad, pie, posiciones, habilidades, rol, contrato y presupuesto. Solo del jugador que tienes "señalado" en pantalla. Archivado desde jul-2023 |
| **Live Editor (Ruls)** | **No** | Liga y equipo aparecen como **solo lectura**. Hay que señalar al jugador en el juego; si estás en otra pantalla, el valor puede no cambiar. Puede cerrar el juego |
| **Touchline** | Solo dentro de la Liga Máster | Su fichaje "se completa al final del siguiente día jugado": usa el propio proceso de la Liga Máster, no mueve plantillas a mano. Solo FL26 v1.4 |

**Conclusión:** mover plantillas en memoria fuera de la Liga Máster no lo ha resuelto nadie públicamente.

---

## 3. Qué ya aprendió Phoenix (tus documentos)

| Intento | Resultado | Por qué |
|---|---|---|
| Leer memoria directo con Sider | ❌ cierra el juego | ReShade/DLSS liberan memoria en cualquier momento |
| Escribir Velocidad 99 en memoria | ❌ la pantalla sigue en 90 | El juego rehace sus copias desde el archivo; la pantalla lee otra versión |
| Cambiar `Player.bin` / option file + recargar | ✅ | Funciona sin reiniciar |
| Interruptor escondido + botón "Activar" revivido | ✅ | Recarga sin entrar a Editar |

**Lección registrada por la otra cuenta:** "cambiar la memoria es frágil; mejor camino: los archivos". Esta investigación lo confirma.

---

## 4. La pista más importante: por qué falla "a veces"
Según tus propias pruebas del 10-oct, el juego **saca los equipos de dos lugares distintos**:

| Opción "Datos Actual. en vivo" | Los equipos salen de… |
|---|---|
| **Activada** | la **base** (`PlayerAssignment.bin` de Phoenix-DB) |
| Desactivada | el **option file** (`EDIT00000000`) |

Y hay un problema conocido sin resolver (punto 1 de "Problemas conocidos"): **después de entrar a Editar, el juego dice "Activada" pero usa los equipos del option file.**

**Hipótesis principal (a comprobar):** el "a veces sí, a veces no" ocurre cuando el fichaje está en una fuente y el juego está leyendo la otra. Por ejemplo: entraste a Editar antes del partido, o el fichaje se escribió solo en una de las dos.

Otras causas posibles:
1. El aviso de recarga llega antes de que el archivo termine de escribirse.
2. El juego no pasa por el punto donde revisa el interruptor (depende de la pantalla).
3. Las stats nuevas con el experimento B (sin parche C) **nunca se probaron** (punto 2 de "Problemas conocidos").
4. Exe distinto: los parches revisan los bytes antes de escribir y, si no coinciden, no hacen nada (correcto, pero silencioso).

---

## 5. Plan recomendado (para aprobar)

1. **Escribir siempre en las dos fuentes.** Que cada fichaje vaya al option file **y** a `PlayerAssignment.bin`, siempre iguales. Así no importa cuál lea el juego. (Revisar si Sync ya lo hace en todos los modos.)
2. **Detectar la fuente real.** Arreglar el problema conocido nº 1: detectar la relectura de 11 archivos que hace Editar y marcar "usando option file".
3. **Avisar en vez de fallar en silencio.** Si el interruptor no se puede encender o el exe no coincide, mostrarlo en el overlay y en Link.
4. **Confirmar después de recargar.** Tras recargar, leer (sin escribir) un dato de control para saber si el fichaje entró, y si no, pedir "Activar" otra vez.
5. **Prueba guiada** con tu amigo: 10 fichajes, anotar en cuáles falló y en qué pantalla estabas.

**Dato que ayuda mucho:** en Parsec solo corre el juego del **host**. Basta con que la recarga funcione bien en la PC del host.

---

## 6. Qué sí se podría hacer en memoria (más adelante)
Cosas pequeñas y del partido actual, no plantillas:
- Forma del jugador (flecha) según su racha.
- Público del estadio (ya existe un módulo, Crowd Attendance).
- Presupuesto en la Liga Máster.
Siempre con la regla de Phoenix: revisar bytes antes de escribir, nunca durante un partido, y que falle sin cerrar el juego.
