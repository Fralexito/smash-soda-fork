#!/bin/sh
# Genera el PDF de la base de conocimiento (Linux: pandoc + fpdf2 [pip install fpdf2]; alternativa wkhtmltopdf). Salida: base-conocimiento-PES21.pdf
set -e
cd "$(dirname "$0")"
FECHA=$(date +%Y-%m-%d\ %H:%M)
{
  printf '%s\n' "---" "title: \"Phoenix Evolution Patch · Manual y base de conocimiento (PES 2021)\"" "subtitle: \"Auditoría de parches · generado $FECHA\"" "lang: es" "---" ""
  cat ../MANUAL-PARCHE-PHOENIX.md; printf '\n\n\\newpage\n\n'
  cat 00-INDICE.md; printf '\n\n\\newpage\n\n'
  cat 01-MOTOR-PES2021.md; printf '\n\n\\newpage\n\n'
  cat 02-PARCHE-PHOENIX.md; printf '\n\n\\newpage\n\n'
  cat 03-INVESTIGACION-PARCHES.md; printf '\n\n\\newpage\n\n'
  cat 04-ESTRATEGIA-PHOENIX.md; printf '\n\n\\newpage\n\n'
  cat 05-PUERTA-EN-VIVO.md; printf '\n\n\\newpage\n\n'
  cat parches/conmegol-26.md; printf '\n\n\\newpage\n\n'
  cat parches/sudamerican-2026.md; printf '\n\n\\newpage\n\n'
  cat ../sider/RIESGOS-SIDER.md; printf '\n\n\\newpage\n\n'
  cat ../sider/AUDITORIA-SIDER.md; printf '\n\n\\newpage\n\n'
  cat ../sider/VINCULO-TIEMPO-REAL.md; printf '\n\n\\newpage\n\n'
  cat ../PRUEBAS.md
} | sed -e 's/✅/[PROBADO]/g' -e 's/🔎/[OBSERVADO]/g' -e 's/❓/[HIPÓTESIS]/g' -e 's/❌/[NO]/g' -e 's/⏳/[PENDIENTE]/g' -e 's/⚠️/[OJO]/g' -e 's/🟢/[VERDE]/g' -e 's/🟡/[AMARILLO]/g' -e 's/🟠/[NARANJA]/g' -e 's/🔴/[ROJO]/g' -e 's/⇄/<->/g' -e 's/↔/<->/g' -e 's/−/-/g' -e 's/⇒/=>/g' -e 's/\xef\xb8\x8f//g' -e 's/↑/^/g' -e 's/📘 //g' -e 's/🏆 //g' -e 's/🏆//g' -e 's/⚡//g' > /tmp/bc_todo.md
pandoc /tmp/bc_todo.md -f markdown -t html5 -s --metadata lang=es -c /dev/null -o /tmp/bc_todo.html --css estilo.css --self-contained 2>/dev/null || pandoc /tmp/bc_todo.md -f markdown -t html5 -s -o /tmp/bc_todo.html
# 1) Compacto (fpdf2, fuentes base, ~30 KB): html2pdf.py. 2) Si no hay fpdf2, wkhtmltopdf (~100 KB).
if python3 -c "import fpdf" 2>/dev/null; then
  python3 html2pdf.py /tmp/bc_todo.html base-conocimiento-PES21.pdf
else
  wkhtmltopdf --quiet --encoding utf-8 --page-size A4 --margin-top 15mm --margin-bottom 15mm --margin-left 14mm --margin-right 14mm /tmp/bc_todo.html /tmp/bc_todo.pdf
  if command -v qpdf >/dev/null 2>&1; then qpdf --compress-streams=y --object-streams=generate --recompress-flate --compression-level=9 /tmp/bc_todo.pdf base-conocimiento-PES21.pdf; else cp /tmp/bc_todo.pdf base-conocimiento-PES21.pdf; fi
fi
echo "PDF: $(pwd)/base-conocimiento-PES21.pdf"
