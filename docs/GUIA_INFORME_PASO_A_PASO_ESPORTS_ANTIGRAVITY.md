# Guía Maestra e Informe Paso a Paso: Suite Esports, Moderación y Telemetría en Vivo de Phoenix Link (v0.3.58)

> **Autor:** Antigravity (Gemini) · Google DeepMind  
> **Fecha:** 29 de Octubre de 2026  
> **Proyecto:** Phoenix Link & Phoenix Evolution  
> **Destinatario:** Frank / Fralex (`Fralexito`)  
> **Rama Git:** `smash-soda-fork@plan-ux` y `phoenixevolution@borrador`  
> **Objetivo:** Documento pedagógico, técnico y exhaustivo para que cualquier Inteligencia Artificial (Claude, GPT, Gemini) o desarrollador humano pueda comprender el diseño, replicar la implementación y mantener o ampliar estas funciones sin pérdida de contexto.

---

## 1. Contexto y Filosofía del Rediseño

Phoenix Link es la aplicación compañera y gestora de partidas multijugador para PES 2021 en Windows. Conecta la experiencia de sala de Parsec con la simulación en tiempo real de PES mediante Sider y ViGEmBus.

Este conjunto de funciones transforma a Phoenix Link de un mero gestor de mandos a una **plataforma integral de retransmisión y competición de Esports**, dotándola de:
1. **Interactividad para espectadores** (Predicciones con puntos y barra animada estilo Twitch/Kick).
2. **Protección y telemetría de red** (Radar de latencia y alerta proactiva de laggers para evitar cortes en el partido).
3. **Automatización competitiva** (Copas relámpago con bracket de 4 jugadores y rotación de mandos en 1 clic; modo Rey de la Pista autónomo).
4. **Herramientas de moderación en caliente** (Matriz global de atajos de teclado `Ctrl + Shift`).
5. **Producción para streaming** (Escudo anti-doxxing y Overlay transparente dedicado para OBS y TikTok Studio).

---

## 2. Arquitectura de Estado y Comunicación

Para mantener la aplicación ultrarrápida y evitar tocar innecesariamente el núcleo C++ compilado, se implementó un patrón de **Estado Reactivo Desacoplado**:
- **C++ (Motor):** Mantiene la verdad sobre los mandos físicos/virtuales (ViGEm), estado de la sala Parsec y la IPC con PES/Sider (`puente.js`).
- **Nivel UI (JavaScript ES Modules):** Los módulos competitivos (`predicciones.js`, `bracket.js`, `reydelapista.js`) almacenan su estado persistente en `localStorage` bajo el espacio de nombres `phoenix.*`.
- **Sincronización Multiventana / Multiwidget:** Cada mutación de estado emite un evento `CustomEvent` en `window` (ej. `phoenix:prediccion`, `phoenix:bracket`), permitiendo que cualquier componente (la barra de Puestos, el Marcador de Partido o el Overlay externo) se actualice a 60 FPS de forma instantánea.

---

## 3. Desglose Módulo por Módulo (Paso a Paso)

### Módulo 1: Sistema de Predicciones y Votaciones en Vivo (`predicciones.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/predicciones.js`
- **¿Qué resuelve?:** Convierte a los usuarios en «Mirando» de simples observadores a participantes activos.
- **Mecánica técnica:**
  1. El host abre una predicción desde el botón `+ CREAR PREDICCIÓN` o desde el Partido.
  2. Define los dos equipos (Local y Visitante) o usa los nombres configurados en el partido.
  3. Los espectadores votan por `Equipo A`, `Empate` o `Equipo B`.
  4. La barra calcula porcentajes ponderados (`Math.round((votos / total) * 100)`) y renderiza tres segmentos con colores tácticos (`var(--acc)`, gris y púrpura `#b300ff`).
  5. Cuando arranca el partido, el host puede cerrar las votaciones (`🔒 CERRAR VOTACIONES`).
  6. **Hook Automático con Partido:** Al finalizar el partido (`partido.js` -> `finalizar()`), el sistema detecta si ganó `a`, `b` o hubo empate `x`, premia con +100 puntos de reputación a los acertantes y envía un anuncio al chat de la sala (`accion("chat.enviar", ...)`).

