# -*- coding: utf-8 -*-
"""Pruebas de «Elegir cara» sin abrir PES. Uso: python -I test_caras.py
Genera un paquete PHX_FAC_SEL con 3 elecciones de ejemplo en una carpeta de PRUEBA (en D:, el
mismo disco que el Conmegol, para que haya enlaces duros), sobre una COPIA del sider.ini.
Al final borra solo lo que la prueba creo (enlaces y copias propias, nunca los originales).
"""
import os, sys, shutil, tempfile, unittest, hashlib

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import nucleo, caras

INI_REAL = r"C:\PES21\eFootball PES 2021\Sider_7.4.1\sider.ini"
LEYENDA = r"D:\Frank\IA\CLAUDE\mi-parche\LEYENDA_NOMBRES.txt"
PRUEBA_PAQ = os.path.join(AQUI, "_prueba_paquetes")
EJEMPLOS = {"41180": "conmegol", "41513": "sudamerican", "42538": "juego"}


def md5(p):
    with open(p, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()


class T(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.md5_real = md5(INI_REAL)
        cls.ix = caras.Indice()
        cls.d = tempfile.mkdtemp(prefix="phx_caras_")
        shutil.copy2(INI_REAL, os.path.join(cls.d, "sider.ini"))
        cls.l = nucleo.Launcher(cls.d, LEYENDA)
        cls.el = caras.Elecciones(cls.l)

    @classmethod
    def tearDownClass(cls):
        assert md5(INI_REAL) == cls.md5_real, "¡el sider.ini real cambió!"
        shutil.rmtree(cls.d, ignore_errors=True)
        shutil.rmtree(PRUEBA_PAQ, ignore_errors=True)  # solo enlaces/copias de la prueba

    def test_1_lista_y_nombres(self):
        js = self.ix.jugadores()
        self.assertGreater(len(js), 5000)
        self.assertEqual(js[0][0], 0)  # primero los de riesgo ALTO
        self.assertIn("65342", [i for g, i in js if g == 0])
        self.assertIn("Riquelme", self.ix.nombre("conmegol", "65342")[0])
        self.assertIn("Boninsegna", self.ix.nombre("sudamerican", "65342")[0])
        self.assertTrue(self.ix.otra_persona("65342"))
        self.assertFalse(caras.misma_persona("Juan Román Riquelme", "A. Boninsegna"))
        self.assertTrue(caras.misma_persona("Flávio Silva", "Flávio"))

    def test_2_crilayla_minicara_base(self):
        id_ = next(i for i, d in self.ix.mini.items()
                   if "base" in d and any(f["tipo"] == "cpk" for f in d["base"]))
        arch = [a for a in self.ix.archivos(id_, "base", self.ix.mini[id_]["base"]).values() if a["origen"] == "cpk"]
        self.assertTrue(arch)
        a = arch[-1]
        b = self.ix.leer(a)
        self.assertEqual(b[:4], b"DDS ")
        self.assertEqual(len(b), a["entrada"][2])  # tamaño descomprimido exacto
        p, res = self.ix.minicara_png(id_, "base", os.path.join(self.d, "cache"))  # BC7 -> texconv
        self.assertIsNotNone(p, res)

    def test_3_vista_previa_png(self):
        cache = os.path.join(self.d, "cache")
        hechas = 0
        id_ = next(i for i, d in self.ix.mini.items() if "conmegol" in d and "sudamerican" in d)
        for fam in ("conmegol", "sudamerican"):
            p, res = self.ix.minicara_png(id_, fam, cache)
            if p:
                with open(p, "rb") as f:
                    self.assertEqual(f.read(8), b"\x89PNG\r\n\x1a\n")
                self.assertRegex(res, r"^\d+x\d+$")
                hechas += 1
        self.assertGreater(hechas, 0)

    def test_4_info_version(self):
        i = self.ix.info("41180", "conmegol")
        self.assertGreater(i["archivos"], 3)
        self.assertRegex(i["resolucion"], r"^\d+x\d+$")

    def test_5_paquete_con_3_elecciones(self):
        for id_, op in EJEMPLOS.items():
            self.el.elegir(id_, op, self.ix.etiqueta(id_))
        self.assertEqual(len(self.el.de_perfil()), 3)
        destino = caras.carpeta_paquete(PRUEBA_PAQ)
        pasos, res = caras.planificar(self.ix, self.el.de_perfil(), destino)
        self.assertGreater(res["enlaces"], 0)   # Conmegol suelto en D: -> enlace duro
        self.assertGreater(res["copias"], 0)    # Sudamerican y base vienen de .cpk -> copia
        hechos = caras.aplicar(self.ix, pasos, destino)
        self.assertEqual(hechos["enlaces"] + hechos["copias"], len(pasos))
        for dst, a, kind in pasos:
            self.assertTrue(os.path.isfile(dst), dst)
            rel = os.path.relpath(dst, destino).replace("\\", "/").lower()
            self.assertEqual(rel, a["ruta_logica"].lower())
            self.assertRegex(rel, r"^asset/model/character/face/real/(41180|41513|42538)/")
            if kind == "enlace":
                self.assertTrue(os.path.samefile(dst, a["ruta"]))
                self.assertGreaterEqual(os.stat(dst).st_nlink, 2)
            else:
                self.assertEqual(os.path.getsize(dst), a["bytes"])
        # volver a aplicar no borra: el anterior va a _anteriores
        caras.aplicar(self.ix, pasos, destino)
        self.assertTrue(os.listdir(os.path.join(PRUEBA_PAQ, "_anteriores")))
        # la linea cpk.root queda la primera y encendida en la COPIA del ini
        self.assertTrue(self.l.asegurar_raiz(destino, "caras elegidas"))
        roots = [x for x in self.l.leer() if x["clave"] == "cpk.root"]
        self.assertEqual(roots[0]["valor"], destino)
        self.assertTrue(roots[0]["encendido"])
        self.assertFalse(self.l.asegurar_raiz(destino))  # segunda vez no cambia nada
        # «Quitar caras elegidas» = solo apagar
        self.l.alternar(roots[0]["linea"], False)
        self.assertTrue(os.path.isdir(destino))
        with open(self.l.cambios, encoding="utf-8") as f:
            self.assertIn("CARA elegida: id 41513", f.read())
        print("\n   paquete:", res, "->", hechos)


if __name__ == "__main__":
    unittest.main(verbosity=2)
