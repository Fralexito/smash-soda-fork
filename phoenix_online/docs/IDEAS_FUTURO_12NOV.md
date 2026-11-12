# Ideas para el futuro — búsqueda amplia (12 nov 2026)

Para Frank. Todo en español sencillo. Esto es un **cajón de ideas**: nada está hecho, nada se ha descargado ni ejecutado.

## Cómo se hizo y qué límites tiene (leer primero)

- Se hicieron unas 40 búsquedas web (inglés, y también ruso, chino, español, turco/indonesio). La búsqueda en ruso, chino, turco e indonesio devolvió casi solo páginas en inglés de pesmodding. **No salieron comunidades de portugués, árabe ni polaco** (no hubo tiempo de insistir). Eso queda pendiente.
- **No repite** las 12 ideas de ayer (`BUSQUEDA_PROFUNDA_12NOV.md` §3): mercado por Live Update, telemetría Sider, plataforma de liga, cuadros, Glicko-2, estadísticas de partido, clientes sin pantalla, registro de peticiones, bot de Discord, módulos de competición, relay propio, datos de ligas nuevas.
- Lo que no pude confirmar lleva la marca **«sin verificar»**. Cuando una licencia no salió clara, lo digo.
- evoweb.uk, pesmodding.com y web.archive.org están bloqueados desde la nube. Aparecen en el buscador, pero solo vi el resumen del buscador. Van en la lista **«leer desde la PC»** (sección 3).
- Dificultad: **baja** = una tarde; **media** = unos días; **alta** = semanas o mucho aprendizaje.
- Aviso general: lo legal (escudos, datos del juego, voces) es responsabilidad de Frank.

---

## 1. Las 15 mejores ideas (tabla)

| # | Idea | Para qué parte | Dificultad | Impacto | Fuente |
|---|------|----------------|-----------|---------|--------|
| 1 | Crónica automática de cada jornada con plantillas (sin IA de pago) — W5 | Web | baja | Alto | https://github.com/abadugu13/football_llm_commentary , Prosebot (LREC-COLING 2024) |
| 2 | Calendarios y nombres de clubes desde datos de dominio público — W1 | Web | baja | Medio | https://github.com/openfootball |
| 3 | Temporadas de 4 meses con «reinicio suave» y niveles 1–10 con insignias — O1 | Online + Web | media | Alto | https://support.faceit.com/hc/en-us/articles/10605788025884 |
| 4 | Check-in antes del partido + los dos jugadores reportan y el servidor compara — O2/O5 | Online | media | Alto | https://www.challengermode.com/llms.txt |
| 5 | Marcador en vivo y «espectadores conectados» con Supabase Realtime — W11 | Web | media | Alto | https://supabase.com/docs/guides/realtime |
| 6 | Clips automáticos: cuando el módulo Sider ve un gol, OBS guarda la repetición — V5 | Overlay + Parche | media | Alto | https://obsproject.com/forum/resources/obsautoreplay.1894/ |
| 7 | OpenSkill (Weng-Lin) para el co-op 2v2 en vez de TrueSkill — O3 | Online | media | Medio | https://pypi.org/project/openskill/ (MIT) |
| 8 | «Atmósfera Phoenix»: público que reacciona al marcador y estadio lleno según la importancia del partido — P1/P2 | Parche | media | Alto | https://www.pesmodding.com/2026/09/pes-2021-home-crowd-bronca-whistles.html |
| 9 | Perfiles de árbitro (duro / permisivo) sorteados por la web — P4 | Parche + Web | baja | Medio | https://www.pesmodding.com/2026/05/pes-2021-referee-strictness-by-aoba.html |
| 10 | Pase de temporada GRATIS con logros (sin dinero) — O6/O7 | Online + Web | media | Alto | https://korben.info/en/mighty-doom-bethesda-shut-it-down-players-brought-it-back.html |
| 11 | Porra de predicciones con puntos virtuales — W7 | Web | media | Medio | https://esportsinsider.com/2025/08/valorant-shotcall-twitch-extension-esports-predictions |
| 12 | Bandeja de noticias y rumores de fichajes generada con plantillas — W6 | Web + App | media | Alto | https://www.fmscout.com/a-fm26-modding-support.html (idea, ver W6) |
| 13 | Creador de escudos y avatares propios (sin marcas de nadie) — W13/W14 | Web | media | Medio | https://allsvgicons.com/avatar-generators/open-peeps/ (CC0) |
| 14 | Subtítulos en directo en el stream con Whisper local — V4 | Overlay | baja | Medio | https://github.com/occ-ai/obs-localvocal |
| 15 | Veto de estadio en la sala (cada jugador tacha estadios, como el «veto de mapa») — O9 | Online | media | Medio | https://prosettings.net/blog/what-is-faceit-and-how-do-i-play-it/ |

---

## 2. Todas las ideas por parte del ecosistema

Formato de cada una: **qué es** · **fuente y licencia** · **propuesta concreta** · **dificultad**.

### 2-A. PARCHE (Sider + Phoenix Core)

**P1. Público que reacciona al marcador**
- Qué es: un módulo Sider (`sifflet.lua`, de Parigo-57) que mete silbidos y «bronca» cuando el equipo local va perdiendo. Todavía en desarrollo según el autor.
- Fuente: https://www.pesmodding.com/2026/09/pes-2021-home-crowd-bronca-whistles.html · Licencia: sin verificar (mod de comunidad; leer el texto de la descarga).
- Propuesta: un módulo propio «Atmósfera Phoenix» (escrito por Frank, sin copiar) que lea el marcador y el minuto (`match.stats()`, ya apuntado ayer) y suba o baje el ruido del público. Con un archivo de ajustes `.ini` para apagarlo.
- Dificultad: media.

**P2. Asistencia del estadio ajustable**
- Qué es: `crowd_attendance.lua` (FletchAJX) deja poner de 0 % a 100 % el público de local y de visitante por separado.
- Fuente: https://www.pesmodding.com/2026/10/pes-2021-crowd-attendance-module.html · Licencia: sin verificar.
- Propuesta: que la web mande «partido grande = estadio lleno; jornada normal = 70 %; final = 100 %» y que el módulo lo aplique. Se ve en el stream y da sensación de importancia.
- Dificultad: baja (si se usa el módulo tal cual) / media (si se conecta con la web).

**P3. Clima por equipo y clima decidido por la web**
- Qué es: `WeatherConditions.lua` hace que llueva más en equipos ingleses que en españoles; hay un `StadiumServer.lua` que puede mandar sobre el clima elegido en el menú.
- Fuente: https://www.pesmodding.com/2022/10/pes-2021-weather-conditions-for-teams-v1.html y https://www.pesmodding.com/2023/08/weather-conditions-for-teams-v30.html · Licencia: sin verificar.
- Propuesta: el clima de cada partido de liga se sortea en la web (un número guardado en Supabase) y los dos jugadores online lo reciben igual. Así no hay peleas por «tú pusiste lluvia». Necesita Sider 7.1.0 o más para el servidor de estadios.
- Dificultad: media.

