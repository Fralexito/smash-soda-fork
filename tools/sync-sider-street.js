// ==============================================================================
// Phoenix Link — Sincronizador en Caliente de Físicas Sider (Modo Street)
// Autor: Antigravity (Gemini) · Google DeepMind
// Fecha: 29 de Octubre de 2026
//
// Este micro-demonio corre en segundo plano y recibe las peticiones desde la UI
// de Phoenix Link para modificar phoenix_core.json en vivo mientras PES 2021 corre.
// ==============================================================================

const http = require("http");
const fs = require("fs");
const path = require("path");

const PUERTO = 38888;
const RUTA_JSON = "D:\\Frank\\Games_\\Conmegol Patch\\SiderAddons\\phoenix_core\\phoenix_core.json";

function responderCORS(res, status, datos) {
  res.writeHead(status, {
    "Content-Type": "application/json",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type"
  });
  res.end(JSON.stringify(datos));
}

const server = http.createServer((req, res) => {
  if (req.method === "OPTIONS") {
    responderCORS(res, 204, {});
    return;
  }

  // GET /status: Consulta el estado del archivo phoenix_core.json
  if (req.method === "GET" && req.url === "/status") {
    if (!fs.existsSync(RUTA_JSON)) {
      responderCORS(res, 404, { ok: false, error: "Archivo phoenix_core.json no encontrado" });
      return;
    }
    try {
      const contenido = JSON.parse(fs.readFileSync(RUTA_JSON, "utf8"));
      responderCORS(res, 200, {
        ok: true,
        muro_rebote: contenido.funciones?.muro_rebote,
        muro_friccion: contenido.funciones?.muro_friccion,
        pase_vel_k: contenido.funciones?.pase_vel_k,
        cancha_semilargo: contenido.cancha_semilargo,
        cancha_semiancho: contenido.cancha_semiancho
      });
    } catch (err) {
      responderCORS(res, 500, { ok: false, error: err.message });
    }
    return;
  }

  // POST /sync-street: Aplica los valores de físicas directamente
  if (req.method === "POST" && req.url === "/sync-street") {
    let body = "";
    req.on("data", chunk => { body += chunk; });
    req.on("end", () => {
      try {
        const datos = JSON.parse(body);
        if (!fs.existsSync(RUTA_JSON)) {
          responderCORS(res, 404, { ok: false, error: "No existe phoenix_core.json en la ruta Sider" });
          return;
        }

        const original = JSON.parse(fs.readFileSync(RUTA_JSON, "utf8"));
        if (!original.funciones) original.funciones = {};

        // Actualizar parámetros si vienen en el payload
        if (typeof datos.muro_rebote === "number") original.funciones.muro_rebote = datos.muro_rebote;
        if (typeof datos.muro_friccion === "number") original.funciones.muro_friccion = datos.muro_friccion;
        if (typeof datos.pase_vel_k === "number") original.funciones.pase_vel_k = datos.pase_vel_k;
        if (typeof datos.cancha_semilargo === "number") original.cancha_semilargo = datos.cancha_semilargo;
        if (typeof datos.cancha_semiancho === "number") original.cancha_semiancho = datos.cancha_semiancho;
        if (typeof datos.muros_core === "boolean") original.funciones.muros_core = datos.muros_core;
        if (typeof datos.cancha_paredes === "boolean") original.cancha_paredes = datos.cancha_paredes;

        fs.writeFileSync(RUTA_JSON, JSON.stringify(original, null, 2), "utf8");
        console.log(`[SYNC SIDER] Físicas actualizadas con éxito: rebote=${datos.muro_rebote}, friccion=${datos.muro_friccion}, preset=${datos.preset || 'manual'}`);

        responderCORS(res, 200, {
          ok: true,
          mensaje: "phoenix_core.json actualizado en caliente",
          aplicado: {
            muro_rebote: original.funciones.muro_rebote,
            muro_friccion: original.funciones.muro_friccion,
            pase_vel_k: original.funciones.pase_vel_k
          }
        });
      } catch (err) {
        console.error("[SYNC SIDER] Error al procesar:", err);
        responderCORS(res, 400, { ok: false, error: err.message });
      }
    });
    return;
  }

  responderCORS(res, 404, { error: "Ruta no encontrada" });
});

server.listen(PUERTO, "127.0.0.1", () => {
  console.log(`=======================================================`);
  console.log(` Phoenix Link — Sincronizador en Caliente de Físicas Sider`);
  console.log(` Escuchando en http://127.0.0.1:${PUERTO}`);
  console.log(` Destino: ${RUTA_JSON}`);
  console.log(`=======================================================`);
});
