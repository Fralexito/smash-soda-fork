# Más de 8 mandos en PES 2021 — lo que hace Phoenix Link (0.3.41–0.3.42, EXPERIMENTAL)

**¿Ya se puede jugar con más de 8 mandos? Todavía NO del todo.** Link ya crea los puestos 9 a 16 y escribe el archivo
que PES necesita para ellos, pero PES solo los usará cuando esté instalado el **módulo de Sider de mandos extra**
(se hace aparte, en `D:\Frank\Games_\PRUEBA_CANCHA\modulo_sider`). Sin ese módulo, los puestos 9+ existen en Link
y en Windows, pero PES los ignora.

Complementa a `docs/ORDEN-MANDOS-PES.md` (el orden de los puestos 1–8 en settings.dat).

## Por qué hace falta un archivo nuevo

- `settings.dat` de PES tiene **8 casillas** de jugador (GUID del mando + mapa de botones). No se toca: sigue igual
  que con «Orden de mandos en PES» (casilla N = puesto N, del 1 al 8).
- El motor del partido tiene 24 ranuras y el gestor de mandos admite 32 (ver libro, `cap_60_k_estrategia_mas_mandos.md`).
- Para los puestos 9+ Link escribe **`phoenix_mandos_extra.dat`** junto a `settings.dat`:
  `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\phoenix_mandos_extra.dat`

### Formato del archivo (versión 1)

| Posición | Tamaño | Qué es |
|---|---|---|
| 0x00 | 4 | `PHXM` (texto) |
| 0x04 | u32 | versión = 1 |
| 0x08 | u32 | N = cantidad de casillas |
| 0x0C | N × 0x40 | casilla i = puesto 9+i, **mismo formato que settings.dat**: GUID de instancia DirectInput (16 bytes) + 24 botones de 2 bytes (48 bytes) |

- Todo en little endian. GUID a cero = ese puesto está desenchufado.
- El mapa de botones es el de la casilla 1 de `settings.dat` (el mapa Xbox que ya usa PES); si está vacío, el Xbox de fábrica.
- Si bajas a 8 puestos y había archivo, Link lo deja con N = 0 (así el módulo no lee mandos viejos).
- Código: `SmashSoda/phoenix/core/MandosExtra.{h,cpp}`; se escribe desde `OrdenPes.cpp`.

### Reglas (las mismas que settings.dat)

1. Solo con **PES cerrado** (y Settings.exe cerrado). Si está abierto, Link avisa y espera.
2. **Copia de seguridad** antes del primer cambio de cada sesión: `phoenix_mandos_extra.dat.phoenix-AAAAMMDD-HHMMSS`
   (si antes no había archivo, no hay nada que copiar).
3. **Escritura atómica**: se escribe `…phoenix-tmp` y se cambia de golpe; si algo falla, el archivo de antes queda intacto.
4. Solo funciona con **«Orden de mandos en PES»** encendido (es el mismo módulo, en Mandos › Puestos).
5. Link **no toca sider.ini** ni instala módulos de Sider.

## Cuántos mandos virtuales aguanta ViGEmBus (medido)

19-oct-2026, PC de Fralex, ViGEmBus 1.21.442: se pidieron 40 mandos Xbox 360 seguidos, tres veces.
Resultado: **21, 19 y 16** mandos a la vez (contando los que ya hubiera); el siguiente falla con `0xE0000007`
(TARGET_NOT_PLUGGED_IN: Windows no llega a enchufarlo a tiempo). Lo único seguro es **16**, por eso el tope de Link
es 16 y no 22. El programa de la medición crea los mandos, espera 3 s y los quita todos.

## Paso a paso (anfitrión)

1. Abre Link → **Mandos › Puestos**. Abajo, en la tarjeta **ORDEN DE MANDOS EN PES**, enciende el interruptor.
2. En la misma tarjeta, **PUESTOS MÁXIMOS**: súbelo de 8 a los que quieras (hasta 16). Sale la marca EXPERIMENTAL.
3. Ve a **Mandos › Tu mando › Mandos virtuales** (despliega la tarjeta) y sube **Xbox** (por ejemplo a 12). Desde 0.3.43 sube directo hasta 16 y «Puestos máximos» se ajusta solo.
   Hazlo con la sala cerrada; Windows tarda unos segundos en registrar cada mando nuevo.
4. Vuelve a **Puestos**: los puestos 9+ salen con la etiqueta **EXPERIMENTAL · Necesita el módulo de mandos extra**.
   Se meten en el Equipo A o B igual que los demás (botones A / B o «+ AÑADIR PUESTO»).
5. Con **PES cerrado**, la tarjeta pasa a «Ordenado» y muestra las casillas **PES 1–8** y **EXTRA 9–16** con ✓.
   Debajo: la copia de `settings.dat` y la de mandos extra.
6. Instala el módulo de Sider de mandos extra (va aparte) y abre PES.

Para volver a lo de siempre: Puestos máximos = 8 (Link quita los mandos que sobran, primero los de PlayStation).

## Qué cambió en Link (0.3.41)

- Ajuste nuevo `puestosMax` (8–16, por defecto 8) en las preferencias; acción `mandos.puestosMax{n}`.
- `mandos.cantidad` acepta hasta `puestosMax` mandos (antes 8 fijo).
- Equipos A/B hasta 16 puestos (`core/EquiposPuestos.h`): el mapa sigue siendo de 8 letras y solo crece si un puesto 9+
  tiene equipo, así las preferencias viejas siguen valiendo.
- Todas las vistas de la sala (árbitro, lados, partido, sala clasificatoria…) miran hasta 16 puestos; candado de
  puesto, «!cambio N» y «tomar puesto» aceptan hasta 16.
- Estado nuevo para la interfaz: `mandos.puestosMax|puestosTope|puestosPes`, `mandos.lista[].extra`,
  `ordenPes.casillas[].extra`, `ordenPes.copiaExtra` (ver `SmashSoda/phoenix/web/PROTOCOLO.md`).
- Interfaz: se amplió la de siempre (Puestos y Tu mando), sin rehacer nada.

## Pruebas

- `pruebas-link\correr_mandosextra.bat` → formato (firma, versión, N, casillas, GUID a cero, archivos no válidos),
  mapa de la casilla 1, escritura atómica (sin temporal sobrante; archivo bloqueado = queda intacto) y copia (una por
  sesión, con el contenido anterior). **TODO OK**.
- `pruebas-link/equipos_test.cpp` (compila con g++ o cl) → casos nuevos de 9 a 16 puestos. **todo bien**.
- Compilado con `cmake --build build --config Release` (nunca `COMPILAR_PHOENIX.bat`).
- Vista previa: simulador con 12 puestos: fila del 10 con la marca EXPERIMENTAL y casillas EXTRA 9–12.

## Lo que falta (fuera de Link)

- El módulo de Sider que lee `phoenix_mandos_extra.dat` y hace que PES use los mandos 9+.
- Probarlo en un partido real cuando termine la pausa de pruebas.

> Si tras compilar no ves «Puestos máximos»: la carpeta `x64\Release\ui` quedó vieja (la copia falla con Link abierto). Cierra Link y vuelve a compilar con `cmake --build`.
