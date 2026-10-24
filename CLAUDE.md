# Estação meteorológica e-paper (fork de lmarzen/esp32-weather-epd)

Repositório PÚBLICO. Wi-Fi e chaves em `platformio/src/config.cpp`: nunca fazer commit com valores reais.

## Compilar no Pi

- Na pasta `platformio/`: `nice ~/.platformio/penv/bin/pio run -e esp32c6_supermini -j 2`.
- Sempre com `-j 2`: com os 4 núcleos em paralelo, a 24/10/2026 o Pi (2 GB) ficou sem memória a
  compilar para o ESP32-C6 e reiniciou-se pelo watchdog. Evitar compilar perto das 6:00.
