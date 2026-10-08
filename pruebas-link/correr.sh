#!/bin/bash
# Pruebas de la lógica pura de la interfaz web (sin Windows). Uso: bash pruebas-link/correr.sh
set -e
R="$(cd "$(dirname "$0")/.." && pwd)"
S="$R/SmashSoda/phoenix/web"
mkdir -p /tmp/phx_pruebas
g++ -std=c++17 -O1 -Wall -Wextra -Wno-unused-parameter -I"$R/SmashSoda" -I"$R/external" \
  "$R/pruebas-link/web_test.cpp" "$S/Puente.cpp" "$S/MonitorRed.cpp" "$S/Partido.cpp" "$S/PerfilesSala.cpp" \
  -o /tmp/phx_pruebas/web_test
/tmp/phx_pruebas/web_test
