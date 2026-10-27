#!/bin/bash
set -e
G="/d/Frank/Games_/Conmegol Patch"; C="$G/SiderAddons/phoenix_core"
if tasklist | grep -qi "PES2021.exe"; then echo "PES sigue abierto"; exit 1; fi
cp "$C/phoenix_core.log" "$C/phoenix_core_sesion_1343.log" 2>/dev/null || true
cp "$C/phoenix_core.dll" /c/dev/phoenix-core-wt/phoenix_core.0.4.26.dll.bak
cp /c/dev/phoenix-core-wt/paquetes/phoenix_core/build/Release/phoenix_core.dll "$C/phoenix_core.dll"
python - <<'PY'
import re,json
p=r'D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\phoenix_core.json'
t=open(p,encoding='utf-8').read()
if 'siempre_disparo_free' not in t:
    t=re.sub(r'("pelota_sonda"\s*:\s*true)',r'\1,\n    "siempre_disparo_free": true',t,1)
open(p,'w',encoding='utf-8').write(t); json.loads(t); print('json ok', 'siempre_disparo_free' in t)
PY
rm -f "$C/arrancando.flag"
cd "$G" && cmd.exe //c start "" "PES2021 Start.exe"
sleep 6; tasklist | grep -iE "PES2021|sider" || true
