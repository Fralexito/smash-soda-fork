# html2pdf.py - HTML (pandoc) -> PDF compacto con fpdf2 y fuentes base (no incrusta fuentes). Uso: python3 html2pdf.py entrada.html salida.pdf
# PDF compacto de la base de conocimiento: HTML (pandoc) -> fpdf2 con fuentes base (no incrusta fuentes).
import re, sys
from html.parser import HTMLParser
from fpdf import FPDF
from fpdf.enums import TableBordersLayout, XPos, YPos

SUBS = {'→':'->','←':'<-','↔':'<->','≥':'>=','≤':'<=','Σ':'Suma','·':'-','–':'-','—':'-','…':'...','“':'"','”':'"','‘':"'",'’':"'",'×':'x','≈':'~','✓':'OK','•':'-','€':'EUR',' ':' '}
def limpiar(t):
    for a,b in SUBS.items(): t=t.replace(a,b)
    return t.encode('latin-1','replace').decode('latin-1')

class P(HTMLParser):
    """Convierte el HTML en una lista de bloques: ('h',nivel,texto) ('p',texto) ('li',nivel,texto) ('pre',texto) ('table',filas) ('hr',) ('pb',)"""
    def __init__(s):
        super().__init__(); s.bloques=[]; s.pila=[]; s.txt=''; s.tabla=None; s.fila=None; s.celda=None; s.ul=0; s.enpre=False
    def start(s,tag): s.pila.append(tag)
    def handle_starttag(s,tag,attrs):
        a=dict(attrs)
        if tag in('h1','h2','h3','h4','h5','p','pre','li'): s.cerrar(); s.txt=''
        if tag=='pre': s.enpre=True
        if tag in('ul','ol'): s.cerrar(); s.ul+=1
        if tag=='table': s.cerrar(); s.tabla=[]
        if tag=='tr': s.fila=[]
        if tag in('td','th'): s.celda=''
        if tag=='br': 
            if s.celda is not None: s.celda+='; '
            else: s.txt+='\n'
        if tag=='hr': s.cerrar(); s.bloques.append(('hr',))
        if tag in('strong','b') and s.celda is None: s.txt+='**'
        if tag=='div' and 'newpage' in str(a): s.bloques.append(('pb',))
        s.pila.append(tag)
    def handle_endtag(s,tag):
        if tag in('strong','b') and s.celda is None: s.txt+='**'
        if tag in('h1','h2','h3','h4','h5'): s.bloques.append(('h',int(tag[1]),s.txt.strip())); s.txt=''
        elif tag=='p' and s.celda is None and not s.enpre:
            if s.txt.strip(): s.bloques.append(('p',s.txt.strip()))
            s.txt=''
        elif tag=='li': 
            if s.txt.strip(): s.bloques.append(('li',s.ul,s.txt.strip()))
            s.txt=''
        elif tag=='pre': s.bloques.append(('pre',s.txt.rstrip('\n'))); s.txt=''; s.enpre=False
        elif tag in('ul','ol'): s.ul-=1
        elif tag in('td','th'): s.fila.append(s.celda.strip()); s.celda=None
        elif tag=='tr': s.tabla.append(s.fila); s.fila=None
        elif tag=='table': s.bloques.append(('table',s.tabla)); s.tabla=None
        if s.pila and s.pila[-1]==tag: s.pila.pop()
    def cerrar(s):
        if s.txt.strip() and s.celda is None: s.bloques.append(('p',s.txt.strip()))
        s.txt=''
    def handle_data(s,d):
        if s.celda is not None: s.celda+=d
        elif s.enpre: s.txt+=d
        else: s.txt+=re.sub(r'\s+',' ',d)

class PDF(FPDF):
    def footer(s):
        s.set_y(-12); s.set_font('Helvetica','',8); s.set_text_color(120); s.cell(0,8,f'{s.page_no()}/{{nb}}',align='C'); s.set_text_color(0)

def escribir_rico(pdf,texto,tam=9.5):
    """Texto con **negrita** en línea."""
    partes=texto.split('**'); 
    for i,p in enumerate(partes):
        pdf.set_font('Helvetica','B' if i%2 else '',tam); pdf.write(4.6,p)
    pdf.ln(4.6)

src=open(sys.argv[1],encoding='utf-8').read()
body=re.search(r'<body[^>]*>(.*)</body>',src,re.S).group(1)
body=re.sub(r'<header[^>]*>(.*?)</header>','',body,flags=re.S)
body=re.sub(r'<nav[^>]*>.*?</nav>','',body,flags=re.S)
titulo=re.search(r'<h1 class="title">(.*?)</h1>',src,re.S); sub=re.search(r'<p class="subtitle">(.*?)</p>',src,re.S)
p=P(); p.feed(limpiar(body)); p.cerrar()

