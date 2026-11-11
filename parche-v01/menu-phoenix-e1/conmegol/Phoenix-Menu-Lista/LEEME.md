# Phoenix-Menu-Lista (Conmegol) - pantalla NATIVA reutilizada (prueba A)
«Phoenix» (evento 20 de TopMenuNew.json) abre la lista nativa TopMenu/TopMenuOptionSub (10 items, con «Atras» nativo). Prueba A: TODOS los items re-cableados a TopMenu/TopMenu (ninguno borra/guarda datos); return -> TopMenu/TopMenu.
Solo JSON: sin Core, sin DLL, sin Present. Se apoya en lo ya probado (re-cablear eventos).
ENCENDER: quitar el ";" de  cpk.root = ".\livecpk\Phoenix-Menu-Lista"  en SiderAddons\sider.ini (esta linea va POR ENCIMA de Phoenix-Menu: el primero de la lista manda). PES cerrado y sin sider.exe huerfano.
APAGAR: poner el ";" otra vez. Phoenix-Menu sigue funcionando como antes (ida y vuelta).
Backup del ini: pes-menusespaldo_E1_conmegol_20261110\sider.ini.antes_lista
Nota: el ítem «Aplicar nueva Actualizacion» (23) de Conmegol tambien apunta a TopMenuOptionSub: mostrara esta misma lista.
Pruebas siguientes: B (rotulos con all.str en sitio) y C (destinos reales). Ver DISENO_PANTALLA_PROPIA_CORE.md §15.3.
