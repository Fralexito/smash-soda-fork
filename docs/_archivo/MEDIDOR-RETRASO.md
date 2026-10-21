# Medidor de retraso del mando (Link 0.2.16)

**Para qué sirve:** cuando alguien dice «tengo lag» o «el mando me va tarde», saber **quién** va con retraso y **por qué** (su ping, su conexión que corta, el vídeo…), sin adivinar.

**Estado (11 oct 2026):** compila, pruebas automáticas OK (`pruebas-link/retraso_test.cpp`) y probado en el simulador. **Falta verlo con un invitado real** (paso 5).

---

## 1. Dónde se ve

- **Sala › Conexión:** cada invitado tiene su tarjeta de siempre (ping, gráfica) y debajo un bloque nuevo **RETRASO DEL MANDO**:
  - el **retraso estimado** en ms con su semáforo (VA BIEN / SE NOTA / CON RETRASO) y la causa principal;
  - **VÍDEO**: lo que tarda tu PC en codificar + la suya en decodificar;
  - **PÉRDIDA**: % de paquetes de vídeo que Parsec tuvo que reenviar (últimos ~10 s);
  - **JITTER MANDO**: cuánto varía el tiempo entre mensajes seguidos de su mando;
  - **SALTOS/MIN**: veces que su mando se quedó retenido y llegó todo de golpe;
  - abajo, en pequeño: cada cuántos ms llega su mando (media y p95) y cuántos mensajes por segundo.
  - Arriba, **¿Cómo se calcula?** lo explica todo dentro de Link.
- **Mandos › Puestos:** junto al ping de cada jugador sentado, un punto de color y «~74». Pasa el ratón por encima para ver el detalle.
- **Chat de la sala (solo durante un partido):** si alguien sigue **en rojo 10 s seguidos**, el bot avisa: «Aviso de retraso: Lucho va con retraso (~180 ms, ping alto)…». Como mucho una vez cada 3 min por jugador y una cada 30 s en total.

## 2. Cómo se calcula

**Retraso estimado = ping (ida y vuelta) + vídeo (codificar + decodificar) + jitter de su mando.**

| Semáforo | Retraso estimado | o pérdida | o saltos |
|---|---|---|---|
| Verde (va bien) | ≤ 80 ms | < 2 % | < 6 por minuto |
| Ámbar (se nota) | ≤ 130 ms | ≥ 2 % | ≥ 6 por minuto |
| Rojo (con retraso) | más de 130 ms | ≥ 5 % | ≥ 20 por minuto |

Manda la peor de las tres reglas.

**De dónde salen los datos del mando:** Link apunta la hora exacta a la que llega a tu PC cada mensaje del mando de cada invitado (antes de bloqueos y mapeos). Mientras lo mueve, los mensajes llegan a ritmo fijo (por ejemplo, cada 8 ms). Si su conexión va mal, ese ritmo se rompe:
- **Jitter:** los intervalos varían mucho entre sí.
- **Salto:** un hueco (≥ 40 ms y ≥ 3 veces lo normal) seguido de **varios mensajes de golpe**. Es la huella de la red reteniendo sus pulsaciones y soltándolas juntas. Una pausa normal del jugador **no** cuenta (después de una pausa llega un mensaje, no una ráfaga).
- Si no toca el mando, no hay datos del mando (sale «se medirá cuando lo mueva»).

**Coste:** se mide en el hilo de entrada con arrays fijos (ventana de los últimos 128 intervalos y saltos del último minuto), sin bloqueos ni memoria nueva por mensaje. La interfaz lee un resumen una vez por segundo.

## 3. Lo que NO se puede medir

El retraso **dentro de la casa del invitado**: su mando (sobre todo por Bluetooth), su PC y su pantalla o tele (con el «modo juego» apagado, +50 ms o más). Si todo le sale verde y aún nota lag, el problema suele estar ahí: que pruebe con cable y con el modo juego de la tele.

## 4. Qué hacer según la causa

| Causa | Qué suele ser | Qué probar |
|---|---|---|
| ping alto | Está lejos o su internet es lento | Cable en vez de WiFi; cerrar descargas |
| mando a saltos | WiFi saturado o cortes cortos | Cable o WiFi de 5 GHz; acercarse al router |
| pierde paquetes | Conexión inestable | Cable; en Sala › Opciones bajar los Mbps |
| vídeo lento | Su PC (o la tuya) tarda en (de)codificar | Que cierre programas; en su Parsec, decodificador por hardware |
| mando irregular | Su mando o su conexión envían a trompicones | Cable para el mando y para internet |

## 5. Prueba con un invitado real

1. Abre la sala y que entre un amigo con mando.
2. **Sala › Conexión:** que mueva el stick sin parar 5 s. Debe salir «Llegada del mando: cada X ms… mensajes/s» y el semáforo.
3. Si puede, que se pase al WiFi lejos del router (o que alguien en su casa descargue algo): deben subir el jitter o los saltos.
4. **Mandos › Puestos:** comprueba el punto y «~N» junto a su ping.
5. Para el aviso del chat: con un partido empezado, alguien en rojo 10 s seguidos.

## 6. Archivos

- `SmashSoda/phoenix/core/MedidorRetraso.*` — lógica pura (ventana deslizante, evaluación, pérdida, antispam).
- `SmashSoda/phoenix/host/RetrasoMando.*` — enganche con el hilo de entrada (`Hosting::pollInputs`).
- `SmashSoda/phoenix/web/RetrasoWeb.cpp` — 1 vez por segundo junta todo, estado `retraso` y aviso del chat.
- `SmashSoda/phoenix/web/ui/js/vistas/retraso.js` — bloque de Conexión, leyenda e indicador de Puestos.
- `pruebas-link/retraso_test.cpp` — pruebas.