pdf=PDF('P','mm','A4'); pdf.set_margins(14,14,14); pdf.set_auto_page_break(True,16); pdf.add_page()
pdf.set_font('Helvetica','B',18); pdf.multi_cell(0,9,limpiar(re.sub('<[^>]+>','',titulo.group(1))) if titulo else 'Base de conocimiento', new_x=XPos.LMARGIN, new_y=YPos.NEXT)
if sub: pdf.set_font('Helvetica','I',10); pdf.set_text_color(90); pdf.multi_cell(0,6,limpiar(re.sub('<[^>]+>','',sub.group(1))), new_x=XPos.LMARGIN, new_y=YPos.NEXT); pdf.set_text_color(0)
pdf.ln(3)
TAM={1:15,2:12.5,3:11,4:10,5:9.5}
for bi,b in enumerate(p.bloques):
  k=b[0]
  try:
      if k=='pb': pdf.add_page()
      elif k=='hr': pdf.ln(2); pdf.set_draw_color(180); pdf.line(14,pdf.get_y(),196,pdf.get_y()); pdf.ln(3)
      elif k=='h':
          if b[1]==1 and pdf.get_y()>40: pdf.add_page()
          pdf.ln(2 if b[1]>2 else 4); pdf.set_font('Helvetica','B',TAM.get(b[1],10)); pdf.set_text_color(20,40,90); pdf.multi_cell(0,TAM.get(b[1],10)*0.5+1,b[2].replace('**',''), new_x=XPos.LMARGIN, new_y=YPos.NEXT); pdf.set_text_color(0); pdf.ln(1)
      elif k=='p': escribir_rico(pdf,b[1]); pdf.ln(1.2)
      elif k=='li':
          x=14+4*b[1]; pdf.set_x(x); pdf.set_font('Helvetica','',9.5); pdf.write(4.6,'- '); 
          pdf.set_left_margin(x+4); pdf.set_x(x+4); escribir_rico(pdf,b[2]); pdf.set_left_margin(14); pdf.set_x(14)
      elif k=='pre':
          pdf.set_font('Courier','',7.8); pdf.set_fill_color(242); pdf.multi_cell(0,3.9,b[1],fill=True, new_x=XPos.LMARGIN, new_y=YPos.NEXT); pdf.set_font('Helvetica','',9.5); pdf.ln(1.5)
      elif k=='table':
          filas=[f for f in b[1] if f]
          if not filas: continue
          n=max(len(f) for f in filas); filas=[f+['']*(n-len(f)) for f in filas]
          # anchos proporcionales al texto (mín 8 %)
          tam=7.6 if n>=5 else 8.2
          util=182.0; cw=tam*0.36  # ancho medio por carácter (mm) para Helvetica
          largos=[max(min(len(f[c]),70) for f in filas) for c in range(n)]; tot=sum(largos) or 1
          anchos=[util*l/tot for l in largos]
          # mínimo: la palabra más larga de la columna (no se puede partir) + relleno
          minw=[min(60,max(len(w) for f in filas for w in (f[c].split() or ['']))*cw+3) for c in range(n)]
          for _ in range(3):
              fijos=sum(max(a,m) for a,m in zip(anchos,minw)); 
              if fijos<=util: break
              libres=[i for i in range(n) if anchos[i]>minw[i]]
              exceso=fijos-util; suma=sum(anchos[i]-minw[i] for i in libres) or 1
              for i in libres: anchos[i]-=exceso*(anchos[i]-minw[i])/suma
          anchos=[max(a,m) for a,m in zip(anchos,minw)]; s=sum(anchos); anchos=[a*100/s for a in anchos]
          pdf.set_font('Helvetica','',tam); pdf.set_draw_color(190)
          with pdf.table(col_widths=anchos, text_align='LEFT', line_height=tam*0.55, borders_layout=TableBordersLayout.MINIMAL, padding=0.8, first_row_as_headings=False) as t:
              for i,f in enumerate(filas):
                  r=t.row()
                  for c in f:
                      if i==0: pdf.set_font('Helvetica','B',tam)
                      r.cell(c)
                      pdf.set_font('Helvetica','',tam)
          pdf.ln(2.5)

  except Exception as e:
    print('BLOQUE',bi,b[0],str(b)[:200]); raise
pdf.output(sys.argv[2]); print('ok',sys.argv[2],len(p.bloques),'bloques')
