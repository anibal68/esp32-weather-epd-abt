#!/bin/bash
# Desenha o ecrã no Pi (sem placa) com o src/ecra.cpp verdadeiro e dados de
# exemplo; grava esboco_normal.png e esboco_extremo.png e lista sobreposições.
set -uo pipefail
T=$(mktemp -d)
trap "rm -rf $T" EXIT
cd "$(dirname "$0")"
P=..
g++ -std=gnu++17 -O1 -Wall -I . -I $P/include -I $P/lib/esp32-weather-epd-assets \
    sim.cpp $P/src/ecra.cpp -o $T/simular || exit 1
r=0
for caso in normal extremo; do
  echo "== $caso"
  $T/simular $T/$caso.ppm $caso || r=1
  python3 -c "from PIL import Image; Image.open('$T/$caso.ppm').save('esboco_$caso.png')"
done
exit $r
