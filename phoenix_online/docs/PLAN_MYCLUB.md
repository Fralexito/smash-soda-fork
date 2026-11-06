# Plan de myClub (Phoenix Online). Actualizado el 05-nov-2026

Estado: myClub funciona en el PES base contra nuestro servidor. Se puede ver el club, la plantilla, el DT, contratos, cazatalentos y estadísticas, fichar un DT con GP y jugar un partido completo contra la COM. El resultado y los GP quedan guardados.
Detalles técnicos: `ESPEC_MYCLUB.md`. Cómo se logró: `GUIA_MYCLUB_REVIVIDO.md`.

## Orden acordado con Frank (05-nov)
1. **Catálogo automático de jugadores**
   - Hecho: 661 equipos y 17 583 jugadores con nombre (`tools/catalogo_pesdb.py`).
   - Hecho: la posición y la nación ya se leen de Player.bin (§34).
   - Falta: las habilidades, para calcular la media, la rareza y el precio reales.
2. **Fichajes**
   - Hecho en el servidor (05-nov, `ESPEC_MYCLUB.md` §33): Representantes, Cazatalentos y renovar contratos, pagando con GP o con monedas gratis.
   - Hecho en el servidor (05-nov noche, §49): subastas de cazatalentos de la casa.
   - Falta: probar todo en vivo.
3. **Progreso de jugadores**
   - Hecho en el servidor: experiencia y nivel tras cada partido.
   - Hecho en el servidor: contrato real (partidos x 90, §47), gasto por partido y cansancio (§45, apagado).
   - Falta: confirmar la curva real de experiencia y la escala del cansancio.
4. **Rival COM variado**
   - Hecho: rotación entre 7 clubes grandes.
   - Hecho en el servidor (§44): la dificultad sube con 3 victorias seguidas y baja con 2 derrotas. Falta probarla en vivo.
   - El escudo «MD» del Madrid es el oficial del juego, que no tiene licencia: no es un fallo.
5. **Para mucha gente**
   - Identidad propia por persona: el pase web de Link, validado en Supabase. Hoy todas las copias CPY comparten el mismo SteamID.
   - Base de datos en Supabase en lugar de la PC.
   - Panel web para GP, monedas y eventos.

## Arquitectura a largo plazo, manejada desde la web (Frank, 05-nov)
Frank preguntó si todo se puede armar y mantener desde su página. Sí, en 3 piezas:
1. **Servidor Phoenix Online.** Habla con PES. Hoy corre en la PC y más adelante en un VPS. No guarda reglas fijas: las lee de Supabase.
2. **Supabase** (el mismo proyecto de la web):
   - Datos: clubes, GP, monedas, historial de partidos y eventos.
   - Reglas: GP por partido, agentes, cazatalentos, precios, rival COM y dificultad.
3. **La web (panel):**
   - Inicio de sesión: el pase da la identidad propia en PES.
   - Botones para dar GP o monedas, armar agentes y eventos, y elegir el rival.
   - Clasificación y vista de cada club.
   - Los datos de Konami solo se muestran con permiso, porque mostrarlos tiene riesgo legal.
- Mantenible porque las reglas se cambian desde la web, sin tocar código ni reiniciar el servidor.
- El servidor sigue siendo un programa aparte: una web estática no puede hablar con PES.
- Va en el paso 5. Antes de eso, cada sección de config.json se pasa tal cual a una tabla.

## Menú del jugador de myClub (visto en vivo, 05-nov 07:17)
Se abre en Miembros con X (A en Xbox) y luego D (Y en Xbox). Opciones:
- Añadir a plantilla.
- **Renovación del contrato:** solo aparece si al contrato le quedan pocos partidos. Hecho en el servidor (+10 partidos por GP o monedas).
- **Bloquear / Desbloquear** (para que no se venda ni se use por error).
- **Convertir en Entrenador de experiencia:** el jugador pasa a dar experiencia a otros.
- **Liberar:** soltar al jugador, quizá a cambio de GP.
- **Usar objeto:** Boleto de contrato (x0), un objeto que renueva sin gastar GP.
- Probado en vivo (05-nov): bloquear/desbloquear, liberar y convertir en entrenador.
- Hecho en el servidor (§48), falta probarlo en vivo: usar un entrenador de experiencia, el Boleto de contrato (`admin_myclub.py dar-boletos`) y recuperar el cansancio.

## Decisiones de Frank
- **Abandonar un partido vs COM = derrota** (05-nov). Opción `myclub.abandono_es_derrota`.
- **Monedas de myClub:** solo como premio gratis, con la opción `myclub.monedas_gratis`. **Nunca se venden** (regla: nada de dinero real).
- **Dar GP o monedas a una persona:** `tools/admin_myclub.py`. Hoy es por consola; más adelante será un botón en la web.
- **Modelo:** usar Opus para leer el juego (ingeniería inversa) y avisar cuando se pueda volver a Sonnet.