**P4. Perfiles de árbitro**
- Qué es: «Referee Strictness» de Aoba: ajusta lo estricto que es el árbitro (incluye volver a dar roja directa por quitar ocasión clara). Pide Sider 7.x con LuaJIT.
- Fuente: https://www.pesmodding.com/2026/05/pes-2021-referee-strictness-by-aoba.html (la descarga tiene contraseña; yo no la copié) · Licencia: sin verificar.
- Propuesta: tres perfiles («Estricto», «Normal», «Permisivo») sorteados por jornada en la web y mostrados en la ficha del partido. Los «personajes» de árbitro dan historias a la liga. No encontré ningún módulo de «personalidad» de árbitro ya hecho: sería propio.
- Dificultad: baja a media.

**P5. Lesiones más reales (para Carrera y Liga offline)**
- Qué es: «Increased Training Injuries» (Kingpasto, v2.0, funciona con SP Football Life 2026) sube la frecuencia y la duración de lesiones de entrenamiento; «Realistic Injury Durations» ajusta los plazos a los reales.
- Fuente: https://www.pesmodding.com/2026/01/pes-2021-increased-training-injuries.html y https://www.pesmodding.com/2025/12/realistic-injury-durations-mod-for.html · Licencia: sin verificar. Aviso: la página dice que las lesiones parecen aleatorias, sin mirar la resistencia del jugador.
- Propuesta: para Phoenix Carrera, copiar el *concepto* (tabla de duración por tipo de lesión) y usar la tabla en la app móvil. Para el parche, leer cómo lo hace el módulo antes de decidir.
- Dificultad: media.

**P6. Cansancio y calendario apretado**
- Qué es: el mod «X Pro Evolution 3» (Holland) toca aguante y otros ajustes de juego. Páginas de «Gogosh Patch V90» hablan de calendario apretado y descanso, pero salen en sitios espejo dudosos: **sin verificar**.
- Fuente: https://www.pesmodding.com/2022/12/pes-2021-gameplay-x-pro-evolution-3.html · Licencia: sin verificar.
- Propuesta: regla de liga «tres partidos en cinco días = jugadores con −X de forma» calculada por la web (porque la web sabe el calendario) y enviada como Live Update. Es una idea que se apoya en tu idea de ayer 3-1.
- Dificultad: alta.

**P7. Cámaras de transmisión dinámicas**
- Qué es: un mod nuevo «Dynamic broadcast cameras for PES 2021» (nov 2026).
- Fuente: https://www.pesmodding.com/2026/11/dynamic-broadcast-cameras-for-pes-2021.html · Licencia: sin verificar. **Leer desde la PC** (sitio bloqueado en la nube; solo vi el título).
- Propuesta: si funciona bien, ponerlo por defecto en «partidos de stream» de la liga.
- Dificultad: baja (instalar) · media (integrar).

**P8. Menú Phoenix dentro del juego (tecla F1)**
- Qué es: «Sider Mod Menu» abre un menú de Sider con F1.
- Fuente: https://www.pesmodding.com/2026/10/sider-mod-menu-v12-modern-mouse.html · Licencia: sin verificar.
- Propuesta: un panel propio (idea) «Phoenix · Jornada X · Rival · Hora del partido» dentro del juego. Hay que comprobar antes si el overlay de Sider puede mostrar texto en medio del partido (la doc de Sider tiene `overlay_on`).
- Dificultad: media.

**P9. Himno, celebraciones y chaqueta de himno**
- Qué es: «Custom Celebrations» (necesita Sider 7.3.3 o 7.3.4, justo tu versión) y «Anthem Jacket» añaden celebraciones y la chaqueta de los himnos.
- Fuente: https://www.pesmodding.com/2026/10/pes-2021fl-2026-custom-celebrations-add.html y https://www.pesmodding.com/2026/10/anthem-jacket-mod-for-pes-2021-sider.html · Licencia: sin verificar.
- Propuesta: «Celebración del campeón» de la Galaxy League: el equipo campeón de la jornada desbloquea una celebración propia (ligado a logros, O7).
- Dificultad: media.

**P10. Escenas de VAR (solo decoración)**
- Qué es: mods comunitarios que añaden escenas de VAR antes del partido o en penales. No es un VAR real.
- Fuente: https://www.pesmodding.com/2021/10/pes-2021-var-entranse-scene.html y https://www.pesmodding.com/2026/08/pes-2021-scenes-var-01-fix.html · Licencia: sin verificar.
- Propuesta: probarlo solo para el look de transmisión. **No** vender como VAR real: no revisa nada. Importante en una liga con disputas.
- Dificultad: baja.

**P11. Alineación automática para partidos que juega la CPU**
- Qué es: «FL26 Chatty Auto Lineup v1.02» (oct 2026): alinea sola.
- Fuente: https://www.pesmodding.com/2026/10/fl26-pes-2021-chatty-auto-lineup-v102.html · Licencia: sin verificar. Leer desde la PC.
- Propuesta: para el modo «Liga offline» (si algún día lo haces) o para equipos sin dueño.
- Dificultad: baja.

**P12. Anti-trampa de dificultad**
- Qué es: un módulo Lua (Holland) que anula los valores internos de «dificultad dinámica» al sacar de centro.
- Fuente: https://www.pesmodding.com/2026/01/efootball-pes-2021-anti-cheat-sider-lua.html · Licencia: sin verificar. Leer desde la PC.
- Propuesta: no vale mucho online (ahí no hay IA), pero es un ejemplo de cómo tocar valores al sacar. Útil para Carrera offline «justa».
- Dificultad: baja a media.

**P13. Audio por equipo y ambiente de estadio**
- Qué es: «Team Audio Server (reviving)» y mods de ambiente (p. ej. Nápoles, gradas del Fenerbahçe, cánticos de FIFA convertidos).
- Fuente: https://www.pesmodding.com/2026/03/pes-2021-team-audio-server-reviving.html y https://www.pesmodding.com/2022/06/pes-2021-atmosphere-mod-2022-fifa.html · Licencia: sin verificar. **Ojo**: cánticos tomados de FIFA son material de otra empresa → ver sección 4.
- Propuesta: grabar o pedir a la comunidad de la liga cánticos **propios** («cánticos Phoenix») y cargarlos por equipo. Eso sí es tuyo y legal.
- Dificultad: media.

