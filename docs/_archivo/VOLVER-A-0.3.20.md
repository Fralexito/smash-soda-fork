# Volver a Phoenix Link 0.3.20 (antes del plan de experiencia)

El plan de experiencia (`plan-phoenix-link.html`) se aplicó en la rama **`plan-ux`**, con un commit pequeño por cambio.
Antes de tocar nada se dejó guardada la versión anterior de tres formas. Usa la que te sea más fácil.

## 1. Lo más rápido: la copia del programa (sin compilar)

Hay una copia completa de la carpeta de Link 0.3.20 (programa + interfaz + todo lo que va al lado):

- Carpeta completa: `C:\dev\PhoenixLink-respaldo\Link-0.3.20-completo\`
- Solo el programa: `C:\dev\PhoenixLink-respaldo\PhoenixLink-0.3.20.exe`
- Y junto al programa nuevo: `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\PhoenixLink-0.3.20.exe`

Pasos:
1. Cierra Phoenix Link.
2. Copia **todo** el contenido de `C:\dev\PhoenixLink-respaldo\Link-0.3.20-completo\` encima de la carpeta donde tienes Link
   (en el PC de desarrollo: `C:\dev\smash-soda-fork\_phoenix-link\x64\Release\`). Acepta reemplazar.
3. Abre `PhoenixLink.exe`. Abajo a la izquierda debe decir **PHOENIX LINK 0.3.20**.

Importante: copia la carpeta completa, no solo el `.exe`. La interfaz vive en la carpeta `ui\` de al lado, y un 0.3.20 con la `ui\` nueva
mostraría las pantallas nuevas.

Tus ajustes, tu cuenta unida y tus fichajes **no** se tocan: viven en `%APPDATA%` y en la carpeta del juego, no aquí.

## 2. Compilar 0.3.20 desde la etiqueta

La etiqueta **`antes-plan-ux-0.3.20`** marca el commit exacto de antes del plan (también está en GitHub).

```bat
cd /d C:\dev\smash-soda-fork\_phoenix-link
git status
git switch rediseño-phoenix-portal
cmake --build build --config Release --parallel
```

- `git status` primero: si sale algo sin guardar, guárdalo antes (commit) para no perderlo.
- La rama `rediseño-phoenix-portal` sigue exactamente donde estaba (el plan solo está en `plan-ux`), así que al volver a ella ya tienes 0.3.20.
- Si quieres el commit exacto sin ramas: `git switch --detach antes-plan-ux-0.3.20`.
- **Nunca** uses `COMPILAR_PHOENIX.bat` ni `--force`: borran lo que no esté guardado en git.
- CMake copia la carpeta `ui\` al lado del programa, pero no borra archivos viejos. Las pantallas nuevas son archivos que 0.3.20 no usa,
  así que no molestan; si quieres dejarla limpia, borra `x64\Release\ui\` antes de compilar.

## 3. Quedarte con parte del plan

Cada cambio del plan es un commit aparte en `plan-ux` (lista en `REGISTRO-LINK.md`, versiones 0.3.21 en adelante).
Para quitar solo uno: `git revert <commit>` dentro de `plan-ux` y compilar. Para ver la lista: `git log --oneline antes-plan-ux-0.3.20..plan-ux`.

## Qué NO se toca al volver

- La web y Supabase: el plan no cambió nada allí (lo de la web va como prompt en `docs/PROMPT-WEB-plan-ux.md`).
- Tus ajustes de Link (`%APPDATA%\Phoenix Link`, `phoenix-ui.json`), tu cuenta unida y tus fichajes en el juego.
- Lo poco que la interfaz nueva guarda aparte (si ya viste la bienvenida, el sentado automático…) vive en el almacenamiento interno de la ventana.
  0.3.20 no lo lee, así que no estorba.
