/* Avisos meteorológicos do IPMA para a zona IPMA_AREA (config.cpp).
 *
 * https://api.ipma.pt/open-data/forecast/warnings/warnings_www.json
 * Lista com uma entrada por zona e tipo (~200, ~35 KB); "green" = sem aviso.
 * Os avisos vão para owm_alerts_t para o resto do programa os desenhar:
 *   event = "Aviso laranja: Vento" (texto em Latin-1, como as letras do ecrã;
 *           a cor dá a urgência, ver ALERT_URGENCY no locale_pt_PT.inc)
 *   tags  = tipo de aviso (o ecrã fica com o mais grave de cada tipo)
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>

#include "config.h"
#include "open_meteo.h"

static const char *IPMA_HOST = "api.ipma.pt";

/* Tipo de aviso do IPMA (UTF-8) -> texto para o ecrã (Latin-1). As palavras
 * correspondem às TERM_* do locale_pt_PT.inc, que escolhem o ícone.
 */
static String tipoParaEcra(const String &tipo)
{
  if (tipo == "Agitação Marítima") { return "Agita\347\343o Mar\355tima"; }
  if (tipo == "Precipitação")      { return "Precipita\347\343o"; }
  return tipo; // Vento, Trovoada, Nevoeiro, Neve, Tempo Quente, Tempo Frio
}

static String corParaEcra(const String &nivel)
{
  if (nivel == "yellow") { return "amarelo"; }
  if (nivel == "orange") { return "laranja"; }
  if (nivel == "red")    { return "vermelho"; }
  return "";
}

/* "2026-10-24T09:52:00" (hora de Portugal) -> Unix. */
static int64_t lerHora(const char *s)
{
  tm t = {};
  if (!s || sscanf(s, "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
  {
    return 0;
  }
  t.tm_year -= 1900;
  t.tm_mon  -= 1;
  t.tm_isdst = -1;
  return mktime(&t);
}

int getIpmaAvisos(WiFiClientSecure &client, std::vector<owm_alerts_t> &alerts)
{
  JsonDocument filtro;
  filtro[0]["idAreaAviso"]       = true;
  filtro[0]["awarenessTypeName"] = true;
  filtro[0]["awarenessLevelID"]  = true;
  filtro[0]["startTime"]         = true;
  filtro[0]["endTime"]           = true;

  JsonDocument doc;
  int httpResponse = pedirJson(client, IPMA_HOST,
                               "/open-data/forecast/warnings/warnings_www.json",
                               doc, &filtro);
  if (httpResponse != HTTP_CODE_OK)
  {
    return httpResponse;
  }

  time_t agora = time(nullptr);
  alerts.clear();
  for (JsonObject a : doc.as<JsonArray>())
  {
    if (IPMA_AREA != a["idAreaAviso"].as<const char *>())
    {
      continue;
    }
    String cor = corParaEcra(a["awarenessLevelID"].as<String>());
    owm_alerts_t aviso = {};
    aviso.start = lerHora(a["startTime"].as<const char *>());
    aviso.end   = lerHora(a["endTime"]  .as<const char *>());
    if (cor.isEmpty() || aviso.end <= agora)
    {
      continue; // verde (sem aviso) ou já terminou
    }
    aviso.tags  = tipoParaEcra(a["awarenessTypeName"].as<String>());
    aviso.event = "Aviso " + cor + ": " + aviso.tags;
    Serial.println("IPMA: " + aviso.event);
    alerts.push_back(aviso);
    if (alerts.size() == OWM_NUM_ALERTS)
    {
      break;
    }
  }
  return httpResponse;
} // end getIpmaAvisos
