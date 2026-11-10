# -*- coding: utf-8 -*-
"""Pruebas del nucleo sobre una COPIA del sider.ini real (nunca el original).
Uso: python -I test_launcher.py
"""
import os, sys, shutil, tempfile, unittest, hashlib

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import nucleo

INI_REAL = r"C:\PES21\eFootball PES 2021\Sider_7.4.1\sider.ini"
LEYENDA = r"D:\Frank\IA\CLAUDE\mi-parche\LEYENDA_NOMBRES.txt"


def md5(p):
    return hashlib.md5(open(p, "rb").read()).hexdigest()


class T(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.md5_real = md5(INI_REAL)

    @classmethod
    def tearDownClass(cls):
        assert md5(INI_REAL) == cls.md5_real, "¡el sider.ini real cambió!"

    def setUp(self):
        self.d = tempfile.mkdtemp(prefix="phx_launcher_")
        shutil.copy2(INI_REAL, os.path.join(self.d, "sider.ini"))
        self.orig = open(os.path.join(self.d, "sider.ini"), "rb").read()
        self.l = nucleo.Launcher(self.d, LEYENDA)

    def tearDown(self):
        shutil.rmtree(self.d, ignore_errors=True)

    def ini(self):
        return open(self.l.ini, "rb").read()

    def it(self, tecnico):
        return [x for x in self.l.leer() if x["tecnico"] == tecnico][0]

    def test_lee_interruptores_y_leyenda(self):
        its = self.l.leer()
        self.assertGreaterEqual(len([x for x in its if x["clave"] == "cpk.root"]), 9)
        self.assertGreaterEqual(len([x for x in its if x["clave"] == "lua.module"]), 8)
        self.assertEqual(self.it("PHX_ENV_SKY")["amigable"], "Cielo 4096")
        self.assertEqual(self.it("phx_ref.lua")["amigable"], "Referee: estado.json / resultado.json / escenas")
        self.assertFalse(self.it("PHX_IO_PAD")["encendido"])
        self.assertTrue(self.it("PHX_ENV_SKY")["encendido"])

    def test_alternar_ida_y_vuelta_es_identico(self):
        x = self.it("PHX_FX_BLM")
        self.assertTrue(self.l.alternar(x["linea"], False))
        self.assertIn(b';cpk.root = ".\\content\\livecpk\\PHX_FX_BLM"', self.ini())
        self.assertFalse(self.it("PHX_FX_BLM")["encendido"])
        self.l.alternar(x["linea"], True)
        self.assertEqual(self.ini(), self.orig)

    def test_respaldo_y_cambios_md(self):
        x = self.it("netblock.lua")
        self.l.alternar(x["linea"], True)
        baks = [f for f in os.listdir(self.d) if f.startswith("sider.ini.bak_")]
        self.assertEqual(len(baks), 1)
        self.assertEqual(open(os.path.join(self.d, baks[0]), "rb").read(), self.orig)
        cam = open(self.l.cambios, encoding="utf-8").read()
        self.assertIn("ENCENDIDO: lua.module `netblock.lua`", cam)

    def test_deshacer(self):
        a = self.it("PHX_ENV_SKY")
        b = self.it("phx_mon.lua")
        self.l.alternar(a["linea"], False)
        self.l.alternar(b["linea"], False)
        self.l.deshacer()
        self.assertFalse(self.it("PHX_ENV_SKY")["encendido"])
        self.assertTrue(self.it("phx_mon.lua")["encendido"])
        self.l.deshacer()
        self.assertEqual(self.ini(), self.orig)
        self.assertIsNone(self.l.deshacer())

    def test_perfiles(self):
        self.assertEqual(open(self.l.ruta_perfil("V1"), "rb").read(), self.orig)
        self.assertEqual(self.l.perfil, "V2")
        x = self.it("PHX_KIT_TRN")
        self.l.alternar(x["linea"], True)
        v2 = self.ini()
        self.assertEqual(open(self.l.ruta_perfil("V2"), "rb").read(), v2)
        self.assertEqual(open(self.l.ruta_perfil("V1"), "rb").read(), self.orig)  # V1 congelado
        self.l.cambiar_perfil("V1")
        self.assertEqual(self.ini(), self.orig)
        with self.assertRaises(PermissionError):
            self.l.alternar(x["linea"], False)
        self.l.cambiar_perfil("V2")
        self.assertEqual(self.ini(), v2)
        self.l.guardar_v1()
        self.assertEqual(open(self.l.ruta_perfil("V1"), "rb").read(), v2)
        l2 = nucleo.Launcher(self.d, LEYENDA)
        self.assertEqual(l2.perfil, "V2")

    def test_deshacer_cambio_de_perfil(self):
        self.l.alternar(self.it("PHX_FX_LUT")["linea"], False)
        v2 = self.ini()
        self.l.cambiar_perfil("V1")
        self.l.deshacer()
        self.assertEqual(self.l.perfil, "V2")
        self.assertEqual(self.ini(), v2)

    def test_mover_prioridad(self):
        orden = [x["tecnico"] for x in self.l.leer() if x["clave"] == "cpk.root"]
        x = self.it(orden[1])
        self.assertTrue(self.l.mover(x["linea"], -1))
        nuevo = [x["tecnico"] for x in self.l.leer() if x["clave"] == "cpk.root"]
        self.assertEqual(nuevo[0], orden[1])
        self.assertFalse(self.l.mover(self.it(nuevo[0])["linea"], -1))
        self.l.deshacer()
        self.assertEqual(self.ini(), self.orig)

    def test_conserva_finales_de_linea_y_resto(self):
        self.l.alternar(self.it("phx_core.lua")["linea"], False)
        a = self.orig.splitlines(keepends=True)
        b = self.ini().splitlines(keepends=True)
        self.assertEqual(len(a), len(b))
        dif = [i for i in range(len(a)) if a[i] != b[i]]
        self.assertEqual(len(dif), 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
