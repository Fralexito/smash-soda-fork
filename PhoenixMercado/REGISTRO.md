# Bitácora del frente MERCADO (solo se AÑADE; nunca editar ni borrar)
Formato: `AAAA-MM-DD HH:MM (Lima) | Cuenta A/B | MERCADO | Qué cambió | Archivos/commit | HECHO / A MEDIAS / PENDIENTE | Siguiente paso`
Protocolo: `COORDINACION.md` (raíz) → «Dos cuentas de Claude».

---
## 2026-10-08 08:50 (Lima) · Cuenta A (chat WEB) · VOLCADO INICIAL reconstruido desde git y `claude/phoenix-mercado-estado.md`
⚠️ Escrito por el chat WEB: **el chat MERCADO debe revisarlo y añadir debajo lo que falte** (lo que está a medias, datos de pruebas, lo que Fralex pidió en ese chat).

**Rama `mercado-fase0`, carpeta `PhoenixMercado/` (C++20, fuera de la build de Phoenix Link).** Commits: eca9883 fase 0 (API, SHA-256, copias, consola, pruebas) · ee4881b fase 1 (option file leer/mover/guardar verificado, lector CPK/Player.bin, catálogo) · cca823a saves nuevos · 95da52c emparejamiento Phoenix ID ↔ ID local + integridad (huella, modo seguro, informe) · 1de7c0a subida de catálogo por lotes y equivalencias a la web · 666a2b3 saneo de catálogo según CHECK de la web.
**Probado en la PC de Fralex (49/49):** /eco y /yo; modo compartido (token de Phoenix Link) aceptado por la web; option file cifrar/descifrar idéntico; 740 equipos; fichaje visible en el juego conservando rostro (Lautaro Blanco → Sporting CP); modo seguro si option file y base son de parches distintos.
**Parches:** ConmeGOL Patch 26 (`download/CGP_database.cpk`, save 239200) · Sudamerican Project (`download/SP_Subs.cpk`, save 292733975847239680). Fralex no tiene Football Life instalado. Ambos parches comparten carpeta save.
**Emparejamiento Conmegol → Sudamerican:** 10.676 automáticos, 3.849 a revisar, 3.716 sin candidato; clubes 464 auto / 136 a revisar.
**Estado de la web (8 oct, todo desplegado):** /catalogo, /fichajes, /fichajes/aplicados, /huella, /clave-publica (Ed25519 id k67bd032d50), /plantillas, /correcciones, /reportes/lote, /liga/cambios (firmado), /liga/aplicado, **/equivalencias** POST (formato `phoenix-mercado/emparejamiento@0.x`, máx. 3000 filas por llamada, solo staff; lo confirmado por staff nunca se pisa; en «revisar» el pes_id_local va en `candidatos`) y GET (solo automatico/confirmado, páginas de 5000). Clubes CPU, tope 40, agente libre = sin club. Contrato: `claude/mercado-api.md`.
**Orden obligatorio:** subir catálogo real (`/catalogo`) ANTES que equivalencias (phoenix_id = `lm_jugadores.id` debe existir). Al subir el real, pedir a WEB que borre los datos demo.
**Decisiones de Fralex:** option file «Mundial» con fichajes reales solo dentro de su switcher; dos DTs nunca con el mismo jugador; detectar y revertir ediciones fraudulentas también sin internet y reportar al reconectar; al cerrarse un trato en la web, el programa aplica el traspaso solo; «Liga Máster» también se refiere a la LM interna del PES (quiere que los fichajes entre DTs se vean dentro y fuera); se integrará en Phoenix Link.
**Pendiente:** aplicar traspasos en cola (juego cerrado), restauración automática por huella, switcher «Liga Phoenix» (CPY.ini propio), habilidades de portero y media, pantalla dentro de Phoenix Link.
**No verificado:** si 1de7c0a ya se probó contra la web real; resultado del primer envío de equivalencias.
---
2026-10-08 08:50 (Lima) | Cuenta A | MERCADO | Protocolo de dos cuentas añadido (COORDINACION.md copiado de la rama LINK, CLAUDE.md, esta bitácora) | COORDINACION.md, CLAUDE.md, PhoenixMercado/REGISTRO.md | HECHO | Chat MERCADO: completar el volcado; subir catálogo real y luego equivalencias
