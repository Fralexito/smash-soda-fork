# Tablero del online Phoenix (se actualiza con cada avance)

Última actualización: 10-nov-2026, 23:50 (hora del servidor UTC).
PES del online: solo el base `C:\PES21\eFootball PES 2021`.

## Resumen
- **Probado en el PES y funcionando:** 20 de 48 apartados (**42 %**).
- **Programado (falta probarlo en el PES):** 14 más. Con ellos, **34 de 48 ya están hechos (71 %)**.
- **A medias** (abre bien pero sin contenido todavía): 6.
- **Falta hacer:** 8.
- Comandos del juego que el servidor ya controla: **206 de 439 (47 %)**; sin contar los modos aún no empezados (cooperativo, Matchday, eventos, PES League, clanes, tienda): **202 de 313 (65 %)**.

**Leyenda:** ✅ probado en el PES · 🟢 programado, sin probar en el PES · 🟡 a medias (abre, pero vacío) · ❌ falta · 🚫 no se hará (a propósito)

**Lo que más falta:** la prueba con **2 PES reales**. Es lo único que confirma un partido online completo (los apartados 9 y 20).

## 1. Entrar al online
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 1 | Conectar, aceptar condiciones, crear usuario | ✅ | desde el 02-nov |
| 2 | Menú online y avisos (incluido el «código Phoenix») | ✅ | |
| 3 | Perfil y «Editar perfil» | ✅ | |
| 4 | Datos en vivo (sin actualización pendiente) | ✅ | |
| 5 | Mantenimiento, misma versión y mismo parche obligatorio | 🟢 | apagado hasta probar con 2 PES |

## 2. Partida rápida y Divisiones (1 contra 1)
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 6 | Buscar rival | ✅ | |
| 7 | «Revisando estado de conexión» | ✅ | superado el 10-nov |
| 8 | «Enviando y recibiendo datos» | ✅ | con el rival de prueba |
| 9 | Carga y partido completo | 🟢 | el relé está listo; **falta la prueba con 2 PES** |
| 10 | Resultado y cierre del partido | 🟢 | |
| 11 | Pantalla de división (D12, partido x de 10) | ✅ | probada el 10-nov: D12, partido 1/10, 300 para ascender |
| 12 | Puntos, rating, ascenso y descenso de D12 a D1 | 🟢 | reglas cambiables sin tocar código |
| 13 | Pantalla «Cambios en el rating» | 🟢 | |
| 14 | Récords por división | 🟢 | |
| 15 | Resultados de partidos | ✅ | abre bien (vacío hasta jugar partidos) |
| 16 | Ranking de Divisiones | ✅ | abre; arreglado que salían nombres repetidos |
| 17 | Análisis del rival | 🟡 | vacío hasta jugar partidos |
| 18 | Abandono durante el partido (3-0) | 🟢 | quien sale con el partido en juego pierde 3-0; un corte de conexión no cuenta |

## 3. Amistoso con un amigo (salas)
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 19 | Crear sala, unirse, lista de salas | ✅ | |
| 20 | Partido en la sala | 🟢 | va directo entre las dos PC; **falta la prueba con 2 PES** |

## 4. myClub
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 21 | Entrar a myClub y club propio | ✅ | |
| 22 | Plantilla, alineación y plan de juego | ✅ | |
| 23 | Partido contra la COM | ✅ | primer partido completo el 05-nov; récord contra la COM guardado |
| 24 | GP y premios por partido | ✅ | |
| 25 | Fichajes: agentes, cazatalentos y subastas | ✅ | con los jugadores de tu parche |
| 26 | Contratos, entrenadores y experiencia | ✅ | |
| 27 | Cambiar el nombre del club y de la plantilla | 🟢 | falta encontrar la pantalla en el PES para probarlo |
| 28 | Historial | 🟡 | vacío hasta conocer sus tipos |
| 29 | Regalos | 🟡 | vacío |
| 30 | Logros | 🟡 | vacío |
| 31 | Ranking de clubes | ✅ | probado: muestra «Phoenix FC» con escudo |
| 32 | Ranking semanal | 🟡 | vacío; el PES lo bloquea hasta jugar 10 partidos 1 contra 1 |
| 33 | myClub contra otra persona | ❌ | |
| 34 | Monedas gratis (interruptor) | 🟡 | programado; falta tu decisión de encenderlo |
| — | Tienda con dinero real | 🚫 | nada de dinero real |

## 5. Otros modos del online
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 36 | Cooperativo (2 contra 2, 3 contra 3) | ❌ | el relé ya admite más de 2 |
| 37 | Matchday | ❌ | |
| 38 | Eventos y competiciones online | ❌ | idea: torneos de la liga Phoenix desde la web |
| 39 | PES League | ❌ | |
| 40 | Clanes | ❌ | |

## 6. Lo propio de Phoenix (fuera del juego)
| # | Apartado | Estado | Nota |
|---|---|---|---|
| 41 | Launcher: «Jugar Offline» | ✅ | probado el 10-nov |
| 42 | Launcher: «Jugar Online» con la cuenta de la web | 🟢 | falta probarlo de punta a punta |
| 43 | Tu web recibe los clubes de myClub | ✅ | conectado el 10-nov, sin la llave maestra |
| 44 | Tu web recibe las Divisiones (ranking online) | 🟢 | migración 165 aplicada; falta la tabla en la página |
| 45 | Vincular el PES con la cuenta de la web | 🟢 | el servidor está listo; falta el botón en la web |
| 46 | Dar GP y monedas desde la web (admin) | 🟢 | |
| 47 | Desvío del online con Phoenix Core (sin «hosts») | 🟢 | Core 0.5.66 listo; se instala después de la prueba de Street |
| 48 | Amigos por Tailscale | ❌ | falta instalar Tailscale (con tu permiso) |
| 49 | Servidor en internet (VPS, sin tu PC encendida) | ❌ | |

## Historial de este tablero
- 10-nov 23:20: primera versión.
- 10-nov 23:35: probadas en el PES la pantalla de división, resultados y los dos rankings (ningún cierre); arreglos de ranking; abandono 3-0 programado.
- 10-nov 23:50: ranking de clubes con nombre real y escudo; récord contra la COM en myClub; al abrir myClub ya no queda ningún comando sin respuesta.
