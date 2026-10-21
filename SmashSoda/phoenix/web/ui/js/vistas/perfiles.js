// =============================================================================
//  PERFILES DE AMIGOS (GENTE › PERFILES PES). PES guarda solo 32 «Datos personales» (Usuario 1…32: nombre + configuración).
//  Link guarda los de TODOS tus amigos y pone en PES solo los de hoy. Dos pasos, con PES cerrado:
//    1) TRAER: copia los Usuarios de PES a Link (solo lee PES; así nada se pierde).
//    2) PREPARAR LA SALA: eliges quiénes juegan hoy (hasta lo que quepa) y Link los pone en los huecos de PES.
//  Guía: docs/MANDOS.md («PERFILES-DE-AMIGOS»)
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Chip, cx } from "../ui.js";

// Etiquetas de «Editar Datos personales» (PES 2021 ConmeGOL). Los valores son los BYTES que guarda PES; otros parches pueden
// llamar distinto a las opciones, por eso el texto vive aquí y no en el código de Link.
const SINO = [[0, "Sí"], [1, "No"]];
const BASICO = [[0, "Básico"], [1, "Avanzado"]];
const GRUPOS = [
  { titulo: "Cursor", campos: [
    ["cambiarCursor", "Cambiar cursor", [[1, "Semiasistido"], [2, "Sin ayuda"], [0, "Asistido (sin comprobar)"]]],
    ["cursorSiguiente", "Cursor de siguiente jugador", SINO],
  ] },
  { titulo: "Pases y disparo", campos: [
    ["nivelPases", "Nv. de ayuda en pases", [[1, "Nivel 1"], [2, "Nivel 2"]]],
    ["paseProfundidad", "Tipo de pase en profundidad", BASICO],
    ["disparo", "Tipo de disparo", BASICO],
  ] },
  { titulo: "Ayudas", campos: [
    ["guiaDireccion", "Guía de dirección", SINO],
    ["guiaObjetivo", "Guía de objetivo", SINO],
    ["guiaPosicion", "Guía de posicionamiento", SINO],
    ["amago", "Amago automático", SINO],
    ["entrada", "Entrada automática", [[1, "Semiautomático"], [2, "No"], [0, "Automático (sin comprobar)"]]],
    ["companero", "Controles de compañero", [[0, "Asistido"], [1, "Manual"]]],
  ] },
];
const BOTONES = ["×", "○", "□", "△", "L1", "R1", "L2", "R2", "Ninguno"];
const ETIQ_ATAQUE = ["Pase bombeado", "Pase en profundidad", "Disparo", "Pase raso", "Cambiar cursor", "Controles manuales", "Esprintar", "Controles especiales", "Botón 9", "Botón 10"];
const ETIQ_DEFENSA = ["Entrada deslizante", "Salir con el portero", "Presión de compañeros", "Presión", "Botón 5", "Botón 6", "Botón 7", "Botón 8", "Botón 9", "Botón 10"];

