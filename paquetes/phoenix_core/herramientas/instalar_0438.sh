#!/bin/bash
set -e
G="/d/Frank/Games_/Conmegol Patch"; C="$G/SiderAddons/phoenix_core"; R=/c/dev/phoenix-core-wt/paquetes/phoenix_core
if tasklist | grep -qi "PES2021.exe"; then echo "PES sigue abierto"; exit 1; fi
cp "$C/phoenix_core.log" "$C/phoenix_core_sesion_1555.log" 2>/dev/null || true
cp "$C/phoenix_core.dll" /c/dev/phoenix-core-wt/phoenix_core.0.4.36.dll.bak
cp "$R/build/Release/phoenix_core.dll" "$C/phoenix_core.dll"
cp "$G/SiderAddons/modules/phoenix_marcador_free.lua" "$G/SiderAddons/modules/phoenix_marcador_free.lua.v0.4b.bak"
cp "$R/SiderAddons/modules/phoenix_marcador_free.lua" "$G/SiderAddons/modules/phoenix_marcador_free.lua"
python - <<'PY'
import json
p=r'D:\Frank\Games_\Conmegol Patch\SiderAddons\phoenix_core\phoenix_core.json'
t=open(p,encoding='utf-8').read()
for k,v in (('siempre_disparo_modo','7'),('season_forzada','1'),('medidor_real','1'),('vigia_escritura','1'),('suplentes_cero','1'),('jugadores_sonda','true')):
    if '"%s"'%k not in t: t=t.replace('"siempre_disparo_free": true','"siempre_disparo_free": true,\n    "%s": %s'%(k,v),1)
    else:
        import re; t=re.sub(r'("%s"\s*:\s*)\d+'%k,r'\g<1>%s'%v,t)
open(p,'w',encoding='utf-8').write(t); json.loads(t); print('json ok')
PY
rm -f "$C/arrancando.flag"
cd "$G" && cmd.exe //c start "" "PES2021 Start.exe"
sleep 6; tasklist | grep -iE "PES2021|sider" || true
