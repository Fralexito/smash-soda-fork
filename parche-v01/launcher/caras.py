# -*- coding: utf-8 -*-
"""Phoenix Launcher - «Elegir cara» (sin interfaz).

- Indice de versiones de cara por jugador (base / Conmegol / Sudamerican) desde pes-db\\terreno.
- Lista de archivos de cada version (carpeta suelta o dentro de .cpk), resolucion y peso.
- Vista previa: minicara .dds -> PNG en cache (DXT1/3/5 y RGB 32; descompresor CRILAYLA para
  las del juego base, que vienen comprimidas). Sin librerias externas.
- Elecciones por perfil en perfiles\\caras_elegidas.json.
- Paquete PHX_FAC_SEL: enlaces duros cuando el origen es un archivo suelto del mismo disco;
  copia (con aviso y tamano) cuando viene de un .cpk o de otro disco. Nunca borra: lo anterior
  se mueve a _anteriores.
"""
import os, re, csv, json, struct, zlib, difflib, datetime, threading

TERRENO = r"D:\Frank\IA\CLAUDE\pes-db\terreno"
SIDERS = {
    "base": r"C:\PES21\eFootball PES 2021\Sider_7.4.1",
    "conmegol": r"D:\Frank\Games_\Conmegol Patch\SiderAddons",
    "sudamerican": r"D:\SP2026\Sudamerican Project Sider",
    "sud_wc26": r"D:\SP2026\Sudamerican Project Sider WC26",
}
JUEGOS = {"base": r"C:\PES21\eFootball PES 2021", "conmegol": r"D:\Frank\Games_\Conmegol Patch",
          "sudamerican": r"D:\SP2026", "sud_wc26": r"D:\SP2026"}
FAM = {"base": "base", "conmegol": "conmegol", "sudamerican": "sudamerican", "sud_wc26": "sudamerican"}
VERSIONES = ("base", "conmegol", "sudamerican")
NOMBRE_VERSION = {"base": "Juego base", "conmegol": "Conmegol", "sudamerican": "Sudamerican"}
OPCIONES_EXTRA = {"juego": "Ninguna (usar la del juego)", "defecto": "Por defecto (orden de prioridad)"}
PAQUETES = r"D:\Frank\Games_\PHX_paquetes"
NOMBRE_PAQUETE = "PHX_FAC_SEL"


def ahora():
    return datetime.datetime.now()


# ================================================================ CPK
_toc_cache = {}
_toc_lock = threading.Lock()


def _utf(buf):
    import sys
    if r"D:\Frank\IA\CLAUDE\pes-db\panel-duelo" not in sys.path:
        sys.path.insert(0, r"D:\Frank\IA\CLAUDE\pes-db\panel-duelo")
    import cpk_toc
    return cpk_toc.utf(buf)


def toc(path):
    """{ruta_logica_minusculas: (offset_absoluto, FileSize, ExtractSize, ruta_original)}"""
    with _toc_lock:
        if path in _toc_cache:
            return _toc_cache[path]
    with open(path, "rb") as f:
        h = f.read(16)
        hd = _utf(f.read(struct.unpack("<Q", h[8:16])[0]))[0]
        f.seek(hd["TocOffset"])
        t = f.read(16)
        if t[:4] != b"TOC ":
            raise ValueError("sin TOC: " + path)
        rows = _utf(f.read(struct.unpack("<Q", t[8:16])[0]))
    add = min(hd["TocOffset"], hd.get("ContentOffset") or hd["TocOffset"])
    d = {}
    for r in rows:
        ruta = (r.get("DirName") or "") + "/" + r["FileName"]
        d[ruta.lower()] = (r["FileOffset"] + add, r["FileSize"], r["ExtractSize"], ruta)
    with _toc_lock:
        _toc_cache[path] = d
    return d


