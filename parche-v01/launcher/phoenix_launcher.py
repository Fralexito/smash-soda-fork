# -*- coding: utf-8 -*-
"""Phoenix Launcher - ventana (tkinter). Usa nucleo.py y caras.py.

Uso:  pythonw phoenix_launcher.py [--sider "C:\\PES21\\eFootball PES 2021\\Sider_7.4.1"] [--pestana caras]
"""
import os, sys, shutil, argparse, threading, queue
import tkinter as tk
from tkinter import messagebox

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nucleo
import caras

SIDER_POR_DEFECTO = r"C:\PES21\eFootball PES 2021\Sider_7.4.1"
LEYENDA_ORIGEN = r"D:\Frank\IA\CLAUDE\mi-parche\LEYENDA_NOMBRES.txt"

FONDO = "#05060F"
PANEL = "#0D1024"
PANEL2 = "#151a38"
CIAN = "#00E5FF"
PURPURA = "#7B2CBF"
HUESO = "#F1EDE4"
GRIS = "#8A8FA8"
ROJO = "#FF4D6D"
AMBAR = "#FFB547"
F = ("Segoe UI", 12)
FB = ("Segoe UI", 12, "bold")
FT = ("Segoe UI", 20, "bold")
FS = ("Segoe UI", 12)


def mb(n):
    return ("%.1f MB" % (n / 1e6)).replace(".", ",")


