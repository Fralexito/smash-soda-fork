# Parche Phoenix v0.1 (PES 2021 base de Frank)

- `launcher/` — Phoenix Launcher: pestaña Interruptores (encender/apagar partes del `sider.ini`, perfiles V1/V2, deshacer, CAMBIOS.md, Jugar) y pestaña Caras (elegir qué versión de cara gana y crear el paquete PHX_FAC_SEL con enlaces duros). Tests: `launcher/test_launcher.py` y `launcher/test_caras.py`.
- `terreno/` — Inventario por ruta de caras, minicaras, kits, estadios y balones (base, Conmegol, Sudamerican), choques y propuesta de carpetas PHX_. Empezar por `terreno/INFORME_TERRENO_DB.md`. `crudo_rutas.tsv` (221 MB) no se sube: se regenera con `terreno/scripts/1_recolectar.py`.
- `bitacora/` — Bitácora del parche y leyenda de nombres técnicos.

Solo código e índices: ningún archivo de los parches ni del juego.