**P14. Voz por TTS para nombres de jugadores nuevos**
- Qué es: Piper es un motor de voz que corre en tu PC sin internet. El motor se dice MIT; **las voces tienen cada una su propia licencia** (la voz española «Sharvard» sale como «solo uso no comercial» en una página de terceros).
- Fuente: https://tts.ai/voices/piper/?lang=es (terceros, sin verificar); el repositorio oficial no pude abrirlo: sin verificar. Mirar la tarjeta de cada voz en Hugging Face.
- Propuesta: generar en lote los audios de nombres para jugadores inventados de la liga (los narradores del juego no los conocen). Hay que saber cómo mete PES 2021 los nombres hablados (formato de audio): no investigado.
- Dificultad: alta.

### 2-B. ONLINE (servidor Phoenix Online)

**O1. Temporadas con «reinicio suave»**
- Qué es: FACEIT hace temporadas de 4 meses; al acabar, todos «se acercan al centro» sin empezar de cero.
- Fuente: https://support.faceit.com/hc/en-us/articles/10605788025884-Season-7-in-FACEIT-CS2-Matchmaking-FAQ · Licencia: no aplica (es una idea de diseño).
- Propuesta: tu ELO/Glicko guarda una columna `temporada`. Al cerrar, la nota nueva = 70 % la anterior + 30 % la media. Guardas una foto de la tabla final (historial). Premios para top 3 (ver O6).
- Dificultad: media.

**O2. Check-in antes del partido de liga**
- Qué es: Challengermode tiene un paso formal de «confirmar que vienes» antes de crear el cuadro y de jugar.
- Fuente: https://www.challengermode.com/llms.txt (resumen oficial; leído por buscador) · Licencia: idea.
- Propuesta: 15 minutos antes de la hora acordada, el servidor pide «¿vienes?». Quien no confirma pierde por incomparecencia o se reprograma. Menos «mi rival no apareció» y menos peleas.
- Dificultad: media.

**O3. OpenSkill para parejas (co-op 2v2)**
- Qué es: una librería abierta que hace lo que TrueSkill (equipos), sin su problema de marca/patente de Microsoft.
- Fuente: https://pypi.org/project/openskill/ y https://github.com/vivekjoshy/openskill.py (el artículo científico lo cita en OpenDebates/openskill.py: puede haberse movido; comprobar) · Licencia: MIT (según listados de paquetes).
- Propuesta: para Team Play 2v2 usar OpenSkill; para 1v1 mantener Glicko-2 (de ayer). El README de otro paquete dice que Microsoft solo deja usar «TrueSkill» en juegos de Xbox o proyectos no comerciales (sin verificar con Microsoft).
- Dificultad: media.

**O4. Ladder sin escapatoria**
- Qué es: en las ladders de Challengermode no puedes irte hasta que termine. Evita que alguien se borre cuando va perdiendo.
- Fuente: https://support.challengermode.com/pt/participar-de-um-torneio/ladders · Licencia: idea.
- Propuesta: regla propia: «si te inscribes en una división, tu nota se queda hasta el cierre de temporada». Se apoya en tus divisiones online (migración 165).
- Dificultad: baja.

**O5. Doble reporte con comparación automática**
- Qué es: Challengermode usa reporte automático de resultados + equipo de moderación para disputas.
- Fuente: https://www.capterra.com/p/242125/Challengermode/ (resumen de terceros, **sin verificar**) · Licencia: idea.
- Propuesta: los dos jugadores reportan marcador; el servidor lo compara con lo que dice `SET_PLAYDATA_MATCH` (ya recibes ese paquete). Si los tres coinciden: se cierra solo. Si no: pasa a la lista de «disputas» de tu admin. Se junta con tu idea 3-3 (clips).
- Dificultad: media.

**O6. Pase de temporada gratis (sin dinero)**
- Qué es: proyectos de fans que revivieron juegos reconstruyeron también el pase de batalla: Mighty DOOM (fans reconstruyeron pase, tienda e inventario) y Classic Warzone (pase de temporada con skins, vía Discord). Return of Reckoning añadió una primera temporada clasificatoria de 6 semanas.
- Fuente: https://korben.info/en/mighty-doom-bethesda-shut-it-down-players-brought-it-back.html , https://aggregoctt.github.io/classic-warzone-revived-after-years-of-work-from-c-EBimCkbXU_2Nt969iQp-mw , https://www.pcgamer.com/a-year-in-the-life-of-the-warhammer-online-fan-server · Licencia: no aplica.
- Propuesta: «Pase Phoenix»: 30 niveles que se llenan jugando (PX por partido, por logro). Premios solo **cosméticos y gratis**: insignias, marcos de perfil, títulos, celebraciones (P9). Nada que dé ventaja ni que se compre. Sin pasarela de pago (tu regla).
- Dificultad: media.

**O7. Logros (achievements)**
- Qué es: medallas por hitos. En las fuentes de arriba no salió una lista concreta de logros, así que esta es **idea propia**.
- Propuesta: tabla `logros` en Supabase + reglas que se evalúan al llegar `SET_PLAYDATA_MATCH`: «Primer gol», «Remontada de 2 goles», «Portería a cero ×3», «Gana a alguien de división superior», «10 partidos sin abandonar». Cada logro suma PX al pase (O6).
- Dificultad: media.

