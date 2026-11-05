# Plan de myClub (Phoenix Online). Actualizado el 05-nov-2026

Estado: myClub funciona en el PES base contra nuestro servidor. Se puede ver el club, la plantilla, el DT, contratos, cazatalentos y estadísticas, fichar un DT con GP y jugar un partido completo contra la COM. El resultado y los GP quedan guardados.
Detalles técnicos: `ESPEC_MYCLUB.md`. Cómo se logró: `GUIA_MYCLUB_REVIVIDO.md`.

## Orden acordado con Frank (05-nov)
1. **Catálogo automático de jugadores**
   - Hecho: 661 equipos y 17 583 jugadores con nombre (`tools/catalogo_pesdb.py`).
   - Falta: descifrar en Player.bin la posición, la nación y las habilidades, para calcular la media, la rareza y el precio reales.
2. **Fichajes**
   - Cazatalentos, Representantes (agentes) y subastas, pagando con GP (o con monedas gratis).
   - Faltan los lectores de sus comandos de compra.
3. **Progreso de jugadores**
   - Hecho en el servidor: experiencia y nivel tras cada partido.
   - Falta: confirmar la curva real, el cansancio y renovar o vender contratos.
4. **Rival COM variado**
   - Hecho: rotación entre 7 clubes grandes.
   - Falta: subir la dificultad con las victorias (new_cpu_level).
   - El escudo «MD» del Madrid es el oficial del juego, que no tiene licencia: no es un fallo.
5. **Para mucha gente**
   - Identidad propia por persona: el pase web de Link, validado en Supabase. Hoy todas las copias CPY comparten el mismo SteamID.
   - Base de datos en Supabase en lugar de la PC.
   - Panel web para GP, monedas y eventos.

## Decisiones de Frank
- **Monedas de myClub:** solo como premio gratis, con la opción `myclub.monedas_gratis`. **Nunca se venden** (regla: nada de dinero real).
- **Dar GP o monedas a una persona:** `tools/admin_myclub.py`. Hoy es por consola; más adelante será un botón en la web.
- **Modelo:** usar Opus para leer el juego (ingeniería inversa) y avisar cuando se pueda volver a Sonnet.

## Ideas de Frank, apuntadas para después
- **Leyendas y jugadores propios:** se puede, si están en la base de datos del juego o del parche. El servidor solo reparte los números de jugador. Todos los que jueguen online necesitan la misma base.
- **Usar otra base de datos o el parche propio:** volver a correr el catálogo con esa base. Si cambian los números de los jugadores, pasar los clubes existentes a la base nueva (dejarlo automático).
- **Diseño y color de las cartas:**
  - Son imágenes del juego: se cambian con un mod gráfico de Sider (prender/apagar), y solo lo ve quien tenga el parche.
  - El servidor decide la rareza de cada jugador.
  - Falta ubicar los archivos de las texturas de las cartas.
- **App tipo The Gaffer, pero mejor:** tiene su propio hilo, «App móvil · Planear app tipo The Gaffer». Se apoyará en este servidor y en Supabase, para usarla desde cualquier lugar.
- **Probar myClub con el parche de Frank:** después del paso 1, y con su permiso.
- **Partidos entre dos personas:** emparejar a 2 jugadores y el relé de juego (ver `INVENTARIO_PESBUL.md`). Hacen falta dos PES conectados a la vez.

## Riesgos y límites
- Los datos de jugadores son de Konami: mostrarlos en una web pública tiene riesgo legal. Revisarlo antes de publicar.
- No cobrar nada por myClub sin abogado.
- Lo que sale de la base del juego (catalogo.json, los .bin) y config.json no se suben nunca.
