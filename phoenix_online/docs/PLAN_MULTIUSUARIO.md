# Plan multiusuario: identidad, Supabase y panel web (05-nov-2026, borrador)

Estado: SOLO PLAN. Nada aplicado en Supabase (proyecto fiibiyijojkxqlsrhcil, compartido con la web): aplicar la migración necesita el SÍ de Frank y coordinar con Cuenta 2 (la web).

## 1. Identidad propia por persona
Problema: todas las copias CPY mandan el mismo SteamID (239200), así que hoy todos caerían en el mismo club.
Solución en 3 piezas:
1. **Pase web**: la persona inicia sesión en la web Phoenix y pide un «pase» (token corto, caduca). Se guarda en `phoenix_pases` (Supabase) con su usuario.
2. **Phoenix Link** escribe el pase en un archivo local que lee **Phoenix Core** (DLL) y lo mete en el login del juego (campo de nombre o `username` de CMD_LOGIN).
3. **Servidor**: en CMD_LOGIN valida el pase contra Supabase y usa `usuario_phoenix` como clave del club en lugar de steam_hex. Sin pase válido → error de login claro.
Mientras tanto (pruebas): `vinculos.json` local por steam_hex.

## 2. Datos en Supabase (espejo, no sustituto)
- El servidor sigue usando SQLite local (rápido, sin internet obligatorio).
- Un hilo «espejo» sube cada club cambiado a `myclub_clubes` (upsert por usuario) cada N segundos. Si Supabase falla, no pasa nada: reintenta.
- Clave del servicio solo en variable de entorno de la PC/VPS (`PHOENIX_SUPABASE_KEY`), nunca en el repo ni en el chat.

## 3. Reglas editables desde la web
- Tabla `myclub_reglas` (una fila, columna jsonb): GP por partido, agentes, cazatalentos, precios, dificultad, logros…
- El servidor la lee al arrancar y cada 60 s y la mezcla sobre `config.json["myclub"]` (lo de la web manda). Así se cambian reglas sin reiniciar.
- Solo administradores (rol en `profiles`) pueden escribirla (RLS).

## 4. Panel web (Cuenta 2)
- Ver clubes (GP, monedas gratis, partidos, racha, nivel COM).
- Botones: dar GP/monedas (inserta en `myclub_movimientos`; el servidor lo aplica y lo marca), editar reglas, armar agentes/eventos.
- Clasificación. Datos de Konami (nombres/fotos) solo con permiso (riesgo legal).

## 5. Orden propuesto
1. Migración 162 (borrador en `supabase/162_myclub_borrador.sql`) → SÍ de Frank + revisión de Cuenta 2.
2. Espejo de clubes (solo escritura) + lectura de reglas.
3. Panel web de solo lectura; luego botones.
4. Pase de identidad (Link + Core + servidor).
