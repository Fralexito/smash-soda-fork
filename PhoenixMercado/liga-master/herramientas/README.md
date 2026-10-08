# Herramientas de línea de comandos (C) para guardados de Liga Máster

Pequeños ejecutables de **investigación** que usan `terceros/pesxcrypter`. No forman parte de la build de Phoenix Mercado ni de Phoenix Link.
Nunca escriben sobre su entrada: siempre producen un archivo nuevo.

| Herramienta | Qué hace |
|---|---|
| `dec` | Descifra un guardado (`ML…`, `EDIT…`), lo vuelve a cifrar y comprueba si el resultado es idéntico byte a byte |
| `info` | Muestra cabecera, tamaños, descripción (nombre de 128 B + texto info) y serial |
| `enc2` | Cifra datos planos del mismo tamaño usando cabecera/logo/serial del original; opcionalmente cambia el nombre de 128 B y el texto info (lo que muestra el menú Cargar) |

## Compilar (Linux/macOS con gcc; en Windows con MinGW o MSVC equivalente), desde esta carpeta
```
T=../../terceros/pesxcrypter
for t in dec info enc2; do gcc -O2 -o $t $t.c $T/crypt.c $T/masterkey.c $T/mt19937ar.c -I$T; done
```
Los ejecutables y los guardados (`ML*`, `*.bin`) **no se suben al repo**.