## Ideas de Frank, apuntadas para después
- **Leyendas y jugadores propios:** se puede, si están en la base de datos del juego o del parche. El servidor solo reparte los números de jugador. Todos los que jueguen online necesitan la misma base.
- **Varias cartas del mismo jugador con distinto valor** (pedido de Frank):
  - (a) Seguro: copias del jugador en la base del parche, con otro número y otras medias.
  - (b) Por probar en el paso 2: un mismo gameplayer_id con default_rarity, default_overall y enhanced_parameters distintos en la ficha del servidor. Falta confirmar que PES los usa en el partido.
- **Jugadores propios en la base:** se agregan en la base del parche y el catálogo los toma solos.
- **Usar otra base de datos o el parche propio:** volver a correr el catálogo con esa base. Si cambian los números de los jugadores, pasar los clubes existentes a la base nueva (dejarlo automático).
- **Diseño y color de las cartas:**
  - Son imágenes del juego: se cambian con un mod gráfico de Sider (prender/apagar), y solo lo ve quien tenga el parche.
  - El servidor decide la rareza de cada jugador.
  - Falta ubicar los archivos de las texturas de las cartas.
- **App tipo The Gaffer, pero mejor:** tiene su propio hilo, «App móvil · Planear app tipo The Gaffer». Se apoyará en este servidor y en Supabase, para usarla desde cualquier lugar.
- **Probar myClub con el parche de Frank:** después del paso 1, y con su permiso.
- **Partidos entre dos personas:** emparejar a 2 jugadores y el relé de juego (ver `INVENTARIO_PESBUL.md`). Hacen falta dos PES conectados a la vez.

## «El mejor myClub que existe»: ideas que los fans pidieron y nunca salieron (Frank, 05-nov)
Regla: nada se paga con dinero real, y cada cosa se puede prender o apagar.
- **Solo con el servidor (lo más fácil):**
  - Intercambiar jugadores entre personas, y préstamos.
  - Mercado de traspasos con precio por oferta y demanda.
  - Misiones diarias y semanales con premio.
  - Modo draft: elegir el equipo entre opciones al azar.
  - Torneos y ligas propias entre amigos. Frank lo pidió el 05-nov: cada persona juega con SU club de myClub.
    - El torneo se arma en la web: cruces y fechas.
    - El servidor empareja a los dos del cruce en un partido myClub.
    - El resultado llega solo y la tabla de la web se actualiza.
    - Necesita antes el partido de 2 personas.
  - Vender jugadores a otras personas por GP desde la web o la app (Frank, 05-nov). El servidor pasa la ficha de un club al otro.
  - Cartas que evolucionan con TUS partidos, de forma única.
  - Leyendas como premio, nunca en venta.
  - Historial del club: trofeos, goleadores y rivalidades.
- **Con la web o la app:**
  - Página de cada club y clasificación.
  - Avisos al celular.
  - Manejar el mercado y los intercambios desde el celular.
- **Con mods (más difícil):** pantallas nuevas dentro de PES y diseño de cartas (Sider).

## Riesgos y límites
- Los datos de jugadores son de Konami: mostrarlos en una web pública tiene riesgo legal. Revisarlo antes de publicar.
- No cobrar nada por myClub sin abogado.
- Lo que sale de la base del juego (catalogo.json, los .bin) y config.json no se suben nunca.

## Jugar online con un amigo (pregunta de Frank, 05-nov 09:35)
Los dos PC deben tener la MISMA base de jugadores (mismo parche, misma versión): el servidor manda números de jugador.
Falta, en orden:
1. Instalador «Phoenix»: parche + conexión de un clic (Link/Core en vez de hosts).
2. Control de versión: al entrar, el cliente/Link manda la versión del parche y el servidor avisa si no coincide (en vez de dejar que el juego se caiga).
3. Servidor alcanzable desde internet: puerto abierto en la PC de Frank o VPS.
4. Cada persona con su propia copia de PES 2021.
- Opciones para que el amigo llegue a la PC de Frank (05-nov 09:38): (1) red virtual Tailscale/Radmin VPN — recomendada para pruebas, sin tocar el router; (2) abrir puertos en el router (puede fallar con IP compartida/CGNAT); (3) VPS para muchos. Con (1), el servidor debe anunciar la IP de la red virtual (gamerelay, STUN, SVRLIST): revisar config antes de la prueba. Descarga del programa solo con permiso de Frank.
