# Mapeo de Touchline (v0.8.0 → v0.9.9.4) y qué añadir a Phoenix
Investigado el 30-oct-2026. Fuentes: notas de versión en GitHub (0.9.9, 0.9.9.1, 0.9.9.3, 0.9.9.4), guía de instalación oficial, web del autor (askandalus.com), fichas de pes-files (0.8.0, 0.9.0, 0.9.9).
No se pudo leer el código: el repositorio es "solo releases" (programa cerrado).

---

## 1. Cómo está construido (arquitectura)

| Pieza | Qué es |
|---|---|
| **Programa de escritorio** | En Python, ventana propia al lado del juego. Windows 10/11, sin firma digital |
| **Lectura** | Lee la partida guardada de Liga Máster (club, fecha, plantilla, tácticas del rival). **Nunca edita el save**: el juego guarda solo |
| **Escritura en vivo** | Lee y escribe la **memoria del juego** mientras corre ("puente en vivo"). Necesita **permisos de administrador** y **FL26 v1.4** (o un exe con el mismo código) |
| **Módulos Lua de Sider** | `MLRuntime.lua`, `MLImmersion.lua`, `MLRegenAliases.lua`, `MLForcedTransfers.lua` |
| **Datos propios** | `content\ml-immersion\profiles` dentro de Sider (su "base de datos" de la carrera) |
| **Instalador** | Busca Sider, muestra todo lo que va a cambiar, respalda `sider.ini`, nunca borra archivos del usuario, desactiva `BudgetSetter.lua` (choca) |
| **Seguridad de versión** | Revisa solo las partes del exe que necesita (no exige el archivo idéntico byte a byte) |
| **Modo sin juego** | Sin v1.4 o sin admin → funciones que solo leen el save |

**Lección clave:** separa lo que "lee" (siempre funciona) de lo que "escribe en vivo" (solo con versión exacta). Si algo falla, baja a modo seguro en vez de romperse.

---

## 2. Mapa de funciones

Leyenda de la columna **Cómo**: 📖 lee el save · ⚡ escribe en vivo · 🖥️ solo en la app

### A. Mundo y narrativa
| Función | Detalle | Cómo |
|---|---|---|
| Historias | Hechos de tu carrera se vuelven historias | 📖🖥️ |
| Bandeja de entrada | Mensajes de jugadores, staff y directiva, en formato carta | 🖥️ |
| Jugadores con opinión | Reaccionan a minutos, resultados, contrato | 📖🖥️ |
| Directiva con memoria | Recuerda lo que prometiste y cómo te fue | 📖🖥️ |
| Prensa | Preguntas de prensa que responde el usuario | 🖥️ |
| Afición | Ánimo de la hinchada | 📖🖥️ |
| Eventos aleatorios | Ingresos inesperados, congelamiento de gastos (desactivado por defecto) | ⚡ |
| Archivo / historia | Temporadas pasadas guardadas | 🖥️ |
| Mundo | Noticias generales del fútbol | 📖🖥️ |

### B. Dinero y club
| Función | Detalle | Cómo |
|---|---|---|
| Patrocinadores y socios de estadio | Pagan al cumplir objetivos | ⚡ (presupuesto) |
| Mandatos de temporada | Metas de la directiva (p. ej. 10 victorias de visita) | 📖 |
| Fondos de la directiva | Botón para cobrar al cumplir una meta | ⚡ |
| Bonos por contrato | Bonos de jugadores salen del presupuesto de sueldos | ⚡ |
| Contratar staff | Cobra a los presupuestos al presionar el botón | ⚡ |
| Finanzas | Ingresos de la temporada en el formato del juego | 📖 |
| Logros | Cambian cómo te ve la directiva (no dan dinero) | 🖥️ |
| Interruptor "Economía en vivo" | Si se apaga, los pagos esperan | 🖥️ |

### C. Plantilla y jugadores
| Función | Detalle | Cómo |
|---|---|---|
| Dorsales para cualquier club | Editar/intercambiar números; se ven en el juego y quedan al guardar; deshacer | ⚡ |
| Arcos de forma | Sube la flecha de forma un paso por un partido si la historia lo merece | ⚡ |
| Entrenamiento con staff | Sube un atributo trabajado por meses | ⚡ |
| Desarrollo en vivo | Cambios de stats en tiempo real | 📖 |
| Regens con nombre nuevo | Juveniles generados reciben nombres nuevos | ⚡ (Lua) |
| Traspasos forzados | Agentes libres/regens pasan directo a clubes top 30 según su nivel | ⚡ (Lua) |
| Repatriar cedidos | Costo según el tiempo de cesión | ⚡ |
| Negociar fichajes | Oferta, contraoferta, sueldo, años, rol; cláusula = el club no puede negarse; se completa al final del siguiente día jugado | ⚡ |
| Poner en venta / suspender | Solo con botón | ⚡ |
| Retiro → cuerpo técnico | Jugadores retirados ofrecidos como entrenadores | 🖥️ |
| Caras | Minicaras del propio juego; foto propia; iniciales para regens | 📖 |

