# 16 · Player.bin — ficha completa de un jugador (calibrada con capturas del juego)

Fecha: 9 oct 2026 · Autor: Claude (cuenta del chat de Player.bin) para FRALEX · Estado: 🔎 OBSERVADO / ✅ CONFIRMADO con pantallas del juego
Complementa y amplía §J de `01-MOTOR-PES2021.md`. No sustituye nada.

## 0. Cómo leer este documento (clasificación por capa)

Cada dato lleva una etiqueta de **a qué parche aplica**. No hay que encasillarse en un solo parche:

| Etiqueta | Significa |
|---|---|
| 🧩 MOTOR | Es del motor de PES 2021 → vale para **TODOS** los parches (ConmeGOL, Sudamerican, Football Life, Evoweb, Gogosz, Phoenix). |
| 🅲 CONMEGOL 26 | Dato propio de ConmeGOL 26 (plantillas, tamaños, rutas, versión). |
| 🅢 SUDAMERICAN | Dato propio de Sudamerican Project 2026. |
| ⏳ PENDIENTE | Aún no comprobado en ese parche. |

Regla: el **formato** del registro (bits, tamaños) es 🧩 MOTOR (lo define el .exe). Los **valores** (qué jugador, qué stats) son del parche.

## 1. Archivo y registro 🧩 MOTOR (formato) · 🅲 (tamaños de ConmeGOL 26)

- Ruta viva en ConmeGOL 26: `D:\Frank\Games_\Conmegol Patch\SiderAddons\olmosjr23\Database\common\etc\pesdb\Player.bin` (base real de olmosjr23; 5.894 jugadores difieren de `CGP_database.cpk`). 🅲
- Formato: cabecera WESYS 16 B + zlib. Descomprimido: **9.359.064 B = 29.997 registros × 312 B**. 🅲 (el tamaño de registro 312 B es 🧩)
- `pid` = u32 en el byte +8 del registro. 🧩
- Convención de bits: se lee el registro entero como número little-endian; campo = `(reg >> bit) & máscara`.
- Habilidades (stats): 6 bits; **valor mostrado = crudo + 40**. En pantalla puede diferir ±1 (ver §6).

## 2. Estadísticas (6 bits, valor = crudo + 40) — ✅ CONFIRMADO 🧩

| Stat en pantalla | Bit | Stat | Bit |
|---|---|---|---|
| Actitud ofensiva | 370 | Velocidad | 306 |
| Control del balón | 281 | Aceleración | 344 |
| Regate | 352 | Equilibrio | 376 |
| Conservación del balón (regate ajustado) | 416 | Contacto físico | 390 |
| Pase raso | 263 | Salto | 294 |
| Pase bombeado | 402 | Resistencia | 338 |
| Finalización | 396 | Actitud defensiva | 275 |
| Cabeceo | 288 | Recuperación de balón | 312 |
| Balón parado | 250 | Agresividad | 384 |
| Efecto | 332 | Potencia de tiro | 358 |

Portero (6 bits, +40): Despejar **269** · Cobertura **300** · Reflejos **320** · Actitud de portero **326** · Atajar **364**
(confirmado con Joan García, Livakovic, Szczęsny: 83/87/87/87/82).

## 3. Otros campos pequeños 🧩 — ✅ CONFIRMADO

| Campo | Bit | Bits | Nota |
|---|---|---|---|
| Uso del pie malo | 454 | 2 | el juego muestra guardado + 1 |
| Precisión del pie malo | 462 | 2 | idem |
| Forma | 438 | 3 | el juego muestra guardado + 1 |
| Resistencia a lesiones | 458 | 2 | idem |

## 4. Posiciones (2 bits: 0 = C, 1 = B, 2 = A) — ✅ CONFIRMADO 🧩

PT 350 · DFC 468 · LI 318 · LD 474 · MCD 414 · MC 456 · MI 466 · MD 460 · MO 464 · EI 472 · ED 476 · SD 478 · DC 470.
Confirmado con Isak, Mbappé, Dembélé, João Pedro (SD/DC en A, EI en B).

## 5. Habilidades especiales y COM (1 bit cada una, rango 480–531) 🧩

✅ Resueltas (bit → nombre que muestra el juego):

