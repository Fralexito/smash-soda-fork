"""Actualización en vivo propia: un .cpk en la carpeta «actualizacion» se ofrece a cada PES por
CMD_GET_DLC_FILELIST, y el propio PES lo baja con su descargador (un GET HTTP, comprobado por SHA-256).

Para qué: dos amigos solo pueden jugar si sus datos de jugadores son idénticos (las sumas de CHECK_CHEAT).
Si el servidor publica la plantilla, todos quedan con los mismos datos.

Reglas vistas en vivo por AzrakHQ (05/06-nov, R: liveupdate.py, MIT):
- la fila es un mapa {name, size, url, hash (sha256 en minúsculas), dlc_version}. dlc_version es la versión
  de la actualización en texto entero («1.02» en version_wu.bin -> «102») y tiene que ser MAYOR que la que
  el PES tiene guardada, si no, no la baja;
- hay que SEGUIR ofreciéndola: el PES vuelve a preguntar al entrar a cada modo en línea; si la lista llega
  vacía calcula sus sumas con los datos base, aunque la actualización siga guardada;
- el .cpk debe llevar common/etc/version/version_wu.bin (tres campos de 10 bytes: versión de datos,
  versión del juego, etiqueta).
Gana el .cpk más nuevo de la carpeta. Para dejar de ofrecerlo, se quita el archivo.

El lector de CPK es propio (formato CRI CPK: cabecera «CPK », tablas @UTF, a veces enmascaradas).
"""
import hashlib
import logging
import os
import re
import struct

log = logging.getLogger("phoenix_online.actualizacion")
ARCHIVO_VERSION = "common/etc/version/version_wu.bin"
_CACHE = {}  # ruta -> (mtime, tamaño, ficha o None)


def _desenmascarar(b):
    out = bytearray(len(b))
    m = 0x655F
    for i, c in enumerate(b):
        out[i] = c ^ (m & 0xFF)
        m = (m * 0x4115) & 0xFFFFFFFF
    return bytes(out)


def _tabla_utf(crudo):
    if crudo[:4] != b"@UTF":
        crudo = _desenmascarar(crudo)
    if crudo[:4] != b"@UTF":
        raise ValueError("tabla @UTF ilegible")
    tam, filas_off, textos_off, datos_off, _nombre, ncols, largo_fila, nfilas = struct.unpack(">IIIIIHHI", crudo[4:32])
    t = crudo[8:8 + tam]
    textos = t[textos_off:datos_off]

    def texto(o):
        fin = textos.index(b"\0", o)
        return textos[o:fin].decode("utf-8", "replace")

    formatos = {0: "B", 1: "b", 2: "H", 3: "h", 4: "I", 5: "i", 6: "Q", 7: "q", 8: "f", 9: "d"}

    def leer(p, tipo):
        if tipo in formatos:
            f = ">" + formatos[tipo]
            return struct.unpack_from(f, t, p)[0], p + struct.calcsize(f)
        if tipo == 0xA:
            return texto(struct.unpack_from(">I", t, p)[0]), p + 4
        if tipo == 0xB:
            o, n = struct.unpack_from(">II", t, p)
            return (datos_off + o, n), p + 8
        raise ValueError(f"tipo {tipo}")

    cols, p = [], 24
    for _ in range(ncols):
        banderas = t[p]
        p += 1
        nombre = texto(struct.unpack_from(">I", t, p)[0])
        p += 4
        clase, tipo = banderas & 0xF0, banderas & 0x0F
        fijo = None
        if clase == 0x30:
            fijo, p = leer(p, tipo)
        cols.append((nombre, clase, tipo, fijo))
    filas = []
    for r in range(nfilas):
        p, fila = filas_off + r * largo_fila, {}
        for nombre, clase, tipo, fijo in cols:
            if clase == 0x50:
                fila[nombre], p = leer(p, tipo)
            else:
                fila[nombre] = fijo if clase == 0x30 else None
        filas.append(fila)
    return filas


