# Auditoría del online Phoenix (10-nov-2026)

PES del online: **solo el PES base `C:\PES21\eFootball PES 2021`** (base del parche de Frank).
Servidor: `phoenix_online/` (Python). Este documento dice qué funciona, qué falta y cada riesgo con su
solución preventiva. Se actualiza en cada ronda.

## 1. Qué funciona hoy (confirmado en vivo con el PES base)
- Entrar al online: país, EULA, login, crear usuario, menú online.
- Vestíbulo, crear salón, lista de jugadores del salón.
- Partida rápida: buscar rival y cancelar (con un solo jugador).
- Comprobación de red con nuestro relé (18 ms, igual que PESBUL) y Divisiones en línea (D12).
- myClub: club propio por cuenta, plantilla, contratos, entrenadores, subastas de la casa,
  recuperar forma, partido vs COM con GP, reglas que se cambian sin reiniciar.

## 2. Qué falta (de más a menos importante)
1. **Partido entre 2 personas.** Programado el 10-nov (fases 2 y 3 del relé). Falta probarlo con
   **dos PES** (dos PCs: Frank + un amigo). Ojo: el amistoso de sala libre va directo entre las dos
   PCs (P2P); Partida rápida y Divisiones van por nuestro relé.
2. Identidad definitiva por persona (pase de la web): hoy hay una identidad **provisional** por IP.
3. Espejo en Supabase: tablas de la migración 162 CREADAS el 10-nov (RLS en las 4, función del pase solo para usuarios con sesión). Falta: poner la clave de servicio en la PC (variable PHOENIX_SUPABASE_KEY) y encender `supabase.activo`; botón «Pedir pase» en la web.
4. Detalles de myClub que esperan ingeniería inversa: Boleto ×0 y la carta «PT 40» del entrenamiento.

