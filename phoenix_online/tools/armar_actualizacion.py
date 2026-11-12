"""Arma el .cpk de actualización en vivo de Phoenix para que todos los PES tengan los MISMOS datos.

Uso (en la PC del servidor):
    python tools/armar_actualizacion.py 1.02 <carpeta_con_tablas> actualizacion/Phoenix_102.cpk

<carpeta_con_tablas> lleva las tablas de la base del parche TAL COMO están en el parche (ya empaquetadas
WESYS), por ejemplo Player.bin, PlayerAssignment.bin, Team.bin, Coach.bin... Van a common/etc/pesdb/.
Con «-» como carpeta se arma una actualización de PRUEBA que solo lleva version_wu.bin (no cambia
ningún dato del juego): sirve para comprobar que el PES la baja y la monta.
La versión tiene que SUBIR cada vez que se publica otra («1.02» -> «1.03»), si no el PES no la baja.
Nunca subir el .cpk ni las tablas a GitHub (son datos del juego/parche).
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from phoenix_online import actualizacion  # noqa: E402


def main(version, carpeta, salida):
    tablas = [] if carpeta == "-" else sorted(x for x in os.listdir(carpeta) if x.lower().endswith(".bin"))
    if not tablas and carpeta != "-":
        sys.exit(f"no hay .bin en {carpeta}")
    with tempfile.TemporaryDirectory() as tmp:
        ver = os.path.join(tmp, "version_wu.bin")
        with open(ver, "wb") as f:
            f.write(actualizacion.archivo_version(version))
        archivos = [(actualizacion.ARCHIVO_VERSION, ver)]
        archivos += [(f"common/etc/pesdb/{x}", os.path.join(carpeta, x)) for x in tablas]
        os.makedirs(os.path.dirname(os.path.abspath(salida)), exist_ok=True)
        actualizacion.escribir_cpk(salida, archivos)
    ficha = actualizacion.actual(os.path.dirname(os.path.abspath(salida)))
    print(f"listo: {salida} ({os.path.getsize(salida)} bytes) con {len(tablas)} tablas")
    if ficha:
        print(f"se ofrecerá como versión {ficha[1]['dlc_version']}, sha256 {ficha[1]['hash']}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:])
