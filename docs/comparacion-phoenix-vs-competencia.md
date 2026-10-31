# Phoenix vs. la competencia — comparación franca (30-oct-2026)

Fuentes: web y Patreon de My Football Legacy, releases de Touchline en GitHub y su web (askandalus.com), foro PES 2021 de EvoWeb, y los documentos de Phoenix (RESUMEN-WEB, phoenix-link-estado, phoenix-mercado-estado; estado al 8-oct-2026).
Aviso: no pude abrir el hilo de EvoWeb de My Football Legacy ni el README de Touchline (bloqueados). Lo que digo de ellos sale de sus páginas oficiales y notas de versión.

---

## 1. Quiénes son

### My Football Legacy (MFL) — el rival directo
- **Qué es:** una web que hace de "Liga Máster" alrededor de cualquier juego de fútbol: EA FC, PES 2021, Football Life y eFootball, en PC o consola.
- **Cómo funciona cada fecha:** abres el partido en la app → lo juegas en tu juego (amistoso, contra la CPU, en sofá o por Parsec) → subes capturas del resultado y una IA lee marcador, goleadores y estadísticas (o escribes el marcador a mano).
- **Multijugador:** uno crea la partida y comparte un código; los demás eligen club. Comparten tabla, mercado y título. Las ofertas entre amigos llegan a la bandeja y se pueden rechazar.
- **Profundidad:** finanzas, directiva, prensa con IA, academia, entrenamiento, analítica, préstamos bancarios, base de jugadores global, Mundial y "Legacy Champions".
- **No toca nada del juego:** no instala ni modifica archivos. Lo venden como ventaja: "un parche nunca rompe tu carrera".
- **Negocio:** primera temporada gratis; para seguir pagas almacenamiento/temporadas (Patreon desde US$2.99/mes). Solo paga quien crea la partida; los invitados juegan gratis.
- **Tamaño:** dicen tener **6.000+ managers registrados**. Su hilo en EvoWeb tiene 323 respuestas y 22 mil vistas. Existe desde enero de 2025.
- **Equipo:** **una sola persona** (Daniel M.).

### Touchline — el que hace lo mismo que tu Mercado, pero en solitario
- **Qué es:** un programa para Windows que corre al lado de Football Life 2026. Lee tu save de Liga Máster y crea el mundo alrededor: jugadores con opinión, directiva con memoria, prensa, staff.
- **La clave:** **escribe al juego en vivo** con módulos Lua de Sider: dorsales, forma, habilidades, aforo según el partido, despidos del técnico.
- **Límites:** solo un jugador, solo FL26 (y necesita la versión 1.4 del ejecutable o una igual). Sin multijugador, sin web.
- **Ritmo:** 5 versiones en 15 días (0.9.9 → 0.9.9.4, del 10 al 25-oct-2026). Gratis, 10 idiomas.
- **Equipo:** una persona (Ahmad Sheikh-Khalil, gerente de IT), que también hizo **Prologue** (lo mismo para Ser una Leyenda).

### Los demás (menos peligrosos)
- **PESM (en desarrollo):** "Football Manager completo en vivo sobre la Liga Máster". Aún no sale. Vigilar.
- **ML Manager Pro (Ruls):** web gratis de objetivos de carrera y análisis de jugadores. Básica.
- **ML Transfer Assistant (DcV253):** app de ayuda de fichajes. Pequeña.
- **UML / Football Life Mod Studio:** no compiten contigo; son contenido (base de datos, ligas). Son posibles **aliados**.

---

## 2. Tabla comparativa

| Aspecto | Phoenix | My Football Legacy | Touchline |
|---|---|---|---|
| Multijugador entre amigos | Sí (diseñado para liga) | **Sí, ya funcionando** | No |
| Fichajes aparecen dentro del juego | **Sí** (Mercado escribe el option file) | No | Sí (en vivo, un jugador) |
| Respeta el parche de cada jugador (caras, IDs) | **Sí** (Phoenix ID + equivalencias) | No aplica | Solo FL26 |
| Control de la sala de Parsec (mandos, roles, anti-trampa) | **Sí** (Phoenix Link) | No | No |
| Ingreso del resultado | Previsto automático (Link lee el marcador) | **IA con capturas, ya funciona** | Lee el save |
| Torneos, perfiles, comunidad, monedas | **Sí** | No (solo carrera) | No |
| Juegos soportados | PES 2021, FL (FL27 pendiente) | **4 juegos, PC y consola** | Solo FL26 |
| Instalación | Web + programa | **Nada (solo web)** | Programa |
| Riesgo de romperse con un parche | **Alto** (formato binario por parche) | Ninguno | Alto (exige exe 1.4) |
| Usuarios reales hoy | ~50 (tu liga), aún en beta | **6.000+** | Desconocido |
| Publicado | **No** (beta privada, Link sin compilar) | Sí | Sí |
| Idioma / región | **Español, Sudamérica, ConmeGOL/Sudamerican** | Inglés | 10 idiomas |
| Prensa / directiva / historias con IA | No | **Sí** | **Sí** |
| Cobro | Por torneos (plan) | Por temporadas extra | Gratis |

---

## 3. Lo que tienes que oír (lo duro)