| Bit | Habilidad | Bit | Habilidad |
|---|---|---|---|
| 482 | Sombrero | 506 | Vaselina |
| 483 | Pase cruzado | 507 | Pase al primer toque |
| 484 y 487 | Pase en profundidad (dos bits, mismo texto) | 510 | COM Llegador |
| 485 | Elástica | 511 | Remate al primer toque |
| 486 | Espíritu de lucha | 518 | Tiro de larga distancia |
| 488 | Pase bombeado bajo | 519 | Bicicleta |
| 489 | COM Mago del balón | 520 | COM Cañonero |
| 490 | Patadón por bajo (PT) | 522 | Pase largo de portero |
| 491 | Picardía | 523 | Dos toques |
| 492 | Capitanía (por descarte) | 524 | Finalización acrobática |
| 493 | Centro con rosca | 525 | Rebote interior |
| 494 | Disparo descendente | 526 | COM Misil con el balón |
| 495 | Cabeceador | 527 | Cañonero (habilidad) |
| 496 | Patadón en largo (PT) | 528 | Amago por detrás y giro |
| 497 | Marsellesa | 529 | COM Experto en pases largos |
| 499 | Disparo ascendente | 530 | Despeje acrobático |
| 500 | Pisar el balón | 531 | COM As del eslalon |
| 501 | Especialista en penaltis | 502 | Parapenaltis |
| 503 | Interceptador | 504 | Marcaje |
| 505 | Espuela | | |

⏳ NO resueltas: **Cortada** (sale con los bits 514, 498/515 y a veces 480 según el jugador → probablemente el juego muestra un solo nombre para varias habilidades internas) · bit **514** encendido en Bernal sin mostrar nada · **COM Centrador** (sin bit) · grupo 479/516 «As en la manga» · 498.

## 6. Curiosidades y advertencias

1. **Diferencias de ±1** entre Player.bin y la pantalla: NO se explican por edad (hipótesis retirada: Gordon, Balde, Bernal y Eric García casi idénticos). Explicación más probable (🔎 por confirmar, doc 11 §D): la **Liga Máster no lee Player.bin al entrar a la carrera** y guarda su propia copia de los jugadores. Se comprueba con una prueba de reinicio.
2. «Pase en profundidad» ocupa **dos bits** con el mismo texto; «Cortada» parece lo contrario (un texto, varios bits).
3. Los porteros usan **otros 5 stats** en el mismo registro (269/300/320/326/364) además de los 20 de campo.
4. Pie malo, forma y lesiones se guardan **restando 1** respecto a lo que muestra el juego.
5. Para encontrar a un jugador por sus stats basta cruzar los 20 valores con tolerancia ±1: así se localizaron Isak 115079, Mbappé 110718, Dembélé 110626, Pedro (Flamengo) 111408, João Pedro Chelsea 127787, João Pedro Botafogo 135249, Livakovic 113596, Szczęsny 40937, Cancelo 47787, Bernal 171610, Balde 147233, Gordon 119835, Eric García 126337, Koundé 110784, Yamal 162114, Cubarsí 165696 (pid de ConmeGOL 26 🅲).
6. Livakovic, Hamza Abdelkarim y Gabriel Jesús ya no están en el Barça de la plantilla actual (experimento «Vestuario Barça» desactualizado en 3 jugadores).

## 7. Aún sin mapear (para próximas sesiones)

Rol en el equipo (Leyenda, Estrella Nv.2…), Personalidad (Jugador individualista, Pasión/Aplomo, Técnica/Fuerza, Clarividencia/Instinto), Influencia, tablas de tácticas ofensivas/defensivas (pares tipo 29/89), «Hoja de progreso» (curva de desarrollo), cortada/Centrador.

## 8. Qué parche aprovecha qué (para sacar el parche Phoenix)

| Tema | Todos los parches (🧩) | ConmeGOL 26 (🅲) | Sudamerican (🅢) |
|---|---|---|---|
| Formato de registro y bits | ✅ | — | ⏳ comprobar con 2 jugadores |
| Cantidad de jugadores | — | 29.997 (olmosjr23) | 29.323 |
| Equipos / competiciones | — | 749 / 90 | 726 / 80 |
| Carpeta de guardados | — | `239200` | `292733975847239680` |
| Sider | — | 7.3.3 (~57 módulos) | 7.3.4 (`netblock.lua` bloquea online) |
| Estilo | — | casi todo por Sider/livecpk | CPK clásicos gigantes (`SP_Subs.cpk`) |

Football Life 27, Evoweb, Gogosz y ConmeGOL 27: ⏳ pendiente (ver `12-GUIA-MULTIPARCHE.md`).

## 9. Siguiente paso seguro

Escribir **un** valor de **un** jugador en una **copia** de Player.bin (con respaldo y con aprobación de FRALEX) y verificarlo en el juego. Todavía no se ha escrito nada en Player.bin.
