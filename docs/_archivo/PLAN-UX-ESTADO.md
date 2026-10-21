# Plan de experiencia en Link: qué se hizo, qué falta y qué probar (19-oct-2026)

Rama `plan-ux` (desde la etiqueta `antes-plan-ux-0.3.20`). Un commit por cambio, 0.3.21 → 0.3.34. Volver atrás: `docs/VOLVER-A-0.3.20.md`.
Mapa del menú nuevo: `docs/GUIA-MENU-NUEVO.md`. Lo que depende de la web: `docs/PROMPT-WEB-plan-ux.md`.

## Hecho (probado en el simulador, compilado sin errores)

| Versión | Fase | Cambio |
|---|---|---|
| 0.3.21 | 0 | Mensajes que mandan a otra pantalla llevan el botón que la abre; ViGEmBus con botón |
| 0.3.22 | 0 | Conexión y Juegos fuera de «MÁS» |
| 0.3.23 | 0 | «Soltar rival» pasa a Otras salas (hoy Jugar) |
| 0.3.24 | 1 | Semáforo Phoenix único (guía paso 1, Antes de abrir, Diagnóstico y Mi PC) |
| 0.3.25 | 1 | La pausa de la web dice su causa (`estado.web.codigo`); ARREGLAR TODO espera a que cierres PES y sigue solo |
| 0.3.26 | 1 | Asistente de bienvenida |
| 0.3.27 | 2 | Pantalla INICIO: jugar con un amigo, buscar rival, amigos, entrar a la mejor sala |
| 0.3.28 | 2 | Sentado automático con «solo mis amigos» |
| 0.3.29 | 2 | Tarjeta de resultado con Revancha |
| 0.3.30 | 2 | «Otras salas» → «Jugar», orden por invitación/amigos/ping |
| 0.3.31 | 3 | Glosario: «Conectar mi cuenta», «Mi juego» |
| 0.3.32 | 3 | Mismos nombres en el menú y en el título |
| 0.3.33 | 3 | «Ajuste fino» plegado en Transmisión |
| 0.3.34 | 3 | Moderación y Permisos junto a Gente; «Lo fundamental» arriba en Ajustes |

Pruebas del motor (MSVC): escenarios 55/55, módulos OK, fichajes 121/121, fichas 73/73, traspasos 107/107, actualización 42/42.
No se corrieron `correr_perfiles` (pide una copia del SYSTEM00000000) ni `correr_usuarios_vivo` (pide PES abierto).

## Del plan que NO se hizo en Link (y por qué)

- Vincular por navegador, aprobación automática de anfitrión, PC de invitado, «Jugar ahora», cuántos buscan rival, «En revisión», instalador .exe,
  publicar versiones: **dependen de la web** → `PROMPT-WEB-plan-ux.md`.
- Medidor de subida automático: necesita medir la subida desde Link (hay un `/v1/diagnostico` de red que podría servir); queda para otra vez.
- Ping y retraso del mando por puesto con «Arreglar mando», «Ocultar el mando repetido» de DS4Windows, perfiles de sala de un toque
  («Amigos/Pública/Torneo»; hoy hay perfiles guardados a mano en Opciones), «Iniciar con Windows»: no estaban en el encargo de esta ronda.
- Desconexión a mitad («esperar 2 min / seguir sin él»): depende del árbitro, aún sin probar en real.

## Sin probar en la app real (solo simulador)

1. Que la interfaz cargue en el WebView2 de Link 0.3.34 sin errores (abrir Link y mirar INICIO).
2. JUGAR CON UN AMIGO de punta a punta con Parsec real: abre sala, copia enlace, invitación que llega a la web/al amigo.
3. Sentado automático con un invitado real que pulsa su mando estando en «Mirando» (y con «solo mis amigos»: depende del nombre del perfil).
4. Semáforo con datos reales: ViGEmBus ausente, Parsec sin sesión, web en pausa (`HOST_NO_AUTORIZADO` en una cuenta nueva).
5. ARREGLAR TODO esperando a que se cierre PES de verdad (detecta el cierre y sigue).
6. Revancha tras un partido real (con el árbitro automático y a mano).
7. Asistente de bienvenida en una PC limpia (sin cuenta / sin ViGEmBus).
8. Botón INSTALAR VIGEMBUS: que el navegador baje el instalador oficial 1.22.0.
