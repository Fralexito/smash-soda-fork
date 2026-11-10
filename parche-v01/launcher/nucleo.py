# -*- coding: utf-8 -*-
"""Phoenix Launcher - nucleo (sin interfaz). Se prueba sin abrir PES.

Todo lo que toca disco pasa por aqui:
- leer sider.ini y sacar los interruptores (cpk.root / lua.module)
- encender/apagar = quitar/poner ';' al principio de la linea
- copia sider.ini.bak_<fecha> antes de cada guardado + pila de deshacer
- perfiles V1 estable (congelado) y V2 en pruebas
- CAMBIOS.md automatico
"""
import os, re, json, shutil, datetime, subprocess

CLAVES = ("cpk.root", "lua.module")
RX = re.compile(r'^(?P<pre>\s*)(?P<off>;?)\s*(?P<clave>cpk\.root|lua\.module)\s*=\s*"(?P<valor>[^"]*)"\s*$')
PERFILES = {"V1": "V1_estable", "V2": "V2_pruebas"}
NOMBRE_PERFIL = {"V1": "V1 estable", "V2": "V2 en pruebas"}


def ahora():
    return datetime.datetime.now()


# ---------------------------------------------------------------- leyenda
def leer_leyenda(ruta):
    """Devuelve {nombre_tecnico_minusculas: nombre_amigable}."""
    out = {}
    if not ruta or not os.path.isfile(ruta):
        return out
    with open(ruta, encoding="utf-8", errors="replace") as f:
        for linea in f:
            linea = linea.rstrip("\n")
            m = re.match(r"^(PHX_[A-Z0-9_]+|[\w.-]+\.lua)\s{2,}(.+)$", linea.strip())
            if not m:
                continue
            tecnico, resto = m.group(1), m.group(2)
            amigable = re.split(r"\s{2,}|\(antes", resto)[0].strip()
            out[tecnico.lower()] = amigable
    return out


def nombre_corto(valor):
    return re.split(r"[\\/]", valor.rstrip("\\/"))[-1]


# ---------------------------------------------------------------- ini
def leer_texto(ruta):
    with open(ruta, "rb") as f:
        datos = f.read()
    nl = "\r\n" if b"\r\n" in datos else "\n"
    texto = datos.decode("utf-8", errors="surrogateescape")
    return texto.replace("\r\n", "\n").split("\n"), nl


def escribir_texto(ruta, lineas, nl):
    datos = nl.join(lineas).encode("utf-8", errors="surrogateescape")
    tmp = ruta + ".tmp_launcher"
    with open(tmp, "wb") as f:
        f.write(datos)
    os.replace(tmp, ruta)


def interruptores(lineas, leyenda=None):
    leyenda = leyenda or {}
    res = []
    for i, l in enumerate(lineas):
        m = RX.match(l)
        if not m:
            continue
        corto = nombre_corto(m.group("valor"))
        res.append({
            "linea": i,
            "clave": m.group("clave"),
            "valor": m.group("valor"),
            "tecnico": corto,
            "amigable": leyenda.get(corto.lower(), ""),
            "encendido": m.group("off") == "",
        })
    return res


def alternar_linea(linea, encender):
    m = RX.match(linea)
    if not m:
        raise ValueError("no es una linea de cpk.root/lua.module: %r" % linea)
    cuerpo = '%s = "%s"' % (m.group("clave"), m.group("valor"))
    return m.group("pre") + ("" if encender else ";") + cuerpo


