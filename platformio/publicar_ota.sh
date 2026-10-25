#!/bin/bash
# Publica o firmware compilado para a estação o instalar por Wi-Fi (ota):
# copia o firmware.bin e o número de include/versao.h para ~/estacao/publico,
# servidos em https://estacao.ablabs.win (Cloudflare Access, Service Token).
# Antes: aumentar VERSAO_FIRMWARE e compilar (./compilar.sh).
# O firmware.bin tem a password do Wi-Fi e o token: fica só nessa pasta (chmod 600).
set -euo pipefail
cd "$(dirname "$0")"
BIN=.pio/build/esp32c6_supermini/firmware.bin
DESTINO=~/estacao/publico
VERSAO=$(sed -n 's/^#define VERSAO_FIRMWARE \([0-9]\+\).*/\1/p' include/versao.h)
[ -n "$VERSAO" ] || { echo "VERSAO_FIRMWARE não encontrada em include/versao.h"; exit 1; }
[ "$BIN" -nt include/versao.h ] || { echo "o $BIN é mais antigo do que o versao.h: compilar primeiro"; exit 1; }
install -m 600 "$BIN" "$DESTINO/firmware.tmp"
mv "$DESTINO/firmware.tmp" "$DESTINO/firmware.bin"
echo "$VERSAO" > "$DESTINO/versao.tmp"
mv "$DESTINO/versao.tmp" "$DESTINO/versao.txt"
ls -l "$DESTINO"
echo "Publicada a versão $VERSAO: a estação instala-a da próxima vez que acordar."
