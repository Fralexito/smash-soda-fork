# cesped_segunda_instancia.py — PREPARA (no instala) un pitch_st002.fpk/.fpkd con DOS instancias del césped:
#  (1) la pequeña (escalada 30x20, con líneas) y (2) un suelo grande sin líneas (copia SIN escalar con los 4 nombres de textura de línea
#  cambiados por uno de igual longitud, y 3 cm más abajo). Salida: trabajo_campo\pitch2\ (pitch_st002.fpk, pitch_st002.fpkd). Probar mañana.
import glob, os, re, shutil, struct, subprocess
B = r"D:\Frank\Games_\PRUEBA_CANCHA\herramientas_comunidad"
G = B + r"\GzsTool\extraido\GzsTool.exe"; F = B + r"\FoxTool\extraido\FoxTool v0.2.6\FoxTool.exe"
W = os.path.join(B, "trabajo_campo", "pitch2"); shutil.rmtree(W, ignore_errors=True); os.makedirs(W)
# --- fpk: pequeña (AB ya generada) + base
shutil.copy(os.path.join(B, "trabajo_campo", "var_AB", "pitch_st002.fpk"), W)
subprocess.run([G, "pitch_st002.fpk"], cwd=W, capture_output=True)
esc = os.path.join(W, "pitch_st002_fpk", "Assets", "pes16", "model", "bg", "st002", "scenes")
orig = glob.glob(os.path.join(B, "trabajo_camp_nou", "pitch_st002_fpk", "**", "pitch_st002.fmdl"), recursive=True)[0]
d = bytearray(open(orig, "rb").read())
for nombre in (b"line_alp.tga", b"line_nrm.tga", b"line_srm.tga", b"line_mtl.tga"):
    i = d.find(nombre); assert i > 0, nombre
    d[i:i + 12] = b"turf_nrm.tga"
# bajar 3 cm los vértices (y) de los bloques de vértices
for off, c in ((0x7c0, 126), (0xdb0, 1088), (0x4118, 60), (0x43ec, 7364)):
    for k in range(c):
        o = off + 12 * k + 4; y = struct.unpack_from("<f", d, o)[0]; struct.pack_into("<f", d, o, y - 0.03)
open(os.path.join(esc, "pitch_st002_base.fmdl"), "wb").write(d)
x = open(os.path.join(W, "pitch_st002.fpk.xml"), encoding="utf-8").read()
x = x.replace("</Entries>", '    <Entry FilePath="/Assets/pes16/model/bg/st002/scenes/pitch_st002_base.fmdl" />\n  </Entries>')
open(os.path.join(W, "pitch_st002.fpk.xml"), "w", encoding="utf-8").write(x)
subprocess.run([G, "pitch_st002.fpk.xml"], cwd=W, capture_output=True)
# --- fpkd: añadir la 2.ª entidad
shutil.copy(os.path.join(B, "trabajo_camp_nou", "pitch_st002.fpkd"), W)
subprocess.run([G, "pitch_st002.fpkd"], cwd=W, capture_output=True)
fox = glob.glob(os.path.join(W, "pitch_st002_fpkd", "**", "pitch_st002.fox2"), recursive=True)[0]
subprocess.run([F, "-d", fox], capture_output=True)
s = open(fox + ".xml", encoding="utf-8-sig").read()
g = open(glob.glob(os.path.join(B, "trabajo_goal", "goal_post_fpkd", "**", "goal_post.fox2.xml"), recursive=True)[0], encoding="utf-8-sig").read()
bloque = re.search(r'<entity class="StadiumModel".*?</entity>\s*<entity class="TransformEntity".*?</entity>', g, re.S).group(0)
bloque = bloque.replace("0x03172CB0", "0x00000600").replace("0x03172D20", "0x00000700").replace("0x03172C40", "0x00000100")
bloque = bloque.replace("StadiumModel0000", "StadiumModel_pitch_base").replace("/Assets/pes16/model/bg/common/goal/scenes/goal.fmdl", "/Assets/pes16/model/bg/st002/scenes/pitch_st002_base.fmdl")
s = s.replace('<class name="TransformEntity" super="" version="0" />', '<class name="StadiumModel" super="" version="2" />\n    <class name="TransformEntity" super="" version="0" />', 1)
s = s.replace('arraySize="1">\n          <value key="StadiumModel_pitch">0x00000400</value>', 'arraySize="2">\n          <value key="StadiumModel_pitch">0x00000400</value>\n          <value key="StadiumModel_pitch_base">0x00000600</value>', 1)
s = s.replace("</entities>", bloque + "\n  </entities>", 1)
open(fox + ".xml", "w", encoding="utf-8-sig").write(s)
subprocess.run([F, fox + ".xml"], capture_output=True)
xx = open(os.path.join(W, "pitch_st002.fpkd.xml"), encoding="utf-8").read()
subprocess.run([G, "pitch_st002.fpkd.xml"], cwd=W, capture_output=True)
print("fpk", os.path.getsize(os.path.join(W, "pitch_st002.fpk")), "fpkd", os.path.getsize(os.path.join(W, "pitch_st002.fpkd")))
# verificación: reabrir el fpkd nuevo
V = os.path.join(W, "verif"); os.makedirs(V); shutil.copy(os.path.join(W, "pitch_st002.fpkd"), V)
subprocess.run([G, "pitch_st002.fpkd"], cwd=V, capture_output=True)
f2 = glob.glob(os.path.join(V, "**", "pitch_st002.fox2"), recursive=True)[0]; subprocess.run([F, "-d", f2], capture_output=True)
t = open(f2 + ".xml", encoding="utf-8-sig").read(); print("entidades StadiumModel:", t.count('class="StadiumModel"'), "Enlighten:", t.count('class="EnlightenStadiumModel"'), "base:", "pitch_st002_base.fmdl" in t)
