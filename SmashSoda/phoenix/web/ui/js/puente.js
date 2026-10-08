// =============================================================================
//  Puente con el motor (C++). Protocolo (ver PROTOCOLO.md):
//    JS → C++ : {t:"hola"} · {t:"pedir", id, accion, datos}
//    C++ → JS : {t:"bienvenida", datos} · {t:"resp", id, ok, datos|error}
//               {t:"estado", datos} · {t:"evento", nombre, datos}
//  Dentro de Phoenix Link usa window.chrome.webview; en un navegador normal
//  (vista previa) usa el simulador, que implementa las mismas acciones.
// =============================================================================

const ESPERA_NORMAL_MS = 8000;
const ESPERA_WEB_MS = 30000; // acciones que esperan a la web de la liga
const ACCIONES_LENTAS = new Set(["amigos.invitar", "web.soltarRival", "diag.ejecutar", "sala.abrir", "sala.aplicar"]);

export class ErrorPuente extends Error {
  constructor(codigo, mensaje) {
    super(mensaje || codigo);
    this.codigo = codigo;
  }
}

class Puente {
  constructor() {
    this.siguienteId = 1;
    this.pendientes = new Map();
    this.oyentes = new Map();
    this.transporte = null;
    this.simulado = false;
  }

  async iniciar() {
    const wv = window.chrome && window.chrome.webview;
    if (wv && typeof wv.postMessage === "function") {
      wv.addEventListener("message", (e) => this._recibir(e.data));
      this.transporte = (msg) => wv.postMessage(msg);
    } else {
      // Vista previa fuera de la app: motor simulado con datos de ejemplo
      const { crearSimulador } = await import("./simulador.js");
      this.simulado = true;
      const sim = crearSimulador((msg) => setTimeout(() => this._recibir(msg), 0));
      this.transporte = (msg) => setTimeout(() => sim.recibir(JSON.parse(JSON.stringify(msg))), 0);
    }
    this.transporte({ t: "hola" });
  }

  /** Pide una acción al motor. Devuelve una promesa con los datos o lanza ErrorPuente. */
  pedir(accion, datos = {}) {
    if (!this.transporte) return Promise.reject(new ErrorPuente("SIN_MOTOR", "El motor no está listo."));
    const id = this.siguienteId++;
    const espera = ACCIONES_LENTAS.has(accion) ? ESPERA_WEB_MS : ESPERA_NORMAL_MS;
    return new Promise((resolver, rechazar) => {
      const temporizador = setTimeout(() => {
        this.pendientes.delete(id);
        rechazar(new ErrorPuente("SIN_RESPUESTA", "El motor no respondió a tiempo."));
      }, espera);
      this.pendientes.set(id, { resolver, rechazar, temporizador });
      try {
        this.transporte({ t: "pedir", id, accion, datos });
      } catch (e) {
        clearTimeout(temporizador);
        this.pendientes.delete(id);
        rechazar(new ErrorPuente("ENVIO", String(e && e.message || e)));
      }
    });
  }

  on(tipo, fn) {
    if (!this.oyentes.has(tipo)) this.oyentes.set(tipo, new Set());
    this.oyentes.get(tipo).add(fn);
    return () => this.oyentes.get(tipo).delete(fn);
  }

  _emitir(tipo, datos) {
    const set = this.oyentes.get(tipo);
    if (!set) return;
    for (const fn of set) {
      try { fn(datos); } catch (e) { console.error("[puente] oyente de", tipo, e); }
    }
  }

  _recibir(msg) {
    if (typeof msg === "string") {
      try { msg = JSON.parse(msg); } catch { return; }
    }
    if (!msg || typeof msg !== "object") return;
    switch (msg.t) {
      case "resp": {
        const p = this.pendientes.get(msg.id);
        if (!p) return;
        this.pendientes.delete(msg.id);
        clearTimeout(p.temporizador);
        if (msg.ok) p.resolver(msg.datos ?? {});
        else p.rechazar(new ErrorPuente(msg.error?.codigo || "ERROR", msg.error?.mensaje || "Error"));
        return;
      }
      case "bienvenida": this._emitir("bienvenida", msg.datos || {}); return;
      case "estado": this._emitir("estado", msg.datos || {}); return;
      case "evento": this._emitir("evento:" + msg.nombre, msg.datos || {}); return;
      default: return;
    }
  }
}

export const puente = new Puente();