**O8. Anti-abandono**
- Qué es: ya apuntaste anti-trampas; aquí un matiz: contar abandonos en rojo y poner cola de castigo corta.
- Fuente: idea propia, inspirada en lo de FACEIT (aceptar partida con ventana de tiempo, https://prosettings.net/blog/what-is-faceit-and-how-do-i-play-it/ ).
- Propuesta: «ventana de aceptar» de 30 s al encontrar rival; si uno no acepta, vuelve a la cola y recibe un aviso. 3 avisos = 10 minutos fuera.
- Dificultad: media.

**O9. Veto de estadio**
- Qué es: en FACEIT, tras aceptar partida, hay veto de mapa. Aquí sería quitar estadios.
- Fuente: https://prosettings.net/blog/what-is-faceit-and-how-do-i-play-it/ · Licencia: idea.
- Propuesta: en la sala de amigos / partido de liga, el servidor ofrece 5 estadios y cada jugador tacha 2; se juega el que sobra. Requiere que el parche acepte estadio forzado (Sider tiene `StadiumServer`, ver P3).
- Dificultad: media.

**O10. Lección de proyectos de fans: firmar el lanzador**
- Qué es: el lanzador de TurboRivals (fans que revivieron Need for Speed Rivals) no está firmado y las fuentes dudan de la seguridad de su código de autenticación.
- Fuente: https://racinggames.gg/article/fans-revive-need-for-speed-rivals-a-year-after-ea-pulled-the-plug · Licencia: no aplica.
- Propuesta: en tu Launcher Phoenix: publicar **sumas de comprobación (SHA-256)** de cada versión en la web y explicar «cómo comprobarlo». Los jugadores desconfían de .exe de desconocidos; esto lo arregla gratis.
- Dificultad: baja.

**O11. Reglas claras de «necesitas tu propia copia»**
- Qué es: «The Crew Unlimited» (servidor de fans) pide ya tener el juego comprado.
- Fuente: https://dtptips.com/ (ver búsqueda "The Crew Revival"; resumen de terceros, sin verificar).
- Propuesta: en la web de Phoenix poner una frase fija: «Necesitas tu propia copia de PES 2021. No damos el juego». Te protege.
- Dificultad: baja.

### 2-C. WEB (liga, Astro + Supabase)

**W1. Datos de dominio público con openfootball**
- Qué es: repositorios con calendarios, clubes y estadios del mundo, en texto simple y JSON. Sin clave ni registro.
- Fuente: https://github.com/openfootball · https://github.com/openfootball/clubs · https://github.com/openfootball/football.json · Licencia: dominio público / CC0 (World Cup figura CC0 1.0). No verifiqué si todos siguen activos.
- Propuesta: bajar los calendarios reales de 2–3 ligas **una sola vez** y usarlos como plantilla para el calendario de la Galaxy League («jornada 5: ordenado como la Liga»). Los nombres de clubes reales están en `clubs`. Bonito para Carrera también.
- Dificultad: baja.

**W2. football-data.org (nivel gratis) para la «liga real» al lado**
- Qué es: API de fútbol; el nivel gratis da 12 competiciones y 10 llamadas por minuto, con marcadores retrasados.
- Fuente: https://api.football-data.org/register (términos de 2018: pueden haber cambiado) · https://www.thestatsapi.com/blog/football-data-org-free-tier-limits-2026 (terceros) · Licencia: términos propios (no es código); sin verificar el uso comercial.
- Propuesta: una pestaña «Liga real vs Galaxy»: tabla real del fin de semana y tus resultados. Pedir 1 vez cada hora y guardar en Supabase (para no pasar el límite).
- Dificultad: media.

**W3. Datos de StatsBomb (abiertos, solo no comercial)**
- Qué es: eventos de partidos reales (tiros, pases) y «360» en partidos elegidos.
- Fuente: https://github.com/statsbomb/open-data · Licencia: acuerdo de usuario en `LICENSE.pdf`, **uso no comercial**; leerlo antes. (Las métricas 360 completas son de pago, según un resumen.)
- Propuesta: usarlo **solo para pruebas** de tus gráficos (mapa de tiros, red de pases), con datos reales mientras no tienes los de Phoenix. Si algún día la liga cobra algo o muestra anuncios, no usar (monetización).
- Dificultad: media.

**W4. Gráficos de cancha (mapa de tiros, pases)**
- Qué es: en Python existe `mplsoccer` para dibujar campos de fútbol con datos de StatsBomb; en `socceraction` hay lectores de datos.
- Fuente: https://mplsoccer.readthedocs.io/en/latest/mplsoccer.soccer.statsbomb.html y https://socceraction.readthedocs.io/en/latest/documentation/data/statsbomb.html · Licencia: sin verificar.
- Propuesta: tú ya usas JS vanilla: dibuja el campo con SVG y pinta los tiros. Solo tendrás datos de tiros si algún paquete del juego los trae (**sin verificar**; `SET_PLAYDATA_MATCH` trae `rating` y goles, no se sabe de coordenadas).
- Dificultad: media.

**W5. Crónica automática de cada jornada**
- Qué es: texto generado a partir de datos. El repo `football_llm_commentary` tiene una idea simpática: se escribe una plantilla con «el texto + el disparador» (un trocito de código que mira el estado del partido). Existe también Prosebot (plataforma comunitaria para resúmenes de partidos) y un estudio de Cardiff sobre modelos abiertos para comentar en tiempo real.
- Fuente: https://github.com/abadugu13/football_llm_commentary · https://preview.aclanthology.org/setup/2024.lrec-main.15 · https://orca.cardiff.ac.uk/171222/1/1-s2.0-S0950705124008530-main.pdf · Licencia: sin verificar (mirar LICENSE del repo).
- Propuesta: **sin IA**: 40 frases con huecos («{ganador} se impuso por {diferencia} a {perdedor}, con doblete de {goleador}»). Tu servidor las rellena al cerrar la jornada y la web las muestra como «Crónica». Opcional más adelante: IA local.
- Dificultad: baja.

**W6. Noticias y rumores de fichajes dinámicos**
- Qué es: Football Manager tiene bandeja de noticias; un mod nuevo reescribe con IA los diálogos y prensa (usa claves de API).
- Fuente: https://www.fmscout.com/a-fm26-modding-support.html · Licencia: sin verificar. No encontré un generador de noticias de FM de código abierto.
- Propuesta: tabla `noticias` generada por plantillas: «Racha de 5 victorias», «Líder nuevo», «Rumor: {club} pregunta por {jugador}» cuando hay jugadores a punto de acabar contrato en tu mercado (idea 3-1). Se muestra en portada con la caja cerrable.
- Dificultad: media.

**W7. Porra con puntos virtuales**
- Qué es: Valorant «Shotcall» y la extensión de Twitch para la FIFA eWorld Cup dejan al público predecir y ganar puntos y subir en un ranking.
- Fuente: https://esportsinsider.com/2025/08/valorant-shotcall-twitch-extension-esports-predictions · https://exmachinagroup.com/case-study/ea-fifa-twitch-extension · Licencia: no aplica.
- Propuesta: «La Porra Phoenix»: antes de la jornada, cada usuario predice los resultados; gana puntos sin valor de dinero y sale en un ranking. **Nunca** premios con dinero (ni apuestas): evitar problemas legales (sección 4).
- Dificultad: media.

**W8. Página de partido para compartir**
- Qué es: ballchasing.com (Rocket League) deja subir una repetición y obtener una página con estadísticas y visor 3D, para compartir en cualquier chat.
- Fuente: https://ballchasing.com/doc/faq · Licencia: servicio de terceros.
- Propuesta: cada partido de liga tiene su propio enlace corto (`/partido/ABC123`) con marcador, goleadores, notas y el clip. Con tarjeta bonita para Discord/WhatsApp (imagen de previsualización).
- Dificultad: media.

**W9. Ficha de jugador con radar**
- Qué es: ProClubsHead (para EA FC) compara dos clubes con su «forma de liga» en una sola pantalla. No es de PES ni oficial de EA.
- Fuente: https://proclubshead.com/25/compare-clubs-league-form/gen4-1088049-936321 (ejemplo) · Licencia: sitio de terceros.
- Propuesta: «Comparar clubes» en la web: últimos 5 partidos, goles a favor/contra, historial entre ellos. Y radar de 5 ejes para cada jugador (goles, nota media, pases, regates, recuperaciones — según lo que traiga el paquete).
- Dificultad: media.

**W10. Niveles 1 a 10 con insignias**
- Qué es: FACEIT muestra 10 niveles según Elo.
- Fuente: https://csdb.gg/guides/faceit-cs2-guide/ (terceros) · Licencia: idea.
- Propuesta: tu ELO se traduce a «Nivel Phoenix 1–10» con insignias por color. Se ve de un vistazo y motiva a subir. (Sin verificar los puntos de corte de FACEIT; elige los tuyos.)
- Dificultad: baja.

**W11. Marcador en vivo + espectadores conectados (Supabase Realtime)**
- Qué es: Supabase tiene «Broadcast» (mensajes rápidos), «Presence» (quién está conectado) y «Postgres Changes» (cambios en tablas). Es ideal para un marcador en directo.
- Fuente: https://supabase.com/docs/guides/realtime · https://supabase.com/blog/supabase-realtime-multiplayer-general-availability · Licencia: Supabase Realtime es de código abierto (sin verificar cuál); tú ya usas Supabase.
- Propuesta: la web escucha un canal `partido:123`; tu módulo Sider (telemetría de ayer) manda goles y minuto al servidor, y éste los emite. «Aquí ahora: 12 personas viendo». Cuidado con el plan FREE (límite de conexiones: **sin verificar el número**).
- Dificultad: media.

**W12. Hall de la fama e historial de temporadas**
- Qué es: guardar cada temporada cerrada (O1) con campeón, goleador y racha.
- Fuente: idea propia, inspirada en FACEIT/Pro League (https://www.devdiscourse.com/article/sports-games/3968349-how-faceit-built-its-place-in-competitive-counter-strike ).
- Propuesta: página «Salón de campeones». Fotos o avatares de los ganadores.
- Dificultad: baja.

**W13. Creador de escudos propio**
- Qué es: una herramienta en la web donde el jugador arma su escudo con formas, colores y letras. Evita usar escudos reales (los clubes reales tienen derechos).
- Fuente: idea propia. Para piezas listas con licencia libre: https://opengameart.org/content/cc0-character-portraits (CC0) · Licencia: ver cada pieza.
- Propuesta: lienzo SVG con 30 formas y 40 símbolos hechos por ti. Se guarda como imagen en Supabase (cuidado con el espacio del plan FREE) y se exporta a tamaño del juego, listo para cargar como emblema.
- Dificultad: media.

**W14. Avatares CC0 para perfiles**
- Qué es: Open Peeps es una librería de dibujos para armar personajes; su licencia es CC0 (se puede usar sin pedir permiso). Hay además generadores de retratos con licencia CC0 pero sin foto real.
- Fuente: https://allsvgicons.com/avatar-generators/open-peeps/ · https://www.geeknative.com/101966/portrait-generator-this-ai-has-been-trained-to-draw-fantasy-portraits-free-for-your-use (retratos de fantasía CC0; **su disponibilidad es dudosa**, la página avisa de problemas de seguridad) · Licencia: CC0.
- Propuesta: avatar por defecto para quien no sube foto. No encontré un generador de **caras realistas** con licencia clara (CC0): para caras de PES hay que seguir con las tuyas.
- Dificultad: baja.

**W15. Contratos y liberar jugadores (reglas del «mercado»)**
- Qué es: la guía de Soccerverse explica cómo renovar contratos y soltar jugadores. **Cuidado:** Soccerverse es un juego basado en blockchain/criptomoneda (sin verificar a fondo): copiar solo las reglas, no el dinero.
- Fuente: https://guide.soccerverse.com/managers/renewing-player-contracts y https://guide.soccerverse.com/managers/releasing-players · Licencia: texto de terceros; solo como inspiración.
- Propuesta: en tu mercado de fichajes (3-1): contrato de N temporadas, cláusula de salida, plazo de ventana. Sin dinero real.
- Dificultad: media.

**W16. Ventanas de fichajes y reglas de cesiones**
- Qué es: la Kings League tiene su «mercato» con fechas y un reglamento de traspasos y cesiones existe en ligas virtuales como mystify.
- Fuente: https://kingsleague.pro/mercato/ · https://www.mystify.co.uk/rules/transfers · Licencia: textos de terceros; sin verificar detalles.
- Propuesta: calendarios de ventana (abre/cierra), tope de fichajes por ventana y cesiones. Que la web bloquee lo que no se pueda.
- Dificultad: baja.

### 2-D. OVERLAY (Phoenix Overlay / stream)

**V1. Marcador (scorebug) HTML controlado por OBS WebSocket**
- Qué es: dos páginas HTML: una se pone como fuente de navegador y otra controla todo; hablan con OBS por WebSocket. Hay opción de OCR y cinta de noticias.
- Fuente: https://obsproject.com/forum/resources/html-scorebug-with-ocr-support-manual-control-dashboard-and-automated-ticker.2293/updates (el repo se llama `david-mcgaughy/scorebug`) · Licencia: sin verificar. **Aviso**: la contraseña de OBS va escrita dentro del HTML; no hagas eso.
- Propuesta: para Phoenix Overlay, copiar la *forma* (una página de panel + una de pantalla) pero con la contraseña en un archivo aparte.
- Dificultad: media.

**V2. Marcador por sockets (Node)**
- Qué es: `jamesql/scoreboard` — marcador para OBS controlado por sockets (fútbol, hockey, billar). Última actualización: marzo 2023 (puede estar parado).
- Fuente: https://awesome.ecosyste.ms/projects/github.com%2Fjamesql%2Fscoreboard · Licencia: MIT.
- Propuesta: leer su manera de enviar «gol» → la pantalla se anima. Con licencia MIT puedes aprender y hasta reutilizar piezas con la nota de licencia.
- Dificultad: baja.

**V3. Arquitectura de «paquetes de overlay»**
- Qué es: `rl-overlay-hub` (Rocket League): overlays hechos con HTML/CSS/JS que escuchan la API de estadísticas del juego; comunidad que sube su propio diseño.
- Fuente: https://awesome.ecosyste.ms/projects/github.com%2Fmanucabral%2Frl-overlay-hub · Licencia: MIT.
- Propuesta: Phoenix Overlay con carpeta `temas/` (Galaxy cian-púrpura y Sudario azul-dorado). Cada tema es solo HTML+CSS; así cualquier streamer de tu liga puede crear el suyo.
- Dificultad: media.

**V4. Subtítulos en directo con Whisper local**
- Qué es: el complemento LocalVocal de OBS pone subtítulos en vivo usando Whisper en tu PC (sin internet). `whisper.cpp` es MIT.
- Fuente: https://github.com/occ-ai/obs-localvocal · https://github.com/ggerganov/whisper.cpp (sin abrir; resumen de terceros) · Licencia: whisper.cpp MIT; LocalVocal aparece como MIT en un listado y como GPL-2.0 en otro → **mirar el archivo LICENSE**.
- Propuesta: para narradores/streamers de tu liga: subtítulos en español sin pagar. También útil para actas de partidos narrados.
- Dificultad: baja.

**V5. Clips automáticos de goles**
- Qué es: OBS tiene «buffer de repetición» (guarda los últimos N segundos), pero **no detecta nada solo**: hay que pulsar una tecla.
- Fuente: https://obsproject.com/forum/resources/obsautoreplay.1894/ (script en Python) · https://joshuapotter.github.io/replay-buffer-pro · https://openclip.app/for/esports-organizations/obs-studio.md (OpenClip trabaja sobre grabaciones; sin verificar) · Licencia: sin verificar en cada una.
- Propuesta: tu módulo Sider (telemetría) ve que cambió el marcador → manda un aviso a Phoenix Overlay → éste dispara por OBS WebSocket «guardar repetición» y nombra el clip «J5_Gol_{jugador}_min67». Sin IA, sin adivinar: se apoya en un dato exacto del juego.
- Dificultad: media.

**V6. Tablero de eventos local (idea tomada de FMMatchLens)**
- Qué es: un complemento para Football Manager 26 (necesita BepInEx 6) que recoge eventos del partido y los muestra en una página HTML estática: tiros, pases, acciones defensivas, tarjetas.
- Fuente: https://sortitoutsi.net/content/77535/fmmatchlens-live-match-analytics-replay-mod (repo osnsyc/FMMatchLens) · Licencia: sin verificar.
- Propuesta: la misma cadena en tu mundo: Sider escribe un archivo JSON con eventos → una página HTML local lo lee → se pone en OBS. Pendiente comprobar si Lua puede escribir archivos (ya lo dudaste ayer).
- Dificultad: alta.

**V7. Predicciones con puntos de canal en Twitch**
- Qué es: Twitch ya tiene «Predicciones» con puntos de canal (que no son dinero). Las extensiones de verdad (Shotcall, eWorld Cup) requieren trabajo de Twitch y servidor.
- Fuente: https://esportsinsider.com/2025/08/valorant-shotcall-twitch-extension-esports-predictions · Licencia: no aplica. No encontré una extensión de código abierto de predicciones: **sin verificar**.
- Propuesta: no hacer una extensión. Usar las Predicciones normales de Twitch y que Phoenix Overlay muestre en pantalla «Predicción abierta: Galaxy FC vs Orión». Más sencillo.
- Dificultad: baja (versión simple).

### 2-E. APP (Phoenix Carrera, móvil)

**A1. Openfoot Manager (leer, no copiar)**
- Qué es: gestor de fútbol de escritorio de código abierto hecho con Tauri + React.
- Fuente: https://github.com/openfootmanager/openfootmanager · Licencia: **GPLv3** (si copias código, tu app debe ser GPLv3 también).
- Propuesta: leer cómo organizan equipos, partidos y semanas para tu base de datos. No copiar código.
- Dificultad: baja (leer).

**A2. Wunderkind (React Native) — bucle de «ticks» semanales**
- Qué es: app móvil de gestión con motor de personalidad de 8 rasgos y un ciclo semanal sin conexión.
- Fuente: https://github.com/cadesile/wunderkind-app · Licencia: **sin verificar** (no la mostró el buscador; mirar el repo).
- Propuesta: tu Carrera avanza por «semanas»; cada semana (offline) se resuelve entrenamiento, lesiones, moral, ofertas. Personalidad de jugador con 3–4 rasgos (ambicioso, leal, rebelde).
- Dificultad: media.

**A3. The Manager (Fastify + React + SQLite)**
- Qué es: simulación web con pila parecida a la que podrías usar.
- Fuente: https://awesome.ecosyste.ms/projects/github.com%2Fvvasconceloss%2Fthe-manager · Licencia: sin verificar.
- Propuesta: ver cómo guardan el estado de la liga en SQLite (la app móvil puede usar SQLite local, sin servidor).
- Dificultad: baja (leer).

**A4. Gaffer (Unity), roguelike de gestión**
- Qué es: un repositorio llamado Gaffer, juego roguelike de gestión móvil en Unity 6, aún en diseño.
- Fuente: https://github.com/balkac/Gaffer · Licencia: sin verificar. (No es el mismo que «The Gaffer» comercial; **sin verificar** cuál es cuál.)
- Propuesta: roguelike = cada temporada elegir 1 de 3 «cartas» (mejora de club, riesgo, patrocinador). Da ganas de repetir. Se ve en la app como botones grandes.
- Dificultad: media.

**A5. Modo «solo lectura» para importar guardados**
- Qué es: `fmsave` lee guardados de Football Manager 26 y los pasa a CSV/JSON/Excel; **solo lee**, no escribe, no usa red, no lee memoria del juego.
- Fuente: https://pypi.org/project/fmsave/ (repo rhiever/fmsave) · Licencia: MIT.
- Propuesta: copiar esa promesa de seguridad en Phoenix Carrera: «importar mi plantilla desde un archivo exportado» (CSV), sin tocar partidas. Más fácil de confiar. Se conecta con los editores de guardado de ayer (3-1).
- Dificultad: media.

**A6. Conferencias de prensa y diálogos**
- Qué es: un mod de FM26 reescribe con IA las conferencias, charlas y reuniones; necesita una clave de API de pago (OpenRouter, Groq, Google, OpenAI).
- Fuente: https://www.fmscout.com/a-fm26-modding-support.html · Licencia: sin verificar.
- Propuesta: versión Phoenix: 30 preguntas de prensa con 3 respuestas (humilde / seguro / provocador) que cambian moral y reputación. Sin IA. Si algún día se añade IA, **nunca pedirás la clave a nadie por chat**: la guardas tú en el servidor.
- Dificultad: media.

**A7. Objetivos de directiva, cantera y redes sociales (ideas propias)**
- Qué es: elementos típicos de FM. En la búsqueda salieron como categorías, sin proyecto abierto que copiar (**idea propia**).
- Propuesta: (a) la directiva pone 2 objetivos por temporada (ej. «top 5», «vender por X»); (b) cantera: cada 12 semanas salen 3 jóvenes con nombre inventado y avatar CC0 (W14); (c) un «muro» ficticio con 5 plantillas de tuits de aficionados según resultados.
- Dificultad: media.

### 2-F. STREET (futsal/calle, en pausa)

**S1. Ranking de estilo (flair) además de victorias**
- Qué es: una wiki de fans describe un FIFA Street 26 imaginario con escaleras por temporada y «ranking de estilo». **Es una wiki de fans y las fechas no cuadran**: sin verificar.
- Fuente: https://gameideas.fandom.com/wiki/Fifa_Street_26 · Licencia: wiki de fans (idea).
- Propuesta: cuando Street se retome: puntos por regates y trucos, no solo goles. Depende de que el juego dé ese dato (**sin verificar**).
- Dificultad: alta.

**S2. Reglas rápidas tipo Kings League**
- Qué es: liga de fútbol 7 con cartas/comodines y un mercado propio. De las búsquedas solo vi la página del «mercato»; el resto lo conozco de forma general y **no lo verifiqué aquí**.
- Fuente: https://kingsleague.pro/mercato/ · Licencia: marca de otros.
- Propuesta: para Street: partido de 12 minutos, «carta comodín» al azar (doble gol, penal extra) decidida por la web. Solo si el parche puede aplicar reglas distintas.
- Dificultad: alta.

**S3. Juegos arcade como referencia (Sociable Soccer)**
- Qué es: Sociable Soccer Champions: fútbol arcade rápido, con una oferta de un jugador y multijugador. Las fechas de salida se contradicen en las páginas de tienda → **sin verificar**.
- Fuente: https://store.playstation.com/en-us/concept/10002444 (ejemplo) · Licencia: no aplica.
- Propuesta: solo mirar videos para sacar ideas de ritmo y partidas de 5 min. No hay módulos conocidos.
- Dificultad: baja (mirar).

**S4. UFL: cómo sirven a «clubes» creados por los jugadores**
- Qué es: UFL tiene juego cruzado opcional y las fuentes citan más de 7 millones de jugadores y 1,5 millones de clubes creados en beta (cifras de terceros, sin verificar).
- Fuente: https://footballgamingzone.com/more-games/is-ufl-crossplay-cross-platform-cross-progression-explained/ · Licencia: no aplica.
- Propuesta: «club propio» en Street: nombre, escudo creado con W13 y 5 jugadores amigos. Se usa de base para clanes (diferido por ti; solo guardar).
- Dificultad: media.

### 2-G. HERRAMIENTAS PARA HACER MODS

**H1. Plugin de Blender para modelos FMDL**
- Qué es: `pes-fmdl-blender` abre y guarda modelos FMDL (PES 2018 en adelante). Funciona bien con Blender 2.8.x/2.9.x; en versiones nuevas puede fallar.
- Fuente: https://github.com/the4chancup/pes-fmdl-blender · Licencia: sin verificar. Última versión vista: v0.7.2.
- Propuesta: si algún día haces tu propio balón, red o cartel de estadio, esto es la puerta de entrada. Instalar Blender 2.93 solo para esto.
- Dificultad: media.

**H2. Librerías de archivos de PES**
- Qué es: `pes-file-tools`: librerías y herramientas para manipular archivos de PES.
- Fuente: https://github.com/the4chancup · Licencia: sin verificar.
- Propuesta: mirar si trae un lector de `.cpk`/`.fpk` en un lenguaje que domines, para automatizar la construcción de tus actualizaciones en vez de hacerlo a mano.
- Dificultad: media.

**H3. Convertidores FTEX ⇄ DDS, CPK y GzsTool**
- Qué es: para abrir texturas (.ftex) hay un convertidor para PES 2021 basado en `ftextool` de Atvaark; para empaquetar CPK existe «PES CPK Tool v1.0.0» (StpN_17); para abrir `.fpk` se usa GzsTool.
- Fuente: https://www.pesmodding.com/2020/10/pes-2021-ftex-to-dds-converter-v10.html y https://www.pesmodding.com/2021/11/pes-cpk-tool-v100-by-stpn17.html · Licencia: sin verificar. **Leer desde la PC.**
- Propuesta: un script tuyo que haga «carpeta con DDS → .ftex → CPK» con un comando, así cada cambio de escudo/cartel sale en un minuto.
- Dificultad: media.

**H4. Cambiador de versión de edit**
- Qué es: «PES 2021 Edit Version Changer» (mayo 2021): cambia la versión del archivo de datos editado.
- Fuente: https://www.pesmodding.com/2021/05/pes-2021-edit-version-changer.html · Licencia: sin verificar. **Leer desde la PC.**
- Propuesta: relevante para tu Live Update (la versión que sube `version_wu.bin`): ver cómo calcula esa versión.
- Dificultad: baja.

**H5. Aumentar texturas con Real-ESRGAN**
- Qué es: un modelo abierto que agranda imágenes sin que se vean borrosas.
- Fuente: https://github.com/xinntao/Real-ESRGAN · Licencia: BSD-3-Clause (código y pesos; se debe conservar el aviso de copyright). Otra herramienta basada en él, Upscayl, es AGPL.
- Propuesta: solo para **tus propias** imágenes (escudos, carteles, fondos de la web). Para texturas de Konami: no se vende ni se publica (sección 4).
- Dificultad: baja.

**H6. Editores de opciones de PES 6 como modelo de editor**
- Qué es: `PESEditor` (PeterC10, para PES 6) y `COFPES-OF-Editor-6` (Python, MIT, descifra/cifra archivos de opciones de PES 6).
- Fuente: https://github.com/PeterC10/PESEditor · https://gitee.com/loa123/COFPES-OF-Editor-6 · Licencia: PESEditor sin verificar; COFPES MIT.
- Propuesta: no sirven para PES 2021, pero son un buen ejemplo de editor sencillo en Python con importar/exportar CSV. Si un día haces «Editor Phoenix» para tu base de datos, copiar la idea del CSV.
- Dificultad: baja (leer).

**H7. Comunidad PES 6**
- Qué es: lo buscado dio casi solo páginas de relleno (edx.ipeindia.org: spam). Que aún hay gente que lo juega se ve en SoccerGaming, pero no pude confirmar fechas.
- Fuente: https://soccergaming.com/forums/threads/pes-6-question.6459567/ · Licencia: no aplica.
- Propuesta: nada por ahora. Anotado por si buscas ideas viejas (editor de marcadores, selector de scoreboard).
- Dificultad: —

### 2-H. IA Y DATOS (sirven a varias partes)

**D1. Nota de jugador propia a partir de estadísticas**
- Qué es: el juego ya manda un `rating` en `SET_PLAYDATA_MATCH`. Se podría recalcular una nota **Phoenix** que pese cosas distintas por posición (idea propia; no encontré proyecto abierto que lo haga para PES).
- Propuesta: fórmula simple en `core/` con test: delantero = goles y tiros; defensa = recuperaciones y pases; portero = paradas. Guardar ambas notas para comparar.
- Dificultad: media.

**D2. Valor de las acciones (VAEP)**
- Qué es: `socceraction` convierte eventos de partido en un «valor de cada acción». Muy académico.
- Fuente: https://socceraction.readthedocs.io/ · Licencia: sin verificar.
- Propuesta: solo guardar la idea; hace falta tener eventos (pases, tiros) y hoy no los tienes.
- Dificultad: alta.

**D3. Narrador con IA que corre en tu PC**
- Qué es: el estudio de Cardiff dice que se puede afinar un modelo abierto para comentar en directo con una PC normal; `football_llm_commentary` es un ejemplo pequeño.
- Fuente: https://orca.cardiff.ac.uk/171222/1/1-s2.0-S0950705124008530-main.pdf · https://github.com/abadugu13/football_llm_commentary · Licencia: sin verificar.
- Propuesta: texto de narrador para el stream (no voz). Primero ideas de W5; la IA después. Con voz, ver P14.
- Dificultad: alta.

**D4. Predicción de resultados**
- Qué es: modelos simples (Elo + Poisson). No salió un proyecto concreto en estas búsquedas: idea propia.
- Propuesta: con tu ELO calcular «probabilidad de victoria» antes de cada partido de la Porra (W7), mostrando «favorito 62 %». Fácil y espectacular.
- Dificultad: baja.

**D5. Lo que NO usaría de la lista de IA**
- `FM Analytics` (aplicación Django): sin mantenimiento desde 2021 (según su listado).
- Servicios de «caras con IA»: no encontré uno con licencia clara; mejor no.

---

## 3. Sitios para leer desde la PC de Frank (la nube no los abre)

Nada de esto lo he leído en profundidad; solo salieron títulos y resúmenes en el buscador.

- **pesmodding.com** (todos los mods de abajo):
  - Cámaras de transmisión dinámicas: https://www.pesmodding.com/2026/11/dynamic-broadcast-cameras-for-pes-2021.html
  - Bronca/silbidos del público: https://www.pesmodding.com/2026/09/pes-2021-home-crowd-bronca-whistles.html
  - Módulo de asistencia: https://www.pesmodding.com/2026/10/pes-2021-crowd-attendance-module.html
  - Árbitros: https://www.pesmodding.com/2026/05/pes-2021-referee-strictness-by-aoba.html y «Realistic Tempo and Referee v2»: https://www.pesmodding.com/2026/04/pes-2021-realistic-tempo-and-referee-v2.html
  - Lesiones: https://www.pesmodding.com/2026/01/pes-2021-increased-training-injuries.html
  - Menú de Sider: https://www.pesmodding.com/2026/10/sider-mod-menu-v12-modern-mouse.html
  - Gestor de estadios: https://www.pesmodding.com/2025/10/pes-2021-stadium-manager.html
  - Cambiador de versión de edit: https://www.pesmodding.com/2021/05/pes-2021-edit-version-changer.html
  - Sider 7.4.0: https://www.pesmodding.com/2026/04/pes-2021-sider-v740.html (**mira si te interesa subir de 7.3.4**; sin verificar qué cambia)
  - Etiqueta de módulos Sider: https://www.pesmodding.com/search/label/PES%202021%20Sider%20Modules
  - Etiqueta de herramientas: https://www.pesmodding.com/search/label/PES%202021%20Tools
  - «CPK Protection Toolkit» (apareció en la búsqueda; sin verificar qué hace): https://cpkprotectiontoolkit.github.io/
- **evoweb.uk**: «The Retro-PES Corner» (comunidad PES viejo), hilos de gameplay realista de PES 2021: https://evoweb.uk/threads/what-is-the-most-realistic-gameplay-mod-for-pes-2021.91365/latest ; VAR para penales: https://evoweb.uk/threads/pes-2021-can-benefit-from-var-for-penalties-only.81830/latest
- **web.archive.org**: ya conocido de ayer.
- **soccergaming.com** (FIFA Live Editor, hilos PES 6): https://soccergaming.com/forums/threads/fifa-21-live-editor.6468943/ (la búsqueda sí lo devolvió; no lo abrí).
- **sortitoutsi.net** y **fmscout.com** (comunidad de Football Manager): para mods de FM como inspiración.
- **Documentos en PDF** del estudio de Cardiff y Prosebot: se pueden leer, son largos.

---

## 4. Cosas que evitar

- **Cheat tables y trampas**: «Live Editor» de FIFA (xAranaktu) existe y su wiki dice que puede saltarse el anticheat de EA y que se puede banear la cuenta. Es una herramienta para FIFA, no para tu proyecto. No copiar la parte de «saltarse el anticheat»: https://github.com/xAranaktu/FIFA-23-Live-Editor (archivado en sept 2023).
- **Material de otras empresas**: cánticos convertidos desde FIFA, escudos oficiales de clubes, fotos de caras de jugadores reales, comentarios grabados de EA: no los uses ni subas a tu sitio. Lo de «cánticos de FIFA convertidos a PES 2021» (P13) es de la comunidad, pero tú no debes redistribuirlo.
- **Vender datos o texturas del juego**: nunca. Tampoco publicar el binario con datos de Konami (AzrakHQ ya lo avisa).
- **Apuestas o premios en dinero** en predicciones (W7): son juego de azar en muchos países. Solo puntos virtuales sin valor.
- **Sistemas de «Pass» con compra**: tu regla es sin dinero real. No copiar el modelo de pago de Mighty DOOM (que sí tenía compras dentro de la app).
- **Cripto/blockchain**: Soccerverse (W15) usa ese mundo. No copiarlo.
- **TrueSkill de Microsoft**: marca/patente restrictiva según README de terceros; usa OpenSkill.
- **Código con licencia fuerte** (GPL/AGPL) si piensas cerrar el tuyo: Openfoot Manager (GPLv3), Upscayl (AGPL), WiiLink/OpenEleven (de ayer). Leer sí, copiar no.
- **Claves de API en el chat**: para cualquier IA de pago (A6), nunca las pongas en el chat; se guardan en el servidor.
- **Páginas basura que salieron en las búsquedas** (relleno): `edx.ipeindia.org`, `emilyleiapp.flothemes.com`, `autodiscover.killingthyme.net`, `grandavehousing.calpoly.edu` (páginas espejo/SEO de PES). No son fuente. Sobre todo las de «Gogosh Patch V90» en esos sitios (sin verificar y sospechosas).
- **Guías de «crack» o de saltar protecciones**: no entran.
- **Bots que juegan solos** o macros: ya excluidos ayer.
- **Contraseñas de descargas de terceros**: no las copié ni las uso aquí. Descarga solo de los autores originales y revisa el antivirus.

---

## 5. Conteo

- Ideas con número propio: **P1–P14 (14), O1–O11 (11), W1–W16 (16), V1–V7 (7), A1–A7 (7), S1–S4 (4), H1–H7 (7), D1–D5 (5) = 71** (de las cuales 3 son «leer solamente» y 2 son «no usar»; unas 62 son propuestas reales).

## 6. Siguientes pasos que sugiero (corto)

1. W5 (crónica con plantillas) y W1 (calendarios de openfootball): son lo más barato y lo que más se nota en la web.
2. O2 + O5 (check-in y doble reporte): cierran el problema de «mi rival no apareció / mintió en el marcador».
3. Desde la PC: abrir los enlaces de la sección 3 de pesmodding y decidirte por P1/P2/P4.