def indice_cpk(ruta):
    with open(ruta, "rb") as f:
        cab = f.read(16)
        if cab[:4] != b"CPK ":
            raise ValueError("no es un CPK")
        tam = struct.unpack("<Q", cab[8:16])[0]
        info = _tabla_utf(f.read(tam))[0]
        toc, contenido = info.get("TocOffset"), info.get("ContentOffset")
        if not toc:
            return info, []
        f.seek(toc)
        cab = f.read(16)
        filas = _tabla_utf(f.read(struct.unpack("<Q", cab[8:16])[0]))
        base = min(toc, contenido) if contenido else toc
        for r in filas:
            r["_abs"] = base + r["FileOffset"]
        return info, filas


def leer_de_cpk(ruta, interno):
    """Bytes de un archivo guardado SIN comprimir dentro del CPK, o None."""
    for r in indice_cpk(ruta)[1]:
        nombre = ((r.get("DirName") or "") + "/" if r.get("DirName") else "") + (r.get("FileName") or "")
        if nombre == interno and r.get("FileSize") == r.get("ExtractSize"):
            with open(ruta, "rb") as f:
                f.seek(r["_abs"])
                return f.read(r["FileSize"])
    return None


def _version(ruta):
    crudo = leer_de_cpk(ruta, ARCHIVO_VERSION)
    if not crudo:
        return None
    v = crudo[:10].split(b"\0", 1)[0].decode("ascii", "replace").replace(".", "")
    return v if re.fullmatch(r"\d{1,6}", v) and int(v) > 0 else None


def actual(carpeta):
    """(ruta, {name, size, hash, dlc_version}) de la actualización a ofrecer, o None."""
    try:
        archivos = [os.path.join(carpeta, x) for x in os.listdir(carpeta) if x.lower().endswith(".cpk")]
    except OSError:
        return None
    for ruta in sorted(archivos, key=os.path.getmtime, reverse=True):
        st = os.stat(ruta)
        c = _CACHE.get(ruta)
        if c is None or c[:2] != (st.st_mtime, st.st_size):
            ficha = None
            try:
                ver = _version(ruta)
                if ver is None:
                    log.warning("actualización %s sin %s: no se ofrece", os.path.basename(ruta), ARCHIVO_VERSION)
                else:
                    h = hashlib.sha256()
                    with open(ruta, "rb") as f:
                        for trozo in iter(lambda: f.read(1 << 20), b""):
                            h.update(trozo)
                    ficha = {"name": os.path.basename(ruta), "size": st.st_size, "hash": h.hexdigest(),
                             "dlc_version": ver}
                    log.info("actualización ofrecida: %s (versión %s, %d bytes)", ficha["name"], ver, st.st_size)
            except (OSError, ValueError, KeyError, IndexError, struct.error) as e:
                log.warning("actualización %s ilegible: %s", os.path.basename(ruta), e)
            c = _CACHE[ruta] = (st.st_mtime, st.st_size, ficha)
        if c[2] is not None:
            return ruta, c[2]
    return None


def fila(carpeta, url_base):
    a = actual(carpeta)
    return None if a is None else {**a[1], "url": f"{url_base}/actualizacion/{a[1]['name']}"}


def archivo_para(carpeta, nombre):
    """Solo el archivo que se ofrece ahora (ninguna otra ruta del disco)."""
    a = actual(carpeta)
    if a is None or a[1]["name"].lower() != str(nombre).lower():
        return None
    return a[0]


