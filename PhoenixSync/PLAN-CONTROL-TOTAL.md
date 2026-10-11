# Control total de Phoenix: option file oficial, fichajes y stats

10 de octubre de 2026 · frente SYNC · para FRALEX y para cualquier IA que siga el trabajo.
Pedido de FRALEX: «un option file oficial en mi web; cuando alguien modifique algo, se modifica ese y se descarga automáticamente en todos» y «acceso total a modificar los fichajes y las estadísticas de los jugadores, ya sea en Liga Máster o en lo normal».

## 1. Cómo funciona (en simple)

```
                ┌──────────────── WEB (Supabase) ────────────────┐
                │ Foto oficial vN (option file + base)           │
                │ Cambios del grupo: mover · quitar · stats      │
                │ Panel «Control total» (solo admin)             │
                └───────▲────────────────────────────┬───────────┘
   admin publica la foto│                            │ cada PC baja lo nuevo
   (sync-publicar-oficial)                         ▼
         PC de FRALEX ──► Phoenix Sync ──► respaldo ──► Phoenix Link coloca ──► phoenix.lua recarga
                                                                        (al volver al menú y entrar a un modo)
```

- **Foto oficial:** es el punto de partida común. La publica el admin y las demás PCs la bajan solas (la primera vez siempre; después, en modo automático).
- **Cambios del día a día:** viajan como mensajes cortos (unos cientos de bytes, no 11 MB).
- **Admin = acceso total:** desde el panel web (cuando exista) o desde su PC con Sync.

## 2. Estado de cada pieza

| Pieza | Estado | Dónde |
|---|---|---|
| Fichajes `mover` / `quitar` entre PCs | ✅ Hecho y probado (simulación de 2 PCs) | `core/SyncCompartido.cpp` |
| Mis fichajes también en mi base (con «Activar») | ✅ Hecho (10 oct) | `entregarMiBase` |
| Aviso de recarga `recargar.txt` + phoenix.lua v0.18b automática | ✅ Hecho; v0.18b instalada | `avisarRecarga`, `sider/pruebas/phoenix-v0.18b-prueba.lua` |
| **Stats en Player.bin** (20 de campo, 5 de portero, pie malo, forma, lesiones, 13 posiciones) | ✅ Hecho (10 oct). Lectura real de Lamine = pantalla. Falta verlo en el juego tras un cambio | `core/FichasBase.*`, `sync-ficha`, `sync-stats` |
| **Operación de grupo `stats`** (respaldo → entrega → recarga; deshacer) | ✅ Hecho y probado (simulación) | `aplicarStats` |
| **Foto oficial**: publicar (admin) y bajar sola | ✅ Hecho y probado con una web simulada | `publicarOficial`, `ponerOficial`, `sync-oficial` |
| Cambios creados en el panel web | ✅ Sync los acepta si traen la huella de la foto oficial | `motivoIncompatible` |
| Web: rutas `/v1/sync/oficial`, bucket, tipo `stats` solo para el admin, panel «Control total» | ⏳ **PENDIENTE-WEB** | `prompts/PROMPT-WEB-oficial-y-control-total.md` |
| Web: grupo FRALEX + amigo en automático y `GET /v1/sync/grupos` | ⏳ **PENDIENTE-WEB** | `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` |
| Link: «Automático: siempre» por defecto e instalador con v0.18b | ⏳ PENDIENTE-LINK | `prompts/PROMPT-LINK-modo-por-defecto.md` |
| **Liga Máster · fichajes** (vender, fichar para ti, IA ↔ IA, dinero, valor) | ✅ Probado en el juego (guías 08–09); se aplica desde la web firmada con `sincronizar --ml` | `core/LigaMaster.*`, `core/Sincronizacion.*` |
| **Liga Máster · stats** (±1 con las barras de crecimiento) | 🟡 Método probado en el juego (guías 24–29, herramienta `palanca`); **falta programarlo dentro de Sync** | §4 |

## 3. Comandos nuevos (PowerShell, en la carpeta de PhoenixSync.exe)

```
.\PhoenixSync sync-campos                       # lista de stats y rangos
.\PhoenixSync sync-ficha 162114                 # stats actuales de un jugador (162114 = Lamine)
.\PhoenixSync sync-stats 162114 velocidad 95    # cambia una stat aquí y, si hay grupo, la publica (solo admin)
.\PhoenixSync sync-publicar-oficial             # (admin) tu option file + base = foto oficial
.\PhoenixSync sync-oficial                      # baja la foto oficial si hay una nueva (--forzar = aunque sea la misma)
.\PhoenixSync sync-deshacer                     # deshace lo último (fichajes, stats o foto oficial)
```

Reglas que no cambian: respaldo siempre antes, archivos nuevos (nunca encima), comprobación de huella, todo o nada, y los cambios solo los coloca Link.

## 4. Lo que falta (en orden)

1. **WEB:** pegar `prompts/PROMPT-WEB-grupo-prueba-fichajes.md` y después `prompts/PROMPT-WEB-oficial-y-control-total.md` en el chat WEB.
2. **Prueba en el juego (FRALEX, 5 minutos):** `sync-stats 162114 velocidad 95` → vuelve al menú → Partido → ficha de Lamine: ¿95? Después `sync-deshacer`.
3. **LINK:** pegar `prompts/PROMPT-LINK-modo-por-defecto.md`.
4. **Liga Máster · stats en Sync** (cuando FRALEX lo apruebe):
   - operación `lm-stats` `{ jugador_id, campo, direccion }`;
   - con el juego cerrado o en una ranura que no esté cargada, buscar la tabla D por anclas (guía 29 §4.3);
   - barra del campo = 9.999 (sube) o 0 (baja);
   - guardar en un archivo nuevo y comprobar que solo cambian 2 bytes por barra;
   - prueba en una ranura de prueba antes de la carrera real.

   Es el mismo método de `herramientas/palanca`, ya probado en el juego.
5. **Liga Máster · panel web:** pestaña que crea cambios en `lm_*` para `/mercado/v1/liga/cambios` (ya firmados y probados).