export function PerfilesPes() {
  const [d, setD] = useState(null);
  const [sel, setSel] = useState(null);       // ids elegidos «para hoy» (se inicializa con lo guardado)
  const [buscar, setBuscar] = useState("");
  const [editando, setEditando] = useState(null);
  const [nuevo, setNuevo] = useState("");
  const [ocupado, setOcupado] = useState(false);
  const [puestoV, setPuestoV] = useState(1);   // prueba en vivo: puesto 1…8
  const [perfilV, setPerfilV] = useState("");  // prueba en vivo: perfil elegido
  const [mandos, setMandos] = useState(null);  // prueba en vivo: Usuario que tiene cada mando
  const [perfilL, setPerfilL] = useState("");  // ligar por ID: perfil elegido
  const [idL, setIdL] = useState("");          // ligar por ID: ID de Parsec
  const [form, setForm] = useState(null);   // formulario «Mi perfil de PES»: {id, nombre, parsecId, opciones}

  const cargar = async () => { const r = await accion("perfiles.estado", {}, { silencioso: true }); if (r) { setD(r); setSel((s) => s || r.seleccion || []); } };
  useEffect(() => { cargar(); const id = setInterval(cargar, 3000); return () => clearInterval(id); }, []);

  if (!d) return html`<${Tarjeta} interior="padding:22px 24px"><div class="ayuda">${t("Revisando…", "Checking…")}</div></${Tarjeta}>`;
  const perfiles = d.perfiles || [];
  const huecos = d.huecos || [];
  const elegidos = sel || [];
  const protegidos = huecos.filter((h) => h.protegido).length;
  const caben = 32 - protegidos;
  const q = buscar.trim().toLowerCase();
  const lista = perfiles.filter((p) => !q || p.nombre.toLowerCase().includes(q)).sort((a, b) => (elegidos.includes(b.id) - elegidos.includes(a.id)) || a.nombre.localeCompare(b.nombre));
  const sinGuardado = !d.ruta;

  const poner = async (ids) => { setSel(ids); accion("perfiles.seleccion", { ids }, { silencioso: true }); };
  const alternar = (id) => poner(elegidos.includes(id) ? elegidos.filter((x) => x !== id) : [...elegidos, id]);
  const resumen = (inf) => inf ? t(`${inf.nuevos} nuevo(s), ${inf.actualizados} actualizado(s), ${inf.iguales} sin cambios.`, `${inf.nuevos} new, ${inf.actualizados} updated, ${inf.iguales} unchanged.`) : "";

  const traer = async () => {
    setOcupado(true);
    const r = await accion("perfiles.traer", {});
    setOcupado(false);
    if (r) { setD(r); avisar(t("Usuarios de PES guardados en Link. ", "PES users saved in Link. ") + resumen(r.informe), "ok", 6000); }
  };
  const preparar = async () => {
    if (!(await confirmar({ titulo: t("¿Preparar la sala?", "Prepare the room?"), texto: t(`Link pondrá a estos ${elegidos.length} amigos en los huecos de PES (con copia de seguridad antes). Lo que había en esos huecos queda guardado en Link.`, `Link will put these ${elegidos.length} friends in PES's slots (with a backup first). What was in those slots stays saved in Link.`) }))) return;
    setOcupado(true);
    const r = await accion("perfiles.preparar", { ids: elegidos });
    setOcupado(false);
    if (r) { setD(r); avisar(t(`Listo: ${elegidos.length} amigos en PES. Abre PES y elige tu nombre con L2/R2 en la pantalla de mandos.`, `Done: ${elegidos.length} friends in PES. Open PES and pick your name with L2/R2 on the controller screen.`), "ok", 8000); }
  };
  const proteger = async (n, si) => { const r = await accion("perfiles.proteger", { hueco: n, si }); if (r) setD(r); };
  // ---- PRUEBA EN VIVO (beta): poner un perfil en el Usuario que tiene un mando, con PES ABIERTO ----
  const verMandos = async () => { const r = await accion("perfiles.mandos", {}); if (r) setMandos(r.mandos || []); };
  const aplicarVivo = async () => {
    if (!perfilV) return;
    const p = perfiles.find((x) => x.id === perfilV);
    if (!(await confirmar({ titulo: t("¿Poner este perfil en el mando?", "Put this profile on the controller?"), texto: t(`Link escribirá «${p?.nombre || ""}» en el Usuario que tiene el mando ${puestoV} de PES, con PES abierto. Lo que había queda guardado en Link y se puede restaurar.`, `Link will write «${p?.nombre || ""}» into the user controller ${puestoV} has in PES, with PES open. What was there stays saved in Link and can be restored.`) }))) return;
    setOcupado(true);
    const r = await accion("perfiles.aplicar", { puesto: puestoV, id: perfilV });
    setOcupado(false);
    if (r) { setD(r); verMandos(); avisar(t(`Listo: el mando ${puestoV} ahora usa «${r.informe?.vivoAplicado?.nombre || p?.nombre || ""}». Míralo en la pantalla de elegir lados.`, `Done: controller ${puestoV} now uses «${p?.nombre || ""}». Check the side-selection screen.`), "ok", 7000); }
  };
  const restaurarVivo = async () => { setOcupado(true); const r = await accion("perfiles.restaurar", {}); setOcupado(false); if (r) { setD(r); verMandos(); avisar(r.vivoAviso || t("Usuarios restaurados como estaban.", "Users restored as they were."), "ok", 6000); } };
  const borrar = async (p) => {
    if (!(await confirmar({ titulo: t("¿Borrar este perfil de Link?", "Delete this profile from Link?"), texto: t(`«${p.nombre}» se quita de Link (queda una copia en la carpeta de respaldos). No toca PES.`, `«${p.nombre}» is removed from Link (a copy stays in the backups folder). PES is not touched.`) }))) return;
    const r = await accion("perfiles.borrar", { id: p.id });
    if (r) { setD(r); setSel((s) => (s || []).filter((x) => x !== p.id)); }
  };
  const renombrar = async () => {
    const r = await accion("perfiles.renombrar", { id: editando, nombre: nuevo.trim() });
    if (r) { setD(r); setEditando(null); setNuevo(""); avisar(t("Nombre cambiado.", "Name changed."), "ok", 4000); }
  };

  const abrirForm = async (id) => {
    const r = await accion("perfiles.opciones", id ? { id } : {});
    if (r) { const pf = perfiles.find((x) => x.id === id); setForm({ id: id || "", nombre: r.nombre || "", parsecId: pf && pf.parsecId ? String(pf.parsecId) : "", opciones: r.opciones }); }
  };
  const setOp = (k, v) => setForm((f) => ({ ...f, opciones: { ...f.opciones, [k]: v } }));
  const setBoton = (i, v) => setForm((f) => ({ ...f, opciones: { ...f.opciones, botones: f.opciones.botones.map((b, j) => (j === i ? v : b)) } }));
  const guardarForm = async () => {
    const pid = Number(form.parsecId);
    const datos = { id: form.id || undefined, nombre: form.nombre.trim(), opciones: form.opciones };
    if (form.parsecId !== "" && pid > 0) datos.parsecId = pid; else if (form.id) datos.parsecId = 0;
    setOcupado(true);
    const r = await accion("perfiles.editar", datos);
    setOcupado(false);
    if (r) { setD(r); setForm(null); avisar(t("Perfil guardado. Si esa persona se sienta, Link se lo pone solo.", "Profile saved. If that person sits, Link applies it automatically."), "ok", 6000); }
  };
  const fila = (etiqueta, control) => html`<div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap"><span style="font-size:13px;flex:1;min-width:170px">${etiqueta}</span>${control}</div>`;
  const selOp = (k, lista) => html`<select class="campo" style="max-width:230px" value=${form.opciones[k]} onChange=${(e) => setOp(k, Number(e.currentTarget.value))}>${lista.map(([v, tx]) => html`<option key=${v} value=${v}>${tx}</option>`)}</select>`;
  const bloqueBotones = (titulo, etiquetas, desde) => html`<div class="mono mut" style="font-size:12px;margin-top:6px">${titulo}</div>${etiquetas.map((e, i) => fila(e, html`<select class="campo" style="max-width:120px" value=${form.opciones.botones[desde + i]} onChange=${(ev) => setBoton(desde + i, Number(ev.currentTarget.value))}>${BOTONES.map((b, v) => html`<option key=${v} value=${v}>${b}</option>`)}</select>`))}`;

  return html`<div class="col" style="gap:14px">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("USUARIO AUTOMÁTICO", "AUTOMATIC USER")} derecha=${html`<${Chip} tipo=${d.auto ? "ok" : "warn"}>${d.auto ? t("ACTIVADO", "ON") : t("APAGADO", "OFF")}</${Chip}>`}/>
      <div class="ayuda" style="font-size:14px">${t("Liga a cada persona con su perfil. Cuando se siente en un puesto (en la pantalla de elegir lados de PES), Link le pone SU nombre y SU configuración en el Usuario de su mando, sin que haga nada. Al levantarse, Link devuelve lo que había. Si la persona cambia sus opciones en PES, Link lo guarda en su perfil.", "Link each person to their profile. When they take a seat (on PES's side-selection screen), Link puts THEIR name and THEIR settings on their controller's user automatically. When they leave, Link gives back what was there. If they change their options in PES, Link saves it to their profile.")}</div>
      <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap">
        <${Boton} tipo=${d.auto ? "suave" : "lleno"} deshabilitado=${ocupado} al=${async () => { const r = await accion("perfiles.auto", { si: !d.auto }); if (r) setD(r); }}>${d.auto ? t("APAGAR AUTOMÁTICO", "TURN OFF") : t("ACTIVAR AUTOMÁTICO", "TURN ON")}</${Boton}>
        ${d.autoLog ? html`<span class="ayuda" style="font-size:13px">${d.autoLog}</span>` : null}
      </div>
      <div class="mono mut" style="font-size:12px">${t("QUIÉN ESTÁ SENTADO", "WHO IS SEATED")}</div>
      ${(d.sentados || []).length ? html`<div style="display:flex;flex-direction:column;gap:6px">${d.sentados.map((s) => html`<div key=${s.puesto} class="caja" style="display:flex;gap:8px;align-items:center;flex-wrap:wrap;padding:6px 10px">
        <span class="mono mut" style="font-size:12px">${t("P", "S")}${s.puesto}</span>
        <span style="font-size:13px;flex:1;min-width:120px;overflow-wrap:anywhere">${s.nombre}${s.simulado ? "" : ""} <span class="mono mut" style="font-size:11px">#${s.parsecId}</span></span>
        <select class="campo" value=${s.perfil || ""} style="max-width:200px" onChange=${async (e) => { const id = e.currentTarget.value; const r = id ? await accion("perfiles.vincular", { id, parsecId: s.parsecId }) : (s.perfil ? await accion("perfiles.vincular", { id: s.perfil, parsecId: 0 }) : null); if (r) setD(r); }}>
          <option value="">${t("— sin perfil —", "— no profile —")}</option>
          ${perfiles.map((p) => html`<option key=${p.id} value=${p.id}>${p.nombre}</option>`)}
        </select>
        ${s.simulado ? html`<${Boton} tipo="suave mini" al=${async () => { const r = await accion("perfiles.simular", { puesto: s.puesto, parsecId: 0 }); if (r) setD(r); }}>${t("QUITAR", "REMOVE")}</${Boton}>` : null}
      </div>`)}</div>` : html`<div class="ayuda" style="font-size:13px">${t("Nadie sentado ahora.", "Nobody seated right now.")}</div>`}
      ${perfiles.filter((p) => p.parsecId).length ? html`<div class="mono mut" style="font-size:12px">${t("PERFILES LIGADOS", "LINKED PROFILES")}</div>
        <div style="display:flex;gap:6px;flex-wrap:wrap">${perfiles.filter((p) => p.parsecId).map((p) => html`<${Chip} key=${p.id}>${p.nombre} · #${p.parsecId} <a style="cursor:pointer;margin-left:6px" title=${t("Quitar vínculo", "Unlink")} onClick=${async () => { const r = await accion("perfiles.vincular", { id: p.id, parsecId: 0 }); if (r) setD(r); }}>✕</a></${Chip}>`)}</div>` : null}
      <details class="sec-detalle"><summary>${t("Ligar a alguien que no está sentado / probar sin gente", "Link someone who isn't seated / test without people")}</summary>
        <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-top:8px">
          <select class="campo" value=${perfilL} onChange=${(e) => setPerfilL(e.currentTarget.value)} style="flex:1;min-width:150px">
            <option value="">${t("Perfil…", "Profile…")}</option>
            ${perfiles.map((p) => html`<option key=${p.id} value=${p.id}>${p.nombre}</option>`)}
          </select>
          <input type="number" class="campo" min="1" value=${idL} placeholder=${t("ID de Parsec", "Parsec ID")} onInput=${(e) => setIdL(e.currentTarget.value)} style="max-width:150px"/>
          <${Boton} tipo="suave mini" deshabilitado=${!perfilL || !Number(idL)} al=${async () => { const r = await accion("perfiles.vincular", { id: perfilL, parsecId: Number(idL) }); if (r) setD(r); }}>${t("LIGAR", "LINK")}</${Boton}>
          <${Boton} tipo="suave mini" deshabilitado=${!Number(idL)} al=${async () => { const r = await accion("perfiles.simular", { puesto: puestoV, parsecId: Number(idL) }); if (r) setD(r); }}>${t("SENTAR DE PRUEBA EN P", "TEST-SEAT AT S")}${puestoV}</${Boton}>
        </div>
        <div class="ayuda" style="font-size:12px;margin-top:6px">${t("El puesto se elige abajo, en «Prueba en vivo». Un sentado de prueba no es gente de verdad: sirve para ver cómo Link le pone el perfil.", "The slot is chosen below, in «Live test». A test seat isn't a real person: it shows how Link applies the profile.")}</div>
      </details>
    </${Tarjeta}>


    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("MIS PERFILES", "MY PROFILES")} derecha=${html`<span class="mono mut" style="font-size:13px">${perfiles.length}</span>`}/>
      <div class="ayuda" style="font-size:14px">${t("Cada perfil es el nombre y la configuración de una persona. Ligado = Link se lo pone solo cuando se siente.", "Each profile is a person's name and settings. Linked = Link applies it automatically when they sit.")}</div>
      ${d.error ? html`<div class="caja" style="font-size:13px"><span class="punto bad"></span> ${d.error}</div>` : null}
      ${form ? html`<div class="caja" style="display:flex;flex-direction:column;gap:10px;padding:14px 16px;border:1px solid var(--accent, #22d3ee)">
        <div class="mono" style="font-size:13px">${form.id ? t("EDITAR PERFIL", "EDIT PROFILE") : t("PERFIL NUEVO", "NEW PROFILE")}</div>
        <div class="ayuda" style="font-size:13px">${t("Es lo mismo que «Editar Datos personales» de PES. Escribe el nombre como quiere aparecer la persona y elige sus opciones. Cuando se siente, Link se lo pone solo.", "Same as PES's «Edit personal data». Type the name as the person wants to appear and choose their options. When they sit, Link applies it automatically.")}</div>
        ${fila(t("Datos personales (nombre)", "Personal data (name)"), html`<input type="text" class="campo" style="max-width:260px" maxLength=${31} value=${form.nombre} placeholder=${t("Nombre real (máx. 31)", "Real name (max 31)")} onInput=${(e) => { const v = e.currentTarget.value; setForm((f) => ({ ...f, nombre: v })); }}/>`)}
        ${fila(t("ID de Parsec de la persona (opcional)", "Person's Parsec ID (optional)"), html`<input type="number" class="campo" min="1" style="max-width:170px" value=${form.parsecId} placeholder="ID" onInput=${(e) => { const v = e.currentTarget.value; setForm((f) => ({ ...f, parsecId: v })); }}/>`)}
        <div class="mono mut" style="font-size:12px;margin-top:6px">${t("CONFIGURACIÓN DE COMANDOS", "BUTTON CONFIGURATION")}</div>
        ${fila(t("Preajuste", "Preset"), selOp("preajuste", [[0, "Normal"], [1, "Alternativa"], [2, "Personalizado (sin comprobar)"]]))}
        ${fila(t("Desplazamiento de jugadores", "Player movement"), selOp("desplazamiento", [[0, "Stick izquierdo"], [1, "Cruceta"]]))}
        ${form.opciones.preajuste === 2 ? html`<div class="ayuda" style="font-size:12px">${t("Botones del preajuste Personalizado. Los nombres de las acciones pueden variar según el parche; los botones son ×, ○, □, △ (C del teclado = □).", "Buttons for the Custom preset. Action names may vary by patch.")}</div>${bloqueBotones(t("ATAQUE", "ATTACK"), ETIQ_ATAQUE, 0)}${bloqueBotones(t("DEFENSA", "DEFENSE"), ETIQ_DEFENSA, 10)}` : null}
        ${GRUPOS.map((g) => html`<div key=${g.titulo}><div class="mono mut" style="font-size:12px;margin:6px 0">${g.titulo.toUpperCase()}</div><div style="display:flex;flex-direction:column;gap:6px">${g.campos.map(([k, e, lista]) => html`<div key=${k}>${fila(e, selOp(k, lista))}</div>`)}${g.titulo === "Cursor" ? fila(t("Tipo de cursor", "Cursor type"), html`<span class="mut" style="font-size:13px">${t("Datos personales (fijo)", "Personal data (fixed)")}</span>`) : null}</div></div>`)}
        <div style="display:flex;gap:8px;justify-content:flex-end;margin-top:6px">
          <${Boton} tipo="suave" al=${() => setForm(null)}>${t("CANCELAR", "CANCEL")}</${Boton}>
          <${Boton} tipo="lleno" deshabilitado=${ocupado || !form.nombre.trim()} al=${guardarForm}>${t("GUARDAR PERFIL", "SAVE PROFILE")}</${Boton}>
        </div>
      </div>` : null}
      <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
        <div style="flex:1;min-width:160px"><input type="text" class="campo" maxLength=${40} value=${buscar} placeholder=${t("Buscar amigo…", "Search friend…")} onInput=${(e) => setBuscar(e.currentTarget.value)}/></div>
        <${Boton} tipo="lleno mini" deshabilitado=${ocupado} al=${() => abrirForm("")}>${t("+ NUEVO PERFIL", "+ NEW PROFILE")}</${Boton}>
      </div>
      ${perfiles.length ? html`<div style="display:flex;flex-direction:column;gap:6px;max-height:420px;overflow:auto">
        ${perfiles.filter((p) => !q || p.nombre.toLowerCase().includes(q)).sort((a, b) => (!!b.parsecId - !!a.parsecId) || a.nombre.localeCompare(b.nombre)).map((p) => html`<div key=${p.id} class="caja" style="display:flex;flex-direction:column;gap:6px;padding:8px 12px">
          <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
            <b style="font-size:15px;overflow-wrap:anywhere;flex:1;min-width:0">${p.nombre}</b>
            ${p.parsecId ? html`<${Chip} tipo="ok">${t("LIGADO · #", "LINKED · #")}${p.parsecId}</${Chip}>` : html`<${Chip}>${t("SIN LIGAR", "NOT LINKED")}</${Chip}>`}
            <${Boton} tipo="suave mini" al=${() => abrirForm(p.id)}>${t("EDITAR", "EDIT")}</${Boton}>
            <${Boton} tipo="suave mini" al=${() => borrar(p)}>${t("BORRAR", "DELETE")}</${Boton}>
          </div>
        </div>`)}
      </div>` : html`<div class="ayuda" style="font-size:13px">${t("Aún no hay perfiles. Abre «Avanzado» y pulsa «Traer de PES».", "No profiles yet. Open «Advanced» and press «Bring from PES».")}</div>`}
    </${Tarjeta}>

    <details class="sec-detalle"><summary style="font-size:14px">${t("AVANZADO · traer de PES, preparar la sala con PES cerrado, los 32 huecos y la prueba en vivo", "ADVANCED · bring from PES, prepare the room with PES closed, the 32 slots and the live test")}</summary>
      <div class="col" style="gap:14px;margin-top:10px">
    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("PERFILES DE AMIGOS", "FRIEND PROFILES")} derecha=${html`<${Chip} tipo=${d.pesAbierto ? "warn" : "ok"}>${d.pesAbierto ? t("PES ABIERTO", "PES OPEN") : t("PES CERRADO", "PES CLOSED")}</${Chip}>`}/>
      <div class="ayuda" style="font-size:14px">${t("PES solo guarda 32 «Datos personales» (nombre + botones de cada amigo). Aquí guardas los de TODOS tus amigos y en PES pones solo los de hoy. Dos pasos, con PES cerrado.", "PES only keeps 32 «Personal data» (each friend's name + buttons). Here you keep ALL your friends' and put only today's in PES. Two steps, with PES closed.")}</div>
      ${sinGuardado ? html`<div class="caja" style="font-size:13px"><span class="punto bad"></span> ${t("No encuentro el guardado de PES. Abre PES, juega un partido y ciérralo.", "I can't find PES's save. Open PES, play a match and close it.")}</div>` : html`<div class="ayuda mono" style="font-size:12px">${d.ruta}</div>`}
      ${(d.candidatos || []).length > 1 ? html`<details class="sec-detalle"><summary>${t("Hay varios guardados: elegir otro", "Several saves: choose another")}</summary>
        <div style="display:flex;flex-direction:column;gap:6px">${d.candidatos.map((c) => html`<div key=${c} style="display:flex;gap:8px;align-items:center"><span class="mono" style="font-size:12px;flex:1;min-width:0;overflow-wrap:anywhere">${c}</span>${c === d.ruta ? html`<${Chip} tipo="ok">${t("EN USO", "IN USE")}</${Chip}>` : html`<${Boton} tipo="suave mini" al=${async () => { const r = await accion("perfiles.ruta", { ruta: c }); if (r) setD(r); }}>${t("USAR", "USE")}</${Boton}>`}</div>`)}</div></details>` : null}
      ${d.avisoAuto ? html`<div class="caja" style="font-size:13px"><span class="punto ok"></span> ${d.avisoAuto}</div>` : null}
      ${d.error ? html`<div class="caja" style="font-size:13px"><span class="punto bad"></span> ${d.error}</div>` : null}
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("1 · TRAER MIS USUARIOS DE PES", "1 · BRING MY PES USERS")}/>
      <div class="ayuda" style="font-size:14px">${t("Copia a Link los Usuarios que hay ahora en PES. Solo lee PES, no cambia nada. Hazlo cuando cambies algo en PES (Link también lo hace solo al cerrar PES).", "Copies the users currently in PES into Link. It only reads PES, nothing changes. Do it whenever you change something in PES (Link also does it by itself when PES closes).")}</div>
      <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap">
        <${Boton} tipo="lleno" deshabilitado=${sinGuardado || ocupado} al=${traer}>${t("TRAER DE PES", "BRING FROM PES")}</${Boton}>
        <span class="mono mut" style="font-size:13px">${perfiles.length} ${t("perfiles en Link", "profiles in Link")}</span>
      </div>
      ${(d.informe?.avisos || []).map((a, i) => html`<div class="ayuda" style="font-size:12px" key=${i}>⚠ ${a}</div>`)}
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("2 · ¿QUIÉNES JUEGAN HOY?", "2 · WHO PLAYS TODAY?")} derecha=${html`<span class="mono" style="font-size:14px">${elegidos.length} / ${caben}</span>`}/>
      <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
        <div style="flex:1;min-width:160px"><input type="text" class="campo" maxLength=${40} value=${buscar} placeholder=${t("Buscar amigo…", "Search friend…")} onInput=${(e) => setBuscar(e.currentTarget.value)}/></div>
        <${Boton} tipo="suave mini" deshabilitado=${!elegidos.length} al=${() => poner([])}>${t("QUITAR TODOS", "CLEAR")}</${Boton}>
      </div>
      ${lista.length ? html`<div style="display:flex;flex-direction:column;gap:6px;max-height:420px;overflow:auto">
        ${lista.map((p) => html`<div key=${p.id} class="caja" style="display:flex;flex-direction:column;gap:6px;padding:8px 12px">
          <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
            <label style="display:flex;align-items:center;gap:10px;cursor:pointer;flex:1;min-width:0">
              <input type="checkbox" checked=${elegidos.includes(p.id)} onChange=${() => alternar(p.id)}/>
              <b style="font-size:15px;overflow-wrap:anywhere">${p.nombre}</b>
            </label>
            ${p.enHueco ? html`<${Chip} tipo="ok">${t("EN PES · HUECO ", "IN PES · SLOT ")}${p.enHueco}</${Chip}>` : html`<${Chip}>${t("SOLO EN LINK", "LINK ONLY")}</${Chip}>`}
            <${Boton} tipo="suave mini" al=${() => { setEditando(editando === p.id ? null : p.id); setNuevo(p.sinNombre ? "" : p.nombre); }}>${t("NOMBRE", "NAME")}</${Boton}>
            <${Boton} tipo="suave mini" al=${() => borrar(p)}>${t("BORRAR", "DELETE")}</${Boton}>
          </div>
          ${editando === p.id ? html`<div style="display:flex;gap:8px;align-items:center"><div style="flex:1;min-width:0"><input type="text" class="campo" maxLength=${31} value=${nuevo} placeholder=${t("Nombre del amigo (máx. 31)", "Friend's name (max 31)")} onInput=${(e) => setNuevo(e.currentTarget.value)}/></div>
            <${Boton} tipo="lleno mini" deshabilitado=${!nuevo.trim()} al=${renombrar}>${t("GUARDAR", "SAVE")}</${Boton}></div>` : null}
        </div>`)}
      </div>` : html`<div class="ayuda" style="font-size:13px">${perfiles.length ? t("Nadie coincide con la búsqueda.", "Nobody matches the search.") : t("Aún no hay perfiles. Pulsa «Traer de PES» arriba.", "No profiles yet. Press «Bring from PES» above.")}</div>`}
      <div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap">
        <${Boton} tipo="lleno" deshabilitado=${d.pesAbierto || sinGuardado || ocupado || !elegidos.length || elegidos.length > caben} al=${preparar}>${t("PREPARAR LA SALA", "PREPARE THE ROOM")}</${Boton}>
        ${d.pesAbierto ? html`<span class="ayuda" style="font-size:13px">${t("Cierra PES para preparar la sala.", "Close PES to prepare the room.")}</span>` : null}
        ${elegidos.length > caben ? html`<span class="ayuda" style="font-size:13px;color:var(--bad)">${t(`Solo caben ${caben} (hay ${protegidos} huecos protegidos).`, `Only ${caben} fit (${protegidos} slots are protected).`)}</span>` : null}
      </div>
      ${d.informe && d.informe.puestos?.length ? html`<div class="ayuda" style="font-size:13px">${t(`Hecho: ${d.informe.puestos.length} amigos en PES, ${d.informe.escritos} huecos escritos.`, `Done: ${d.informe.puestos.length} friends in PES, ${d.informe.escritos} slots written.`)}${d.informe.copia ? html` <span class="mono" style="font-size:11px;overflow-wrap:anywhere">${t("Copia: ", "Backup: ")}${d.informe.copia}</span>` : ""}</div>` : null}
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:12px">
      <${Titulo} texto=${t("PRUEBA EN VIVO (BETA)", "LIVE TEST (BETA)")} derecha=${html`<${Chip} tipo=${d.pesAbierto ? "ok" : "warn"}>${d.pesAbierto ? t("PES ABIERTO", "PES OPEN") : t("ABRE PES", "OPEN PES")}</${Chip}>`}/>
      <div class="ayuda" style="font-size:14px">${t("Con PES abierto en la pantalla de elegir lados, pon el perfil de un amigo en el Usuario que tiene un mando. Se ve al instante, sin reiniciar. Lo que había en ese Usuario queda guardado en Link y lo puedes restaurar.", "With PES open on the side-selection screen, put a friend's profile into the user a controller has. It shows instantly, no restart. What was in that user stays saved in Link and can be restored.")}</div>
      <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
        <select class="campo" value=${puestoV} onChange=${(e) => setPuestoV(Number(e.currentTarget.value))} style="max-width:130px">
          ${[1, 2, 3, 4, 5, 6, 7, 8].map((n) => html`<option key=${n} value=${n}>${t("Puesto ", "Slot ")}${n}</option>`)}
        </select>
        <select class="campo" value=${perfilV} onChange=${(e) => setPerfilV(e.currentTarget.value)} style="flex:1;min-width:160px">
          <option value="">${t("Elige el perfil…", "Choose the profile…")}</option>
          ${perfiles.map((p) => html`<option key=${p.id} value=${p.id}>${p.nombre}</option>`)}
        </select>
        <${Boton} tipo="lleno" deshabilitado=${!d.pesAbierto || !perfilV || ocupado} al=${aplicarVivo}>${t("PONER EN EL MANDO", "PUT ON CONTROLLER")}</${Boton}>
      </div>
      <div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center">
        <${Boton} tipo="suave mini" deshabilitado=${!d.pesAbierto || ocupado} al=${verMandos}>${t("VER USUARIO DE CADA MANDO", "SHOW EACH CONTROLLER'S USER")}</${Boton}>
        <${Boton} tipo="suave mini" deshabilitado=${!(d.vivo || []).length || ocupado} al=${restaurarVivo}>${t("RESTAURAR LO ANTERIOR", "RESTORE PREVIOUS")}${(d.vivo || []).length ? " (" + d.vivo.length + ")" : ""}</${Boton}>
      </div>
      ${mandos ? html`<div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(170px,1fr));gap:6px">${mandos.map((m) => html`<div key=${m.puesto} class="caja" style="display:flex;gap:8px;align-items:center;padding:6px 10px"><span class="mono mut" style="font-size:12px">${t("P", "S")}${m.puesto}</span><span style="font-size:13px;flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${m.usuario === 0 ? t("— sin Usuario aún", "— no user yet") : (m.nombre || t("Usuario ", "User ") + m.usuario)}</span><span class="mono mut" style="font-size:11px">${m.usuario === 0 ? "" : "#" + m.usuario}</span></div>`)}</div>` : null}
    </${Tarjeta}>

    <${Tarjeta} interior="padding:22px 24px;display:flex;flex-direction:column;gap:10px">
      <${Titulo} texto=${t("LOS 32 HUECOS DE PES", "PES'S 32 SLOTS")}/>
      <div class="ayuda" style="font-size:13px">${t("Así están ahora. El candado 🔒 protege un hueco: Link nunca lo toca (úsalo para el tuyo). Link rellena los demás de atrás hacia delante.", "This is how they are now. The lock 🔒 protects a slot: Link never touches it (use it for yours). Link fills the others from the back.")}</div>
      <div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(170px,1fr));gap:6px">
        ${huecos.map((h) => html`<div key=${h.n} class="caja" style="display:flex;align-items:center;gap:8px;padding:6px 10px">
          <span class="mono mut" style="font-size:12px;width:20px">${h.n}</span>
          <span style="flex:1;min-width:0;font-size:13px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${h.nombre || html`<span class="mut">—</span>`}</span>
          <button title=${h.protegido ? t("Protegido: Link no lo toca", "Protected: Link won't touch it") : t("Proteger este hueco", "Protect this slot")} style=${"background:none;border:0;cursor:pointer;font-size:15px;opacity:" + (h.protegido ? 1 : 0.35)} onClick=${() => proteger(h.n, !h.protegido)}>🔒</button>
        </div>`)}
      </div>
    </${Tarjeta}>

      </div>
    </details>
  </div>`;
}
