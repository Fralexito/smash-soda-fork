# Cómo replicar la investigación del campo 2D (para otro humano u otra IA)

Objetivo: encontrar en la memoria de `PES2021.exe` datos del partido en vivo (posiciones, y mañana otros) **sin escribir nada** en el juego.
Resultado ya obtenido: ver [CAMPO-2D-INVESTIGACION.md](CAMPO-2D-INVESTIGACION.md). Uso en Link: [CAMPO-2D.md](CAMPO-2D.md).

## 0. Requisitos y reglas

- Windows, Visual Studio Build Tools (usa `vcvars64.bat`), PES 2021 abierto con un partido contra la CPU.
- **Solo lectura.** `OpenProcess(PROCESS_VM_READ …)` y nada más. Pausar el proceso (`NtSuspendProcess`/`NtResumeProcess`) solo para investigar, siempre con reanudación garantizada; Link nunca lo hace.
- Los nombres de los `.exe` de prueba no pueden contener «instal» (Windows pide elevación).
- Mismo ejecutable de referencia: 458 806 784 bytes, SHA-256 `5E27A78246DBE4F1E66CB60EC56E86A739303708B02D4048F6FEC6B570BC224A`. Con otro ejecutable todo lo de abajo hay que volver a comprobarlo.
- El juego **se pausa si se minimiza**: déjalo al frente (o en ventana) todo el rato. Teclas en menús: X = confirmar, Z = atrás, Enter y flechas también.

## 1. Método general (sirve para cualquier dato, no solo posiciones)

1. **Escaneo inicial:** leer toda la memoria privada escribible del proceso y guardar los floats con pinta del dato (p. ej. 0.02 ≤ |v| ≤ 6000). ~45 millones de candidatos.
2. **Filtrar por pasos de tiempo controlado:** reanudar el juego ~50 ms, congelarlo, releer **todos** los candidatos con el juego quieto, comparar con la lectura anterior. 40 pasos. Quedarse con lo que cambia poco entre pasos (para una posición: |Δ| ≤ 0.5 + 0.02·|v|) y cambia en ≥ 1/3 de los pasos. Pasa de 50 M a ~380 mil.
3. **Buscar estructura, no valores sueltos:** el paso por «arrays de punteros» (≥ 20 punteros consecutivos cuyos objetos tienen, todos al mismo desplazamiento, un candidato suave) dio con los jugadores; el análisis por «paso constante» NO sirvió (objetos repartidos en bloques).
4. **Reconocer la forma del dato:** un vector `(x, y≈0.9, z, 1.0)` con w = 1.0 exacto es una posición con altura de cadera. Probar columnas (x,z), (x,y,z)… y exigir: dentro del campo, finitos, varias fichitas distintas.
5. **Verificar con la pantalla:** congelar el juego, hacer captura de pantalla y volcar la memoria en el mismo instante; dibujar las posiciones en planta y compararlas. Un **saque de centro** (reloj 0:00) es la mejor verificación: formaciones conocidas, balón en (0,0).
6. **Firma propia:** escribir una regla que reconozca el objeto sin depender de direcciones (tipo, marcas, vec4 plausible). Las direcciones cambian en cada partido; la firma no.
7. **Protección:** versión del ejecutable (tamaño + SHA-256), valores plausibles, lista completa (18–24), reintento con calma y apagado automático. Nada de escribir.

## 2. Herramientas (todas en `tools/radar/`) y comandos

Compilar cada una: `cl /O2 /EHsc /std:c++17 <archivo>.cpp user32.lib winmm.lib` (tras `vcvars64.bat`). Salidas de ejemplo en `C:\Temp\claude\radar` (no se suben).

