# Cómo repetir el análisis de PES2021.exe en Ghidra (si sale otro exe)

Hecho el 29-oct-2026. Resultado: Ghidra 12.0.4 + GhidraMCP consultable desde Claude Code.

## Por qué el exe completo no se analiza
`PES2021.exe` (458 MB) lleva Denuvo 17.0 [Steam] (Detect It Easy 3.21): la sección `.impdata` ocupa 380 MB (RWX, entropía 6,7-7,0). El código real está en `.trace` (37 MB, R-X). Ghidra intenta recorrer `.impdata` como código y no termina (Constant Propagation, después CreateFunctionCmd). Quitar solo el permiso de ejecución no basta: hay que ELIMINAR el bloque de la copia que carga Ghidra. No se descifra ni se desvirtualiza el protector.

## Pasos (versión rápida)
1. Copiar el exe nuevo a `D:\Frank\IA\CLAUDE\ghidra_proyectos\` (nunca analizar el original). Comprobar versión: sha256 de `.trace` (el de hoy: `cff7b2d9c13a29e454ee461fd4957444c012665671fc8df870d41ffab6589def`). Si el hash cambia, hay otra versión de PES y las direcciones del Core hay que revisarlas.
2. Importar con Ghidra 12.0.4 (`herramientas\Ghidra-12.0.4`) en modo headless con el script `scripts\QuitarImpdata.java` como preScript (elimina `.impdata` y apaga Aggressive Instruction Finder y Decompiler Parameter ID). Ejemplo (ver `importar_fase2.bat`):
   `analyzeHeadless.bat <carpeta> <PROYECTO> -import <exe> -scriptPath <carpeta>\scripts -preScript QuitarImpdata.java -analysisTimeoutPerFile 5400 -overwrite`
   Alternativa más corta: usar un exe «recortado» de 60 MB (existe para el build actual: `D:\Frank\Games_\PRUEBA_CANCHA\copias\exe_pes2021\PES2021_recortado.exe`; ya sin protector). Con ese archivo el análisis tardó ~85 min en 12.0.4 (`importar_recortado.bat`) y ~35 min la vez anterior en 12.1.4.
3. Memoria: el heap por defecto de Ghidra 12.0.4 es 2 GB (G1). Con el exe recortado llegó al 80 % del tope sin Full GC. Para más margen: añadir `MAXMEM=6G` en `...\ghidra_12.0.4_PUBLIC\support\launch.properties` (hay ~6 GB libres con PES cerrado). Comprobar con `jcmd <pid> VM.flags`.
4. Vigilar el avance (el log se calla en fases largas): CPU del proceso Java (`Get-Process`), `jstack <pid>` (`C:\Program Files\Java\jdk-26.0.2\bin`) para ver el analizador (Constant Propagation, Stack, etc.) y `FIN 0` en el log al terminar. No reiniciar si la CPU sube.
5. Nombres: si existe un proyecto anterior más rico, exportar con `scripts\ExportarNombres.java` (CSV dirección,tipo,nombre) e importar con `scripts\ImportarNombres.java` (solo renombra FUN_ automáticas). Hoy el análisis 12.0.4 ya traía los 61.091 nombres de PEStrim: no hizo falta.
6. GhidraMCP: la extensión está en `Ghidra-12.0.4\...\Extensions\GhidraMCP` (v0.2.2, repo 13bm/GhidraMCP, solo para Ghidra 12.0.4: no hay build 12.1). Activar el plugin: Ghidra no se deja manejar por computer-use, así que se edita `%APPDATA%\ghidra\ghidra_12.0.4_PUBLIC\tools\_code_browser.tcd` añadiendo (con Ghidra CERRADO y respaldo `.pre-mcp`):
   `<PACKAGE NAME="GhidraMCP"><INCLUDE CLASS="ghidra.mcp.MCPServerPlugin" /></PACKAGE>` después de `<PACKAGE NAME="Ghidra Core">`.
7. Abrir: `abrir_ghidra2.bat` (ghidraRun.bat con el .gpr). En la ventana del proyecto, doble clic en el programa (abre CodeBrowser; el plugin escucha en `127.0.0.1:8765` y lanza `mcp_bridge.exe`).
8. Registrar en Claude Code (una vez): `claude mcp add -s user ghidra -- "<ruta>\mcp_bridge.exe" --host localhost --port 8765` (ya hecho; `claude mcp list` muestra Connected). Reiniciar Claude Code para ver las herramientas.

## Consultas de solo lectura sin MCP (Ghidra 12.1.4 sobre PEStrim)
`analyzeHeadless ... PEStrim -process -noanalysis -readOnly -postScript <script>`: `DecompilarLista.java`, `Completitud.java`, `BuscarOffset.java`, `ExportarNombres.java`.

## Lecciones (errores que costaron tiempo)
- Un `.bat` escrito con saltos LF o con rutas con `\r`/`\a` en Python se rompe: generar con `chr(92)` y CRLF.
- No fiarse de que «el proceso vive»: medir CPU en dos instantes y mirar `jstack`.
- Si el log no cambia en 10 min y el analizador es el mismo, parar y corregir (así se descubrió `.impdata`).
