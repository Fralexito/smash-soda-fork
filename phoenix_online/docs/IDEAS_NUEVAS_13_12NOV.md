# Ideas nuevas (12 nov 2026, segunda ronda)

No repite las 71 ideas de `IDEAS_FUTURO_12NOV.md` ni las 12 de `BUSQUEDA_PROFUNDA_12NOV.md`.
Cada idea dice de dónde sale: **[web]** = lo leí hoy en una fuente (enlace al final), **[propia]** = propuesta mía, sin fuente.
Límite honesto: fueron 8 búsquedas cortas, no una investigación larga. Los sitios evoweb y pesmodding no se abren desde la nube.

## Las 5 que haría primero

| # | Idea | Para qué sirve | Esfuerzo |
|---|---|---|---|
| 1 | **Banco de pruebas del lector del juego** (AzrakHQ `parsebench`) | Probar una respuesta del servidor contra el lector del propio PES y ver qué entendió, sin adivinar. Habría evitado las 8 pruebas a ciegas del aviso de regalos. | Medio. Necesita su módulo «probe» dentro del PES, así que requiere tu SÍ |
| 2 | **Mando virtual** para que yo pulse LB, RB, X, Y | Hoy esas pantallas dependen de tu mando. Con un mando virtual las pruebo solo. | Bajo, pero instala un controlador (ver riesgo abajo) |
| 3 | **Guardar también las respuestas HTTP** en las capturas | Hoy solo se guarda la pregunta del PES, no lo que contestamos. Por eso tuve que reconstruir la respuesta de regalos a mano. | Muy bajo (lo puedo hacer ya) |
| 4 | **Red propia sin depender de Tailscale** (NetBird o Headscale) | Tus amigos entran a tu red privada sin cuentas de terceros ni tope de dispositivos. | Medio; necesita un servidor siempre encendido |
| 5 | **Tablero de salud del servidor** (jugadores conectados, espera en cola, partidos abandonados) | Ver de un vistazo si el online va bien el día de la prueba y después. | Bajo |

## Online

1. **[web] Banco de pruebas del lector (parsebench).** AzrakHQ documenta cómo ejecutar el lector del juego con nuestros bytes y leer qué guardó en cada campo. Sirve para regalos, Matchday, clanes y cualquier pantalla que «no entiende» la respuesta. Su nota avisa de que un fallo dentro de la llamada deja el juego «tocado» para pruebas reales, así que se usa en sesiones aparte.
2. **[propia] Guardar la respuesta HTTP** junto a la pregunta en `capturas/http.jsonl`, sin secretos.
3. **[propia] Modo «ensayo» por comando:** un archivo `parches_respuesta.json` que cambia cualquier respuesta sin reiniciar el servidor ni cerrar el PES (AzrakHQ tiene algo parecido, `replypatch`). Hoy cada forma nueva de regalos costó cerrar PES, reiniciar y reabrir.
4. **[web] NetBird**: red privada completa de código abierto (BSD-3), con sus propios programas cliente y panel web, gratis si la alojas tú. Es la opción más completa para no depender de Tailscale.
5. **[web] Headscale**: servidor de control propio que usa los programas de Tailscale. Es más ligero, pero los clientes no son oficiales para ese uso y pide más configuración a mano.
6. **[web] ZeroTier**: queda como tercera opción; su controlador propio no tiene panel cómodo y el plan gratis tiene tope de dispositivos.
7. **[web] Servidor gratis en la nube (Oracle Always Free):** sigue existiendo, pero una noticia de InfoQ (julio 2026) dice que redujeron a la mitad el tope gratis (2 núcleos y 12 GB) y que apagan las cuentas que se pasan. Otras guías aún dicen 4 núcleos y 24 GB. No está confirmado en la página de Oracle. Riesgos: no siempre hay cupo y pueden reclamar máquinas sin uso. Alternativas de pago típicas: 4 a 7 dólares al mes.
8. **[web] Medidas útiles para un servidor pequeño:** jugadores conectados, tiempo de espera para emparejar, partidos abandonados e inicios de partido que fallan. Se exponen en una página `/metrics` y se ven con Prometheus y Grafana.
9. **[propia] Aviso al móvil de Frank** cuando el servidor se cae o un módulo se apaga con los interruptores de emergencia.
10. **[propia] «Rival falso» con partido completo:** hoy el rival falso llega a la pantalla de carga. Si se graba el tráfico de un partido real de 2 jugadores el día de la prueba con tu amigo, luego se puede reproducir para ensayar sin él.