### Módulo 2: Radar de Conexión & Alertas de Lagger (`radarconexion.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/radarconexion.js`
- **¿Qué resuelve?:** En PES 2021 online, si un jugador invitado tiene fluctuaciones de ping o pérdida de paquetes (jitter), toda la partida sufre micro-tirones.
- **Mecánica técnica:**
  1. La función `clasificarConexion(parsecId, m)` consulta `retrasoDe(parsecId, m)` y `m.red`.
  2. Rango de corte:
     - 🟢 **Óptimo:** Ping $\le 45$ ms y pérdida $< 1\%$.
     - 🟡 **Medio:** Ping entre $46$ y $85$ ms, o pérdida entre $1\%$ y $3\%$.
     - 🔴 **Crítico:** Ping $> 85$ ms o pérdida $> 3\%$.
  3. Componente `BadgeConexion`: Muestra una pastilla compacta con led brillante y tooltip detallado en cada fila de puesto de `puestos.js`.
  4. Componente `AlertaLaggerSala`: Banner superior en rojo que se activa únicamente si algún jugador sentado en mandos cae en estado crítico, avisando al host con el nombre y los milisegundos exactos.

### Módulo 3: Bracket / Cuadro de Torneo Relámpago (`bracket.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/bracket.js`
- **¿Qué resuelve?:** Elimina la fricción de organizar copas express en sala.
- **Mecánica técnica:**
  1. `iniciarBracket4({ jugadores, m })`: Toma a los miembros de la sala (anfitrión + invitados), los baraja al azar y crea el cuadro: Semifinal 1, Semifinal 2 y Gran Final. Soporta pases directos (`BYE`) si hay menos de 4 personas.
  2. Botón `⚡ CARGAR CRUCE EN MANDOS`: Mediante `mandos.tomar` (para el host) y `mandos.asignar` (para invitados), sienta directamente a los dos contendientes en el Puesto 1 (Equipo A) y Puesto 2 (Equipo B) sin arrastrar manualmente.
  3. Registro del ganador con botón `👑 Ganó`, que avanza automáticamente al clasificado a la casilla de la Gran Final y lo anuncia con fanfarria en el chat de la sala.

### Módulo 4: Overlay Transparente para OBS / TikTok Studio (`overlay.html`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/overlay.html`
- **¿Qué resuelve?:** Proporciona a streamers una fuente limpia para incrustar en OBS Studio como Browser Source sin necesidad de capturar toda la ventana de Phoenix Link.
- **Mecánica técnica:**
  1. Fondo 100% transparente (`background: transparent !important`).
  2. Marcador superior con tipografía `Chakra Petch`, reloj de partido, nombres y goles con sombra de brillo neón.
  3. Lower-third inferior de predicciones: Se despliega suavemente solo cuando hay una votación activa y muestra la barra de porcentajes en tiempo real.
  4. Sincronización reactiva mediante `window.addEventListener("storage", ...)` e intervalo pasivo de 1 segundo.

### Módulo 5: Matriz de Atajos Globales de Moderador (`atajos.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/atajos.js`
- **Atajos implementados:**
  - `Ctrl + Shift + M`: Silenciar / reactivar chat de sala.
  - `Ctrl + Shift + F`: Congelar / reactivar mandos con el Árbitro.
  - `Ctrl + Shift + E`: Todos los jugadores a espectadores (Mirando).
  - `Ctrl + Shift + I`: Invertir lados (Equipo A ⇄ Equipo B) para la vuelta.
  - `Ctrl + Shift + R`: Forzar rotación del Rey de la Pista.
  - `Ctrl + Shift + S`: Activar / desactivar Escudo Streamer Anti-Doxxing.
  - `Ctrl + Shift + K`: Abrir chuleta interactiva de atajos.

