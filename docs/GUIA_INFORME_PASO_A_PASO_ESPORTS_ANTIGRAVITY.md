# Guía Maestra e Informe Paso a Paso: Suite Esports, Modo Street y Funciones Virales de Phoenix Link (v0.3.58)

> **Autor:** Antigravity (Gemini) · Google DeepMind  
> **Fecha:** 29 de Octubre de 2026  
> **Proyecto:** Phoenix Link & Phoenix Evolution  
> **Destinatario:** Frank / Fralex (`Fralexito`)  
> **Rama Git:** `smash-soda-fork@plan-ux` y `phoenixevolution@borrador`  
> **Objetivo:** Documento pedagógico, técnico y exhaustivo para que cualquier Inteligencia Artificial (Claude, GPT, Gemini) o desarrollador humano pueda comprender el diseño, replicar la implementación y mantener o ampliar estas funciones sin pérdida de contexto.

---

## 1. Contexto y Filosofía del Ecosistema

Phoenix Link es la aplicación compañera y gestora de partidas multijugador para PES 2021 en Windows. Conecta la experiencia de sala de Parsec con la simulación en tiempo real de PES mediante Sider y ViGEmBus.

Este conjunto de funciones lleva a Phoenix Link a su máxima expresión como **Suite Integral de Esports, Entretenimiento Comunitario y Modo Street (Fútbol Callejero)**:
1. **Interactividad para espectadores:** Predicciones estilo Twitch/Kick con puntos simbólicos de sala.
2. **Protección y telemetría de red:** Radar de latencia y alerta proactiva de laggers para evitar cortes en el partido.
3. **Automatización competitiva:** Copas relámpago con bracket de 4 jugadores y rotación de mandos en 1 clic; modo Rey de la Pista autónomo.
4. **Moderación y producción:** Matriz global de atajos de teclado `Ctrl + Shift`, Escudo anti-doxxing y Overlay transparente dedicado para OBS y TikTok Studio.
5. **Funciones Virales de Comunidad:**
   - 🎙️ Locutor / Comentarista de voz IA (Web Speech API nativo sin coste).
   - ⚖️ El VAR de la Sala con votación relámpago de 15 segundos y sirena.
   - 🎡 La Rueda de Penitencias con animación física Canvas para el perdedor.
   - 🎖️ Sistema RPG de Rangos, Niveles (XP) y Títulos Legendarios.
   - 🤝 Modo «Pan y Queso» / Draft Callejero por turnos con asignación automática a mandos.
6. **Suite Modo Street (Losa y Cancha Chica):**
   - 🎛️ Panel de Físicas Callejeras para `phoenix_core.json` (Presets: *La Jaula de Hierro*, *Futsal Técnico de Losa*, *Frenesí Arcade*).
   - ⚡ Regla Callejera: *«A 2 Goles se Queda»* / *«Gol de Oro»* con pitazo final y rotación instantánea.
   - 🥜 Botón y Contador *«¡QUÉ CAÑO! / ¡HUACHA!»* con banner animado gigante en overlay y chat.

---

## 2. Arquitectura de Estado y Comunicación

Para mantener la aplicación ultrarrápida y evitar tocar innecesariamente el núcleo C++ compilado, se implementó un patrón de **Estado Reactivo Desacoplado**:
- **C++ (Motor):** Mantiene la verdad sobre los mandos físicos/virtuales (ViGEm), estado de la sala Parsec y la IPC con PES/Sider (`puente.js`).
- **Nivel UI (JavaScript ES Modules):** Los módulos competitivos (`predicciones.js`, `bracket.js`, `reydelapista.js`, `street.js`, `rpgsala.js`, `var.js`, `penitencias.js`, `locutor.js`) almacenan su estado persistente en `localStorage` bajo el espacio de nombres `phoenix.*`.
- **Sincronización Multiventana / Multiwidget:** Cada mutación de estado emite un evento `CustomEvent` en `window` (ej. `phoenix:prediccion`, `phoenix:street_evento`, `phoenix:var_evento`), permitiendo que cualquier componente (la barra de Puestos, el Marcador de Partido o el Overlay externo) se actualice a 60 FPS de forma instantánea.
- **Audio y Voz Nativo:** Uso exclusivo de las APIs estándar del navegador de Windows (`window.speechSynthesis` y `AudioContext` de Web Audio API), generando silbatos, rugidos de grada, sirenas y choques de reja sintetizados en tiempo real sin requerir archivos `.wav` o `.mp3` externos pesados.

---

## 3. Desglose Módulo por Módulo (Paso a Paso)

### Módulo 1: Sistema de Predicciones y Votaciones en Vivo (`predicciones.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/predicciones.js`
- **Mecánica técnica:** Votación a 3 vías (`Equipo A`, `Empate`, `Equipo B`) con cálculo porcentual dinámico y barra animada. Hook automático en `partido.js` (`finalizar()`) para resolver y premiar con +100 puntos de sala a los acertantes anunciándolo en el chat.

### Módulo 2: Radar de Conexión & Alertas de Lagger (`radarconexion.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/radarconexion.js`
- **Mecánica técnica:** Evalúa ping y pérdida de paquetes clasificando en Óptimo ($\le 45$ ms), Medio ($46-85$ ms) y Crítico ($>85$ ms / $>3\%$). Renderiza `BadgeConexion` en cada fila de puesto y `AlertaLaggerSala` en cabecera.

