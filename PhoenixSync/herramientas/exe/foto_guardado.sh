#!/bin/bash
# Foto de SOLO LECTURA de las carpetas de guardado de PES 2021. No escribe nada en el PC del usuario.
# Uso: foto.sh <etiqueta>
set -u
B="$HOME/mnt/eFootball PES 2021 SEASON UPDATE"
T=$(TZ=America/Lima date +%Y%m%d-%H%M%S)
D="$HOME/faseA/${T}_$1"; mkdir -p "$D/copias"
{
  echo "# foto $1 · $(TZ=America/Lima date '+%Y-%m-%d %H:%M:%S') Lima"
  echo "# sha256 | bytes | modificado (Lima) | archivo"
  cd "$B"
  for f in settings.dat 239200/save/* 292733975847239680/save/* 48/save/* SP/*; do
    [ -f "$f" ] || continue
    s=$(sha256sum "$f" | cut -d' ' -f1); n=$(stat -c %s "$f"); m=$(TZ=America/Lima date -d "@$(stat -c %Y "$f")" '+%Y-%m-%d %H:%M:%S')
    echo "$s | $n | $m | $f"
  done
} > "$D/huellas.txt"
# listado (sin leer) de subcarpetas, para ver si aparece algo nuevo
( cd "$B" && find . -type f -printf '%s | %TY-%Tm-%Td %TH:%TM:%.2TS | %p\n' | sort -t'|' -k3 ) > "$D/listado-completo.txt"
# copias para comparar byte a byte (solo en el espacio temporal de Claude)
cd "$B"
for f in settings.dat 239200/save/SYSTEM00000000 239200/save/GRAPHICS000000 239200/save/EDIT00000000; do
  [ -f "$f" ] && cp "$f" "$D/copias/$(echo "$f" | tr '/' '_')"
done
echo "$D"