def crilayla(data):
    """Descompresor CRILAYLA (formato de compresion de los .cpk de CRI)."""
    if data[:8] != b"CRILAYLA":
        return data
    usize, hoff = struct.unpack_from("<II", data, 8)
    out = bytearray(usize + 0x100)
    out[0:0x100] = data[hoff + 0x10:hoff + 0x10 + 0x100]
    pos = len(data) - 0x100 - 1
    pool = 0
    left = 0

    def bits(n):
        nonlocal pos, pool, left
        v = 0
        got = 0
        while got < n:
            if left == 0:
                pool = data[pos]
                left = 8
                pos -= 1
            take = min(left, n - got)
            v = (v << take) | ((pool >> (left - take)) & ((1 << take) - 1))
            left -= take
            got += take
        return v

    end = 0x100 + usize - 1
    hechos = 0
    vle = (2, 3, 5, 8)
    while hechos < usize:
        if bits(1):
            ref = end - hechos + bits(13) + 3
            largo = 3
            for n in vle:
                x = bits(n)
                largo += x
                if x != (1 << n) - 1:
                    break
            else:
                while True:
                    x = bits(8)
                    largo += x
                    if x != 255:
                        break
            for _ in range(largo):
                out[end - hechos] = out[ref]
                ref -= 1
                hechos += 1
        else:
            out[end - hechos] = bits(8)
            hechos += 1
    return bytes(out)


def leer_de_cpk(cpk, entrada):
    off, fsize, esize, _ = entrada
    with open(cpk, "rb") as f:
        f.seek(off)
        b = f.read(fsize)
    if fsize != esize:
        b = crilayla(b)
    return b


