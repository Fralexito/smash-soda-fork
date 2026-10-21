# 08 · Informe y guía: fichajes y dinero en la Liga Máster interna de PES 2021

**Para FRALEX · 9 de octubre de 2026.** Resume el trabajo del 8 de octubre (y lo aprendido el 9) sobre los guardados de la Liga Máster.
Detalle técnico completo: `liga-master/ESTRUCTURA-ML.md` (§1–§20) y `01-MOTOR-PES2021.md` (§C–§I). Diario de pruebas: `../PRUEBAS.md` (Parte 2).

---

## 1. La idea en simple

Cada carrera de Liga Máster es **un archivo propio** en `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\239200\save\`:

- `ML00000000` = ranura 1 del menú «Cargar», `ML00000001` = ranura 2… (la ranura N es el archivo `ML0000000(N−1)`, contado en hexadecimal: la ranura 11 es `ML0000000A`).

Ese archivo es como **el cuaderno de la carrera**: guarda las plantillas de los 700 equipos, tu equipo con todos sus datos, contratos, dinero, calendario y una «ficha de Liga Máster» por jugador.

**Por eso los cambios del option file o de la base NO llegan a una carrera que ya existe**: la carrera tiene su propio cuaderno. Para cambiar una carrera, Phoenix Sync escribe **directamente en ese cuaderno**.

---

## 2. Lo que logramos (todo probado dentro del juego)

| Qué | Estado | Prueba |
|---|---|---|
| Abrir y volver a cerrar el archivo (está cifrado) | ✅ | El juego acepta los guardados editados |
| Traspaso entre dos equipos de la IA (por ejemplo, Isak: Liverpool → Santos) | ✅ | Ranura 9: plantilla, dorsal y alineación correctos |
| Vender un jugador de **tu** equipo a la IA (por ejemplo, Guéhi: City → Real Madrid) | ✅ | Ranuras 8–10: tu Estrategia sin huecos ni porteros en el ataque |
| Fichar **para** tu equipo desde la IA (por ejemplo, Sommer: Inter → City) | ✅ | Pruebas 18 y 19: jugó de titular, un solo contrato |
| Cambiar tu **presupuesto de fichajes** y el **tope salarial** | ✅ | Ranura 12: 500 M en pantalla |
| Cambiar el **valor de mercado** de un jugador | ✅ | Prueba 17: Sommer valía 77,7 M en pantalla |
| Que los cambios **aguanten un partido** y un guardado | ✅ | Ranura 9 jugada y guardada en la 10: el juego no deshizo nada |
| Stats (velocidad, etc.) dentro de una carrera ya empezada | ❌ todavía no | Con Editar → Cargar no llegaron (prueba del 9 oct, 04:53). Falta la prueba de reiniciar el juego |

---

## 3. Cómo está hecho el cuaderno (lo justo para entender)

1. **700 bloques de equipo.** Cada uno tiene su lista de hasta 40 jugadores, sus dorsales y un contador.
2. **La alineación de cada equipo de la IA** (titulares, banca y 6 roles: capitán, lanzadores…). Se guarda **aparte**, en otro bloque.
3. **Tu equipo** guarda mucho más: **12 tablas** con un registro por jugador (datos de plantilla, historial, estado…), tu orden de formación, la lista de la Estrategia, los **contratos** y las **negociaciones abiertas**.
4. **El dinero:** presupuesto de fichajes y tope salarial. El presupuesto salarial que ves en pantalla es el tope menos la suma de sueldos.
5. **Un bloque comprimido** («blob») con la **ficha de Liga Máster de cada jugador**: sueldo, valor de mercado, contrato, competiciones…

Cada pantalla del juego lee una parte distinta. Por eso hay que tocar **todas las partes a la vez**: si no, el jugador queda «medio fichado».

---

## 4. Guía: cómo se hace un fichaje en la Liga Máster

### Lo que hace Phoenix Sync (el programa)
1. **Copia de seguridad** del guardado antes de tocar nada.
2. **Abre** (descifra) el archivo.
3. **Localiza todo por «anclas»** (busca a los jugadores conocidos de tu plantilla), no por direcciones fijas. Así funciona con cualquier carrera y con cualquier equipo.
4. **Mueve al jugador** según el caso:
   - **IA → IA:** sale de la lista del club de origen y entra al final del de destino. Se corrigen las dos alineaciones: si se va un titular, entra un sustituto de su misma posición (un portero solo lo cubre otro portero).
   - **Tu equipo → IA (venta):** además vacía o compacta sus registros en tus 12 tablas igual que lo hace el juego, borra **todos** sus contratos y negociaciones, y arregla tu orden de formación, los roles y la Estrategia.
   - **IA → tu equipo (fichaje):** crea sus registros en tus tablas copiando a un compañero de su posición, lo pone al final de la formación, descuenta el dinero y prepara su ficha del blob. **El contrato no se escribe**: el juego lo crea solo desde la ficha. Si se escribe, queda duplicado.
5. **Dorsal** del que llega: el que se pida → el que tenía en su club → el de su selección → el más alto libre.
6. **Cierra** (vuelve a cifrar) y verifica el archivo.

### Lo que haces tú (FRALEX)
1. **No tengas cargada esa ranura** en el juego mientras se cambia (puedes estar en el menú).
2. Ve a **Liga Máster → Cargar** y elige la ranura.
3. Revisa tres pantallas: **Equipos** (plantilla), **Alineación** del club que recibió al jugador y tu **Estrategia**.
4. Juega un partido y **guarda en otra ranura**, para tener siempre la anterior como respaldo.

---

## 5. Reglas de oro (aprendidas a golpes)

1. **Siempre trabajar sobre una copia** y guardar el original. El juego **autoguarda encima de la ranura cargada** al terminar un partido.
2. **Anotar la huella (md5/sha256)** de cada archivo entregado: así se sabe si es el nuestro o uno que ya reescribió el juego.
3. **Tocar todas las partes a la vez:** listas, dorsales, contador, alineaciones, tablas de tu equipo, contratos y Estrategia.
4. **Sustitutos por posición:** la alineación guarda «números de puesto»; si se quita a alguien sin corregirlos, se corre todo (así apareció un portero en el ataque, ranura 5).
5. **Las alineaciones de la IA van aparte:** si no se actualizan, el que llega sale como un jugador en blanco «DC 0» (ranura 8).
6. **No suponer tamaños:** la banca puede ser de 7 o de 12 según la partida.
7. **Después de un partido** aparece la «ficha del último partido» (solo los que jugaron): no es una tabla de plantilla y no se toca.
8. **El menú Cargar** muestra el «texto info» del guardado, no su nombre: para distinguir ranuras de prueba se cambia la primera línea de ese texto.

---

## 6. Lo que el juego hace por su cuenta (para no pelear con él)

- Tras un partido reescribe partes del archivo (contratos, calendario, fichas de quienes jugaron, puntos), pero **no deshace** las plantillas ni las alineaciones editadas.
- Al avanzar los días hace sus propios fichajes de la IA (cientos en el último día de mercado).
- Crea jugadores propios (los «regens») y los reparte entre la IA.
- Los clubes de la IA **no tienen presupuesto guardado**: fichan por reglas.

---

## 7. Lo que falta

1. **Stats dentro de una carrera ya empezada:** comprobar si cambian al **reiniciar** el juego con un `Player.bin` nuevo en Phoenix-DB. Si no, buscar dónde guarda la carrera las habilidades (en el blob no están; se buscó a fondo).
2. **Agentes libres** (jugadores sin club) en la Liga Máster.
3. **Calendario de partidos:** aún sin localizar del todo.
4. **Conectar con la web:** que un traspaso aprobado en tu página se aplique solo a la carrera elegida (el motor de sincronización firmado ya existe en Phoenix Sync).
5. Que **Phoenix Link** avise: «Fichaje aplicado en tu carrera. Carga la ranura X».

---

## 8. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| Programa (C++) | `PhoenixSync/core/LigaMaster.cpp` y `.h`, `core/Alineacion.h`, `core/BlobLM.*` |
| Pruebas automáticas | `PhoenixSync/pruebas/main.cpp` (más de 290 comprobaciones) |
| Estructura completa | `PhoenixSync/liga-master/ESTRUCTURA-ML.md` |
| Prototipos (Python) | `PhoenixSync/liga-master/prototipos/` |
| Herramientas de cifrado | `PhoenixSync/liga-master/herramientas/` (`dec`, `enc2`, `info`) |
| Diario de pruebas | `PhoenixSync/PRUEBAS.md`, Parte 2 |
| Guardados de referencia en tu PC | `…\239200\save\_referencias_mercado\` |
