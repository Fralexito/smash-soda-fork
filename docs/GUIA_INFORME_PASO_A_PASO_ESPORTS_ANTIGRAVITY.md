# GUÍA E INFORME TÉCNICO PASO A PASO · SUITE ESPORTS, MODO STREET Y AUTOMATIZACIÓN EN VIVO (PHOENIX LINK v0.3.58)

> **Autor:** Antigravity (Gemini) · Google DeepMind  
> **Destinatarios:** Frank (`Fralexito`), desarrolladores, inteligencias artificiales futuras y comunidad de PES 2021 / Phoenix Link.  
> **Fecha:** 29 de Octubre de 2026  
> **Ramas de Trabajo:** `plan-ux` (`SmashSoda/phoenix`) y `borrador` (`C:\dev\pes-liga-automatica`).  
> **Objetivo:** Guía exhaustiva, reproducible y de nivel profesional para comprender, auditar, replicar y extender la suite completa de Esports, Moderación Avanzada, Modo Street Callejero, Locución Sintetizada, Sincronización en Caliente de Físicas Sider y Webhooks de Discord en Phoenix Link.

---

## 1. Resumen Ejecutivo de la Arquitectura

Phoenix Link es el orquestador competitivo para PES 2021 sobre Parsec. Su interfaz web moderna está desarrollada sobre componentes reactivos ultraligeros (`htm` + `preact`) servidos sin empaquetadores lentos. 

Para lograr dinamismo total y máxima diversión comunitaria sin requerir recompilar el binario C++ (`PhoenixLink.exe`), la arquitectura implementa:
1. **Locución y Audio Sintetizado en Vivo:** Mediante `window.speechSynthesis` (voz nativa Windows en español) y `AudioContext` (Web Audio API) para generar silbatos, ovaciones con ruido blanco filtrado, rebotes metálicos y sirenas.
2. **Juicio Popular y VAR en Vivo:** Modal interactivo con cuenta regresiva de 15 segundos y cálculo dinámico de porcentajes de voto para jugadas polémicas.
3. **Ruleta de Penitencias:** Animación con física de desaceleración cúbica en HTML5 Canvas con castigos divertidos para los perdedores.
4. **RPG de Sala y Niveles:** Cálculo de XP, insignias dinámicas (*El Muro*, *El Matador*, *Rey del Caño*) y rangos de sala mostrados en cada mando.
5. **Draft Callejero «Pan y Queso»:** Selección por turnos entre dos capitanes con asignación instantánea a los asientos de mandos.
6. **Modo Street Callejero ($15 \times 10$):** Panel de control táctico con presets de físicas para Sider (`phoenix_core.json`), regla de finalización rápida (*A 2 Goles / Gol de Oro*) y botón de humillación *¡QUÉ CAÑO!* con banner animado para streaming.
7. **Sincronización en Caliente de Físicas Sider:** Micro-demonio Node.js (`tools/sync-sider-street.js`) que escucha peticiones en `http://127.0.0.1:38888/sync-street` y actualiza directamente `D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\phoenix_core.json` en tiempo real.
8. **Discord Webhook en Vivo:** Módulo (`discord.js`) configurado desde Ajustes que envía embeds automáticos de partidos finalizados, veredictos de VAR y penitencias al servidor de Discord.

---

## 2. Inventario de Componentes y Módulos Desarrollados

### 2.1. Locutor IA y Soundboard Sintetizado (`locutor.js`)
- **Voz en Vivo:** Función `narrar(texto, { prioridad, velocidad, tono })`. Selecciona la mejor voz en español del sistema operativo y evita solapamientos molestos.
- **Efectos de Sonido Web Audio API:**
  - `sonarSilbato({ largo })`: Oscilador senoidal dual (2400 Hz y 2460 Hz) con modulación de vibrato de referí.
  - `sonarGol()`: Explosión acústica combinando oscilador de 120 Hz descendente con buffer de ruido blanco filtrado a 800 Hz (sensación de estadio lleno).
  - `sonarReboteMuro()`: Pulso percusivo de 180 Hz a 60 Hz emulando el golpe de pelota contra valla de alambre/hierro de fútbol callejero.
  - `sonarSirenaVAR()`: Barrido de frecuencia senoidal de 400 Hz a 900 Hz simulando alarma de revisión técnica.

### 2.2. Sala de Revisión VAR (`var.js`)
- **Activación:** Se lanza con el botón `🚨 VAR` en la barra de moderación de puestos.
- **Temporizador:** 15 segundos con animación de alerta pulsante.
- **Opciones de Voto:** `⚽ Penal`, `🏊 Piscinazo / Siga`, `🟥 Roja Directa`.
- **Integración:** Al concluir el tiempo, relata el veredicto con la voz IA, lo anuncia en el chat de la sala y lo transmite por webhook a Discord.

### 2.3. Ruleta de Penitencias del Perdedor (`penitencias.js`)
- **Motor Gráfico:** Canvas 2D interactivo con renderizado de 8 gajos multicolor, sombras y puntero superior.
- **Física:** Desaceleración cúbica $1 - (1 - t)^3$ a lo largo de 4.2 segundos.
- **Efecto de Audio:** Clics de giro y silbato de parada.
- **Personalización:** Lista editable persistida en `localStorage` con botón para añadir castigos en caliente.

### 2.4. Progresión RPG de Sala (`rpgsala.js`)
- **Fórmula de Nivel:** Nivel calculado como $\lfloor \sqrt{\text{XP} / 100} \rfloor + 1$.
- **Títulos según Nivel:**
  - Nivel 1–4: *Novato de Sala*
  - Nivel 5–9: *Titular Frecuente*
  - Nivel 10–19: *Capitán de Barrio*
  - Nivel 20–29: *Crack Consagrado*
  - Nivel 30+: *Leyenda Viviente*