# ================================================================ DDS -> PNG
def _rgb565(c):
    return (((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31)


def _bloque_color(b, o, dxt1):
    c0, c1, idx = struct.unpack_from("<HHI", b, o)
    a, bb = _rgb565(c0), _rgb565(c1)
    if c0 > c1 or not dxt1:
        cols = [a + (255,), bb + (255,),
                tuple((2 * a[i] + bb[i]) // 3 for i in range(3)) + (255,),
                tuple((a[i] + 2 * bb[i]) // 3 for i in range(3)) + (255,)]
    else:
        cols = [a + (255,), bb + (255,), tuple((a[i] + bb[i]) // 2 for i in range(3)) + (255,), (0, 0, 0, 0)]
    return [cols[(idx >> (2 * k)) & 3] for k in range(16)]


def _alfa_dxt5(b, o):
    a0, a1 = b[o], b[o + 1]
    bitsv = int.from_bytes(b[o + 2:o + 8], "little")
    if a0 > a1:
        tab = [a0, a1] + [((7 - i) * a0 + i * a1) // 7 for i in range(1, 7)]
    else:
        tab = [a0, a1] + [((5 - i) * a0 + i * a1) // 5 for i in range(1, 5)] + [0, 255]
    return [tab[(bitsv >> (3 * k)) & 7] for k in range(16)]


def dds_a_rgba(b):
    """Devuelve (ancho, alto, bytes RGBA) o None si el formato no se soporta (p. ej. BC7)."""
    if b[:4] != b"DDS ":
        return None
    alto, ancho = struct.unpack_from("<II", b, 12)
    fcc = b[84:88]
    px = bytearray(ancho * alto * 4)
    o = 128
    if fcc in (b"DXT1", b"DXT3", b"DXT5"):
        tam = 8 if fcc == b"DXT1" else 16
        for by in range(0, alto, 4):
            for bx in range(0, ancho, 4):
                if fcc == b"DXT1":
                    cols = _bloque_color(b, o, True)
                else:
                    cols = _bloque_color(b, o + 8, False)
                    if fcc == b"DXT5":
                        al = _alfa_dxt5(b, o)
                    else:
                        v = int.from_bytes(b[o:o + 8], "little")
                        al = [((v >> (4 * k)) & 15) * 17 for k in range(16)]
                    cols = [c[:3] + (al[k],) for k, c in enumerate(cols)]
                for k in range(16):
                    x, y = bx + (k & 3), by + (k >> 2)
                    if x < ancho and y < alto:
                        p = (y * ancho + x) * 4
                        px[p:p + 4] = bytes(cols[k])
                o += tam
        return ancho, alto, bytes(px)
    if fcc == b"\0\0\0\0" and struct.unpack_from("<I", b, 88)[0] == 32:
        rmask = struct.unpack_from("<I", b, 92)[0]
        for i in range(ancho * alto):
            v = b[o + 4 * i:o + 4 * i + 4]
            px[4 * i:4 * i + 4] = bytes((v[2], v[1], v[0], v[3])) if rmask == 0xFF0000 else v
        return ancho, alto, bytes(px)
    return None


def rgba_a_png(ancho, alto, rgba, fondo=(5, 6, 15)):
    filas = bytearray()
    for y in range(alto):
        filas.append(0)
        for x in range(ancho):
            r, g, b, a = rgba[(y * ancho + x) * 4:(y * ancho + x) * 4 + 4]
            filas += bytes(((r * a + fondo[0] * (255 - a)) // 255, (g * a + fondo[1] * (255 - a)) // 255,
                            (b * a + fondo[2] * (255 - a)) // 255))

    def trozo(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)

    return (b"\x89PNG\r\n\x1a\n" + trozo(b"IHDR", struct.pack(">IIBBBBB", ancho, alto, 8, 2, 0, 0, 0))
            + trozo(b"IDAT", zlib.compress(bytes(filas), 6)) + trozo(b"IEND", b""))


TEXCONV = r"D:\Frank\IA\CLAUDE\herramientas\texconv\texconv.exe"


def texconv_a_png(dds, dst):
    """BC7 y otros formatos DX10: usa texconv.exe (DirectXTex de Microsoft, ya en la PC)."""
    import subprocess, tempfile
    if not os.path.isfile(TEXCONV):
        return False
    d = tempfile.mkdtemp(prefix="phx_tex_")
    src = os.path.join(d, "m.dds")
    with open(src, "wb") as f:
        f.write(dds)
    try:
        subprocess.run([TEXCONV, "-nologo", "-y", "-ft", "png", "-f", "R8G8B8A8_UNORM", "-o", d, src],
                       capture_output=True, timeout=30, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        png = os.path.join(d, "m.png")
        if os.path.isfile(png):
            os.replace(png, dst)
            return True
    except Exception:
        pass
    return False


# ================================================================ indice
def _prec(fila):
    """Orden en que cada contenedor se aplica (el ultimo gana)."""
    t, c, pr = fila["tipo"], fila["contenedor"], fila["prioridad"]
    if t == "cpk":
        return (1 if c.lower().startswith("download") else 0, c.lower())
    if t == "content":
        return (2, c.lower())
    return (3, -int(pr) if pr not in ("", None) else -9999, "")


def _es_prdx(c):
    return "prdx" in c.lower()


def _norm(s):
    s = s.lower()
    for a, b in (("á", "a"), ("é", "e"), ("í", "i"), ("ó", "o"), ("ú", "u"), ("ñ", "n"), ("ã", "a"), ("ç", "c")):
        s = s.replace(a, b)
    return re.sub(r"[^a-z ]", "", s).strip()


def misma_persona(a, b):
    a, b = _norm(a), _norm(b)
    if not a or not b:
        return True
    if a in b or b in a or a.split()[-1] == b.split()[-1]:
        return True
    return difflib.SequenceMatcher(None, a, b).ratio() >= 0.6


class Indice:
    def __init__(self, terreno=TERRENO):
        self.terreno = terreno
        self.caras = {}     # id -> fam -> [filas] (sin PRDX)
        self.prdx = {}      # id -> set(fam)
        self.mini = {}      # id -> fam -> [filas]
        self.nombres = {}   # (fam, id) -> (nombre, equipos)
        self.alto = set()
        self._cargar()

    def _cargar(self):
        for r in csv.DictReader(open(os.path.join(self.terreno, "inventario_caras.csv"), encoding="utf-8-sig")):
            if (r["tipo"] == "livecpk" and r["activo"] != "si") or r["tipo"] == "content":
                continue  # content\* (p. ej. Master-League-Selector) no es ruta directa del juego
            fam = FAM[r["fuente"]]
            if _es_prdx(r["contenedor"]):
                self.prdx.setdefault(r["id"], set()).add(fam)
                continue
            self.caras.setdefault(r["id"], {}).setdefault(fam, []).append(r)
        for r in csv.DictReader(open(os.path.join(self.terreno, "inventario_minicaras.csv"), encoding="utf-8-sig")):
            if r["tipo"] == "livecpk" and r["activo"] != "si":
                continue
            if r["via"] == "miniface_server":
                continue
            self.mini.setdefault(r["id"], {}).setdefault(FAM[r["fuente"]], []).append(r)
        p = os.path.join(self.terreno, "nombres_por_parche.csv")
        if os.path.isfile(p):
            for r in csv.DictReader(open(p, encoding="utf-8-sig")):
                self.nombres[(r["fuente"], r["id"])] = (r["nombre"], r["equipos"])
        p = os.path.join(self.terreno, "choques_identidad_jugadores.csv")
        if os.path.isfile(p):
            for r in csv.DictReader(open(p, encoding="utf-8-sig")):
                if r["riesgo"].startswith("ALTO"):
                    self.alto.add(r["id"])
        # quitar duplicado WC26 cuando el Sudamerican principal ya trae ese id
        for d in list(self.caras.values()) + list(self.mini.values()):
            if "sudamerican" in d:
                prin = [x for x in d["sudamerican"] if x["fuente"] == "sudamerican"]
                if prin:
                    d["sudamerican"] = prin

    def nombre(self, fam, id_):
        return self.nombres.get((fam, id_), ("", ""))

    def otra_persona(self, id_):
        ns = [self.nombre(f, id_)[0] for f in VERSIONES if self.nombre(f, id_)[0]]
        return any(not misma_persona(a, b) for i, a in enumerate(ns) for b in ns[i + 1:])

    def jugadores(self):
        """Ids con cara en 2+ versiones: primero ALTO, luego «otra persona», luego el resto."""
        out = []
        for id_, d in self.caras.items():
            if len(d) < 2:
                continue
            grupo = 0 if id_ in self.alto else (1 if self.otra_persona(id_) else 2)
            out.append((grupo, int(id_), id_))
        out.sort()
        return [(g, i) for g, _, i in out]

    def etiqueta(self, id_):
        ns = []
        for f in VERSIONES:
            n = self.nombre(f, id_)[0]
            if n and _norm(n) not in [_norm(x) for x in ns]:
                ns.append(n)
        return " / ".join(ns) or "(sin nombre)"

    def texto_busqueda(self, id_):
        t = [id_]
        for f in VERSIONES:
            t += list(self.nombre(f, id_))
        return " ".join(t).lower()

    # ------------------------------------------------ archivos de una version
    def archivos(self, id_, fam, filas=None):
        """{ruta_logica_min: dict(origen='suelto'|'cpk', ruta_logica, ruta|cpk+entrada, bytes)}"""
        filas = filas if filas is not None else self.caras.get(id_, {}).get(fam, [])
        res = {}
        for r in sorted(filas, key=_prec):
            ej = r["ejemplo"].replace("\\", "/")
            m = re.search(r"(?i)^(.*?/real/%s)/" % re.escape(id_), ej) or \
                re.search(r"(?i)^(.*?/symbol/player/)%s\.dds$" % re.escape(id_), ej)
            if not m:
                continue
            pref = m.group(1)
            mini = pref.lower().endswith("/player/")
            if r["tipo"] == "cpk":
                cpk = os.path.join(JUEGOS[r["fuente"]], *r["contenedor"].split("/"))
                try:
                    t = toc(cpk)
                except Exception:
                    continue
                pl = pref.lower()
                claves = [pl + id_ + ".dds"] if mini else [k for k in t if k.startswith(pl + "/")]
                for k in claves:
                    if k in t:
                        e = t[k]
                        res[k] = {"origen": "cpk", "ruta_logica": e[3], "cpk": cpk, "entrada": e, "bytes": e[2]}
            else:
                raiz = r["contenedor"].replace("<sider>", SIDERS[r["fuente"]])
                if r["tipo"] == "content":
                    raiz = os.path.dirname(raiz)
                base = os.path.join(raiz, *pref.split("/"))
                if mini:
                    p = base + id_ + ".dds"
                    if os.path.isfile(p):
                        res[(pref + id_ + ".dds").lower()] = {"origen": "suelto", "ruta_logica": pref + id_ + ".dds",
                                                              "ruta": p, "bytes": os.path.getsize(p)}
                    continue
                for dp, _, fs in os.walk(base):
                    for fn in fs:
                        p = os.path.join(dp, fn)
                        rl = pref + "/" + os.path.relpath(p, base).replace("\\", "/")
                        res[rl.lower()] = {"origen": "suelto", "ruta_logica": rl, "ruta": p,
                                           "bytes": os.path.getsize(p)}
        return res

    def leer(self, a):
        if a["origen"] == "cpk":
            return leer_de_cpk(a["cpk"], a["entrada"])
        with open(a["ruta"], "rb") as f:
            return f.read()

    def info(self, id_, fam):
        arch = self.archivos(id_, fam)
        res = ""
        for k, a in arch.items():
            if k.endswith("face_bsm_alp.ftex") or k.endswith("face_bsm.ftex"):
                try:
                    if a["origen"] == "suelto":
                        with open(a["ruta"], "rb") as fh:
                            h = fh.read(16)
                    elif a["entrada"][1] == a["entrada"][2]:
                        h = leer_de_cpk(a["cpk"], (a["entrada"][0], 16, 16, ""))
                    else:
                        h = self.leer(a)[:16]
                    if h[:4] == b"FTEX":
                        res = "%dx%d" % struct.unpack_from("<HH", h, 10)
                except Exception:
                    pass
                break
        origen = sorted({a["origen"] for a in arch.values()})
        return {"archivos": len(arch), "bytes": sum(a["bytes"] for a in arch.values()),
                "resolucion": res, "origen": "+".join(origen), "prdx": fam in self.prdx.get(id_, set())}

    def minicara_png(self, id_, fam, cache_dir):
        """(ruta PNG en cache o None, resolucion o motivo)."""
        os.makedirs(cache_dir, exist_ok=True)
        dst = os.path.join(cache_dir, "%s_%s.png" % (fam, id_))
        meta = dst + ".txt"
        if os.path.isfile(dst):
            return dst, open(meta).read() if os.path.isfile(meta) else ""
        filas = self.mini.get(id_, {}).get(fam, [])
        arch = self.archivos(id_, fam, filas)
        if not arch:
            return None, "sin minicara"
        b = self.leer(list(arch.values())[-1])
        try:
            r = dds_a_rgba(b)
        except Exception:
            r = None
        if r:
            w, h, px = r
            with open(dst, "wb") as f:
                f.write(rgba_a_png(w, h, px))
        elif not texconv_a_png(b, dst):
            return None, "formato no soportado"
        res = "%dx%d" % struct.unpack_from("<II", b, 12)[::-1] if b[:4] == b"DDS " else ""
        with open(meta, "w") as f:
            f.write(res)
        return dst, res


# ================================================================ elecciones y paquete
class Elecciones:
    def __init__(self, launcher):
        self.l = launcher
        self.ruta = os.path.join(launcher.pdir, "caras_elegidas.json")
        try:
            self.datos = json.load(open(self.ruta, encoding="utf-8"))
        except Exception:
            self.datos = {}
        self.datos.setdefault("V1", {})
        self.datos.setdefault("V2", {})

    def _guardar(self):
        tmp = self.ruta + ".tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(self.datos, f, ensure_ascii=False, indent=1)
        os.replace(tmp, self.ruta)

    def de_perfil(self, p=None):
        return self.datos[p or self.l.perfil]

    def elegir(self, id_, opcion, nombre=""):
        if self.l.perfil == "V1":
            raise PermissionError("V1 estable esta congelado. Pasa a V2 en pruebas para elegir caras.")
        d = self.de_perfil()
        if opcion == "defecto":
            d.pop(id_, None)
        else:
            d[id_] = {"version": opcion, "nombre": nombre, "fecha": ahora().strftime("%Y-%m-%d %H:%M")}
        self._guardar()
        txt = OPCIONES_EXTRA.get(opcion) or NOMBRE_VERSION.get(opcion, opcion)
        self.l.registrar("CARA elegida: id %s (%s) -> %s" % (id_, nombre or "?", txt))

    def copiar_v2_a_v1(self):
        self.datos["V1"] = json.loads(json.dumps(self.datos["V2"]))
        self._guardar()


def carpeta_paquete(paquetes=PAQUETES):
    """Un solo paquete: se regenera con las elecciones del perfil activo al pulsar «Aplicar caras»."""
    return os.path.join(paquetes, NOMBRE_PAQUETE)


def planificar(indice, elecciones, destino):
    """Pasos [(ruta_destino, archivo, 'enlace'|'copia')] y resumen. «juego» = cara del juego base."""
    unidad = os.path.splitdrive(os.path.abspath(destino))[0].lower()
    pasos, avisos = [], []
    for id_, e in sorted(elecciones.items()):
        fam = "base" if e["version"] == "juego" else e["version"]
        arch = indice.archivos(id_, fam)
        if not arch:
            avisos.append("id %s: no hay cara de %s; no se pone nada" % (id_, NOMBRE_VERSION.get(fam, fam)))
            continue
        for a in arch.values():
            dst = os.path.join(destino, *a["ruta_logica"].split("/"))
            enl = a["origen"] == "suelto" and os.path.splitdrive(a["ruta"])[0].lower() == unidad
            pasos.append((dst, a, "enlace" if enl else "copia"))
    return pasos, {"enlaces": sum(1 for p in pasos if p[2] == "enlace"),
                   "copias": sum(1 for p in pasos if p[2] == "copia"),
                   "bytes_copia": sum(a["bytes"] for _, a, k in pasos if k == "copia"), "avisos": avisos}


def aplicar(indice, pasos, destino):
    """Construye el paquete en una carpeta nueva y la cambia por la anterior (que se guarda en _anteriores)."""
    sello = ahora().strftime("%Y%m%d_%H%M%S")
    nuevo = destino + ".nuevo_" + sello
    os.makedirs(nuevo)
    hechos = {"enlaces": 0, "copias": 0}
    for dst, a, kind in pasos:
        d2 = os.path.join(nuevo, os.path.relpath(dst, destino))
        os.makedirs(os.path.dirname(d2), exist_ok=True)
        if kind == "enlace":
            try:
                os.link(a["ruta"], d2)
                hechos["enlaces"] += 1
                continue
            except OSError:
                pass
        with open(d2, "wb") as f:
            f.write(indice.leer(a))
        hechos["copias"] += 1
    with open(os.path.join(nuevo, "LEEME_PHX_FAC_SEL.txt"), "w", encoding="utf-8") as f:
        f.write("Caras elegidas a mano con Phoenix Launcher (%s). Lo genera el launcher; no editar a mano.\n" % sello)
    if os.path.isdir(destino):
        ant = os.path.join(os.path.dirname(destino), "_anteriores")
        os.makedirs(ant, exist_ok=True)
        os.replace(destino, os.path.join(ant, NOMBRE_PAQUETE + "_" + sello))
    os.replace(nuevo, destino)
    return hechos