### D. Partido
| Función | Detalle | Cómo |
|---|---|---|
| Preparación paso a paso | Asistente de previa | 🖥️ |
| Tácticas del rival completas | Estilo de ataque, salida, zona, posicionamiento, defensa, presión, formación | 📖 |
| Sugerencia de formación | Propone formación contra el rival y explica por qué | 🖥️ |
| Público según el partido | Estatura del club, rival, clásico, fase de copa, pelea por título/descenso, forma, ánimo, entre semana. Modos: apagado, realista, dramático | ⚡ |
| Momentum Shift | Cambio de inercia (no documentado en detalle) | ⚡ |
| Registro de resultados | Lee el resultado del save, incluso en copas | 📖 |

### E. Carrera del técnico
| Función | Detalle | Cómo |
|---|---|---|
| Despido del técnico | Relajado o realista (exigencia según el club). Dos avisos y luego despido real: se simulan partidos y llegan ofertas de otros clubes | ⚡ |

### F. Interfaz y experiencia
| Función | Detalle |
|---|---|
| Páginas | Inicio, Bandeja, Plantilla, Partido, Fichajes, Club (Junta primero), Staff, Calendario, Archivo, Mundo, Ajustes |
| Inicio en 3 columnas | Próximo partido con últimos 5 de ambos, bandeja/noticias, tabla completa |
| Plantilla | Por líneas, con tabla de profundidad y el XI dibujado en una cancha |
| Calendario | Mes en cuadrícula al lado de los próximos partidos |
| Navegación atrás/adelante | Como Football Manager; clic derecho muestra los últimos 20 lugares; B del mando o Escape |
| Mando | Se puede usar la app con mando; interruptor para soltarlo |
| Velocidad | Prepara páginas en segundo plano; guarda solo si algo cambió |
| Idiomas | 10 (incluye español, portugués, árabe con lectura derecha-izquierda, tailandés) |
| Pantallas chicas | Las pestañas pasan a una segunda fila |

### G. Compatibilidad
- Funciona con exes modificados que también tocan el presupuesto (GOAT Rush).
- Si el juego corre como administrador, Touchline también puede leerlo.
- Avisa "equipo no coincide" si el save conectado no es la carrera abierta.

---

## 3. Lo que Touchline NO tiene (tu espacio)
- **Multijugador.** Es solo para un jugador.
- **Web.** Todo vive en una PC.
- **Torneos, comunidad, perfiles.**
- **Control de sala de Parsec, mandos y juego justo.**
- **Mover plantillas completas de forma masiva** (solo fichajes uno por uno, dentro de la Liga Máster).

---

## 4. Qué añadir a Phoenix (priorizado)

Dónde iría: **W** = web · **L** = Phoenix Link · **S** = Phoenix Sync (módulo Lua)

### Prioridad alta (encaja directo con lo que ya tienes)
| Idea | Para Phoenix | Dónde | Dificultad |
|---|---|---|---|
| **Modo seguro por versión** | Revisar solo las partes del exe que usa Sync; si no coincide, bajar a "solo archivo" en vez de fallar a medias | S/L | Media |
| **Instalador que muestra todo antes** | Ya tienes "Arreglar todo"; añadir la lista de cambios + respaldo de `sider.ini` antes de tocar | L | Fácil |
| **Dorsales en vivo** | El DT elige dorsales en la web y aparecen en el juego del host | W+S | Media |
| **Público según el partido** | Clásico de la Galaxy League = estadio lleno; ya existe el módulo Crowd Attendance como base | S | Media |
| **Tácticas del rival + sugerencia** | Antes de un reto, la web muestra cómo juega el rival (de sus partidos anteriores) | W | Media |
| **Negociación de fichajes** | Oferta → contraoferta → sueldo/años/rol en el Mercado de tu Liga Máster web | W | Media |
| **Registro de resultado desde el save** | Respaldo cuando Link no detecta el marcador | L/S | Media |

### Prioridad media (la capa de "experiencia")
| Idea | Para Phoenix | Dónde |
|---|---|---|
| Bandeja con cartas de jugadores y directiva | Avisos de la web con personajes ("tu delantero pide minutos") | W |
| Mandatos de temporada y patrocinadores con objetivos | Metas por club en la Liga Máster web; dinero al cumplir | W |
| Despido del técnico (opcional) | Si un DT acumula malos resultados, la directiva lo avisa y puede quitarle el club | W |
| Prensa | Preguntas tras partidos destacados; la respuesta sale en Noticias | W |
| Arcos de forma | Racha buena = flecha de forma arriba en el siguiente partido del host | S |
| Inicio en 3 columnas | Próximo partido + últimos 5, noticias, tabla | W |
| Navegación atrás/adelante y uso con mando | En Phoenix Link | L |

### Prioridad baja (para después)
| Idea | Dónde |
|---|---|
| Regens con nombre y traspasos forzados | S |
| Retiro → cuerpo técnico | W |
| Eventos aleatorios | W |
| Repatriar cedidos con costo | W |

### No copiar
- **Depender de una sola versión del exe.** Touchline exige FL26 v1.4. Phoenix debe detectar la versión y bajar a modo seguro.
- **Pedir administrador para todo.** Solo para lo que de verdad lo necesite.

---

## 5. La gran diferencia de enfoque
Touchline pone la "historia" **alrededor de una partida de un jugador**.
Phoenix puede poner la historia **alrededor de una liga entera de personas reales**: la prensa habla de tus amigos, la directiva compara DTs reales, los clásicos son rivalidades de verdad. Eso Touchline no lo puede hacer.