- **Insignias Automáticas:**
  - 🧱 *El Muro* (10+ vallas invictas)
  - ⚽ *El Matador* (25+ goles)
  - 🥜 *Rey del Caño* (10+ huachas callejeras)
- **Componente Visual:** `BadgeNivelRPG` insertado en cada fila de jugador ocupada en `puestos.js`.

### 2.5. Draft Callejero «Pan y Queso» (`draft.js`)
- Selección interactiva por turnos entre dos capitanes a partir de los espectadores presentes en la sala.
- Asignación en 1 clic: sienta automáticamente a los elegidos en los mandos del Equipo A y Equipo B llamando la acción nativa `mandos.asignar`.

### 2.6. Panel Modo Street y Físicas Callejeras (`street.js`)
- **Presets de Físicas Sider:**
  - *🧱 La Jaula de Hierro:* Rebote al 90%, fricción 0.98, velocidad de pase x1.4, disparo bomba activado.
  - *⚽ Futsal Técnico de Losa:* Rebote al 40%, fricción 0.94, velocidad de pase x1.1, control fino.
  - *🌪️ Frenesí Arcade:* Rebote al 100%, súper velocidad y rebotes infinitos.
- **Regla Rápida:** Finalización rápida con selector de *90 Minutos*, *Gol de Oro*, *A 2 Goles* o *A 3 Goles*.
- **Contador de Humillación:** Botón `🥜 ¡QUÉ CAÑO!` que reproduce el impacto metálico, suma XP de filigrana, anuncia en el chat y detona el banner en OBS.

### 2.7. Sincronizador en Caliente de Físicas (`tools/sync-sider-street.js`)
- Micro-demonio local en Node.js que expone `http://127.0.0.1:38888/sync-street`.
- Modifica directamente en caliente el archivo:
  `D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\phoenix_core.json`
- Se ejecuta fácilmente mediante el lanzador `tools/INICIAR_SYNC_SIDER.bat`.

### 2.8. Notificador Discord Webhook (`js/servicios/discord.js`)
- Configurable desde `Ajustes › General › Webhook de Discord`.
- Transmite embeds con formato profesional a canales de Discord comunitarios para:
  - Finales de partido (goles, ganador, MVP y modalidad Street/Oficial).
  - Resoluciones de VAR con porcentaje exacto de votantes.
  - Penitencias dictaminadas por la ruleta para el perdedor.

---

## 3. Guía de Verificación y Replicación Paso a Paso

Para comprobar o replicar esta suite en cualquier entorno Windows con Node.js instalado:

```powershell
# 1. Comprobación sintáctica con Node.js
node --check 'SmashSoda/phoenix/web/ui/js/servicios/discord.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/street.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/penitencias.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/var.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/partido.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/ajustes.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/locutor.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/puestos.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/draft.js'
node --check 'SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js'
node --check 'tools/sync-sider-street.js'

# 2. Despliegue en caliente a Release (sin recompilar C++)
New-Item -ItemType Directory -Path 'x64/Release/ui/js/servicios' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/servicios/discord.js' 'x64/Release/ui/js/servicios/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/street.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/penitencias.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/var.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/partido.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/ajustes.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/locutor.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/puestos.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/draft.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js' 'x64/Release/ui/js/vistas/' -Force
Copy-Item 'SmashSoda/phoenix/web/ui/overlay.html' 'x64/Release/ui/' -Force
```

---

## 4. Registro y Trazabilidad de Archivos

| Archivo | Estado | Responsabilidad Técnica |
|---|---|---|
| `SmashSoda/phoenix/web/ui/js/servicios/discord.js` | **Nuevo** | Integración de Discord Webhooks con payloads embeds para partidos, VAR y penitencias. |
| `tools/sync-sider-street.js` | **Nuevo** | Micro-demonio HTTP local que modifica `phoenix_core.json` en tiempo real. |
| `tools/INICIAR_SYNC_SIDER.bat` | **Nuevo** | Lanzador en 1 clic del demonio de sincronización de físicas en Windows. |
| `SmashSoda/phoenix/web/ui/js/vistas/street.js` | **Actualizado** | Integración de hot-sync con Sider y botón directo de sincronización en vivo. |
| `SmashSoda/phoenix/web/ui/js/vistas/penitencias.js` | **Actualizado** | Envío automático de castigos de la ruleta hacia Discord. |
| `SmashSoda/phoenix/web/ui/js/vistas/var.js` | **Actualizado** | Difusión de resoluciones de jugadas polémicas hacia Discord. |
| `SmashSoda/phoenix/web/ui/js/vistas/partido.js` | **Actualizado** | Notificación de finales de partido y resultados a Discord. |
| `SmashSoda/phoenix/web/ui/js/vistas/ajustes.js` | **Actualizado** | Campo interactivo para URL de Webhook de Discord y botón de prueba. |
| `SmashSoda/phoenix/web/ui/js/vistas/locutor.js` | **Activo** | Locución en español y efectos de sonido de estadio y calle. |
| `SmashSoda/phoenix/web/ui/js/vistas/draft.js` | **Activo** | Draft por turnos con sentado en mandos. |
| `SmashSoda/phoenix/web/ui/js/vistas/rpgsala.js` | **Activo** | Progresión por niveles, títulos e insignias. |
| `SmashSoda/phoenix/web/ui/overlay.html` | **Activo** | HUD transparente para OBS con predicciones y banner de caño. |

---
*Documento sellado y registrado por **Antigravity (Gemini) · Google DeepMind** para el ecosistema Phoenix.*
