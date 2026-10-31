# variante_cesped.py <A|B|AB> [instalar] — escala SOLO algunos bloques de vértices del césped pitch_st002.fmdl (prueba del hueco blanco)
#   A  = bloques 0x7c0 (126), 0xdb0 (1088) y 0x4118 (60);   B = bloque 0x43ec (7364);   AB = todos (versión actual)
import os, shutil, struct, subprocess, sys, glob
BASE = r"D:\Frank\Games_\PRUEBA_CANCHA\herramientas_comunidad"; GZS = BASE + r"\GzsTool\extraido\GzsTool.exe"
ADDON = r"D:\Frank\Games_\Conmegol Patch\SiderAddons\content\stadium-server\España\Spotify Camp Nou\AddOn\Goal Post Colour\Global\Asset\model\bg\st002\pitch\#Win\pitch_st002.fpk"
var = sys.argv[1]; KX, KZ = 15.0 / 52.56, 10.0 / 34.06
BLOQ = {"A": [0x7c0, 0xdb0, 0x4118], "B": [0x43ec], "AB": [0x7c0, 0xdb0, 0x4118, 0x43ec]}[var]
CNT = {0x7c0: 126, 0xdb0: 1088, 0x4118: 60, 0x43ec: 7364}
w = os.path.join(BASE, "trabajo_campo", "var_" + var); shutil.rmtree(w, ignore_errors=True); os.makedirs(w)
shutil.copy(os.path.join(BASE, "trabajo_camp_nou", "pitch_st002.fpk"), w)
subprocess.run([GZS, "pitch_st002.fpk"], cwd=w, capture_output=True)
fm = glob.glob(os.path.join(w, "pitch_st002_fpk", "**", "*.fmdl"), recursive=True)[0]
d = bytearray(open(fm, "rb").read())
for off in BLOQ:
    for k in range(CNT[off]):
        o = off + 12 * k
        x = struct.unpack_from("<f", d, o)[0]; z = struct.unpack_from("<f", d, o + 8)[0]
        struct.pack_into("<f", d, o, x * KX); struct.pack_into("<f", d, o + 8, z * KZ)
for off in range(0x4E0, 0x540, 16):
    x = struct.unpack_from("<f", d, off)[0]; z = struct.unpack_from("<f", d, off + 8)[0]
    struct.pack_into("<f", d, off, x * KX); struct.pack_into("<f", d, off + 8, z * KZ)
open(fm, "wb").write(d)
subprocess.run([GZS, "pitch_st002.fpk.xml"], cwd=w, capture_output=True)
out = os.path.join(w, "pitch_st002.fpk"); print("variante", var, os.path.getsize(out))
if len(sys.argv) > 2 and sys.argv[2] == "instalar":
    shutil.copy(out, ADDON); print("instalado", var)
