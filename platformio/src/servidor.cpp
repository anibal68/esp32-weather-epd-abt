/* Servidor da estação (ver servidor.h).
 */

#include <Update.h>
#include "config.h"
#include "servidor.h"
#include "versao.h"

void pedirServidor(WiFiClientSecure &client, HTTPClient &http,
                   const char *caminho)
{
  // raízes do ESP-IDF, como no main.cpp (o Cloudflare muda de certificado)
  extern const uint8_t crt_bundle_ini[] asm("_binary_x509_crt_bundle_start");
  extern const uint8_t crt_bundle_fim[] asm("_binary_x509_crt_bundle_end");
  client.setCACertBundle(crt_bundle_ini, crt_bundle_fim - crt_bundle_ini);
  http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);
  http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);
  http.useHTTP10(true); // sem respostas em blocos ("chunked")
  http.begin(client, SERVIDOR, 443, caminho);
  http.addHeader("CF-Access-Client-Id", CF_ID);
  http.addHeader("CF-Access-Client-Secret", CF_SEGREDO);
}

void verificarAtualizacao()
{
  WiFiClientSecure client;
  HTTPClient http;
  pedirServidor(client, http, "/versao.txt");
  int codigo = http.GET();
  int versao = (codigo == HTTP_CODE_OK) ? http.getString().toInt() : 0;
  http.end();
  client.stop();
  Serial.println("Firmware: vers\343o " + String(VERSAO_FIRMWARE)
                 + ", no servidor " + String(versao) + " (" + String(codigo)
                 + ")");
  if (versao <= VERSAO_FIRMWARE)
  {
    return;
  }

  pedirServidor(client, http, "/firmware.bin");
  codigo = http.GET();
  int tamanho = http.getSize();
  if (codigo != HTTP_CODE_OK || tamanho <= 0 || !Update.begin(tamanho))
  {
    Serial.println("  atualiza\347\343o falhou: " + String(codigo) + ", "
                   + String(tamanho) + " bytes, " + Update.errorString());
    http.end();
    return;
  }
  Serial.println("  a descarregar " + String(tamanho) + " bytes...");
  size_t escritos = Update.writeStream(*http.getStreamPtr());
  http.end();
  // o Update.end verifica a imagem; com erro a versão antiga continua
  if (escritos != (size_t)tamanho || !Update.end())
  {
    Serial.println("  atualiza\347\343o falhou: " + String(escritos) + " de "
                   + String(tamanho) + " bytes, " + Update.errorString());
    Update.abort();
    return;
  }
  Serial.println("  vers\343o " + String(versao) + " instalada, a reiniciar");
  delay(100);
  ESP.restart();
}