### Módulo 6: Modo Rey de la Pista Autónomo (`reydelapista.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/reydelapista.js`
- **Mecánica:** Al finalizar un partido, el ganador retiene sus puestos y suma victorias a su racha; el perdedor es movido a «Mirando» y los siguientes retadores de la cola entran de inmediato. Al alcanzar la racha máxima (2, 3 o 5 victorias), el rey se retira con honores invicto.

### Módulo 7: Ficha MVP y Lanzador de PES (`reportepartido.js`, `inicio.js`)
- **Ficha MVP:** Analiza los goleadores del partido y calcula la valoración del jugador más valioso. Permite copiar un acta oficial con formato Markdown y emojis con 1 clic para pegar en WhatsApp o Discord.
- **Lanzador de PES:** Monitorea en vivo si `PES2021.exe` y `sider.exe` están activos y ofrece el botón `▶ ARRANCAR JUEGO (SIDER + PES)`.

---

## 4. Procedimiento de Verificación y Despliegue

Cualquier agente o desarrollador que modifique la interfaz debe seguir esta secuencia estricta:

```powershell
# 1. Comprobación sintáctica obligatoria con Node.js
node --check 'SmashSoda/phoenix/web/ui/js/vistas/predicciones.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/radarconexion.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/bracket.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/puestos.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/partido.js'
node --check 'SmashSoda/phoenix/web/ui/js/app.js'

# 2. Despliegue en caliente a la carpeta de Release (sin recompilar C++)
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/predicciones.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/radarconexion.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/bracket.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/overlay.html' 'x64/Release/ui/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/puestos.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/partido.js' 'x64/Release/ui/js/vistas/' -Force
```

---

## 5. Registro de Archivos y Trazabilidad

| Archivo | Tipo | Descripción |
|---|---|---|
| `SmashSoda/phoenix/web/ui/js/vistas/predicciones.js` | Nuevo | Lógica de apuestas/votos con barra de porcentajes, puntos de sala y resolución de ganador. |
| `SmashSoda/phoenix/web/ui/js/vistas/radarconexion.js` | Nuevo | Clasificación de calidad de red, badge por puesto y alerta proactiva de laggers. |
| `SmashSoda/phoenix/web/ui/js/vistas/bracket.js` | Nuevo | Generación de llaves eliminatorias de copa relámpago con carga de cruces en 1 clic. |
| `SmashSoda/phoenix/web/ui/overlay.html` | Nuevo | Vista transparente independiente para fuentes de navegador de OBS Studio / TikTok Studio. |
| `SmashSoda/phoenix/web/ui/js/vistas/atajos.js` | Nuevo | Hook global de atajos de moderación `Ctrl + Shift` y modal de referencia. |
| `SmashSoda/phoenix/web/ui/js/vistas/reydelapista.js` | Nuevo | Modo competitivo autónomo «ganador se queda» con límite de racha. |
| `SmashSoda/phoenix/web/ui/js/vistas/reportepartido.js` | Nuevo | Ficha esport, cálculo de MVP del partido y generador de acta para WhatsApp/Discord. |
| `SmashSoda/phoenix/web/ui/js/vistas/puestos.js` | Modificado | Integración de botones Copa, Atajos, Radar de lag, Presets y barra Rey de la Pista. |
| `SmashSoda/phoenix/web/ui/js/vistas/partido.js` | Modificado | Inyección del Widget de Predicciones y resolución automática al pitar el final. |
| `SmashSoda/phoenix/web/ui/js/vistas/inicio.js` | Modificado | Dashboard dinámico de 3 estados y lanzador de PES/Sider. |
| `SmashSoda/phoenix/web/ui/js/app.js` | Modificado | Escudo Streamer anti-doxxing y atajos de cabecera. |

---
*Documento sellado y registrado por **Antigravity (Gemini)** para el ecosistema Phoenix.*
