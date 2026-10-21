# Prompt para el chat LINK — «Automático: siempre» primero y por defecto (phoenix.lua v0.18b)

10 de octubre de 2026. De: chat SYNC. Pedido de FRALEX: «pon esa opción de primero y que sea por defecto».

---

Hola, chat LINK. FRALEX no quiere pulsar ningún botón: los fichajes deben entrar solos al volver al menú y entrar a un modo (Partido → amistoso). Desde el frente SYNC ya está hecho y probado (simulación 10/10) **phoenix.lua v0.18b**: si no hay `modo.txt`, o si dice algo raro, el modo es **AUTO-SIEMPRE** (antes era ACTIVAR). Si lo automático falla 3 veces, sigue cayendo a ACTIVAR. Está instalada en el PC de FRALEX desde hoy.

**Lo que hace falta en Link:**
1. **Tarjeta «Modo de recarga de fichajes»** (SYNC › Puente): poner **primero** 🟣 «Automático: siempre», con la etiqueta **«(recomendado, viene por defecto)»**. Después 🔵 «Automático: solo fichajes nuevos» y al final 🟢 «Activar» (sin la etiqueta de recomendado).
2. **Valor por defecto = `AUTO-SIEMPRE`** para quien nunca eligió. En el PC de FRALEX hay guardado «AUTO-FICHAJES»: cámbialo una vez a AUTO-SIEMPRE (él lo pidió), o enséñale cómo elegirlo.
3. **Instalador de Luas («Módulos del juego»):** sustituir la phoenix.lua v0.18 que lleva Link por la **v0.18b**. Está en la rama `mercado-fase0`, en `PhoenixSync/sider/pruebas/phoenix-v0.18b-prueba.lua`. Instálala con fin de línea LF; la sha256 con LF es `9ed74f457b4c2f407d4851b09b701a5ffb63e3a1f5707b322c13aad6f95526ab`. Si no, los amigos recibirían la v0.18, con ACTIVAR por defecto.
4. Texto sugerido para 🟣: «Los fichajes entran solos cada vez que vuelves al menú principal y entras a un modo. No hay que pulsar nada.»

Contrato de `modo.txt` sin cambios (una palabra; escritura atómica modo.tmp → modo.txt; raíz y modos del cambiador). Compila sin errores antes del commit y anota una línea en `REGISTRO-LINK.md`.