# ---------------------------------------------------------------- launcher
class Launcher:
    def __init__(self, sider_dir, leyenda_ruta=None):
        self.sider = os.path.abspath(sider_dir)
        self.ini = os.path.join(self.sider, "sider.ini")
        self.pdir = os.path.join(self.sider, "perfiles")
        self.cambios = os.path.join(self.sider, "CAMBIOS.md")
        self.estado_ruta = os.path.join(self.pdir, "estado.json")
        os.makedirs(self.pdir, exist_ok=True)
        if leyenda_ruta is None:
            leyenda_ruta = os.path.join(self.pdir, "LEYENDA_NOMBRES.txt")
        self.leyenda = leer_leyenda(leyenda_ruta)
        self.estado = self._leer_estado()
        self._iniciar_perfiles()

    # --- estado persistente
    def _leer_estado(self):
        try:
            with open(self.estado_ruta, encoding="utf-8") as f:
                e = json.load(f)
        except Exception:
            e = {}
        e.setdefault("perfil", "V2")
        e.setdefault("deshacer", [])
        return e

    def _guardar_estado(self):
        tmp = self.estado_ruta + ".tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(self.estado, f, ensure_ascii=False, indent=1)
        os.replace(tmp, self.estado_ruta)

    def ruta_perfil(self, p):
        return os.path.join(self.pdir, PERFILES[p], "sider.ini")

    def _iniciar_perfiles(self):
        """Primera vez: V1 y V2 nacen como copia del sider.ini actual."""
        for p in PERFILES:
            r = self.ruta_perfil(p)
            if not os.path.isfile(r):
                os.makedirs(os.path.dirname(r), exist_ok=True)
                shutil.copy2(self.ini, r)
                self.registrar("Perfil %s creado como copia del sider.ini actual" % NOMBRE_PERFIL[p])
        self._guardar_estado()

    @property
    def perfil(self):
        return self.estado["perfil"]

    # --- lectura
    def leer(self):
        lineas, _ = leer_texto(self.ini)
        return interruptores(lineas, self.leyenda)

    # --- respaldo / deshacer
    def respaldar(self, motivo):
        sello = ahora().strftime("%Y%m%d_%H%M%S_%f")
        bak = os.path.join(self.sider, "sider.ini.bak_" + sello)
        shutil.copy2(self.ini, bak)
        self.estado["deshacer"].append({"bak": bak, "perfil": self.perfil, "motivo": motivo})
        self.estado["deshacer"] = self.estado["deshacer"][-50:]
        self._guardar_estado()
        return bak

    def deshacer(self):
        pila = self.estado["deshacer"]
        while pila:
            paso = pila.pop()
            if os.path.isfile(paso["bak"]):
                shutil.copy2(paso["bak"], self.ini)
                self.estado["perfil"] = paso.get("perfil", self.perfil)
                if self.estado["perfil"] == "V2":
                    shutil.copy2(self.ini, self.ruta_perfil("V2"))
                self._guardar_estado()
                self.registrar("DESHACER: %s (vuelve %s)" % (paso["motivo"], os.path.basename(paso["bak"])))
                return paso["motivo"]
        self._guardar_estado()
        return None

    # --- cambios
    def _sincronizar_perfil(self):
        # V1 es congelado: solo cambia con guardar_v1()
        if self.perfil == "V2":
            shutil.copy2(self.ini, self.ruta_perfil("V2"))

    def alternar(self, indice_linea, encender):
        if self.perfil == "V1":
            raise PermissionError("V1 estable esta congelado. Pasa a V2 en pruebas para cambiar cosas.")
        lineas, nl = leer_texto(self.ini)
        it = [x for x in interruptores(lineas, self.leyenda) if x["linea"] == indice_linea]
        if not it:
            raise ValueError("linea %d no es un interruptor" % indice_linea)
        it = it[0]
        if it["encendido"] == encender:
            return False
        accion = "ENCENDIDO" if encender else "APAGADO"
        motivo = "%s %s %s" % (accion, it["clave"], it["tecnico"])
        self.respaldar(motivo)
        lineas[indice_linea] = alternar_linea(lineas[indice_linea], encender)
        escribir_texto(self.ini, lineas, nl)
        self._sincronizar_perfil()
        amig = (" (%s)" % it["amigable"]) if it["amigable"] else ""
        self.registrar("%s: %s `%s`%s" % (accion, it["clave"], it["tecnico"], amig))
        return True

    def mover(self, indice_linea, direccion):
        """Sube (-1) o baja (+1) un interruptor dentro de su mismo tipo (prioridad)."""
        if self.perfil == "V1":
            raise PermissionError("V1 estable esta congelado. Pasa a V2 en pruebas para cambiar cosas.")
        lineas, nl = leer_texto(self.ini)
        its = interruptores(lineas, self.leyenda)
        act = [x for x in its if x["linea"] == indice_linea]
        if not act:
            return False
        act = act[0]
        mismos = [x for x in its if x["clave"] == act["clave"]]
        k = mismos.index(act) + direccion
        if k < 0 or k >= len(mismos):
            return False
        otro = mismos[k]
        self.respaldar("ORDEN %s %s" % (act["tecnico"], "arriba" if direccion < 0 else "abajo"))
        a, b = act["linea"], otro["linea"]
        lineas[a], lineas[b] = lineas[b], lineas[a]
        escribir_texto(self.ini, lineas, nl)
        self._sincronizar_perfil()
        self.registrar("ORDEN: `%s` %s (cambia de puesto con `%s`)" % (
            act["tecnico"], "sube" if direccion < 0 else "baja", otro["tecnico"]))
        return True

    def asegurar_raiz(self, ruta, motivo="paquete"):
        """Deja `cpk.root = "<ruta>"` ENCENDIDA y la PRIMERA de los cpk.root (gana a todos).
        Devuelve True si cambio algo. Respaldo + CAMBIOS.md como cualquier cambio."""
        if self.perfil == "V1":
            raise PermissionError("V1 estable esta congelado. Pasa a V2 en pruebas para cambiar cosas.")
        lineas, nl = leer_texto(self.ini)
        its = interruptores(lineas, self.leyenda)
        roots = [x for x in its if x["clave"] == "cpk.root"]
        mio = [x for x in roots if os.path.normcase(x["valor"]) == os.path.normcase(ruta)]
        nueva = 'cpk.root = "%s"' % ruta
        if mio and mio[0]["encendido"] and roots[0]["linea"] == mio[0]["linea"]:
            return False
        self.respaldar("PAQUETE %s arriba y encendido" % nombre_corto(ruta))
        if mio:
            del lineas[mio[0]["linea"]]
            lineas2 = lineas
        else:
            lineas2 = lineas
        its2 = interruptores(lineas2, self.leyenda)
        roots2 = [x for x in its2 if x["clave"] == "cpk.root"]
        pos = roots2[0]["linea"] if roots2 else len(lineas2)
        lineas2.insert(pos, nueva)
        escribir_texto(self.ini, lineas2, nl)
        self._sincronizar_perfil()
        self.registrar("PAQUETE `%s` (%s) encendido y puesto el primero de los cpk.root" % (nombre_corto(ruta), motivo))
        return True

    def cambiar_perfil(self, p):
        if p not in PERFILES:
            raise ValueError(p)
        if p == self.perfil:
            return False
        self.respaldar("PERFIL %s -> %s" % (self.perfil, p))
        shutil.copy2(self.ruta_perfil(p), self.ini)
        self.estado["perfil"] = p
        self._guardar_estado()
        self.registrar("PERFIL activo: %s" % NOMBRE_PERFIL[p])
        return True

    def guardar_v1(self):
        r = self.ruta_perfil("V1")
        sello = ahora().strftime("%Y%m%d_%H%M%S")
        shutil.copy2(r, r + ".antes_" + sello)
        shutil.copy2(self.ini, r)
        # las caras elegidas del V2 pasan tambien al V1 (con copia previa)
        cj = os.path.join(self.pdir, "caras_elegidas.json")
        if os.path.isfile(cj):
            shutil.copy2(cj, cj + ".antes_" + sello)
            try:
                d = json.load(open(cj, encoding="utf-8"))
                d["V1"] = d.get("V2", {})
                with open(cj, "w", encoding="utf-8") as f:
                    json.dump(d, f, ensure_ascii=False, indent=1)
            except Exception:
                pass
        self.registrar("V1 estable REEMPLAZADO por el sider.ini actual (copia previa: %s)" % (
            os.path.basename(r) + ".antes_" + sello))

    # --- bitacora
    def registrar(self, texto):
        nuevo = not os.path.isfile(self.cambios)
        with open(self.cambios, "a", encoding="utf-8") as f:
            if nuevo:
                f.write("# CAMBIOS del sider.ini (lo escribe Phoenix Launcher)\n\n")
            f.write("- %s · [%s] %s\n" % (ahora().strftime("%Y-%m-%d %H:%M:%S"), self.perfil, texto))


# ---------------------------------------------------------------- juego
def pes_abierto():
    try:
        out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq PES2021.exe", "/FO", "CSV", "/NH"],
                             capture_output=True, text=True, timeout=10,
                             creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)).stdout
        return "pes2021.exe" in out.lower()
    except Exception:
        return False


def lanzar_sider(sider_dir):
    exe = os.path.join(sider_dir, "sider.exe")
    if not os.path.isfile(exe):
        raise FileNotFoundError(exe)
    subprocess.Popen([exe], cwd=sider_dir)