class App:
    def __init__(self, root, sider_dir, pestana="interruptores"):
        self.root = root
        self.sider_dir = sider_dir
        pdir = os.path.join(sider_dir, "perfiles")
        os.makedirs(pdir, exist_ok=True)
        ley = os.path.join(pdir, "LEYENDA_NOMBRES.txt")
        # la leyenda viva esta en mi-parche; se copia a perfiles para que el launcher funcione solo
        try:
            if os.path.isfile(LEYENDA_ORIGEN) and (not os.path.isfile(ley) or
                    os.path.getmtime(LEYENDA_ORIGEN) > os.path.getmtime(ley)):
                shutil.copy2(LEYENDA_ORIGEN, ley)
        except Exception:
            pass
        self.l = nucleo.Launcher(sider_dir, ley)
        self.cola = queue.Queue()
        self.ix = None
        self.el = caras.Elecciones(self.l)
        self.cache = os.path.join(pdir, "cache_caras")
        self.fotos = []

        root.title("Phoenix Launcher")
        root.configure(bg=FONDO)
        alto = min(940, root.winfo_screenheight() - 80)
        root.geometry("1240x%d" % alto)
        root.minsize(760, 560)

        top = tk.Frame(root, bg=FONDO)
        top.pack(fill="x", padx=18, pady=(16, 6))
        tk.Label(top, text="PHOENIX", font=FT, fg=CIAN, bg=FONDO).pack(side="left")
        tk.Label(top, text=" LAUNCHER", font=FT, fg=PURPURA, bg=FONDO).pack(side="left")
        self._boton(top, "▶  JUGAR", self.jugar, CIAN, FONDO, big=True).pack(side="right")

        perf = tk.Frame(root, bg=FONDO)
        perf.pack(fill="x", padx=18, pady=6)
        tk.Label(perf, text="Perfil:", font=FB, fg=HUESO, bg=FONDO).pack(side="left")
        self.btn_v1 = self._boton(perf, "V1 estable", lambda: self.perfil("V1"))
        self.btn_v1.pack(side="left", padx=(8, 4))
        self.btn_v2 = self._boton(perf, "V2 en pruebas", lambda: self.perfil("V2"))
        self.btn_v2.pack(side="left", padx=4)
        self._boton(perf, "Guardar como V1 estable", self.guardar_v1, PURPURA, HUESO).pack(side="right")
        self._boton(perf, "↶ Deshacer", self.deshacer).pack(side="right", padx=8)
        self._boton(perf, "CAMBIOS.md", self.abrir_cambios).pack(side="right")

        tabs = tk.Frame(root, bg=FONDO)
        tabs.pack(fill="x", padx=18, pady=(6, 0))
        self.tab_btn = {}
        for k, t in (("interruptores", "Interruptores"), ("caras", "Caras")):
            b = tk.Button(tabs, text=t, font=FB, relief="flat", bd=0, padx=18, pady=6, cursor="hand2",
                          command=lambda k=k: self.ver(k))
            b.pack(side="left", padx=(0, 4))
            self.tab_btn[k] = b
        tk.Frame(root, bg=PURPURA, height=2).pack(fill="x", padx=18)

        self.cuerpo = tk.Frame(root, bg=FONDO)
        self.cuerpo.pack(fill="both", expand=True, padx=18, pady=6)
        self.f_int = tk.Frame(self.cuerpo, bg=FONDO)
        self.f_car = tk.Frame(self.cuerpo, bg=FONDO)
        self._armar_interruptores()

        self.estado = tk.Label(root, text="", font=FS, fg=GRIS, bg=FONDO, anchor="w", justify="left")
        self.estado.pack(fill="x", padx=18, pady=(4, 12))
        root.after(100, self._vaciar_cola)
        self.ver(pestana)

    # ------------------------------------------------------------- comun
    def _boton(self, padre, texto, cmd, fondo=PANEL, letra=HUESO, big=False):
        return tk.Button(padre, text=texto, command=cmd, font=("Segoe UI", 14, "bold") if big else FB,
                         bg=fondo, fg=letra, activebackground=CIAN, activeforeground=FONDO,
                         relief="flat", bd=0, padx=14, pady=6, cursor="hand2")

    def aviso(self, txt, error=False):
        self.estado.configure(text=txt, fg=ROJO if error else GRIS)

    def _en_hilo(self, fn, fin):
        def run():
            try:
                r = fn()
                self.cola.put((fin, r, None))
            except Exception as e:
                self.cola.put((fin, None, e))
        threading.Thread(target=run, daemon=True).start()

    def _vaciar_cola(self):
        try:
            while True:
                fin, r, e = self.cola.get_nowait()
                fin(r, e)
        except queue.Empty:
            pass
        self.root.after(100, self._vaciar_cola)

    def ver(self, k):
        self.pestana = k
        for n, b in self.tab_btn.items():
            b.configure(bg=PURPURA if n == k else PANEL, fg=HUESO)
        self.f_int.pack_forget()
        self.f_car.pack_forget()
        if k == "interruptores":
            self.f_int.pack(fill="both", expand=True)
            self.pintar()
        else:
            self.f_car.pack(fill="both", expand=True)
            self._abrir_caras()

    def _pintar_perfil(self):
        p = self.l.perfil
        self.btn_v1.configure(bg=CIAN if p == "V1" else PANEL, fg=FONDO if p == "V1" else HUESO)
        self.btn_v2.configure(bg=CIAN if p == "V2" else PANEL, fg=FONDO if p == "V2" else HUESO)

    # ------------------------------------------------------------- interruptores
    def _armar_interruptores(self):
        self.canvas = tk.Canvas(self.f_int, bg=FONDO, highlightthickness=0)
        sb = tk.Scrollbar(self.f_int, orient="vertical", command=self.canvas.yview)
        self.lista = tk.Frame(self.canvas, bg=FONDO)
        self.lista.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.win = self.canvas.create_window((0, 0), window=self.lista, anchor="nw")
        self.canvas.bind("<Configure>", lambda e: self.canvas.itemconfigure(self.win, width=e.width))
        self.canvas.configure(yscrollcommand=sb.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        self.canvas.bind("<Enter>", lambda e: self.root.bind_all(
            "<MouseWheel>", lambda e: self.canvas.yview_scroll(int(-e.delta / 120), "units")))
        self.canvas.bind("<Leave>", lambda e: self.root.unbind_all("<MouseWheel>"))

    def pintar(self):
        self._pintar_perfil()
        for w in self.lista.winfo_children():
            w.destroy()
        try:
            its = self.l.leer()
        except Exception as e:
            self.aviso("No pude leer sider.ini: %s" % e, True)
            return
        congelado = self.l.perfil == "V1"
        for clave, titulo in (("cpk.root", "GRÁFICOS Y CARPETAS (livecpk) · el de arriba gana"),
                              ("lua.module", "MÓDULOS LUA · orden de carga")):
            tk.Label(self.lista, text=titulo, font=FB, fg=PURPURA, bg=FONDO, anchor="w").pack(
                fill="x", pady=(12, 4))
            grupo = [x for x in its if x["clave"] == clave]
            for n, it in enumerate(grupo):
                self._fila(it, n, len(grupo), congelado)
        if congelado:
            self.aviso("V1 estable está congelado: para probar cosas pasa a «V2 en pruebas».")
        else:
            self.aviso("Perfil V2 en pruebas. Cada cambio guarda antes una copia sider.ini.bak_<fecha>.")

    def _fila(self, it, n, total, congelado):
        f = tk.Frame(self.lista, bg=PANEL)
        f.pack(fill="x", pady=2)
        on = it["encendido"]
        tk.Button(f, text="ENCENDIDO" if on else "APAGADO", width=11, font=FB,
                  bg=CIAN if on else "#2A2E45", fg=FONDO if on else GRIS, relief="flat", bd=0,
                  cursor="hand2", state="disabled" if congelado else "normal",
                  disabledforeground=FONDO if on else GRIS,
                  command=lambda: self.alternar(it)).pack(side="left", padx=8, pady=6)
        txt = tk.Frame(f, bg=PANEL)
        txt.pack(side="left", fill="x", expand=True)
        tk.Label(txt, text=it["amigable"] or it["tecnico"], font=FB, fg=HUESO if on else GRIS,
                 bg=PANEL, anchor="w").pack(fill="x")
        tk.Label(txt, text=it["tecnico"], font=FS, fg=GRIS, bg=PANEL, anchor="w").pack(fill="x")
        if not congelado:
            tk.Button(f, text="▼", font=F, bg=PANEL, fg=HUESO, relief="flat", bd=0, cursor="hand2",
                      state="normal" if n < total - 1 else "disabled",
                      command=lambda: self.mover(it, 1)).pack(side="right", padx=(0, 8))
            tk.Button(f, text="▲", font=F, bg=PANEL, fg=HUESO, relief="flat", bd=0, cursor="hand2",
                      state="normal" if n > 0 else "disabled",
                      command=lambda: self.mover(it, -1)).pack(side="right", padx=2)

    def _seguro(self, fn, *a):
        try:
            return fn(*a)
        except PermissionError as e:
            self.aviso(str(e), True)
        except Exception as e:
            messagebox.showerror("Phoenix Launcher", str(e))

    def alternar(self, it):
        if nucleo.pes_abierto():
            self.aviso("Ojo: PES está abierto. El cambio se aplicará la próxima vez que lo abras.", True)
        self._seguro(self.l.alternar, it["linea"], not it["encendido"])
        self.pintar()

    def mover(self, it, d):
        self._seguro(self.l.mover, it["linea"], d)
        self.pintar()

    def perfil(self, p):
        if self._seguro(self.l.cambiar_perfil, p):
            self.ver(self.pestana)
            if self.el.de_perfil(p) != self.el.de_perfil("V2" if p == "V1" else "V1"):
                self.aviso("Perfil %s activo. Sus caras elegidas son distintas: pulsa «Aplicar caras» "
                           "en la pestaña Caras para ponerlas." % nucleo.NOMBRE_PERFIL[p])

    def guardar_v1(self):
        if messagebox.askyesno("Guardar como V1 estable",
                               "¿Congelar el sider.ini actual (y sus caras elegidas) como «V1 estable»?\n"
                               "El V1 anterior queda guardado como copia."):
            self._seguro(self.l.guardar_v1)
            self.el = caras.Elecciones(self.l)
            self.aviso("V1 estable actualizado.")

    def deshacer(self):
        r = self._seguro(self.l.deshacer)
        self.ver(self.pestana)
        self.aviso("Deshecho: %s" % r if r else "No hay nada que deshacer.")

    def abrir_cambios(self):
        if os.path.isfile(self.l.cambios):
            os.startfile(self.l.cambios)

    def jugar(self):
        if nucleo.pes_abierto():
            messagebox.showwarning("Phoenix Launcher", "Ya hay un PES2021.exe abierto. Ciérralo antes de jugar.")
            return
        try:
            nucleo.lanzar_sider(self.sider_dir)
            self.l.registrar("JUGAR: lanzado sider.exe")
            self.aviso("Lanzado sider.exe. Que empiece el partido.")
        except Exception as e:
            messagebox.showerror("Phoenix Launcher", "No pude lanzar sider.exe: %s" % e)

    # ------------------------------------------------------------- caras
    def _abrir_caras(self):
        self._pintar_perfil()
        if self.ix is not None or getattr(self, "_cargando", False):
            return
        self._cargando = True
        tk.Label(self.f_car, text="Cargando el inventario de caras…", font=FB, fg=GRIS, bg=FONDO).pack(pady=40)
        self._en_hilo(lambda: caras.Indice(), self._caras_cargadas)

    def _caras_cargadas(self, ix, err):
        for w in self.f_car.winfo_children():
            w.destroy()
        if err:
            tk.Label(self.f_car, text="No pude cargar el inventario: %s" % err, font=FB, fg=ROJO, bg=FONDO).pack()
            return
        self.ix = ix
        self.todos = ix.jugadores()
        self.busqueda = {i: ix.texto_busqueda(i) for _, i in self.todos}
        barra = tk.Frame(self.f_car, bg=FONDO)
        barra.pack(fill="x", pady=(6, 6))
        tk.Label(barra, text="Buscar:", font=FB, fg=HUESO, bg=FONDO).pack(side="left")
        self.q = tk.Entry(barra, font=F, bg=PANEL, fg=HUESO, insertbackground=HUESO, relief="flat", width=34)
        self.q.pack(side="left", padx=8, ipady=5)
        self.q.bind("<KeyRelease>", lambda e: self._filtrar())
        self.lbl_n = tk.Label(barra, text="", font=FS, fg=GRIS, bg=FONDO)
        self.lbl_n.pack(side="left", padx=8)
        self._boton(barra, "Quitar caras elegidas", self.quitar_caras).pack(side="right")
        self._boton(barra, "Aplicar caras", self.aplicar_caras, CIAN, FONDO).pack(side="right", padx=8)
        cuerpo = tk.Frame(self.f_car, bg=FONDO)
        cuerpo.pack(fill="both", expand=True)
        izq = tk.Frame(cuerpo, bg=FONDO)
        izq.pack(side="left", fill="y")
        self.lb = tk.Listbox(izq, font=F, bg=PANEL, fg=HUESO, selectbackground=PURPURA, selectforeground=HUESO,
                             relief="flat", highlightthickness=0, width=40, activestyle="none")
        sb = tk.Scrollbar(izq, command=self.lb.yview)
        self.lb.configure(yscrollcommand=sb.set)
        self.lb.pack(side="left", fill="y")
        sb.pack(side="left", fill="y")
        self.lb.bind("<<ListboxSelect>>", lambda e: self._seleccion())
        self.det = tk.Frame(cuerpo, bg=FONDO)
        self.det.pack(side="left", fill="both", expand=True, padx=(14, 0))
        tk.Label(self.det, text="Elige un jugador de la lista.\n\n⚠ = riesgo ALTO (mismo id, persona distinta según "
                 "la tabla maestra)\n≠ = los nombres de los parches no coinciden\n✓ = ya tiene una cara elegida",
                 font=F, fg=GRIS, bg=FONDO, justify="left").pack(anchor="w", pady=20)
        self._filtrar()
        self.aviso("Caras: %d jugadores con más de una versión. Elegir solo guarda; «Aplicar caras» crea el paquete "
                   "PHX_FAC_SEL." % len(self.todos))

    def _marca(self, g, i):
        m = "⚠" if g == 0 else ("≠" if g == 1 else " ")
        return m + ("✓" if i in self.el.de_perfil() else " ")

    def _filtrar(self):
        t = self.q.get().strip().lower() if hasattr(self, "q") else ""
        self.vista = [(g, i) for g, i in self.todos if not t or all(p in self.busqueda[i] for p in t.split())]
        self.lb.delete(0, "end")
        for g, i in self.vista[:3000]:
            self.lb.insert("end", "%s %s · %s" % (self._marca(g, i), i, self.ix.etiqueta(i)))
            if g == 0:
                self.lb.itemconfigure("end", fg=ROJO)
            elif g == 1:
                self.lb.itemconfigure("end", fg=AMBAR)
        self.lbl_n.configure(text="%d de %d · elegidas en %s: %d" % (
            len(self.vista), len(self.todos), self.l.perfil, len(self.el.de_perfil())))

    def _seleccion(self):
        s = self.lb.curselection()
        if not s:
            return
        self.mostrar(self.vista[s[0]][1])

    def mostrar(self, id_):
        self.id_actual = id_
        for w in self.det.winfo_children():
            w.destroy()
        tk.Label(self.det, text="id %s · %s" % (id_, self.ix.etiqueta(id_)), font=("Segoe UI", 15, "bold"),
                 fg=CIAN, bg=FONDO, anchor="w").pack(fill="x")
        el = self.el.de_perfil().get(id_)
        actual = el["version"] if el else "defecto"
        txt = caras.OPCIONES_EXTRA.get(actual) or "Usar la de " + caras.NOMBRE_VERSION.get(actual, actual)
        tk.Label(self.det, text="Elección en %s: %s" % (self.l.perfil, txt), font=F, fg=HUESO, bg=FONDO,
                 anchor="w").pack(fill="x", pady=(2, 8))
        fila = tk.Frame(self.det, bg=FONDO)
        fila.pack(fill="x")
        versiones = [f for f in caras.VERSIONES if f in self.ix.caras.get(id_, {})]
        self.tarjetas = {}
        for fam in versiones:
            self.tarjetas[fam] = self._tarjeta(fila, id_, fam, actual)
        extra = tk.Frame(self.det, bg=FONDO)
        extra.pack(fill="x", pady=12)
        for op in ("juego", "defecto"):
            b = self._boton(extra, caras.OPCIONES_EXTRA[op], lambda op=op: self.elegir(id_, op),
                            CIAN if actual == op else PANEL, FONDO if actual == op else HUESO)
            b.pack(side="left", padx=(0, 8))
        tk.Label(self.det, text="«Ninguna» pone la cara del juego base por encima de todo (si el base la trae). "
                 "«Por defecto» no fuerza nada: manda el orden de los paquetes.\nOjo: tu PES base usa SU jugador "
                 "con ese id. Si el nombre del base no es el de la versión elegida, saldrá esa cara en otra persona.",
                 font=FS, fg=GRIS, bg=FONDO, justify="left", wraplength=760).pack(anchor="w")
        self._en_hilo(lambda: {fam: (self.ix.info(id_, fam), self.ix.minicara_png(id_, fam, self.cache))
                               for fam in versiones}, lambda r, e: self._detalle_cargado(id_, r, e))

    def _tarjeta(self, padre, id_, fam, actual):
        elegida = actual == fam or (actual == "juego" and fam == "base")
        c = tk.Frame(padre, bg=PANEL2 if elegida else PANEL, highlightthickness=2,
                     highlightbackground=CIAN if elegida else PANEL)
        c.pack(side="left", padx=(0, 10), anchor="n")
        tk.Label(c, text=caras.NOMBRE_VERSION[fam], font=FB, fg=PURPURA, bg=c["bg"]).pack(pady=(8, 2))
        img = tk.Label(c, text="…", font=F, fg=GRIS, bg=FONDO, width=16, height=7)
        img.pack(padx=10, pady=4)
        nom, eq = self.ix.nombre(fam, id_)
        tk.Label(c, text=nom or "(sin nombre)", font=FB, fg=HUESO, bg=c["bg"], wraplength=260).pack(padx=8)
        tk.Label(c, text=(eq or "sin equipo")[:60], font=FS, fg=GRIS, bg=c["bg"], wraplength=260).pack(padx=8)
        datos = tk.Label(c, text="leyendo…", font=FS, fg=GRIS, bg=c["bg"], justify="left", wraplength=260)
        datos.pack(padx=8, pady=4)
        op = "juego" if fam == "base" else fam
        txt = "Usar la del juego" if fam == "base" else "Usar esta"
        self._boton(c, txt, lambda: self.elegir(id_, op), CIAN if elegida else PURPURA,
                    FONDO if elegida else HUESO).pack(pady=(0, 10))
        return img, datos

    def _detalle_cargado(self, id_, r, err):
        if getattr(self, "id_actual", None) != id_ or not hasattr(self, "tarjetas"):
            return
        if err:
            self.aviso("No pude leer las versiones: %s" % err, True)
            return
        for fam, (info, (png, res_mini)) in r.items():
            img, datos = self.tarjetas[fam]
            try:
                if png:
                    foto = tk.PhotoImage(file=png)
                    if foto.width() > 150:
                        foto = foto.subsample(2)
                    self.fotos.append(foto)
                    self.fotos = self.fotos[-30:]
                    img.configure(image=foto, text="", width=foto.width(), height=foto.height())
                else:
                    img.configure(text="sin vista previa" if res_mini == "sin minicara"
                                  else "sin vista previa\n(%s)" % res_mini, height=4)
            except Exception:
                img.configure(text="sin vista")
            lineas = ["Cara %s · %d arch. · %s" % (info["resolucion"] or "?", info["archivos"], mb(info["bytes"])),
                      "Minicara %s · %s" % (res_mini or "—", {"suelto": "suelta", "cpk": "en .cpk"}.get(
                          info["origen"], info["origen"])),
                      "PRDX mangas/medias: %s" % ("SÍ" if info["prdx"] else "no")]
            datos.configure(text="\n".join(lineas), fg=AMBAR if info["prdx"] else HUESO)

    def elegir(self, id_, op):
        r = self._seguro(self.el.elegir, id_, op, self.ix.etiqueta(id_))
        if r is None and self.l.perfil == "V1":
            return
        self._filtrar()
        self.mostrar(id_)
        self.aviso("Guardado. Pulsa «Aplicar caras» para crear el paquete PHX_FAC_SEL.")

    def aplicar_caras(self):
        el = self.el.de_perfil()
        if not el:
            self.aviso("No hay caras elegidas en %s." % self.l.perfil, True)
            return
        destino = caras.carpeta_paquete()
        self.aviso("Preparando %d caras…" % len(el))
        self._en_hilo(lambda: caras.planificar(self.ix, el, destino),
                      lambda r, e: self._plan_listo(r, e, destino))

    def _plan_listo(self, r, err, destino):
        if err:
            messagebox.showerror("Phoenix Launcher", str(err))
            return
        pasos, res = r
        msg = ("Paquete PHX_FAC_SEL en:\n%s\n\n%d archivos con ENLACE DURO (no ocupan espacio)\n"
               "%d archivos se COPIAN (%s): vienen de dentro de un .cpk o de otro disco.\n"
               % (destino, res["enlaces"], res["copias"], mb(res["bytes_copia"])))
        if res["avisos"]:
            msg += "\nAvisos:\n- " + "\n- ".join(res["avisos"][:8]) + "\n"
        msg += "\nNo se borra nada: si ya había un paquete, se guarda en _anteriores.\n¿Seguir?"
        if not messagebox.askyesno("Aplicar caras", msg):
            return
        self.aviso("Creando el paquete…")
        self._en_hilo(lambda: caras.aplicar(self.ix, pasos, destino), lambda h, e: self._aplicado(h, e, destino))

    def _aplicado(self, hechos, err, destino):
        if err:
            messagebox.showerror("Phoenix Launcher", "No pude crear el paquete: %s" % err)
            return
        try:
            self.l.asegurar_raiz(destino, "caras elegidas")
        except PermissionError:
            if not any(nucleo.os.path.normcase(x["valor"]) == nucleo.os.path.normcase(destino)
                       for x in self.l.leer()):
                self.aviso("Paquete creado, pero V1 está congelado: su sider.ini no tiene la línea. "
                           "Pasa a V2 y aplica allí.", True)
                return
        self.l.registrar("CARAS aplicadas: %d enlaces, %d copias -> %s" % (hechos["enlaces"], hechos["copias"], destino))
        self.aviso("Caras aplicadas: %d enlaces y %d copias. PHX_FAC_SEL encendido y arriba del todo."
                   % (hechos["enlaces"], hechos["copias"]))

    def quitar_caras(self):
        destino = caras.carpeta_paquete()
        its = [x for x in self.l.leer() if os.path.normcase(x["valor"]) == os.path.normcase(destino)]
        if not its or not its[0]["encendido"]:
            self.aviso("PHX_FAC_SEL ya está apagado (o no existe en este perfil).")
            return
        self._seguro(self.l.alternar, its[0]["linea"], False)
        self.aviso("PHX_FAC_SEL apagado. No se borró nada; se vuelve a encender en la pestaña Interruptores.")


def nitidez_dpi():
    """Evita que Windows estire la ventana borrosa con escalado >100 %."""
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except Exception:
        pass


def main():
    nitidez_dpi()
    ap = argparse.ArgumentParser()
    ap.add_argument("--sider", default=SIDER_POR_DEFECTO)
    ap.add_argument("--pestana", default="interruptores")
    a = ap.parse_args()
    root = tk.Tk()
    App(root, a.sider, a.pestana)
    root.mainloop()


if __name__ == "__main__":
    main()
