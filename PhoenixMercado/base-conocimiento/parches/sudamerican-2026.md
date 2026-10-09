# Parche: Sudamerican Project 2026 (PES 2021) — auditoría de carpeta hecha (8 oct); catálogo y Liga Máster pendientes

| Qué | Dato |
|---|---|
| Nombre interno usado por Mercado | `sudamerican-2026` |
| Carpeta del parche | `D:\SP2026\` |
| Base de datos (CPK) | `D:\SP2026\download\SP_Subs.cpk` |
| Carpeta de guardados | `…\KONAMI\eFootball PES 2021 SEASON UPDATE\292733975847239680\save` |
| Option file | `292733975847239680\save\EDIT00000000` (11.026.772 B cifrado) |

Lo que ya se hizo con él: emparejamiento de jugadores ConmeGOL ↔ Sudamerican (10.676 automáticos, 3.849 a revisar, 3.716 sin
candidato; clubes 464 automáticos / 136 a revisar) y modo seguro cuando se mezclan option file y base de parches distintos.

Pendiente: catálogo propio, guardados de Liga Máster (ninguno auditado aún), comprobar que la estructura coincide con el motor
(`01-MOTOR-PES2021.md`) y repetir las pruebas clave en el juego.

## Auditoría de la carpeta (8 oct 2026, 20:45) — 🔎 OBSERVADO

**Estilo de parche: «clásico» de CPK** (lo contrario de ConmeGOL, que va casi todo por Sider/livecpk).

- `download\DpFileList.bin`: **34 CPK** (cabecera `64 / 0x22 / 64 / 0x2774`, mismo formato que ConmeGOL). Orden: 7 `dt80_*` de Konami →
  `SP_Adboards_1…8` → `SP_Faces_0…12` (**13 CPK de caras de 5–11 GB cada uno**) → `SP_Music` → `SP_Subs` (la base) →
  `db_st_7.0_byJGames` → 3 narradores `VIPER_*` (Bambino Pons, Fernando Solabarrieta, Miguel Simón).
- En `download\` hay CPK que **no** están en la lista (no se cargan): `SP_Subs_WC.cpk` (base del Mundial 2026) y
  `PredsKanas2021_NatAnthems.cpk` (himnos). Hay un `DpFileList Generator` dentro de la carpeta del juego.
- Dos Sider propios, cada uno con sus `SP LAUNCHER <letra de disco>.bat`: **«Sudamerican Project Sider»** (56 módulos, 107 `cpk.root`) y
  **«… WC26»** (Mundial, 66 líneas de módulo, 98 `cpk.root`).
- Mismos problemas que ya vimos en ConmeGOL (ver `../sider/AUDITORIA-SIDER.md`): `camera.lua` + `DynamicWideCam.lua` juntos (pisan la misma
  instrucción), `GFX_lod.lua`, `scoreboard-hexx`. Además: **módulos cargados dos veces** (`MenuServer` en el normal; `env`, `etrace`,
  `camera` y `netblock` en el WC26), módulos de prueba activos (`jittest`, `zlibtest`, `etrace`) y **`netblock.lua`** (bloquea la red del
  juego: incompatible con cualquier online tipo PESBUL).
- Créditos (`LEER.txt`): más de 30 colaboradores de la comunidad (facemakers, Olmos Jr 23, Afandix…), Discord oficial.
- Mismo `PES2021.exe` que la carpeta de ConmeGOL (458.806.784 B, 16/4/2023, FileVersion 1.1.0.0).

**Lección para Phoenix:** CPK gigantes de caras dan un parche pesado y lento de actualizar; ConmeGOL demuestra que casi todo puede ir
suelto por livecpk. Lo ideal: base de datos en un `.cpk` pequeño + resto por Sider, con **un solo** `sider.ini` limpio (sin duplicados,
sin módulos de prueba, sin pares que se pisen).
