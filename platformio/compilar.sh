#!/bin/bash
# Compila a estação meteorológica (ESP32-C6 Super Mini) no Pi.
# -j 1: com 4 em paralelo (botão do PlatformIO no VS Code) o Pi bloqueou por falta de
# memória (24/10/2026, duas vezes). Demora mais, mas não deita o Pi abaixo.
set -euo pipefail
cd "$(dirname "$0")"
nice ~/.platformio/penv/bin/pio run -e esp32c6_supermini -j 1 "$@"
ls -l .pio/build/esp32c6_supermini/firmware.bin
