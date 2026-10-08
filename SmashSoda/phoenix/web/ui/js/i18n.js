// Textos: español como base; cualquier otro idioma del motor cae a inglés.
let idioma = "es";

export function fijarIdioma(codigo) {
  idioma = String(codigo || "es").toLowerCase();
  document.documentElement.lang = esEspanol() ? "es" : "en";
}
export function esEspanol() { return idioma.startsWith("es"); }

/** t("Abrir sala", "Open room") */
export function t(es, en) { return esEspanol() || !en ? es : en; }

export function duracion(seg) {
  if (seg == null || seg < 0) return "—";
  const h = Math.floor(seg / 3600), m = Math.floor((seg % 3600) / 60), s = Math.floor(seg % 60);
  const dos = (n) => String(n).padStart(2, "0");
  return h > 0 ? `${dos(h)}:${dos(m)}:${dos(s)}` : `${dos(m)}:${dos(s)}`;
}

export function haceCuanto(ms) {
  if (!ms) return "";
  const s = Math.max(0, Math.round((Date.now() - ms) / 1000));
  if (s < 10) return t("AHORA", "NOW");
  if (s < 60) return t(`HACE ${s} S`, `${s} S AGO`);
  const m = Math.round(s / 60);
  if (m < 60) return t(`HACE ${m} MIN`, `${m} MIN AGO`);
  const h = Math.floor(m / 60);
  return t(`HACE ${h} H ${m % 60} MIN`, `${h} H ${m % 60} MIN AGO`);
}

export function fecha(ms) {
  if (!ms) return "—";
  try {
    return new Date(ms).toLocaleString(esEspanol() ? "es-PE" : "en-US", { day: "2-digit", month: "short", hour: "2-digit", minute: "2-digit" });
  } catch { return new Date(ms).toISOString(); }
}
