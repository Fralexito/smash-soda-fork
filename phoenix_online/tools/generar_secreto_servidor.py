"""Genera el secreto propio del servidor para el espejo en Supabase «sin llave maestra» (migración 164).

Uso (en la PC del servidor):  python tools/generar_secreto_servidor.py
- Crea un secreto aleatorio y lo guarda como variable de usuario PHOENIX_SERVIDOR_SECRETO (setx).
  El secreto NO se imprime ni se escribe en ningún archivo.
- Imprime solo su HUELLA (sha256): esa huella sí se puede pasar (no sirve para entrar), y se guarda en
  la tabla phoenix_servidores de Supabase para autorizar a este servidor.
Después: reiniciar el servidor desde una ventana NUEVA (para que vea la variable).
"""
import hashlib
import secrets
import subprocess
import sys

secreto = secrets.token_hex(32)
if sys.platform == "win32":
    r = subprocess.run(["setx", "PHOENIX_SERVIDOR_SECRETO", secreto], capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("No se pudo guardar la variable (setx). No se cambió nada.")
else:
    sys.exit("Este ayudante es para Windows; en otro sistema define PHOENIX_SERVIDOR_SECRETO a mano.")
print("Secreto guardado en la variable PHOENIX_SERVIDOR_SECRETO (no se muestra).")
print("Huella para Supabase (phoenix_servidores.hash):", hashlib.sha256(secreto.encode()).hexdigest())
