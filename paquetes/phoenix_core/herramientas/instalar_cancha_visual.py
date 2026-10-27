# instalar_cancha_visual.py — UNA sola tanda: extrae (si falta), modifica, reempaqueta e instala (con copia de seguridad) los assets de la cancha 30x20 del Camp Nou.
# Uso: python instalar_cancha_visual.py [instalar]   (sin argumento solo prepara en trabajo_campo\salida; con «instalar» copia al AddOn)
# Cancha: semilargo 15, semiancho 10 (los mismos que cancha_semilargo / cancha_semiancho de phoenix_core.json).
import os, shutil, struct, subprocess, sys, glob, time
BASE = r"D:\Frank\Games_\PRUEBA_CANCHA\herramientas_comunidad"
GZS = BASE + r"\GzsTool\extraido\GzsTool.exe"
ADDON = r"D:\Frank\Games_\Conmegol Patch\SiderAddons\content\stadium-server\España\Spotify Camp Nou\AddOn\Goal Post Colour\Global"
LARGO, ANCHO = 15.0, 10.0
KX, KZ = LARGO / 52.56, ANCHO / 34.06
SAL = BASE + r"\trabajo_campo\salida"
os.makedirs(SAL, exist_ok=True)

# 1) colisiones .geom: floats 40<|v|<70 hacia el centro 37,5 (red, postes, fondo)
GEOM = ["common/render/model/bg/hit/goal/goal_l.geom", "common/render/model/bg/hit/goal/goal_r.geom",
        "common/anime/FoxAnim/goal_hit.geom", "common/anime/FoxAnim/goal_l.geom", "common/anime/FoxAnim/goal_r.geom"]
dx = 52.5 - LARGO
for rel in GEOM:
    src = os.path.join(BASE, "trabajo_geom", rel.replace("/", os.sep))
    d = bytearray(open(src, "rb").read())
    for i in range(0, len(d) - 3, 4):
        v = struct.unpack_from("<f", d, i)[0]
        if 40 < abs(v) < 70:
            struct.pack_into("<f", d, i, v + dx if v < 0 else v - dx)
    out = os.path.join(SAL, rel.replace("/", os.sep)); os.makedirs(os.path.dirname(out), exist_ok=True); open(out, "wb").write(d)
    print("geom", rel, len(d))

# 2) césped: pitch_st002.fmdl, vértices xyz (paso 12 B) escalados x*KX, z*KZ (+ volúmenes en 0x4E0..0x538)
w = os.path.join(SAL, "pitch"); shutil.rmtree(w, ignore_errors=True); os.makedirs(w)
shutil.copy(os.path.join(BASE, "trabajo_camp_nou", "pitch_st002.fpk"), w)
subprocess.run([GZS, "pitch_st002.fpk"], cwd=w, capture_output=True)
fm = glob.glob(os.path.join(w, "pitch_st002_fpk", "**", "*.fmdl"), recursive=True)[0]
d = bytearray(open(fm, "rb").read()); n = len(d) // 4
f = struct.unpack("<%df" % n, bytes(d[:n * 4]))
def valid(i):
    x, y, z = f[i], f[i + 1], f[i + 2]
    return abs(x) <= 60 and abs(z) <= 40 and abs(y) <= 5 and (abs(x) > 0.01 or abs(z) > 0.01)
i = 0; tot = 0
while i < n - 3:
    if valid(i):
        j = i; c = 0
        while j < n - 3 and valid(j): j += 3; c += 1
        if c >= 20 and max(max(abs(f[k]), abs(f[k + 2])) for k in range(i, j, 3)) > 8:
            for k in range(i, j, 3):
                struct.pack_into("<f", d, k * 4, f[k] * KX); struct.pack_into("<f", d, (k + 2) * 4, f[k + 2] * KZ); tot += 1
        i = j if j > i else i + 1
    else:
        i += 1
for off in range(0x4E0, 0x540, 16):
    x = struct.unpack_from("<f", d, off)[0]; z = struct.unpack_from("<f", d, off + 8)[0]
    struct.pack_into("<f", d, off, x * KX); struct.pack_into("<f", d, off + 8, z * KZ)
open(fm, "wb").write(d)
subprocess.run([GZS, "pitch_st002.fpk.xml"], cwd=w, capture_output=True)
print("césped: vértices escalados", tot, "fpk", os.path.getsize(os.path.join(w, "pitch_st002.fpk")))
shutil.copy(os.path.join(w, "pitch_st002.fpk"), os.path.join(SAL, "pitch_st002.fpk"))

if len(sys.argv) > 1 and sys.argv[1] == "instalar":
    resp = os.path.join(BASE, "trabajo_goal", "respaldo_cancha_" + time.strftime("%H%M")); os.makedirs(resp)
    lista = [(os.path.join(SAL, r.replace("/", os.sep)), os.path.join(ADDON, r.replace("/", os.sep))) for r in GEOM]
    lista.append((os.path.join(SAL, "pitch_st002.fpk"), os.path.join(ADDON, "Asset", "model", "bg", "st002", "pitch", "#Win", "pitch_st002.fpk")))
    log = open(os.path.join(resp, "INSTALADOS.txt"), "w", encoding="utf-8")
    for s, t in lista:
        os.makedirs(os.path.dirname(t), exist_ok=True)
        if os.path.exists(t): shutil.copy(t, os.path.join(resp, os.path.basename(t) + ".orig"))
        shutil.copy(s, t); log.write(t + "\n"); print("instalado", t)
    log.close(); print("Deshacer: borrar los archivos listados en", os.path.join(resp, "INSTALADOS.txt"))
