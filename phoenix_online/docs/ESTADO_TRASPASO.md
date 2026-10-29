# Phoenix Online — estado para retomar (29-oct-2026, 19:50 UTC)

**Qué es:** un servidor online propio para PES 2021, al estilo PESBUL, ligado a las cuentas de la web de Frank. Código en `phoenix_online/` (Python, sin dependencias salvo `cryptography`). Tiene 22 pruebas (`python -m unittest discover -s tests`).

## Ramas (repo Fralexito/smash-soda-fork)
- `claude/project-thread-9ciw66` (PR #3, borrador): servidor, guía y documentación (`phoenix_online/docs/`).
- `claude/online-phoenix-core` (2781290 + c8e3d9f): opción del Core «online_phoenix», apagada por defecto. Hace la redirección dentro del juego. El hilo street ya la revisó, pero **todavía no está integrada ni instalada**.

## Dónde quedó la prueba real (29-oct)
1. PES (Conmegol) se abre **sin Sider**, porque Conmegol esconde los botones online con livecpk\Menu. El botón online es el icono «EF».
2. Con el hosts temporal, PES se conecta a nuestro ConnectGate y completa 7 comandos. Aparece la pantalla «País/Región: Peru», servida por nosotros.
3. **Bloqueo:** al confirmar Peru sale «Se ha producido un error.» y vuelve al menú, sin más red. Sospechosos:
   - fn 140fe2920 / 140fe2fb0 (diálogo AreaSelectAgreementDialog, error 4);
   - valores de `privacy_kind` y `has_area` (¿"0" o "1"? ¿"true"?);
   - un recurso de interfaz que falta sin Sider.
   Siguiente paso: decompilar esas fns con GhidraMCP y probar variantes en el config.

## Pendiente (en orden)
1. Resolver el error tras elegir país. Después: AUTH_STEAM (HTTP, ver qué formato de respuesta espera), CHALLENGECODE, LOGIN_ACCOUNT, SEND_PSID, PLAYER_DATA, crear perfil y lobby.
2. Envío segmentado de respuestas mayores de 0x410 (NclMio::DivBufForEasyEncryptionVer). Hoy la lista de URLs está recortada a 14.
3. Identidad: el SteamID de las copias CPY es falso y es el mismo para todos (239200). Plan: un «pase» de la web, escrito por Phoenix Link y metido por el Core. Validación en Supabase (migración ≥162).
4. Integrar online_phoenix en el Core. Volver a dar el botón online con Sider sin romper Conmegol: una casilla propia en el menú.
5. Prueba entre dos PC (P2P UDP 5730‥5739, TURN).

## Reglas
- La clave NclMio (56 B) vive **solo** en `D:\Frank\IA\CLAUDE\pes-online\nclmio_key.bin`. Nunca va en el chat, los documentos ni los repos.
- Sala limpia: este trabajo **no** lee archivos de PESBUL; solo la especificación cap_72 §11, el exe y Nikow5 (MIT).
- Al terminar una prueba, restaurar hosts con `RESTAURAR_HOSTS.ps1` (como administrador). **A las 19:50 del 29-oct seguían puestas las 5 líneas**: confirmar con Frank.
- Configuración que funciona: `blowfish_le: false`. GET_SVRLIST devuelve ACCOUNT y MENU. Los campos de nación y de perfil van como cadena.

## Para la próxima prueba (nota del 29-oct, 20:10 UTC)
- El servidor de la PC está **apagado** porque la PC se reinició. Para arrancarlo: en `D:\Frank\IA\CLAUDE\pes-online\phoenix_online`, ejecutar `python -m phoenix_online.servidor config.json`. Ya tiene instalado adcc2d1, con su config.json y vinculos.json locales.
- Revisar el hosts: si siguen las 5 líneas PHOENIX_PRUEBA, quitarlas con `RESTAURAR_HOSTS.ps1` (como administrador) en cuanto acabe la prueba.
- Pista del error del país: leído por memoria, el vector de naciones del login manager estaba **vacío** en el momento de confirmar. Sospecha: la lista que se dibuja no es la misma que usa fn 140fe2fb0, o se vacía al cerrar la conexión (DISCONNECT MOV) antes de que se elija el país.
- **Prueba del 29-oct a las 20:19** (con has_area "1" y la zona Lima): el error sigue igual y no llega CMD_GET_EULA_AREA_LIST. La pantalla de zonas (MenuSelectEulaArea, fn 140fe23a0 → 140fe2bc0 pide las zonas) nunca se abre, porque el error 4 salta antes, en 140fe2920. **Siguiente paso:** con REA, averiguar la condición que elige 140fe3f50 o 140fe2920 (estado del manager en +0x94 == 2) y qué dato del servidor la pone.
