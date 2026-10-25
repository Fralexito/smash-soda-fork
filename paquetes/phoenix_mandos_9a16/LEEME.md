# Phoenix · Mandos 9–16 para PES 2021

Módulo de Sider que deja jugar **hasta 16 mandos a la vez** en un mismo PC (8 contra 8) en PES 2021. Funciona **sin Phoenix Link**: sirve cualquier mando que Windows vea por DirectInput (físico o virtual).

Versión del módulo: **0.9.58** (25-oct-2026).

## Qué hace

- **Pantalla de elegir lado con 16 mandos.** Es una réplica de la de PES que se pone encima. PES sigue manejando los mandos 1–8, y los 9–16 se mueven con su palanca o cruceta. Muestra los nombres de «Datos personales» igual que la pantalla real, también el que cambia el mando 1.
- **En el partido**, los mandos 9–16 tienen jugador, flecha de color propia y lo **mueven** como cualquier mando. Sobre su jugador sale lo que diga su «Tipo de cursor» (Datos personales, Nombre de jugador, Mando n.º o nada) y la **barra de potencia** de pases y tiros, igual que en los mandos 1–8. Un usuario sin nombre sale como «Usuario N».
- **Menús de los mandos 9–16** (menús de PES con foco para 16): elegir equipo, estrategia y pausa. Si un equipo solo tiene mandos 9+, uno de ellos usa en los menús un número libre del 1 al 8 («número prestado») y en el saque vuelve al suyo. En la **pausa**, Local/Visitante y Estrategia también funcionan.
- **Botones en la pantalla de lados:** ○ vuelve atrás, **L2/R2** elige el usuario de «Datos personales» (salta los que ya usa otro mando, como PES) y **□** abre «Editar Datos personales» de SU usuario, también en la pausa.
- **Capitán:** en la pantalla de lados, el primero que pulsa **L3** es capitán de su equipo y aparece «CAP» junto a su nombre. Con L3 otra vez lo deja. El anfitrión elige con **F5** (Local) y **F6** (Visitante). El capitán es quien elige el equipo y la estrategia.

## Requisitos

1. **PES 2021** con el ejecutable de la actualización 1.10 (el de ConmeGOL / PATCH10100). El módulo cambia código en direcciones fijas y **comprueba los bytes originales antes de escribir**. Si tu exe es otro, no toca nada y lo anota en el registro.
2. **Sider** para PES 2021 con soporte Lua y LuaJIT FFI (`luajit.ext.enabled = 1`; lo pone el instalador).
3. **Mandos 9–16:** mandos que Windows vea por **DirectInput** y que **no** estén en las 8 casillas de `settings.dat` de PES. Pueden ser:
   - mandos físicos (USB o Bluetooth);
   - mandos virtuales con **ViGEmBus** (como hace Phoenix Link; ViGEm admite hasta 16 a la vez);
   - con Phoenix Link, el módulo además lee `Documentos\KONAMI\eFootball PES 2021 SEASON UPDATE\phoenix_mandos_extra.dat` para que el puesto 9 de Link sea el mando 9 de PES, y así sucesivamente.

## Instalar

1. **Cierra PES.**
2. Clic derecho en `INSTALAR.ps1` › **Ejecutar con PowerShell**, y escribe la carpeta de Sider (la que tiene `sider.ini`). O desde una consola:
   ```
   powershell -ExecutionPolicy Bypass -File INSTALAR.ps1 -Sider "D:\ruta\a\SiderAddons"
   ```
3. El instalador:
   - copia `modules\phoenix_mandos_fase1.lua`, `content\phoenix\lados_replica\` (dibujo de la pantalla) y `livecpk\PhoenixMandos\` (flujo de pausa);
   - guarda una copia de `sider.ini` (`sider.ini.antes-phoenix-mandos-FECHA`) y lo ajusta así:
     - `luajit.ext.enabled = 1`, `overlay.enabled = 1` y `overlay.image-alpha-max = 1.0`;
     - `cpk.root = ".\livecpk\PhoenixMandos"` como **primera** carpeta livecpk;
     - `lua.module = "phoenix_mandos_fase1.lua"` como **último** módulo.
4. Abre PES con Sider. Para comprobar que cargó, mira `SiderAddons\content\phoenix\mandos\fase1.txt`: tiene que empezar con `# fase1 v0.9.58`.

### Instalar a mano (sin el script)

Copia la carpeta `SiderAddons` del paquete encima de la tuya y edita `sider.ini` como en el punto 3.

## Desinstalar

1. Cierra PES.
2. Devuelve la copia `sider.ini.antes-phoenix-mandos-…` como `sider.ini`, o borra las líneas `lua.module = "phoenix_mandos_fase1.lua"` y `cpk.root = ".\livecpk\PhoenixMandos"`.
3. Si quieres, borra `modules\phoenix_mandos_fase1.lua`, `content\phoenix\lados_replica\` y `livecpk\PhoenixMandos\`.

El módulo **no cambia ningún archivo del juego ni tus guardados**. Todos sus cambios son en memoria y se pierden al cerrar PES.

## Cosas a tener en cuenta

- El módulo debe ser el **último** de `sider.ini`. La pantalla Phoenix se abre sola usando la tecla «módulo anterior» de la ventana de Sider (`overlay.vkey.prev-module`).
- Con más de 8 mandos, **↑/↓ del teclado** desplazan la tabla de la pantalla de lados.
- Una persona no debe ocupar dos puestos a la vez (por ejemplo, el mando 1 de PES y un puesto 9+ de Link), porque se moverían dos filas.
- Registro de todo lo que hace: `content\phoenix\mandos\fase1.txt`. Los errores de Lua salen en `sider.log`.

## Lo que todavía no funciona

- La pantalla de lados de 16 es una réplica encima de la de PES; hacerla **nativa** (16 filas dentro de PES) está pendiente.
- Los botones de los mandos 9+ se leen con la distribución tipo Xbox de DirectInput (✕ = 0, ○ = 1, □ = 2, L3 = 8, gatillos por eje Z o botones 6/7).

## Para quien quiera entenderlo o retomarlo

La historia completa, con direcciones, errores e intentos fallidos, está en el libro del proyecto: `PRUEBA_CANCHA\informes\LIBRO_PERSONAL\cap_60_j_mas_de_8_mandos.md` y `cap_60_k_estrategia_mas_mandos.md`.
