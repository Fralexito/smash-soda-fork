"""Relé TURN propio para el partido entre 2 PC (camino probado por AzrakHQ con 2 PES el 06-nov-2026).

El PES solo usa relé si CMD_GET_TURN_SERVER_LIST trae un servidor con port Y tls_port distintos de 0.
Su cliente TURN («ktc») entra por TLS a tls_port con id/pw, confía en el certificado DER que va en
server_certificate y luego pasa a TURN sobre DTLS (puerto_dtls). Lo hace turnrelay.py/turndtls.py
(copiados de AzrakHQ, MIT). Aquí solo: secreto, certificado propio y arranque.

Configuración (config.json, todo opcional):
    "turn_propio": {"activo": true, "puerto_tls": 5349, "puerto": 3479, "puerto_dtls": 5350,
                    "puertos_rele": [49160, 49179], "carpeta": "turn", "ips": ["100.x.y.z"]}
Necesita los paquetes `cryptography` (certificado) y `pyOpenSSL` (DTLS). Sin ellos se apaga solo
y lo dice en el registro; el resto del servidor sigue igual.
Nada de esto instala certificados en Windows: el PES recibe el certificado dentro de la respuesta.
"""
import base64
import datetime
import ipaddress
import logging
import os
import time

log = logging.getLogger("phoenix.turn")

REALM = "pes2021"
VIGENCIA_CREDENCIAL = 12 * 3600


class TurnPropio:
    def __init__(self, cfg):
        c = dict(cfg or {})
        self.puerto_tls = int(c.get("puerto_tls", 5349))
        self.puerto = int(c.get("puerto", 3479))
        self.puerto_dtls = int(c.get("puerto_dtls", 5350))
        r = c.get("puertos_rele") or [49160, 49179]
        self.puertos_rele = (int(r[0]), int(r[1]))
        self.carpeta = c.get("carpeta", "turn")
        self.ips_extra = [i for i in (c.get("ips") or []) if _es_ip(i)]
        self.servidor = None
        self.secreto = None
        self.cert_der = None

    # ---- arranque ----
    async def iniciar(self, anunciar, ips_locales=()):
        """anunciar(ip_local, ip_cliente) -> IP que se le da a ese PES como dirección del relé."""
        try:
            from . import turnrelay, turndtls
        except Exception as e:  # noqa: BLE001
            log.error("TURN propio apagado: no se pudo cargar el relé (%s)", e)
            return False
        if not turndtls.available():
            log.error("TURN propio apagado: falta pyOpenSSL (pip install pyopenssl cryptography). "
                      "Sin DTLS el PES reinicia el TURN cada segundo.")
            return False
        os.makedirs(self.carpeta, exist_ok=True)
        ips = sorted({i for i in (*ips_locales, *self.ips_extra, "127.0.0.1") if i and _es_ip(i)})
        try:
            self.secreto = self._leer_secreto()
            self.cert_der = self._certificado(ips)
        except ImportError:
            log.error("TURN propio apagado: falta el paquete cryptography (pip install cryptography)")
            return False
        srv = turnrelay.TurnServer(self.secreto, REALM, self.puertos_rele, anunciar, own_ips=ips)
        try:
            await srv.start("0.0.0.0", self.puerto_tls, os.path.join(self.carpeta, "turn.crt"),
                            os.path.join(self.carpeta, "turn.key"), dtls_port=self.puerto_dtls)
        except OSError as e:
            log.error("TURN propio: no se pudo escuchar en tcp/%d o udp/%d: %s", self.puerto_tls,
                      self.puerto_dtls, e)
            srv.close()
            return False
        self.servidor = srv
        log.info("TURN propio en tls/%d + dtls/%s (relé udp %d-%d)", self.puerto_tls,
                 srv.dtls_port or "apagado", *self.puertos_rele)
        return True

    def activo(self):
        return self.servidor is not None

    def cerrar(self):
        s, self.servidor = self.servidor, None
        if s is not None:
            s.close()

    # ---- respuesta a CMD_GET_TURN_SERVER_LIST ----
    def credenciales(self, pid, ahora=None):
        from .turnrelay import rest_password
        usuario = f"{int((ahora or time.time()) + VIGENCIA_CREDENCIAL)}:{pid}"
        return usuario, rest_password(self.secreto, usuario)

    def lista(self, pid, direccion):
        usuario, clave = self.credenciales(pid)
        return {"turn_server_list": [{"zone": 1, "address": direccion, "port": self.puerto,
                                      "tls_port": self.puerto_tls}],
                "id": usuario, "pw": clave,
                "server_certificate": base64.b64encode(self.cert_der).decode(),
                "server_certificate_data_size": len(self.cert_der)}

    # ---- secreto y certificado (en la carpeta del servidor, nunca en el repo) ----
    def _leer_secreto(self):
        ruta = os.path.join(self.carpeta, "secreto")
        try:
            with open(ruta, encoding="utf-8") as f:
                s = f.read().strip()
            if s:
                return s
        except OSError:
            pass
        s = base64.urlsafe_b64encode(os.urandom(24)).decode().rstrip("=")
        with open(os.open(ruta, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600), "w", encoding="utf-8") as f:
            f.write(s)
        return s

    def _certificado(self, ips):
        """Certificado propio firmado por sí mismo con las IP por las que entran los jugadores. Se
        rehace si cambian las IP o le quedan menos de 30 días (R: turn.py AzrakHQ)."""
        from cryptography import x509
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import ec
        from cryptography.x509.oid import NameOID
        crt, key = os.path.join(self.carpeta, "turn.crt"), os.path.join(self.carpeta, "turn.key")
        quiero = sorted(set(ips))
        ahora = datetime.datetime.now(datetime.timezone.utc)
        try:
            with open(crt, "rb") as f:
                c = x509.load_pem_x509_certificate(f.read())
            tengo = sorted(str(i) for i in c.extensions.get_extension_for_class(x509.SubjectAlternativeName)
                           .value.get_values_for_type(x509.IPAddress))
            if tengo == quiero and c.not_valid_after_utc > ahora + datetime.timedelta(days=30):
                return c.public_bytes(serialization.Encoding.DER)
        except (OSError, ValueError, x509.ExtensionNotFound):
            pass
        k = ec.generate_private_key(ec.SECP256R1())
        nombre = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "Phoenix Online TURN")])
        c = (x509.CertificateBuilder().subject_name(nombre).issuer_name(nombre).public_key(k.public_key())
             .serial_number(x509.random_serial_number()).not_valid_before(ahora - datetime.timedelta(days=1))
             .not_valid_after(ahora + datetime.timedelta(days=3650))
             .add_extension(x509.SubjectAlternativeName([x509.IPAddress(ipaddress.ip_address(i))
                                                         for i in quiero]), False)
             .add_extension(x509.BasicConstraints(ca=True, path_length=None), True)
             .sign(k, hashes.SHA256()))
        with open(os.open(key, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600), "wb") as f:
            f.write(k.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.TraditionalOpenSSL,
                                    serialization.NoEncryption()))
        with open(crt, "wb") as f:
            f.write(c.public_bytes(serialization.Encoding.PEM))
        log.info("TURN propio: certificado nuevo para %s", ", ".join(quiero))
        return c.public_bytes(serialization.Encoding.DER)


def _es_ip(s):
    try:
        ipaddress.ip_address(str(s))
        return True
    except ValueError:
        return False
