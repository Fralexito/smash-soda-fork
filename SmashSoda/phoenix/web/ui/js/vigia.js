// Avisos automáticos a partir de los cambios de estado del motor
// (alguien entra, pide mando, espera decisión, la web se desconecta…).
import { puente } from "./puente.js";
import { avisar, irA } from "./tienda.js";

// Botón «Ver» de los avisos que piden una decisión: lleva a la portada, donde se decide
const ver = () => ({ texto: t("VER", "VIEW"), al: () => irA("sala", "resumen") });
import { t } from "./i18n.js";

export function vigilarCambios() {
  let previo = null;
  puente.on("estado", (m) => {
    if (!m) return;
    if (previo) revisar(previo, m);
    previo = m;
  });
  // Tras la bienvenida no se avisa de lo que ya estaba
  puente.on("bienvenida", (b) => { previo = b.estado || null; });
}

function ids(lista) { return new Set((lista || []).map((x) => x.parsecId)); }

function revisar(a, b) {
  // Gente que entra
  if (a.sala?.abierta && b.sala?.abierta) {
    const antes = ids(a.invitados);
    for (const g of b.invitados || []) {
      if (!antes.has(g.parsecId) && !g.falso) avisar(t(`${g.nombre} entró a la sala`, `${g.nombre} joined`), "info", 3500);
    }
  }
  // Solicitudes de mando (!cambio / !equipo)
  const solAntes = ids(a.solicitudes);
  for (const x of b.solicitudes || []) {
    if (!solAntes.has(x.parsecId)) avisar(t(`${x.nombre} pide el mando ${x.mando}.`, `${x.nombre} asks for pad ${x.mando}.`), "warn", 9000, ver());
  }
  // Entrada por Parsec esperando decisión
  const espAntes = ids(a.espera);
  for (const x of b.espera || []) {
    if (!espAntes.has(x.parsecId)) avisar(t(`${x.nombre} espera que decidas si juega o mira.`, `${x.nombre} is waiting for your decision.`), "warn", 9000, ver());
  }
  // Web
  if (a.web?.estado !== b.web?.estado) {
    if (b.web?.estado === "conectado") avisar(t("Conectado con la web de la liga.", "Connected to the league website."), "ok");
    else if (b.web?.estado === "sin_conexion") avisar(t("Sin conexión con la web. Se reintenta solo.", "League website offline. Retrying."), "warn");
  }
  // Alertas de red (5 s seguidos por encima de 100 ms)
  const alertasAntes = new Set((a.red || []).filter((r) => r.alerta).map((r) => r.parsecId));
  for (const r of b.red || []) {
    if (r.alerta && !alertasAntes.has(r.parsecId)) avisar(t(`Lag: ${r.nombre} va a ${r.ultimo} ms.`, `Lag: ${r.nombre} at ${r.ultimo} ms.`), "bad", 6000);
  }
  // Sala
  if (a.sala && b.sala && a.sala.abierta !== b.sala.abierta) {
    avisar(b.sala.abierta ? t("Sala abierta. ¡A jugar!", "Room open. Game on!") : t("Sala cerrada.", "Room closed."), b.sala.abierta ? "ok" : "info");
  }
}
