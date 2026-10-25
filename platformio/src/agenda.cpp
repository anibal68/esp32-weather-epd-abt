/* Agenda da Mariana (ver agenda.h). O Pi escreve o JSON só com caracteres
 * ASCII (ç...); o ArduinoJson devolve UTF-8 e aqui passa-se a Latin-1,
 * como as letras do ecrã.
 */

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>
#include "agenda.h"
#include "config.h"

#define AGENDA_IDADE_MAX (3 * 3600) // segundos

/* UTF-8 -> Latin-1; o que não existe em Latin-1 (emojis...) desaparece.
 */
static String latin1(const char *s)
{
  String r;
  if (!s)
  {
    return r;
  }
  for (const uint8_t *p = (const uint8_t *)s; *p; ++p)
  {
    if (*p < 0x80)
    {
      r += (char)*p;
    }
    else if ((*p == 0xC2 || *p == 0xC3) && (p[1] & 0xC0) == 0x80)
    {
      r += (char)(((*p & 0x03) << 6) | (p[1] & 0x3F));
      ++p;
    }
    // outros: salta os bytes de continuação
    else
    {
      while ((p[1] & 0xC0) == 0x80)
      {
        ++p;
      }
    }
  }
  r.trim();
  return r;
}

bool getAgenda(std::vector<dia_agenda_t> &agenda)
{
  agenda.clear();
  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }
  if (!MDNS.begin("estacao"))
  {
    Serial.println("Agenda: mDNS falhou");
    return false;
  }
  IPAddress ip = MDNS.queryHost(AGENDA_PI, 3000);
  MDNS.end();
  if (ip == IPAddress(0, 0, 0, 0))
  {
    Serial.println("Agenda: " + String(AGENDA_PI) + ".local n\343o encontrado");
    return false;
  }
  Serial.println("Agenda: http://" + ip.toString() + ":" + String(AGENDA_PORTA)
                 + "/agenda.json");

  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);
  http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);
  http.useHTTP10(true);
  http.begin(client, ip.toString(), AGENDA_PORTA, "/agenda.json");
  int codigo = http.GET();
  String corpo = (codigo == HTTP_CODE_OK) ? http.getString() : String();
  http.end();
  Serial.println("  " + String(codigo) + ", " + String(corpo.length())
                 + " bytes");
  if (codigo != HTTP_CODE_OK)
  {
    return false;
  }

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, corpo);
  if (erro)
  {
    Serial.println("  JSON: " + String(erro.c_str()));
    return false;
  }
  int64_t idade = (int64_t)time(nullptr) - doc["gerado"].as<int64_t>();
  if (!doc["ok"].as<bool>() || idade > AGENDA_IDADE_MAX)
  {
    Serial.println("  agenda com erro ou antiga (" + String((long)idade)
                   + " s)");
    return false;
  }

  JsonArray dias = doc["dias"];
  for (JsonObject d : dias)
  {
    dia_agenda_t dia;
    dia.titulo = latin1(d["titulo"].as<const char *>());
    dia.hoje   = d["hoje"].as<bool>();
    JsonArray eventos = d["eventos"];
    for (JsonObject e : eventos)
    {
      dia.eventos.push_back({latin1(e["hora"].as<const char *>()), latin1(e["texto"].as<const char *>())});
    }
    agenda.push_back(dia);
  }
  Serial.println("  " + String(agenda.size()) + " dia(s) com eventos");
  return true;
}
