"""Espejo en Supabase (docs/PLAN_MULTIUSUARIO.md §2-§4, migración 162).

El servidor sigue trabajando con su SQLite local (rápido y sin depender de internet). Cada
`cada_s` segundos, en segundo plano:
  1. sube los clubes que cambiaron a `myclub_clubes` (upsert por clave_servidor);
  2. lee `myclub_reglas` y, si cambió, la aplica en caliente (lo de la web manda);
  3. aplica los `myclub_movimientos` pendientes (dar GP, monedas, boletos, recuperar) y los marca.
Si Supabase falla o no hay internet, no pasa nada: se reintenta en la siguiente vuelta.

Dos modos:
  * RECOMENDADO (migración 164, «sin llave maestra»): clave pública de la web en la config
    ("clave_publica") + secreto propio del servidor en la variable PHOENIX_SERVIDOR_SECRETO. Solo puede
    llamar a las funciones phoenix_srv_* (permisos mínimos); en Supabase solo está la huella del secreto.
  * Antiguo: clave service_role en la variable PHOENIX_SUPABASE_KEY (acceso total; mejor no usarlo).
Ningún secreto va en archivos.
Configuración: "supabase": {"activo": true, "url": "https://<proyecto>.supabase.co", "clave_publica": "sb_publishable_…", "cada_s": 30}
"""
import asyncio
import json
import logging
import os
import time
import urllib.error
import urllib.parse
import urllib.request

log = logging.getLogger("phoenix_online.nube")

CAMPO_MOVIMIENTO = {"gp": "gp", "monedas": "monedas", "boletos": "boletos_contrato",
                    "recuperar": "objetos_recuperar"}


