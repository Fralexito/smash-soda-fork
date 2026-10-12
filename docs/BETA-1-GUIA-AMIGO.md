# Phoenix Link · beta 1 — Guía para instalar (una página)

Para quien va a **hostear** con su propia PC (PES 2021 + ConmeGOL + Parsec). Si solo entras a la sala de otro por Parsec, no necesitas nada de esto.

## Antes de empezar
- Windows 10 u 11 de 64 bits e internet. Con el instalador rápido hacen falta **menos de 1 GB**; con el instalador que compila, **12 GB**.
- PES 2021 con ConmeGOL **funcionando** (ábrelo una vez con su lanzador, como siempre) y una cuenta de Parsec.
- Los archivos que te pasa Fralex: `PhoenixLink-instalar-rapido.bat` (recomendado) y `PhoenixJuego-instalar.bat`. El `PhoenixLink-instalar.bat` compila Link en tu PC y solo sirve si el rápido no está disponible.
- Tiempo: **unos 3 minutos** con el instalador rápido (de 30 a 90 si compilas).

## Paso 1 · Instalar Link
1. Doble clic en `PhoenixLink-instalar-rapido.bat`. Si Windows pide permiso, pulsa **Sí**.
2. Lee el aviso, pulsa una tecla y elige dónde instalar (o cierra la ventanita para usar `C:\Program Files`).
3. **Espera.** Baja Link (17 MB) y solo los componentes que le falten a tu PC (runtime de Visual C++, ViGEmBus, WebView2). Si Windows pide reiniciar, reinicia y repite.
4. Termina con **INSTALACION TERMINADA**. Acepta el acceso directo del escritorio.

Si sale **NO SE PUDO TERMINAR**: tu juego no se tocó. Manda a Fralex una foto de la ventana o el archivo `PhoenixLink-instalador.log` (está en `%TEMP%`: pega eso en la barra del Explorador).

## Paso 2 · Poner Phoenix en tu juego
1. **Cierra PES 2021 por completo** (y Sider si queda abierto).
2. Abre **Phoenix Link** (el acceso directo) e inicia sesión en Parsec cuando lo pida.
3. Ve a **SYNC → Módulos del juego**.
   - Si dice que no conoce tu juego: pega la carpeta donde está `PES2021.exe` y pulsa **GUARDAR**.
4. Pulsa **INSTALAR** en `phoenix_referee.lua` y en `phoenix.lua`. Deben quedar en **INSTALADO**.
   Link guarda una copia de `sider.ini` con fecha antes de tocar nada.
5. Abre PES con su lanzador. Vuelve a **SYNC**: la tarjeta «Qué funciona en tu juego» debe mostrar todo en verde.

Sin Link (solo los módulos): doble clic en `PhoenixJuego-instalar.bat`, elige la carpeta del juego y espera **LISTO**.

## Qué esperar en esta beta
| Función | Estado |
|---|---|
| Abrir sala, mandos, puestos, equipos A/B | Listo (lo usa Fralex; con otra persona, esta sesión lo comprueba) |
| Avisos de la web dentro del juego | Listo |
| Marcador, goles, HUD y árbitro | Listo, a probar contigo |
| **Fichajes de Phoenix** | **Desactivados en el parche 27 (próximamente).** No tocan tus jugadores |

## Quitar Phoenix
- Del juego: Link → SYNC → Módulos del juego → **DESINSTALAR**. Sin Link: abre una consola en la carpeta del .bat y ejecuta `PhoenixJuego-instalar.bat desinstalar`. No borra nada: aparta los `.lua` y guarda copias de `sider.ini`.
- De Link: borra la carpeta `Phoenix Link` y el acceso directo. Tus ajustes quedan en `%APPDATA%\Trybuchet\Smash Soda`.

---

# Lista para la PRIMERA sesión de prueba (Fralex + amigo)

**Antes de abrir sala (5 min)**
- [ ] Link abre sin errores y dice **BETA 1** abajo a la izquierda.
- [ ] Sala › Conexión › Diagnóstico del PC: todo verde (WebView2, ViGEmBus, Parsec).
- [ ] SYNC › «Qué funciona en tu juego»: módulos al día; fichajes «desactivados (próximamente)» en el parche 27.
- [ ] Abre PES: en `SiderAddons\sider.log` aparece `[phoenix.lua]` sin «is NOT activated».

**Durante el partido**
- [ ] Entran los dos a la sala; suena/sale el aviso de entrada.
- [ ] Cada uno tiene su puesto y su mando funciona en el juego.
- [ ] El marcador y los goles aparecen en Link.
- [ ] Un aviso de la web sale dentro del juego (barra espaciadora → panel de Sider).
- [ ] Mira si hay retraso en los mandos (medidor de Link).

**Si algo falla, anota**
1. Qué hacías y qué pasó (una frase).
2. La hora aproximada.
3. Una foto de la pantalla de Link.
4. El informe: **Sala › Conexión › Diagnóstico del PC › COPIAR INFORME** y pégalo en el chat.
   (Lleva versiones, parche, módulos y el final de `sider.log`. Tapa tu nombre de usuario de Windows y no lleva contraseñas ni claves; trae una sección «Qué pasa y qué hacer» en lenguaje claro.)

**Reglas de oro:** no abras PES mientras instalas módulos; si dudas, no cambies nada y manda el informe.
