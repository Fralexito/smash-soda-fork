"""Arrancador con vigilancia: abre el servidor y lo vuelve a abrir solo si se cae.

Uso (en la carpeta donde está config.json):
    python tools/arrancar_servidor.py [config.json]

- Si el servidor se cierra por un error, espera unos segundos (5, 10, 30, 60…) y lo reabre.
- Si se cierra porque los puertos están ocupados (ya hay otro servidor abierto), NO insiste.
- Ctrl+C cierra todo de verdad.
- Cada caída queda anotada en registro/arrancador.log.
"""
import os
import subprocess
import sys
import time

SALIDA_PUERTO_OCUPADO = 3
ESPERAS = (5, 10, 30, 60, 120)


def anotar(texto):
    linea = f"{time.strftime('%Y-%m-%d %H:%M:%S')} {texto}"
    print(linea, flush=True)
    try:
        os.makedirs("registro", exist_ok=True)
        with open(os.path.join("registro", "arrancador.log"), "a", encoding="utf-8") as f:
            f.write(linea + "\n")
    except OSError:
        pass


def main():
    config = sys.argv[1] if len(sys.argv) > 1 else "config.json"
    if not os.path.isfile(config):
        anotar(f"No encuentro {config}. Abre esta ventana en la carpeta del servidor.")
        return 2
    caidas_seguidas = 0
    while True:
        inicio = time.time()
        anotar("abriendo el servidor Phoenix Online…")
        try:
            codigo = subprocess.call([sys.executable, "-m", "phoenix_online.servidor", config])
        except KeyboardInterrupt:
            anotar("cerrado con Ctrl+C")
            return 0
        if codigo == 0:
            anotar("el servidor se cerró normalmente")
            return 0
        if codigo == SALIDA_PUERTO_OCUPADO:
            anotar("los puertos están ocupados: ya hay otro servidor abierto. No reintento.")
            return codigo
        # si estuvo bien más de 10 minutos, la cuenta de caídas vuelve a empezar
        caidas_seguidas = 0 if time.time() - inicio > 600 else caidas_seguidas + 1
        espera = ESPERAS[min(caidas_seguidas, len(ESPERAS) - 1)]
        anotar(f"el servidor se cayó (código {codigo}); lo reabro en {espera} s")
        try:
            time.sleep(espera)
        except KeyboardInterrupt:
            anotar("cerrado con Ctrl+C")
            return 0


if __name__ == "__main__":
    sys.exit(main())