# ---------- Escritor de CPK (sin comprimir, solo TOC), con la forma de los CPK del propio PES ----------
# Datos del formato (R: notas de AzrakHQ sobre dt80_100E_x64.cpk, CPKMC2.49.32):
#   0x000 bloque 'CPK ' (u32 0, u64 largo, tabla @UTF «CpkHeader» enmascarada), en 0x7fa '(c)CRI'
#   0x800 bloque 'TOC ' (tabla «CpkTocInfo»: DirName, FileName, FileSize, ExtractSize, FileOffset, ID, UserString)
#   contenido alineado a 0x800; FileOffset relativo a 0x800; filas ordenadas por ruta en MAYÚSCULAS;
#   EnabledPackedSize = EnabledDataSize = 2 * suma de tamaños (rareza de los oficiales).
_ALINEA = 0x800
_FMT = {0: "B", 1: "b", 2: "H", 3: "h", 4: "I", 5: "i", 6: "Q", 7: "q", 8: "f", 9: "d"}
_CERO, _FIJO, _FILA = 0x10, 0x30, 0x50
_U16, _U32, _U64, _TXT, _DAT = 2, 4, 6, 0xA, 0xB


def _alinear(x):
    return (x + _ALINEA - 1) // _ALINEA * _ALINEA


def _armar_utf(nombre, columnas, filas):
    """columnas: [(nombre, clase, tipo, valor_fijo)]; filas: [dict]. Devuelve la tabla SIN enmascarar."""
    textos, indice, datos = bytearray(b"<NULL>\0"), {"<NULL>": 0}, bytearray()

    def s(x):
        if x not in indice:
            indice[x] = len(textos)
            textos.extend(x.encode("utf-8") + b"\0")
        return indice[x]

    nombre_tabla = s(nombre)

    def celda(tipo, v):
        if tipo in _FMT:
            return struct.pack(">" + _FMT[tipo], v)
        if tipo == _TXT:
            return struct.pack(">I", s(v))
        if tipo == _DAT:
            o = len(datos)
            datos.extend(v)
            return struct.pack(">II", o, len(v))
        raise ValueError(tipo)

    esquema = bytearray()
    for n, clase, tipo, fijo in columnas:
        esquema += struct.pack(">BI", clase | tipo, s(n))
        if clase == _FIJO:
            esquema += celda(tipo, fijo)
    cuerpo, largo = bytearray(), 0
    for f in filas:
        antes = len(cuerpo)
        for n, clase, tipo, _ in columnas:
            if clase == _FILA:
                cuerpo += celda(tipo, f[n])
        largo = len(cuerpo) - antes
    filas_off = 0x18 + len(esquema)
    textos_off = filas_off + len(cuerpo)
    relleno = (-(textos_off + len(textos))) % 8
    datos_off = textos_off + len(textos) + relleno
    tam = datos_off + len(datos)
    cab = struct.pack(">4sIIIIIHHI", b"@UTF", tam, filas_off, textos_off, datos_off, nombre_tabla,
                      len(columnas), largo, len(filas))
    return cab + esquema + cuerpo + textos + bytes(relleno) + datos


def _bloque(magia, tabla):
    return magia + struct.pack("<IQ", 0, len(tabla)) + _desenmascarar(tabla)  # la máscara es simétrica


