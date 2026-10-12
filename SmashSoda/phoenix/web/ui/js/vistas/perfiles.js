// =============================================================================
//  PERFILES DE AMIGOS (GENTE › PERFILES PES). PES guarda solo 32 «Datos personales» (Usuario 1…32: nombre + configuración).
//  Link guarda los de TODOS tus amigos y pone en PES solo los de hoy. Dos pasos, con PES cerrado:
//    1) TRAER: copia los Usuarios de PES a Link (solo lee PES; así nada se pierde).
//    2) PREPARAR LA SALA: eliges quiénes juegan hoy (hasta lo que quepa) y Link los pone en los huecos de PES.
//  Guía: docs/PERFILES-DE-AMIGOS.md
// =============================================================================
import { html, useState, useEffect } from "../lib.js";
import { t } from "../i18n.js";
import { accion, avisar, confirmar } from "../tienda.js";
import { Tarjeta, Titulo, Boton, Chip, cx } from "../ui.js";

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

  return html`<div class="col" style="gap:14px">
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
        ${elegidos.length > caben ? html`<span class="ayuda" style="font-size:13px;color:var(--bad, #ff6b6b)">${t(`Solo caben ${caben} (hay ${protegidos} huecos protegidos).`, `Only ${caben} fit (${protegidos} slots are protected).`)}</span>` : null}
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
      ${mandos ? html`<div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(170px,1fr));gap:6px">${mandos.map((m) => html`<div key=${m.puesto} class="caja" style="display:flex;gap:8px;align-items:center;padding:6px 10px"><span class="mono mut" style="font-size:12px">${t("P", "S")}${m.puesto}</span><span style="font-size:13px;flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${m.nombre || t("Usuario ", "User ") + m.usuario}</span><span class="mono mut" style="font-size:11px">#${m.usuario}</span></div>`)}</div>` : null}
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
  </div>`;
}
