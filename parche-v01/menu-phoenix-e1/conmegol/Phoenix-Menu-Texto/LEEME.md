# Phoenix-Menu-Texto (Conmegol)
Cambia el texto del item "Opciones en linea" (Ajustes) a "Phoenix". Pareja del paquete Phoenix-Menu (que remapea ese item, evento 20, a Entrenamiento libre).
all.str = copia del de livecpk\Comentarios (el que manda hoy) con la cadena sustituida EN SITIO (misma longitud, relleno con ceros: no se tocan los offsets).
ENCENDER: quitar el ";" de  cpk.root = ".\livecpk\Phoenix-Menu-Texto"  en SiderAddons\sider.ini (justo debajo de Phoenix-Menu; el PRIMERO de la lista manda). PES cerrado.
OJO: Phoenix-Menu debe remapear tambien 20_onlinesettings (ver RESULTADO_E1). Sin eso el item diria Phoenix pero abriria Opciones en linea.
APAGAR: poner el ";" otra vez. Backup del ini: pes-menusespaldo_E1_conmegol_20261110\sider.ini.antes_texto