def escribir_cpk(salida, archivos):
    """archivos: [(ruta_dentro_del_cpk, ruta_local)]. CPK sin comprimir, como los del PES."""
    archivos = sorted(archivos, key=lambda a: a[0].upper())
    nombres = [a[0] for a in archivos]
    if len({n.upper() for n in nombres}) != len(nombres):
        raise ValueError("rutas repetidas en el CPK")
    tams = [os.path.getsize(a[1]) for a in archivos]
    cols_toc = [("DirName", _FILA, _TXT, None), ("FileName", _FILA, _TXT, None), ("FileSize", _FILA, _U32, None),
                ("ExtractSize", _FILA, _U32, None), ("FileOffset", _FILA, _U64, None), ("ID", _FILA, _U32, None),
                ("UserString", _FIJO, _TXT, "<NULL>")]

    def filas_toc(inicio):
        filas, pos = [], inicio
        for i, (ruta, tam) in enumerate(zip(nombres, tams)):
            carpeta, _, nombre = ruta.rpartition("/")
            filas.append({"DirName": carpeta, "FileName": nombre, "FileSize": tam, "ExtractSize": tam,
                          "FileOffset": pos - _ALINEA, "ID": i})
            pos = _alinear(pos + tam)
        return filas, pos

    tam_toc = 16 + len(_armar_utf("CpkTocInfo", cols_toc, filas_toc(_ALINEA)[0]))  # celdas de ancho fijo
    contenido = _alinear(_ALINEA + tam_toc)
    filas, fin = filas_toc(contenido)
    toc = _bloque(b"TOC ", _armar_utf("CpkTocInfo", cols_toc, filas))
    total = sum(tams)
    H = [("UpdateDateTime", _FILA, _U64, 0), ("FileSize", _CERO, _U64, None), ("ContentOffset", _FILA, _U64, contenido),
         ("ContentSize", _FILA, _U64, fin - contenido), ("TocOffset", _FILA, _U64, _ALINEA),
         ("TocSize", _FILA, _U64, tam_toc), ("TocCrc", _CERO, _U32, None), ("HtocOffset", _CERO, _U64, None),
         ("HtocSize", _CERO, _U64, None), ("EtocOffset", _CERO, _U64, None), ("EtocSize", _CERO, _U64, None),
         ("ItocOffset", _CERO, _U64, None), ("ItocSize", _CERO, _U64, None), ("ItocCrc", _CERO, _U32, None),
         ("GtocOffset", _CERO, _U64, None), ("GtocSize", _CERO, _U64, None), ("GtocCrc", _CERO, _U32, None),
         ("HgtocOffset", _CERO, _U64, None), ("HgtocSize", _CERO, _U64, None),
         ("EnabledPackedSize", _FILA, _U64, 2 * total), ("EnabledDataSize", _FILA, _U64, 2 * total),
         ("TotalDataSize", _CERO, _U64, None), ("Tocs", _CERO, _U32, None), ("Files", _FILA, _U32, len(archivos)),
         ("Groups", _FILA, _U32, 0), ("Attrs", _FILA, _U32, 0), ("TotalFiles", _CERO, _U32, None),
         ("Directories", _CERO, _U32, None), ("Updates", _CERO, _U32, None), ("Version", _FILA, _U16, 7),
         ("Revision", _FILA, _U16, 14), ("Align", _FILA, _U16, _ALINEA), ("Sorted", _FILA, _U16, 1),
         ("EnableFileName", _FILA, _U16, 1), ("EID", _CERO, _U16, None), ("CpkMode", _FILA, _U32, 1),
         ("Tvers", _FILA, _TXT, "CPKMC2.49.32, DLL3.24.00"), ("Comment", _CERO, _TXT, None),
         ("Codec", _FILA, _U32, 0), ("DpkItoc", _FILA, _U32, 0), ("EnableTocCrc", _FILA, _U16, 0),
         ("EnableFileCrc", _FILA, _U16, 0), ("CrcMode", _FILA, _U32, 0), ("CrcTable", _FILA, _DAT, b"")]
    cab = _bloque(b"CPK ", _armar_utf("CpkHeader", [(n, c, t, None) for n, c, t, _ in H],
                                      [{n: v for n, _, _, v in H}]))
    if len(cab) > _ALINEA - 6:
        raise ValueError("cabecera demasiado grande")
    with open(salida, "wb") as o:
        o.write(cab + bytes(_ALINEA - 6 - len(cab)) + b"(c)CRI")
        o.write(toc)
        o.write(bytes(contenido - _ALINEA - tam_toc))
        for (_, local), fila in zip(archivos, filas):
            if o.tell() != fila["FileOffset"] + _ALINEA:
                raise ValueError("desalineado")
            with open(local, "rb") as f:
                for trozo in iter(lambda: f.read(1 << 20), b""):
                    o.write(trozo)
            o.write(bytes(_alinear(o.tell()) - o.tell()))
    return salida


def archivo_version(version, juego="1.01.00", etiqueta="Phoenix"):
    """version_wu.bin: 3 campos de 10 bytes (versión de datos «1.02», versión del juego, etiqueta)."""
    return b"".join(str(x).encode("ascii")[:10].ljust(10, b"\0") for x in (version, juego, etiqueta))