## Parche y juego

11. **[web] Módulo de asistencia de público** (FletchAJX, octubre 2026, Sider 7.x): fija el público local y visitante por separado, de 0 a 100 %. Encaja con «partidos de liga con más o menos gente según la importancia».
12. **[web] Entrenadores reales en el campo** (Ali Redaa, octubre 2026): módulo que pone a los técnicos reales junto al banquillo.
13. **[web] Sider 7.4.0** (abril 2026): ya usas la 7.4.1, así que estás al día.
14. **[propia] Público según el torneo de tu web:** unir la idea 11 con tu liga, para que una final llene el estadio y un partido de relleno no.

## Herramientas

15. **[web] Mando virtual (ViGEmBus + la librería vgamepad de Python).** Riesgo: ViGEmBus está **retirado** desde noviembre de 2023 (última versión 1.22.0). Sigue funcionando y x360ce lo usa todavía, pero ya no recibe arreglos. Hay un proyecto nuevo llamado VIIPER que se inspira en él; no está confirmado que sea su sucesor. Es un controlador de sistema: solo con tu SÍ y de la fuente oficial.
16. **[propia] Lector de pantalla como herramienta fija** (ya hecho hoy): usarlo en todos los hilos para que yo vea avisos y textos sin pedirte capturas.
17. **[propia] Diccionario de mensajes del juego:** la tabla de AzrakHQ `server_errors.tsv` une cada error con su texto en pantalla. Con el lector de pantalla puedo decir al instante qué error del servidor provocó cada aviso.

## Comunidad

18. **[web] Estado en Discord («Jugando Phoenix Online, 2-1, minuto 60»).** Discord permite texto propio, tiempos, tamaño del grupo e imágenes. El juego no lo trae, así que lo mandaría tu launcher. Existen MultiPresence y CrossPlatformPlaying para juegos sin soporte, como referencia.
19. **[propia] Botón «Unirse» en ese estado de Discord** para entrar a la sala de un amigo desde Discord (lo permite la misma función de Discord; depende de tu launcher).

## Lo que evitaría

- Instalar ViGEmBus sin decidirlo tú: es un controlador del sistema y está sin mantenimiento.
- Elegir Oracle gratis para algo que tus amigos usan a diario sin plan B: pueden apagar la máquina.
- Usar `parsebench` en la misma sesión en la que se prueba con jugadores reales.

## Fuentes

- Redes privadas: https://wz-it.com/en/blog/mesh-vpn-comparison-2026-overview/ · https://www.bitdoze.com/netbird-vs-headscale-vs-tailscale/ · https://tech-insider.org/tailscale-alternatives-headscale-netbird-zerotier-nebula-twingate-2026/
- ViGEmBus: https://github.com/nefarius/ViGEmBus · VIIPER: https://github.com/alia5/viiper · x360ce: https://x360ce.com/
- Oracle: https://infoq.com/news/2026/07/oracle-cloud-free-tier-limits/ · https://space-node.net/blog/oracle-cloud-free-tier-vps-2026
- Discord: https://docs.discord.com/developers/platform/rich-presence · MultiPresence: https://awesome.ecosyste.ms/projects/github.com%2Fdekirai%2Fmultipresence
- Sider y módulos: https://www.pesmodding.com/2026/10/pes-2021-crowd-attendance-module.html · https://www.pesmodding.com/2026/10/pes-2021-real-coaches-on-pitch-home.html · https://www.pesmodding.com/2026/04/pes-2021-sider-v740.html
- Medidas de servidor: https://www.mintlify.com/googleforgames/agones/operations/monitoring
- AzrakHQ (MIT): `referencia_azrak_mit/notes/re/functions/parsebench.md`