1. **Ellos ya salieron; tú no.** MFL tiene 6.000 usuarios. Touchline saca versiones cada 3 días. Phoenix tiene web en beta privada, Link sin una compilación real en Windows y Mercado probado en una sola PC. Hoy, para un jugador de afuera, Phoenix no existe.
2. **Tu alcance es enorme para una sola persona.** Web + base de datos + Link (C++) + Mercado (binarios por parche) + parche propio + FL27 + PWA. Ellos ganaron **haciendo una sola cosa bien**. Tú abriste 6 frentes y los repartiste en varios chats y cuentas. Eso cuesta tiempo en coordinación.
3. **Tu mayor fortaleza es también tu mayor riesgo.** Escribir en el option file es lo que nadie más hace en multijugador… y es frágil: cada parche, cada versión (FL27 cambia la llave de cifrado) y cada actualización te obliga a ajustar. MFL eligió no tocar nada a propósito.
4. **Su ingreso de resultados ya es mejor que el tuyo.** Capturas + IA funcionando hoy, y para cualquier juego. Lo tuyo (Link leyendo el marcador) es mejor en teoría, pero solo funciona en PC, con Link, y todavía no existe.
5. **No tienes la capa de "historia".** Prensa, directiva que te despide, jugadores con opinión: es lo que engancha a la gente en MFL y Touchline. Phoenix es más "sistema" que "experiencia".
6. **Dependes de una estructura que no controlas.** Konami, Parsec, Smash Soda (código de otro), los parches de terceros. MFL depende de casi nada.

---

## 4. Lo que tienes y ellos no (lo bueno, sin exagerar)

1. **El circuito completo de una liga competitiva:** sala de Parsec con mandos y roles → partido → resultado → web → mercado → el fichaje aparece en el juego de todos con la cara correcta. Nadie más tiene esa cadena entera.
2. **Juego justo para competir:** control de mandos, host neutral, retraso igualado, registros firmados. MFL confía en que nadie mienta en su captura; para una liga con premios, eso no alcanza.
3. **Una comunidad real y tuya:** 50+ jugadores, Galaxy League, Liga Sudario, Discord. MFL tiene usuarios sueltos; tú tienes una liga organizada.
4. **Región y parches:** español, Sudamérica, ConmeGOL y Sudamerican Project. MFL y Touchline están pensados en inglés y en Europa.
5. **El Phoenix ID entre parches:** 10.676 jugadores emparejados automáticamente entre Conmegol y Sudamerican. Eso es trabajo difícil y valioso.

---

## 5. Qué te conviene copiar (ideas concretas)

| Idea | De quién | Cómo en Phoenix |
|---|---|---|
| Capturas + IA para el resultado | MFL | Plan B cuando no hay Link (consola, PC sin Link). Link sigue siendo el método "oficial" para torneos |
| Código de invitación para unirse a una liga | MFL | Ligas privadas de terceros en tu web |
| Oferta → bandeja → aceptar/rechazar | MFL | Ya tienes ofertas; revisar que la experiencia sea tan simple |
| Primera temporada gratis, paga quien crea | MFL | Modelo para ligas privadas de terceros |
| "Cada parte se prende o apaga al crear la partida" | MFL | Ligas simples vs. profundas |
| Prensa y noticias con IA | MFL / Touchline | Noticias automáticas de la Galaxy League (ya tienes Noticias en la web) |
| Directiva / objetivos / despidos | Touchline | Objetivos de temporada por club |
| Escribir en vivo con Lua (no solo en el option file) | Touchline | Menos dependencia del formato binario; cambios sin reiniciar |
| Sacar versiones chicas y seguido | Touchline | Publicar Phoenix por partes |

## 6. Qué NO copiar
- "No tocar el juego" de MFL: es justo tu diferencia. Mantenlo, pero hazlo **opcional** (que la liga funcione aunque Mercado falle).
- Querer soportar 4 juegos y consola: no es tu público.

---

## 7. Mi recomendación franca

**No compitas con MFL en "carrera para cualquiera". Sé "la plataforma de ligas competitivas de PES/FL en Latinoamérica".** Ahí ellos no pueden seguirte (no controlan la sala ni el juego) y tú ya tienes ventaja.

Orden sugerido:
1. **Publicar lo mínimo que ya sirve:** web + Liga Máster + torneos para tu liga. Que tus 50 jugadores la usen de verdad.
2. **Resultado sin Link:** captura + IA (o escrito a mano y confirmado por ambos). Así la liga funciona aunque Link y Mercado tarden.
3. **Link compilado y probado** con 2–3 amigos (ya está en tu lista de pendientes).
4. **Mercado como extra opcional:** "si lo tienes, los fichajes aparecen en tu juego". Si se rompe con un parche, la liga sigue.
5. **Pausar o achicar** frentes que no ayudan a lo anterior (parche propio, PWA, FL27 completo) hasta tener usuarios reales.
6. **Considerar aliarte** con UML o Football Life Mod Studio (contenido) en lugar de rehacerlo todo.

**En una frase:** tu idea no es peor, es más grande. El riesgo no es que te copien, es que no termines. Achica, publica y crece desde tu liga.