| Herramienta | Qué hace | Ejemplo |
|---|---|---|
| `escaner.cpp` | Búsqueda tipo Cheat Engine. Se maneja con un archivo `cmd.txt` (o teclado numérico): `0` escaneo inicial, `6` pasos de 50 ms, `8` arrays de punteros, `3` análisis | `echo 0 > cmd.txt` … `echo 6 > cmd.txt` … `echo 8 > cmd.txt` |
| `analiza.cpp` | Análisis sin conexión de `hist.bin` | `analiza hist.bin out.txt 120` |
| `sonda.cpp` / `objeto.cpp` | Lee en vivo un array de punteros / vuelca los objetos | `sonda 4631D540 44 208 5 300` |
| `buscaptr.cpp` | Quién apunta a una dirección (para buscar rutas estables) | `buscaptr 4631C000 800` |
| `entes.cpp` | **Escáner por firma de los actores** (~5 s); `entes.exe raw` vuelca las 160 palabras de cada objeto | `entes.exe raw > actores.txt` |
| `entes3.cpp` | Firma relajada: porteros, árbitro y otros tipos (`kind`, código, lado) | `entes3.exe > otros.txt` |
| `bola.cpp`, `bola2.cpp` | Búsqueda del balón: candidatos `(x,y,z[,1.0])` con y de balón, y seguimiento por pasos | `bola2 vec4` luego `bola2 pasos bola_cands.txt 100 50` |
| `vigilante.cpp` | Congela el juego solo cuando detecta un saque de centro y lanza `captura.bat` | `vigilante dirs.txt 900` |

Congelar y capturar a mano (PowerShell): `NtSuspendProcess` → `CopyFromScreen` → `entes.exe raw` → `NtResumeProcess` (ver `docs/CAMPO-2D-INVESTIGACION.md`, §2).

## 3. Receta completa para repetir el hallazgo de las posiciones

1. Abre un partido y déjalo en marcha, juego al frente.
2. `escaner.exe` → `echo 0 > cmd.txt` (espera «Escaneo inicial») → `echo 6 > cmd.txt` (≈ 1 min) → `echo 8 > cmd.txt`.
3. En `punteros.txt` busca bloques con 22–24 valores `(x, ~0.9, z, 1.0)` en el mismo desplazamiento: sale el array en `0x4631D540` (en tu sesión será otra) con `+208` = posición.
4. Comprueba con `sonda` que se mueven suave (2–3 m/s). Confirma con una captura congelada.
5. Para uso real: no hace falta nada de lo anterior; Link ya trae la firma (`core/CampoVivo.cpp`) y `host/CampoVivo.cpp`.

## 4. Errores que ya nos costaron tiempo (evítalos)

- Clasificar como «parado» los segundos de transición al pausar/reanudar descarta justo los datos buenos.
- El análisis por paso constante entre candidatos no encuentra objetos grandes repartidos en bloques.
- `RAW` de `entes.exe`: la palabra `w` está en la columna `3+w` de awk (`$(3+w)`), en PowerShell `Split(' ')[2+w]`.
- Guardar objetos de 640 bytes cruzando el límite de un trozo de 1 MB: leer 640 bytes de solape.
- El juego puede quedar suspendido si algo falla entre `Suspend` y `Resume`: usar siempre `try/finally`.
- Escanear solo la memoria privada escribible (`MEM_PRIVATE` + `PAGE_READWRITE`): 12 s; todo el espacio: 45 s.
- `.bat` creados desde Git Bash salen con LF y `cmd` los interpreta mal: convertir a CRLF.

## 5. Qué otra información valiosa se puede sacar con el mismo método (ideas)

Todas sin escribir en el juego; cada una necesita su propia verificación contra la pantalla.

1. **Balón** (en curso): posición, altura y velocidad → radar completo, tiros/pases, zona de peligro.
2. **Equipo de cada jugador** (en curso) y **quién lleva el balón**.
3. **Velocidades y distancias recorridas** por jugador (derivadas de las posiciones): mapa de calor, kilómetros, sprints.
4. **Posesión real por zonas**, líneas de pase, formaciones en vivo (4-3-3 → 4-4-2).
5. **Eventos**: fuera de juego, saques de banda/esquina/penal por posición del balón; refuerza el árbitro de Link (marcador, minuto y goles ya vienen de Sider).
6. **Penales**: balón quieto en un punto conocido → referencia perfecta, y datos del lanzador y el portero.
7. **Repeticiones para streaming/overlay**: las mismas posiciones sirven para un mini-radar en PhoenixGlass o para el stream del torneo.
8. **Datos de IA** (tablas con nombres de huesos y registros por jugador): estructuras que quedaron sin descifrar.

## 6. Para continuar donde lo dejamos

Pendiente por orden: (1) confirmar el balón (candidatos `kind=0`, y ≈ 0.10–0.11, ver §7 de la guía de investigación) con el juego en marcha; (2) equipo por jugador con un saque de centro congelado; (3) probar la conexión en el PhoenixLink.exe real; (4) mini-radar en el overlay. Registro de cambios: `REGISTRO-LINK.md`.
