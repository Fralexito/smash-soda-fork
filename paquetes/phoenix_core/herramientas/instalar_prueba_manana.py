# instalar_prueba_manana.py <pitch2|ab>  — con PES CERRADO. pitch2 = instala el césped con 2.ª instancia sin líneas (copia de seguridad del AB actual); ab = vuelve al AB.
import os, shutil, sys
B = r"D:\Frank\Games_\PRUEBA_CANCHA\herramientas_comunidad"
T = r"D:\Frank\Games_\Conmegol Patch\SiderAddons\content\stadium-server\España\Spotify Camp Nou\AddOn\Goal Post Colour\Global\Asset\model\bg\st002\pitch\#Win"
os.makedirs(T, exist_ok=True)
resp = os.path.join(B, "trabajo_goal", "respaldo_AB"); os.makedirs(resp, exist_ok=True)
if sys.argv[1] == "pitch2":
    for n in ("pitch_st002.fpk", "pitch_st002.fpkd"):
        t = os.path.join(T, n)
        if os.path.exists(t) and not os.path.exists(os.path.join(resp, n)): shutil.copy(t, os.path.join(resp, n))
        shutil.copy(os.path.join(B, "trabajo_campo", "pitch2", n), t); print("instalado", n)
else:
    shutil.copy(os.path.join(B, "trabajo_campo", "var_AB", "pitch_st002.fpk"), os.path.join(T, "pitch_st002.fpk"))
    f = os.path.join(T, "pitch_st002.fpkd")
    if os.path.exists(f): os.remove(f)
    print("vuelto a AB (fpk AB; sin fpkd propio)")
