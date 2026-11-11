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
