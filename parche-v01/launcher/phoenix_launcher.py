# -*- coding: utf-8 -*-
"""Phoenix Launcher - ventana (tkinter). Usa nucleo.py.

Uso:  pythonw phoenix_launcher.py [--sider "C:\\PES21\\eFootball PES 2021\\Sider_7.4.1"]
"""
import os, sys, shutil, argparse
import tkinter as tk
from tkinter import messagebox

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nucleo

SIDER_POR_DEFECTO = r"C:\PES21\eFootball PES 2021\Sider_7.4.1"
LEYENDA_ORIGEN = r"D:\Frank\IA\CLAUDE\mi-parche\LEYENDA_NOMBRES.txt"

FONDO = "#05060F"
PANEL = "#0D1024"
CIAN = "#00E5FF"
PURPURA = "#7B2CBF"
HUESO = "#F1EDE4"
GRIS = "#8A8FA8"
ROJO = "#FF4D6D"
F = ("Segoe UI", 12)
FB = ("Segoe UI", 12, "bold")
FT = ("Segoe UI", 20, "bold")
FS = ("Segoe UI", 12)


class App:
    def __init__(self, root, sider_dir):
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

        root.title("Phoenix Launcher")
        root.configure(bg=FONDO)
        root.geometry("820x760")
        root.minsize(680, 520)

        top = tk.Frame(root, bg=FONDO)
        top.pack(fill="x", padx=18, pady=(16, 6))
        tk.Label(top, text="PHOENIX", font=FT, fg=CIAN, bg=FONDO).pack(side="left")
        tk.Label(top, text=" LAUNCHER", font=FT, fg=PURPURA, bg=FONDO).pack(side="left")
        self.btn_jugar = self._boton(top, "▶  JUGAR", self.jugar, CIAN, FONDO, big=True)
        self.btn_jugar.pack(side="right")

        perf = tk.Frame(root, bg=FONDO)
        perf.pack(fill="x", padx=18, pady=6)
        tk.Label(perf, text="Perfil:", font=FB, fg=HUESO, bg=FONDO).pack(side="left")
        self.btn_v1 = self._boton(perf, "V1 estable", lambda: self.perfil("V1"))
        self.btn_v1.pack(side="left", padx=(8, 4))
        self.btn_v2 = self._boton(perf, "V2 en pruebas", lambda: self.perfil("V2"))
        self.btn_v2.pack(side="left", padx=4)
        self._boton(perf, "Guardar como V1 estable", self.guardar_v1, PURPURA, HUESO).pack(side="right")

        acc = tk.Frame(root, bg=FONDO)
        acc.pack(fill="x", padx=18, pady=(0, 6))
        self._boton(acc, "↶ Deshacer último cambio", self.deshacer).pack(side="left")
        self._boton(acc, "Abrir CAMBIOS.md", self.abrir_cambios).pack(side="left", padx=8)
        self._boton(acc, "Recargar", self.pintar).pack(side="left")

        # lista con scroll
        cont = tk.Frame(root, bg=FONDO)
        cont.pack(fill="both", expand=True, padx=18, pady=6)
        self.canvas = tk.Canvas(cont, bg=FONDO, highlightthickness=0)
        sb = tk.Scrollbar(cont, orient="vertical", command=self.canvas.yview)
        self.lista = tk.Frame(self.canvas, bg=FONDO)
        self.lista.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.win = self.canvas.create_window((0, 0), window=self.lista, anchor="nw")
        self.canvas.bind("<Configure>", lambda e: self.canvas.itemconfigure(self.win, width=e.width))
        self.canvas.configure(yscrollcommand=sb.set)
        self.canvas.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")
        root.bind_all("<MouseWheel>", lambda e: self.canvas.yview_scroll(int(-e.delta / 120), "units"))

        self.estado = tk.Label(root, text="", font=FS, fg=GRIS, bg=FONDO, anchor="w", justify="left")
        self.estado.pack(fill="x", padx=18, pady=(4, 12))
        self.pintar()

    def _boton(self, padre, texto, cmd, fondo=PANEL, letra=HUESO, big=False):
        b = tk.Button(padre, text=texto, command=cmd, font=("Segoe UI", 14, "bold") if big else FB,
                      bg=fondo, fg=letra, activebackground=CIAN, activeforeground=FONDO,
                      relief="flat", bd=0, padx=14, pady=6, cursor="hand2")
        return b

    def aviso(self, txt, error=False):
        self.estado.configure(text=txt, fg=ROJO if error else GRIS)

    def pintar(self):
        for w in self.lista.winfo_children():
            w.destroy()
        p = self.l.perfil
        self.btn_v1.configure(bg=CIAN if p == "V1" else PANEL, fg=FONDO if p == "V1" else HUESO)
        self.btn_v2.configure(bg=CIAN if p == "V2" else PANEL, fg=FONDO if p == "V2" else HUESO)
        try:
            its = self.l.leer()
        except Exception as e:
            self.aviso("No pude leer sider.ini: %s" % e, True)
            return
        congelado = p == "V1"
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
        b = tk.Button(f, text="ENCENDIDO" if on else "APAGADO", width=11, font=FB,
                      bg=CIAN if on else "#2A2E45", fg=FONDO if on else GRIS, relief="flat", bd=0,
                      cursor="hand2", state="disabled" if congelado else "normal",
                      disabledforeground=FONDO if on else GRIS,
                      command=lambda: self.alternar(it))
        b.pack(side="left", padx=8, pady=6)
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
            self.pintar()

    def guardar_v1(self):
        if messagebox.askyesno("Guardar como V1 estable",
                               "¿Congelar el sider.ini actual como «V1 estable»?\n"
                               "El V1 anterior queda guardado como copia."):
            self._seguro(self.l.guardar_v1)
            self.aviso("V1 estable actualizado.")

    def deshacer(self):
        r = self._seguro(self.l.deshacer)
        self.pintar()
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
    a = ap.parse_args()
    root = tk.Tk()
    App(root, a.sider)
    root.mainloop()


if __name__ == "__main__":
    main()
