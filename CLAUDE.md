# Estação meteorológica e-paper (fork de lmarzen/esp32-weather-epd)

Repositório PÚBLICO. Wi-Fi e chaves em `platformio/src/config.cpp`: nunca fazer commit com valores reais.

## Compilar no Pi

- `platformio/compilar.sh` (= `nice pio run -e esp32c6_supermini -j 1`). Não usar o botão do
  PlatformIO no VS Code (usa os 4 núcleos).
- A 24/10/2026 o Pi (2 GB) bloqueou duas vezes a compilar para o C6. Causa: os ~9400 ícones
  (~100 MB) incluídos de uma vez (`icons.h`) faziam o `cc1plus` passar de 3 GB.
- Solução: `platformio/gerar_icones.py` (corre sozinho antes de cada compilação, `extra_scripts`)
  gera `include/icones_usados.h` só com os ícones usados no código (~660, 2,2 MB). Para usar um ícone
  novo basta escrevê-lo no código (`nome_48x48` ou `getBitmap(nome, 48)`). Nunca incluir
  `icons/icons.h` nem `icons/icons_NxN.h`.
- Swap de 2 GB no SSD (`/swapfile`, prioridade 10, depois da zram) desde 24/10/2026.
- Compilação limpa ~10 min; só um ficheiro ~2,5 min; cc1plus < 200 MB. Evitar perto das 6:00.

## Adaptação ao ESP32-C6 (Arduino 3.x, pioarduino)

- Bateria: `analogReadMilliVolts()` (o ADC antigo não existe no 3.x), x2 pelo divisor.
- LED: no C6 desliga-se o GPIO15 (LED azul do Super Mini); `LED_BUILTIN` é o RGB do GPIO8.
- Pinos do Aníbal (ainda por pôr no `config.cpp`): ver a nota da estação na memória do Claude.
