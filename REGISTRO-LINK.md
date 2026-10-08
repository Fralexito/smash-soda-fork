# Bitácora del frente LINK (solo se AÑADE; nunca editar ni borrar)
Formato: `AAAA-MM-DD HH:MM (Lima) | Cuenta A/B | LINK | Qué cambió | Archivos/commit | HECHO / A MEDIAS / PENDIENTE | Siguiente paso`
Protocolo: `COORDINACION.md` → «Dos cuentas de Claude».

---
## 2026-10-08 08:45 (Lima) · Cuenta A (chat WEB) · VOLCADO INICIAL reconstruido desde git y las notas del proyecto
⚠️ Escrito por el chat WEB, que NO trabajó este frente: se basa en el historial de git y en la memoria del proyecto. **El chat LINK debe revisarlo y añadir debajo lo que falte** (lo que está a medias, lo que no compila, lo que Fralex pidió en ese chat).

**Ramas:** `master` (base del autor + correcciones de compilación del 7 oct) · `experimental-html` (pantallas Sala/Opciones/Actividad estilo prototipo HTML, «Phoenix Portal») · **`rediseño-phoenix-portal` = rama de trabajo de LINK** · `mercado-fase0` = frente MERCADO. Tag estable mencionado por Fralex: `v1.0-base-estable` (no está en el remoto; puede ser solo local).
**Últimos commits de LINK:** 796d5da build en worktree `_phoenix-link` · 55abab2 pantalla Gente (lista + ficha: VIP, mod, teclado, ratón, quitar mando, expulsar, banear) · d712bc8 COORDINACION.md · 5999b0a / f68e517 renombre total a **Phoenix Link** (`PhoenixLink.exe`) · c1b928a Sala con degradado y acentos.
**Decisiones de Fralex:** la app se llama «Phoenix Link» en todos lados (logo nuevo pendiente de él); identidad Galaxy cian+púrpura, nada de Soda Arcade; estructura Sala · Partido · Mandos · Comunidad · Ajustes; español por defecto + idiomas principales; máx. 8 mandos; acercar la interfaz ImGui al prototipo HTML pantalla por pantalla (no incrustar HTML); siempre reversible; Windows 10 y 11; solo funciona con su web; instalador solo para hosts verificados; host neutral y retraso artificial solo editable por staff en oficiales; Phoenix Mercado se integrará dentro de Phoenix Link (opción de segundo código solo para manager).
**Contrato con la web:** API `/phoenix` (`claude/contrato-v1.md`). La web ya devuelve `version_liga` en `/presencia` y `/sala/latido` (para que Link avise a Mercado de cambios de Liga Máster).
**No verificado:** estado de compilación actual de la rama; pantallas pendientes del rediseño; si existe el tag `v1.0-base-estable`.
---
2026-10-08 08:45 (Lima) | Cuenta A | LINK | Protocolo de dos cuentas añadido (COORDINACION.md, CLAUDE.md, esta bitácora) | COORDINACION.md, CLAUDE.md, REGISTRO-LINK.md | HECHO | Chat LINK: completar el volcado con lo que solo él sabe
