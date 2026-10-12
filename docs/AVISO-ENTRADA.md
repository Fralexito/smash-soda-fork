# Aviso cuando alguien entra a la sala (desde 0.2.39)

Para enterarte de que alguien se unió aunque no estés mirando Link (por ejemplo, con PES abierto).

## Qué hace
Cuando un invitado entra, Link puede avisarte de tres formas, y cada una se enciende o apaga por separado:
1. **Sonido:** una campanita corta que genera Link (no necesita archivos). Si guardas un archivo llamado **`entrada.wav`** en la carpeta **`SFX`** que está junto a `PhoenixLink.exe`, suena ese en su lugar (tu sonido propio).
2. **Notificación de Windows:** un globo en la esquina con «Fulano entró a la sala» (~6 s).
3. **Aviso dentro del juego:** sale en el overlay de PES, si el puente de Phoenix está instalado y los «avisos en el juego» están encendidos (Sync › Módulos del juego).

## Dónde se configura
**Ajustes › Sonidos › «Cuando alguien entra»**: interruptor general, y uno por cada aviso. El botón **PROBAR** lanza los avisos activos con un invitado de ejemplo.

## Detalles
- Solo avisa de invitados, no de ti mismo.
- Si PES está a pantalla completa y Windows no muestra el globo (modo concentración, juegos exclusivos), el sonido y el aviso dentro del juego siguen funcionando.
- Los avisos viven en `SmashSoda/phoenix/core/AvisoEntrada.*`; Hosting.cpp solo tiene una línea (`phoenix::aviso::entro`) tras `addNewGuest`.
- El aviso para el overlay del juego se encola (`PhoenixLink::avisoLocalDiferido`) y lo escribe el hilo de Link, para no mezclar hilos.
- Preferencias en `phoenix-ui.json`: `avisoEntrada`, `avisoEntradaSonido`, `avisoEntradaNotif`, `avisoEntradaJuego` (todas encendidas por defecto).