### Módulo 3: Bracket / Cuadro de Torneo Relámpago (`bracket.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/bracket.js`
- **Mecánica técnica:** Genera cuadro eliminatorio de 4 jugadores (Semis y Gran Final) y ofrece botón `⚡ CARGAR CRUCE EN MANDOS` para sentar a los 2 rivales en el Puesto 1 y Puesto 2 con un solo clic.

### Módulo 4: Overlay Transparente para Streamers (`overlay.html`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/overlay.html`
- **Mecánica técnica:** Fondo 100% transparente para Browser Source en OBS Studio / TikTok Studio. Incluye marcador con tipografía deportiva neón, lower-third de predicciones y banner animado de *«¡QUÉ CAÑO!»*.

### Módulo 5: Locutor / Comentarista IA & Soundboard Sintetizado (`locutor.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/locutor.js`
- **Mecánica técnica:** Utiliza `SpeechSynthesisUtterance` en español con tono enérgico para relatar goles y eventos. Incorpora sintetizador Web Audio API para silbatos de árbitro, ovación de gol con ruido blanco filtrado, rebote metálico de reja y sirena de VAR.

### Módulo 6: El VAR de la Sala (`var.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/var.js`
- **Mecánica técnica:** Despliega modal de pantalla completa con sirena de emergencia, cuenta regresiva de 15 segundos y votación relámpago (`Penal`, `Piscinazo`, `Roja`) para que la sala dicte sentencia en jugadas polémicas.

### Módulo 7: La Ruleta del Castigo / Penitencias (`penitencias.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/penitencias.js`
- **Mecánica técnica:** Ruleta Canvas HTML5 interactiva con física de desaceleración cúbica para sortear castigos cómicos al perdedor (cambiar foto en Discord, cantar el himno, jugar con una mano). Permite añadir penitencias personalizadas y anuncia el castigo en el chat.

### Módulo 8: Sistema RPG de Sala (`rpgsala.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js`
- **Mecánica técnica:** Progresión de XP, niveles (Novato, Titular, Capitán, Crack, Leyenda) e insignias (*El Muro*, *El Matador*, *El Destronador*, *Rey del Caño*). Muestra `BadgeNivelRPG` en cada fila de puesto junto al nombre del jugador.

### Módulo 9: Modo «Pan y Queso» / Draft Callejero (`draft.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/draft.js`
- **Mecánica técnica:** Dos capitanes eligen por turnos a los miembros de la sala. Al completar las plantillas, sienta automáticamente a todos los jugadores en los mandos del Equipo A y Equipo B vía `mandos.formacion` y `mandos.asignar`.

### Módulo 10: Suite Modo Street & Físicas Callejeras (`street.js`)
- **Ruta de código:** `SmashSoda/phoenix/web/ui/js/vistas/street.js`
- **Mecánica técnica:** Barra de control táctico para la cancha chica de $15 \times 10$:
  - Presets de físicas para `phoenix_core.json`: *La Jaula de Hierro* (rebote 90%), *Futsal Técnico* (rebote 40%) y *Frenesí Arcade* (rebote 100%).
  - Regla rápida: *«A 2 Goles se Queda»* / *«Gol de Oro»*, pita el final en cuanto se alcanza el objetivo.
  - Botón *«¡QUÉ CAÑO! (+1)»*: Efecto de sonido, relato del locutor y banner de humillación en pantalla.

---

## 4. Procedimiento de Verificación y Despliegue

```powershell
# 1. Comprobación sintáctica obligatoria con Node.js
node --check 'SmashSoda/phoenix/web/ui/js/vistas/locutor.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/var.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/penitencias.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/draft.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/street.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/puestos.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/partido.js'
node --check 'SmashSoda/phoenix/web/ui/js/app.js'

# 2. Despliegue en caliente a la carpeta de Release (sin recompilar C++)
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/locutor.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/var.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/penitencias.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/draft.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/street.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/puestos.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/partido.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/overlay.html' 'x64/Release/ui/' -Force
```

---

## 5. Registro de Archivos y Trazabilidad

| Archivo | Tipo | Descripción |
|---|---|---|
| `SmashSoda/phoenix/web/ui/js/vistas/locutor.js` | Nuevo | Locutor con voz IA en español y soundboard Web Audio API sintetizado. |
| `SmashSoda/phoenix/web/ui/js/vistas/var.js` | Nuevo | Sistema de revisión dramática de VAR con votación de 15 segundos. |
| `SmashSoda/phoenix/web/ui/js/vistas/penitencias.js` | Nuevo | Ruleta de penitencias animada Canvas para el perdedor del partido. |
| `SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js` | Nuevo | Sistema RPG de niveles, XP, insignias y títulos de sala. |
| `SmashSoda/phoenix/web/ui/js/vistas/draft.js` | Nuevo | Modo Pan y Queso / Draft callejero de capitanes por turnos. |
| `SmashSoda/phoenix/web/ui/js/vistas/street.js` | Nuevo | Panel de físicas Modo Street, regla a 2 goles y contador de caños. |
| `SmashSoda/phoenix/web/ui/overlay.html` | Modificado | HUD superior, lower-third de predicciones y banner de caño/huacha. |
| `SmashSoda/phoenix/web/ui/js/vistas/puestos.js` | Modificado | Integración de botones Locutor, VAR, Draft, Castigo, badges RPG y Barra Street. |
| `SmashSoda/phoenix/web/ui/js/vistas/partido.js` | Modificado | Relato y ovación en goles, silbato final, progreso RPG y regla a 2 goles. |

---
*Documento sellado y registrado por **Antigravity (Gemini)** para el ecosistema Phoenix.*
