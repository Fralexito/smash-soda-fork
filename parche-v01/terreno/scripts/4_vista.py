# -*- coding: utf-8 -*-
"""Paso 4: genera ../vista_terreno.html (autocontenida, sin internet) desde los CSV y resumen.json."""
import csv, os, json, html

AQUI = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.dirname(AQUI)
MAX = 4000


def tabla(nombre, max_=MAX):
    rows = list(csv.reader(open(os.path.join(OUT, nombre), encoding="utf-8-sig")))
    return {"cab": rows[0], "filas": rows[1:max_ + 1], "total": len(rows) - 1}


res = json.load(open(os.path.join(OUT, "resumen.json"), encoding="utf-8"))
datos = {
    "resumen": res,
    "tablas": {
        "Identidad jugadores (revisar)": tabla("choques_identidad_jugadores.csv"),
        "Identidad equipos/kits": tabla("choques_identidad.csv"),
        "Misma ruta en varias fuentes": tabla("choques_misma_ruta.csv"),
        "Choques internos (misma fuente)": tabla("choques_internos.csv"),
        "Inventario estadios": tabla("inventario_estadios.csv"),
        "Inventario balones": tabla("inventario_balones.csv"),
        "Inventario kits": tabla("inventario_kits.csv"),
    },
}
plantilla = r"""<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Terreno DB global</title>
<style>
:root{--f:#05060F;--p:#0D1024;--c:#00E5FF;--u:#7B2CBF;--t:#F1EDE4;--g:#8A8FA8}
body{margin:0;background:var(--f);color:var(--t);font:14px/1.45 "Segoe UI",system-ui,sans-serif}
main{max-width:1200px;margin:auto;padding:16px}
h1{color:var(--c);margin:.2em 0}h1 span{color:var(--u)}h2{color:var(--u);font-size:17px;margin-top:28px}
.sub{color:var(--g)}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:10px}
.card{background:var(--p);border-left:3px solid var(--c);padding:10px 12px;border-radius:6px}
.card b{color:var(--c);text-transform:uppercase;font-size:12px;letter-spacing:.06em}
.card div{font-size:13px}
table{border-collapse:collapse;width:100%;font-size:12.5px}
th{position:sticky;top:0;background:var(--u);color:var(--t);text-align:left;padding:5px 6px}
td{border-bottom:1px solid #1d2140;padding:4px 6px;vertical-align:top;word-break:break-word}
tr:hover td{background:#111632}
.tabs{display:flex;flex-wrap:wrap;gap:6px;margin:10px 0}
.tabs button{background:var(--p);color:var(--t);border:1px solid #23284d;border-radius:5px;padding:6px 10px;font-size:13px;cursor:pointer}
.tabs button.on{background:var(--c);color:var(--f);border-color:var(--c)}
input{background:var(--p);color:var(--t);border:1px solid #23284d;border-radius:5px;padding:7px 10px;font-size:13px;width:100%;box-sizing:border-box}
.marco{max-height:65vh;overflow:auto;margin-top:8px;border:1px solid #1d2140;border-radius:6px}
.alto{color:#FF4D6D;font-weight:600}
</style></head><body><main>
<h1>TERRENO <span>DB GLOBAL</span></h1>
<p class="sub">Caras, minicaras, kits, estadios y balones de PES base, Conmegol y Sudamerican · solo índices, nada copiado · __FECHA__</p>
<h2>Cuánto hay en cada fuente</h2><div id="cats" class="cards"></div>
<h2>Choques</h2><div id="choq" class="cards"></div>
<h2>Tablas</h2><div class="tabs" id="tabs"></div>
<input id="q" placeholder="Filtrar (id, nombre, fuente, carpeta…)"><div class="sub" id="cuenta"></div>
<div class="marco"><table id="t"></table></div>
</main><script>
const D=__DATOS__;
const fmt=n=>n.toLocaleString('es');
const cats=document.getElementById('cats');
for(const [c,f] of Object.entries(D.resumen.por_categoria)){
 let h=`<b>${c}</b>`;for(const [k,v] of Object.entries(f))h+=`<div>${k}: ${fmt(v.ids)} ids · ${fmt(v.archivos)} arch · ${fmt(v.mb)} MB</div>`;
 cats.insertAdjacentHTML('beforeend',`<div class="card">${h}</div>`);}
const ch=D.resumen.choques,cq=document.getElementById('choq');
const add=(t,o)=>cq.insertAdjacentHTML('beforeend',`<div class="card"><b>${t}</b>${Object.entries(o).map(([k,v])=>`<div>${k}: ${fmt(v)}</div>`).join('')}</div>`);
add('Mismo id en varias fuentes',ch.misma_ruta);add('Identidad jugadores (711 a revisar)',ch.identidad_jugadores_revisar||{});
add('Identidad equipos (maestra)',Object.assign({},ch.identidad,ch.identidad_ids_maestra));
let act=null;const tabs=document.getElementById('tabs');
for(const n of Object.keys(D.tablas)){const b=document.createElement('button');b.textContent=n;b.onclick=()=>ver(n);tabs.appendChild(b);}
const esc=s=>String(s).replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]));
function ver(n){act=n;[...tabs.children].forEach(b=>b.classList.toggle('on',b.textContent==n));pintar();}
function pintar(){const T=D.tablas[act],q=document.getElementById('q').value.toLowerCase();
 const fs=q?T.filas.filter(r=>r.join(' ').toLowerCase().includes(q)):T.filas;
 document.getElementById('cuenta').textContent=`${fmt(fs.length)} filas mostradas de ${fmt(T.total)} (la vista carga hasta ${fmt(T.filas.length)}; el CSV completo está al lado)`;
 document.getElementById('t').innerHTML='<tr>'+T.cab.map(c=>`<th>${esc(c)}</th>`).join('')+'</tr>'+
  fs.slice(0,1500).map(r=>'<tr>'+r.map(c=>`<td${/^ALTO/.test(c)?' class="alto"':''}>${esc(c)}</td>`).join('')+'</tr>').join('');}
document.getElementById('q').oninput=pintar;ver(Object.keys(D.tablas)[0]);
</script></body></html>"""
import datetime
s = plantilla.replace("__DATOS__", json.dumps(datos, ensure_ascii=False).replace("</", "<\\/")) \
             .replace("__FECHA__", datetime.date.today().isoformat())
open(os.path.join(OUT, "vista_terreno.html"), "w", encoding="utf-8").write(s)
print("vista_terreno.html", round(len(s) / 1e6, 2), "MB")