class Nube:
    def __init__(self, srv, url, clave, cada_s=30, abrir=None, secreto=None):
        self.srv = srv
        self.secreto = secreto   # modo «sin llave maestra»: clave = la pública de la web
        self.base = url.rstrip("/")
        self.url = self.base + "/rest/v1/"
        self.clave = clave
        self.cada_s = max(5, int(cada_s))
        self.reglas_vistas = None
        self.fallos_seguidos = 0
        self._abrir = abrir or urllib.request.urlopen  # en pruebas se sustituye

    # ---------- HTTP ----------
    def _pedir(self, metodo, tabla, consulta="", cuerpo=None, prefer=None):
        url = self.url + tabla + (("?" + consulta) if consulta else "")
        datos = json.dumps(cuerpo).encode() if cuerpo is not None else None
        cab = {"apikey": self.clave, "Authorization": f"Bearer {self.clave}",
               "Content-Type": "application/json", "Accept": "application/json"}
        if prefer:
            cab["Prefer"] = prefer
        req = urllib.request.Request(url, data=datos, headers=cab, method=metodo)
        with self._abrir(req, timeout=15) as r:
            crudo = r.read()
        return json.loads(crudo) if crudo else None

    def _rpc(self, funcion, args):
        """Modo sin llave maestra: POST /rest/v1/rpc/<funcion> con el secreto del servidor."""
        req = urllib.request.Request(self.url + "rpc/" + funcion, method="POST",
                                     data=json.dumps({"p_secreto": self.secreto, **args}).encode(),
                                     headers={"apikey": self.clave, "Content-Type": "application/json",
                                              "Accept": "application/json"})
        with self._abrir(req, timeout=15) as r:
            crudo = r.read()
        return json.loads(crudo) if crudo else None

    async def _llamar(self, funcion, **args):
        return await asyncio.to_thread(self._rpc, funcion, args)

    # ---------- piezas ----------
    @staticmethod
    def fila_club(clave, club, nombre_jugador="", usuario=None):
        com = club.get("vscom") or {}
        fila = {"clave_servidor": clave,
                "nombre": str(club.get("team_name") or nombre_jugador or "")[:100],
                "gp": int(club.get("gp") or 0), "monedas": int(club.get("monedas") or 0),
                "partidos": int(com.get("partidos") or 0), "racha": int(com.get("racha") or 0),
                "nombre_jugador": str(nombre_jugador or "")[:100],
                "nivel_com": int(com.get("nivel_idx") or 0),
                "datos": club, "actualizado": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}
        if usuario:
            fila["usuario"] = usuario  # dueño en la web (pase); así ve su club en su perfil
        return fila

    # La red va en un hilo aparte (to_thread); la base de datos y los clubes SOLO en el hilo del
    # servidor (SQLite no deja usar su conexión desde otro hilo, y así no hay carreras con el juego).
    async def _red(self, *a, **k):
        return await asyncio.to_thread(self._pedir, *a, **k)

    async def subir_clubes(self):
        datos = self.srv.datos
        claves = datos.tomar_cambiados()
        if not claves:
            return 0
        filas = []
        for k in claves:
            club = datos.club(k)
            if club is not None:
                j = datos.jugador(k) or {}
                filas.append(self.fila_club(k, club, j.get("pname", ""), datos.usuario_de_clave(k)))
        try:
            if filas and self.secreto:
                for i in range(0, len(filas), 200):
                    await self._llamar("phoenix_srv_subir_clubes", p_filas=filas[i:i + 200])
            elif filas:
                await self._red("POST", "myclub_clubes", "on_conflict=clave_servidor", filas,
                                "resolution=merge-duplicates,return=minimal")
        except Exception:
            datos.marcar_cambiados(claves)  # se reintenta en la siguiente vuelta
            raise
        return len(filas)

    async def leer_reglas(self):
        if self.secreto:
            filas = await self._llamar("phoenix_srv_reglas") or []
        else:
            filas = await self._red("GET", "myclub_reglas", "id=eq.1&select=reglas,actualizado") or []
        if not filas:
            return []
        marca = filas[0].get("actualizado")
        if marca == self.reglas_vistas:
            return []
        self.reglas_vistas = marca
        reglas = filas[0].get("reglas") or {}
        return self.srv.aplicar_reglas_myclub(reglas) if reglas else []

    async def aplicar_movimientos(self):
        if self.secreto:
            filas = await self._llamar("phoenix_srv_movimientos") or []
        else:
            filas = await self._red("GET", "myclub_movimientos",
                                    "aplicado=is.null&select=id,clave_servidor,tipo,cantidad,motivo&order=id&limit=200") or []
        hechos = 0
        datos = self.srv.datos
        for f in filas:
            campo = CAMPO_MOVIMIENTO.get(f.get("tipo"))
            clave = str(f.get("clave_servidor") or "").lower()
            if not campo or datos.club(clave) is None:
                log.warning("movimiento %s sin club para %s: queda pendiente", f.get("id"), clave)
                continue
            # primero se marca en la nube (si falla, no se aplica: nunca se da dos veces)
            if self.secreto:
                if not await self._llamar("phoenix_srv_marcar_movimiento", p_id=int(f["id"])):
                    continue  # ya estaba aplicado (otra vuelta u otro servidor): no se da dos veces
            else:
                await self._red("PATCH", "myclub_movimientos", f"id=eq.{int(f['id'])}&aplicado=is.null",
                                {"aplicado": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}, "return=minimal")
            club = datos.club(clave)  # releído tras la espera: puede haber cambiado mientras tanto
            club[campo] = max(0, int(club.get(campo) or 0) + int(f.get("cantidad") or 0))
            club.setdefault("admin", []).append({"hora": int(time.time()), "orden": f"web:{f.get('tipo')}",
                                                 "cantidad": int(f.get("cantidad") or 0),
                                                 "motivo": str(f.get("motivo") or "")[:200]})
            club["admin"] = club["admin"][-100:]
            datos.guardar_club(clave, club)
            hechos += 1
        return hechos

    def _usuario_de_token(self, token):
        req = urllib.request.Request(self.base + "/auth/v1/user", method="GET", headers={
            "apikey": self.clave, "Authorization": f"Bearer {token}", "Accept": "application/json"})
        try:
            with self._abrir(req, timeout=15) as r:
                crudo = r.read()
        except urllib.error.HTTPError as e:
            if e.code in (401, 403):
                return None  # sesión caducada o falsa
            raise
        datos = json.loads(crudo) if crudo else {}
        return datos.get("id")

    async def usuario_de_token(self, token):
        """Supabase Auth confirma la sesión de la web que manda el launcher -> uuid o None."""
        return await asyncio.to_thread(self._usuario_de_token, token)

    async def canjear_pase(self, pase):
        """Marca el pase como usado en Supabase y devuelve el usuario (uuid) o None.

        Un solo PATCH condicionado (sin usar y sin caducar): si dos lo intentan a la vez, solo uno
        recibe la fila. Solo viaja el hash; el pase en claro no sale de esta PC."""
        import hashlib
        h = hashlib.sha256(pase.encode()).hexdigest()
        if self.secreto:
            return await self._llamar("phoenix_srv_canjear_pase", p_hash=h)
        ahora = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        filas = await self._red("PATCH", "phoenix_pases",
                                f"hash=eq.{h}&usado=is.null&caduca=gt.{ahora}&select=usuario",
                                {"usado": ahora}, "return=representation") or []
        return filas[0].get("usuario") if filas else None

    async def sincronizar_vinculos(self):
        """Sube los códigos de «Vincular mi PES» y recoge los que la persona ya usó en la web."""
        datos = self.srv.datos
        pendientes = datos.codigos_pendientes()
        if not pendientes:
            return 0
        nuevos = [(c, cod, h) for c, cod, sub, h in pendientes if not sub]
        if nuevos:
            filas = [{"clave_servidor": c, "codigo": cod,
                      "caduca": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(h + datos.VIDA_CODIGO))}
                     for c, cod, h in nuevos]
            if self.secreto:
                await self._llamar("phoenix_srv_subir_codigos", p_filas=filas)
            else:
                await self._red("POST", "phoenix_vinculos", "on_conflict=clave_servidor", filas,
                                "resolution=merge-duplicates,return=minimal")
            datos.marcar_codigos_subidos([c for c, _cod, _h in nuevos])
        hechos = 0
        claves = [c for c, *_ in pendientes]
        for i in range(0, len(claves), 100):
            if self.secreto:
                filas = await self._llamar("phoenix_srv_duenos", p_claves=claves[i:i + 100]) or []
            else:
                lista = ",".join(urllib.parse.quote(c) for c in claves[i:i + 100])
                filas = await self._red("GET", "phoenix_duenos",
                                        f"clave_servidor=in.({lista})&select=clave_servidor,usuario") or []
            for f in filas:
                datos.vincular(f["clave_servidor"], f["usuario"])
                log.info("nube: el club %s quedó vinculado a una cuenta de la web", f["clave_servidor"])
                hechos += 1
        return hechos

    async def vuelta(self):
        if not getattr(self, "_todo_subido", False):
            # primera vuelta: sube TODOS los clubes, así la web los ve aunque nadie juegue hoy
            if hasattr(self.srv.datos, "claves_de_clubes"):
                self.srv.datos.marcar_cambiados(self.srv.datos.claves_de_clubes())
            self._todo_subido = True
        await self.sincronizar_vinculos()
        subidos = await self.subir_clubes()
        cambiadas = await self.leer_reglas()
        movs = await self.aplicar_movimientos()
        if subidos or cambiadas or movs:
            log.info("nube: %d clubes subidos, reglas %s, %d movimientos aplicados",
                     subidos, cambiadas or "sin cambios", movs)

    async def correr(self):
        while True:
            try:
                await self.vuelta()
                if self.fallos_seguidos:
                    log.info("nube: conexión con Supabase recuperada")
                self.fallos_seguidos = 0
            except Exception as e:
                self.fallos_seguidos += 1
                if self.fallos_seguidos in (1, 10) or self.fallos_seguidos % 100 == 0:
                    log.warning("nube: Supabase no responde (%s); el servidor sigue en local", e)
            # si falla mucho, espera más (máximo 10 minutos) para no insistir de más
            await asyncio.sleep(min(600, self.cada_s * (1 + min(self.fallos_seguidos, 20))))


def desde_config(srv, cfg):
    s = cfg.get("supabase") or {}
    if not s.get("activo"):
        return None
    if not s.get("url"):
        log.warning("supabase activo pero falta la url: espejo apagado")
        return None
    secreto = os.environ.get("PHOENIX_SERVIDOR_SECRETO")
    if secreto and s.get("clave_publica"):
        log.info("espejo en Supabase: modo sin llave maestra (funciones phoenix_srv_*)")
        return Nube(srv, s["url"], s["clave_publica"], s.get("cada_s", 30), secreto=secreto)
    clave = os.environ.get("PHOENIX_SUPABASE_KEY")
    if clave:
        log.warning("espejo en Supabase con la llave maestra (service_role): mejor el modo sin llave maestra")
        return Nube(srv, s["url"], clave, s.get("cada_s", 30))
    log.warning("supabase activo pero falta PHOENIX_SERVIDOR_SECRETO (+ clave_publica): espejo apagado")
    return None