## 3. Riesgos y su solución preventiva
| # | Qué puede fallar | Solución | Estado |
|---|---|---|---|
| 1 | Todas las copias mandan el mismo SteamID → todos los amigos en el mismo club | Identidad provisional: si el SteamID es de los compartidos, cada IP tiene su propia clave fija; el PES de la misma PC que el servidor conserva su SteamID (el club de Frank no cambia). Con Tailscale cada amigo tiene IP fija = conserva su club. `identidad.por_ip` permite fijar a mano. | ✅ hecho |
| 2 | El servidor anuncia una IP equivocada (127.0.0.1, IP vieja) → el amigo no conecta o el relé falla | `ip_publica: "auto"`: se anuncia la IP por la que llegó cada cliente (misma PC, red de casa, Tailscale). URLs con `{ip}`. En un VPS se pone la IP fija. | ✅ hecho |
| 3 | Las capturas llenan el disco | Tope total (2 GB) y por archivo (50 MB); al pasarlo se deja de capturar sin cortar a nadie; capturas grandes se guardan comprimidas. | ✅ hecho |
| 4 | La base de datos se daña (corte de luz, cierre a la fuerza) y se pierden los clubes | Modo WAL, revisión al arrancar, **copia automática al arrancar y cada 6 h** (se guardan 28). `tools/respaldo_base.py` para copiar o restaurar. | ✅ hecho |
| 5 | Conexiones «fantasma» (PC apagada, sin internet) que se quedan para siempre | TCP keepalive (~2 min); máximo 16 conexiones por IP. | ✅ hecho |
| 6 | Un pedido HTTP lento o gigante bloquea el servidor | Espera máxima 20 s y cuerpo máximo 4 MB. | ✅ hecho |
| 7 | Un mensaje roto hace crecer la memoria sin límite | Tope de 16 MB por mensaje troceado. | ✅ hecho |
| 8 | El servidor se cae y nadie se entera | `tools/INICIAR_SERVIDOR.bat` lo reabre solo (espera 5, 10, 30, 60 s…). Registro en `registro/servidor.log` (con rotación). | ✅ hecho |
| 9 | Abrir dos servidores a la vez (o PESBUL local abierto) | Si los puertos están ocupados, avisa claro y no reintenta. | ✅ hecho |
| 10 | Actualizar el servidor corta un partido | `tools/mantenimiento.py on`: nadie nuevo entra; `GET /estado` dice cuántos juegan; se cierra cuando hay 0. | ✅ hecho |
| 11 | Un amigo con otro parche/versión entra y el partido se desincroniza | `exigir_versiones` (apagado hasta conocer los valores reales: el servidor ya los registra en cada login). | ✅ listo, apagado |
| 12 | El juego manda un comando que no conocemos | Respuesta mínima NOERR (ya existía) + contador en `GET /estado` para saber cuál implementar primero. | ✅ hecho |
| 13 | Partido de 2 personas: relé incompleto | Fases 2 y 3 programadas (acc9664, ver ESPEC_GAMERELAY_FASE23.md): partida ligada por NTL, lista de 3, reenvío 9/10/12/15, copia TCP deduplicada, eco RTT, tope 2048 B. Falta la prueba real con 2 PES. `tools/rival_falso.py` permite probar el emparejamiento con un solo PES. Pruebas 2-5 (10-nov): el PES ya se liga a la partida y abre 5/14/15; manda por el 15 fragmentos de 16 B (40 dd…, 8f…, 8c…) y los repite cada 5 s. Arreglo: cada bloque es un mensaje propio (antes se juntaban). | ✅ programado, ⏳ prueba real |
| 14 | Desconexión a mitad de partido de 2 | La salida o caída de uno libera su estado en el relé; la partida se cierra al irse los dos (o a las 3 h). El resultado de cada lado se guarda y se avisa si no coinciden. Qué hace el PES del que se queda: se verá en la prueba real. | 🟡 base hecha |
| 15 | El archivo hosts sigue apuntando al servidor cuando está apagado → el online «normal» falla | Bats CONECTAR/DESCONECTAR (ya existen en `tools/amigo`); más adelante Phoenix Core redirige sin tocar hosts. | 🟡 parcial |
| 16 | Amigos fuera de casa no llegan (router, NAT) | Recomendado **Tailscale** (sin abrir puertos del router). Luego un VPS. | 🟡 decisión de Frank |
| 17 | Firewall de Windows bloquea los puertos | Puertos: TCP 10000, 10001, 5000; UDP 5100, 3478, 5730. Lo abre Frank (es ajuste de seguridad: no lo tocamos nosotros). | 🟡 parte de Frank |
| 18 | Dos amigos en la misma casa sin Tailscale comparten IP → mismo club | Con Tailscale cada PC tiene su IP; o `identidad.por_ip`. Solución final: pase web. | 🟡 documentado |
| 19 | Supabase (compartido con la web) se rompe por un cambio nuestro | Nada se aplica sin el SÍ de Frank; el servidor sigue con su base local aunque Supabase falle. | ✅ regla |
| 20 | Claves (NclMio, relé) se filtran | Solo en archivos locales de la PC; nunca en repo, chat ni Drive. | ✅ regla |

## 4. Configuración nueva (`config.ejemplo.json`)
`ip_publica: "auto"`, URLs con `{ip}`, `gamerelay.ip: "auto"`, `identidad`, `respaldos`,
`capturas_activas`, `capturas_total_mb`, `capturas_archivo_mb`, `registro`, `max_conexiones_por_ip`,
`inactividad_max_s` (0 = sin límite: PES calla en los menús), `archivo_mantenimiento`, `exigir_versiones`.
Todas tienen valor por defecto: un `config.json` viejo sigue funcionando igual.

## 5. Plan
1. ✅ Blindaje del servidor (esta ronda; 83 pruebas OK).
2. Instalar en la PC (con respaldo y forma de volver atrás) cuando el PES esté cerrado.
3. ✅ Relé de 2 jugadores (fases 2 y 3) + desconexiones (88 pruebas OK).
4. Prueba real con 2 PES (antes, prueba de emparejamiento con `tools/rival_falso.py`).
5. Pase de identidad web + Supabase (con el SÍ de Frank).
