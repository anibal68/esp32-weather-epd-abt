/* Open-Meteo (sem chave) em vez do OpenWeatherMap. Ver include/open_meteo.h.
 *
 * Documentação: https://open-meteo.com/en/docs
 *               https://open-meteo.com/en/docs/air-quality-api
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "_locale.h"
#include "config.h"
#include "display_utils.h"
#include "open_meteo.h"

static const char *OM_HOST    = "api.open-meteo.com";
static const char *OM_AR_HOST = "air-quality-api.open-meteo.com";

static const float KELVIN = 273.15f;

/* Código de tempo WMO (Open-Meteo) -> id de condição do OpenWeatherMap.
 * https://open-meteo.com/en/docs (WMO Weather interpretation codes)
 * https://openweathermap.org/weather-conditions
 */
static int wmoParaOwm(int wmo)
{
  switch (wmo)
  {
  case 0:  return 800; // céu limpo
  case 1:  return 801; // pouco nublado
  case 2:  return 802; // parcialmente nublado
  case 3:  return 804; // muito nublado
  case 45: return 741; // nevoeiro
  case 48: return 741; // nevoeiro com geada
  case 51: return 300; // chuvisco fraco
  case 53: return 301; // chuvisco
  case 55: return 302; // chuvisco forte
  case 56: return 511; // chuvisco que congela
  case 57: return 511;
  case 61: return 500; // chuva fraca
  case 63: return 501; // chuva moderada
  case 65: return 502; // chuva forte
  case 66: return 511; // chuva que congela
  case 67: return 511;
  case 71: return 600; // neve fraca
  case 73: return 601; // neve
  case 75: return 602; // neve forte
  case 77: return 600; // grãos de neve
  case 80: return 520; // aguaceiros fracos
  case 81: return 521; // aguaceiros
  case 82: return 522; // aguaceiros fortes
  case 85: return 620; // aguaceiros de neve
  case 86: return 621;
  case 95: return 211; // trovoada
  case 96: return 201; // trovoada com granizo
  case 99: return 202;
  default: return 800;
  }
}

/* Fase da lua (0 e 1 = lua nova, 0,5 = lua cheia), como no OWM.
 * O Open-Meteo não a dá; cálculo pelo mês sinódico a partir de uma lua nova
 * conhecida (6/1/2000 18:14 UTC). Erro de algumas horas, chega para o ícone.
 */
static float faseDaLua(int64_t t)
{
  const double LUA_NOVA = 947182440.0;
  const double SINODICO = 29.530588853 * 86400.0;
  double f = fmod((static_cast<double>(t) - LUA_NOVA) / SINODICO, 1.0);
  if (f < 0) { f += 1.0; }
  return static_cast<float>(f);
}

/* Pedido GET e leitura do JSON, com 3 tentativas como no código original.
 * Devolve o código HTTP (ou os mesmos códigos negativos do original).
 */
static int pedirJson(WiFiClientSecure &client, const char *host,
                     const String &uri, JsonDocument &doc)
{
  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + String(host) + uri);
  int httpResponse = 0;
  bool rxSuccess = false;
  int attempts = 0;
  while (!rxSuccess && attempts < 3)
  {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED)
    {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.begin(client, host, 443, uri);
    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK)
    {
      DeserializationError jsonErr = deserializeJson(doc, http.getStream());
      if (jsonErr)
      {
        // -256 offset distinguishes these errors from httpClient errors
        httpResponse = -256 - static_cast<int>(jsonErr.code());
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " "
                   + getHttpResponsePhrase(httpResponse));
    ++attempts;
  }
  return httpResponse;
}

/* Tempo atual, 48 horas e 8 dias. */
int getOpenMeteo(WiFiClientSecure &client, owm_resp_onecall_t &r)
{
  const String vars = "temperature_2m,apparent_temperature,relative_humidity_2m,"
                      "dew_point_2m,pressure_msl,cloud_cover,uv_index,visibility,"
                      "wind_speed_10m,wind_gusts_10m,wind_direction_10m,rain,"
                      "snowfall,weather_code,is_day";
  String uri = "/v1/forecast?latitude=" + LAT + "&longitude=" + LON
               + "&timezone=" + OM_TIMEZONE
               + "&timeformat=unixtime&wind_speed_unit=ms"
               + "&forecast_hours=" + String(OWM_NUM_HOURLY)
               + "&forecast_days=" + String(OWM_NUM_DAILY)
               + "&current=" + vars
               + "&hourly=" + vars + ",precipitation_probability"
               + "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
                 "sunrise,sunset,uv_index_max,precipitation_probability_max,"
                 "rain_sum,snowfall_sum,wind_speed_10m_max,wind_gusts_10m_max,"
                 "wind_direction_10m_dominant,cloud_cover_mean";

  JsonDocument doc;
  int httpResponse = pedirJson(client, OM_HOST, uri, doc);
  if (httpResponse != HTTP_CODE_OK)
  {
    return httpResponse;
  }

  r.lat             = doc["latitude"]  .as<float>();
  r.lon             = doc["longitude"] .as<float>();
  r.timezone        = doc["timezone"]  .as<const char *>();
  r.timezone_offset = doc["utc_offset_seconds"].as<int>();

  JsonObject d = doc["daily"];
  for (int i = 0; i < OWM_NUM_DAILY; ++i)
  {
    owm_daily_t &x = r.daily[i];
    x = {};
    x.dt         = d["time"][i]   .as<int64_t>();
    x.sunrise    = d["sunrise"][i].as<int64_t>();
    x.sunset     = d["sunset"][i] .as<int64_t>();
    x.moonrise   = 0; // o Open-Meteo não dá; os ícones de noite ficam sem lua
    x.moonset    = 0;
    x.moon_phase = faseDaLua(x.dt + 12 * 3600);
    x.temp.max   = d["temperature_2m_max"][i].as<float>() + KELVIN;
    x.temp.min   = d["temperature_2m_min"][i].as<float>() + KELVIN;
    x.temp.day   = (x.temp.max + x.temp.min) / 2;
    x.clouds     = d["cloud_cover_mean"][i]           .as<int>();
    x.uvi        = d["uv_index_max"][i]               .as<float>();
    x.wind_speed = d["wind_speed_10m_max"][i]         .as<float>();
    x.wind_gust  = d["wind_gusts_10m_max"][i]         .as<float>();
    x.wind_deg   = d["wind_direction_10m_dominant"][i].as<int>();
    x.pop        = d["precipitation_probability_max"][i].as<float>() / 100;
    x.rain       = d["rain_sum"][i]                   .as<float>();
    x.snow       = d["snowfall_sum"][i].as<float>() * 10; // cm -> mm
    x.weather.id   = wmoParaOwm(d["weather_code"][i].as<int>());
    x.weather.icon = "01d"; // previsão diária: sempre ícone de dia
  }

  JsonObject h = doc["hourly"];
  for (int i = 0; i < OWM_NUM_HOURLY; ++i)
  {
    owm_hourly_t &x = r.hourly[i];
    x = {};
    x.dt         = h["time"][i]                .as<int64_t>();
    x.temp       = h["temperature_2m"][i]      .as<float>() + KELVIN;
    x.feels_like = h["apparent_temperature"][i].as<float>() + KELVIN;
    x.pressure   = h["pressure_msl"][i]        .as<float>() + 0.5f;
    x.humidity   = h["relative_humidity_2m"][i].as<int>();
    x.dew_point  = h["dew_point_2m"][i]        .as<float>() + KELVIN;
    x.clouds     = h["cloud_cover"][i]         .as<int>();
    x.uvi        = h["uv_index"][i]            .as<float>();
    x.visibility = h["visibility"][i]          .as<int>();
    x.wind_speed = h["wind_speed_10m"][i]      .as<float>();
    x.wind_gust  = h["wind_gusts_10m"][i]      .as<float>();
    x.wind_deg   = h["wind_direction_10m"][i]  .as<int>();
    x.pop        = h["precipitation_probability"][i].as<float>() / 100;
    x.rain_1h    = h["rain"][i]                .as<float>();
    x.snow_1h    = h["snowfall"][i].as<float>() * 10; // cm -> mm
    x.weather.id   = wmoParaOwm(h["weather_code"][i].as<int>());
    x.weather.icon = h["is_day"][i].as<int>() ? "01d" : "01n";
  }

  JsonObject c = doc["current"];
  owm_current_t &x = r.current;
  x = {};
  x.dt         = c["time"]                .as<int64_t>();
  x.sunrise    = r.daily[0].sunrise;
  x.sunset     = r.daily[0].sunset;
  x.temp       = c["temperature_2m"]      .as<float>() + KELVIN;
  x.feels_like = c["apparent_temperature"].as<float>() + KELVIN;
  x.pressure   = c["pressure_msl"]        .as<float>() + 0.5f;
  x.humidity   = c["relative_humidity_2m"].as<int>();
  x.dew_point  = c["dew_point_2m"]        .as<float>() + KELVIN;
  x.clouds     = c["cloud_cover"]         .as<int>();
  x.uvi        = c["uv_index"]            .as<float>();
  x.visibility = c["visibility"]          .as<int>();
  x.wind_speed = c["wind_speed_10m"]      .as<float>();
  x.wind_gust  = c["wind_gusts_10m"]      .as<float>();
  x.wind_deg   = c["wind_direction_10m"]  .as<int>();
  x.rain_1h    = c["rain"]                .as<float>();
  x.snow_1h    = c["snowfall"].as<float>() * 10; // cm -> mm
  x.weather.id   = wmoParaOwm(c["weather_code"].as<int>());
  x.weather.icon = c["is_day"].as<int>() ? "01d" : "01n";

  r.alerts.clear(); // avisos do IPMA: passo seguinte
  return httpResponse;
} // end getOpenMeteo

/* Qualidade do ar das últimas 24 horas (µg/m³, como no OWM). */
int getOpenMeteoAr(WiFiClientSecure &client, owm_resp_air_pollution_t &r)
{
  String uri = "/v1/air-quality?latitude=" + LAT + "&longitude=" + LON
               + "&timeformat=unixtime"
               + "&hourly=carbon_monoxide,nitrogen_dioxide,sulphur_dioxide,"
                 "ozone,pm10,pm2_5,ammonia"
               + "&past_hours=" + String(OWM_NUM_AIR_POLLUTION)
               + "&forecast_hours=0";

  JsonDocument doc;
  int httpResponse = pedirJson(client, OM_AR_HOST, uri, doc);
  if (httpResponse != HTTP_CODE_OK)
  {
    return httpResponse;
  }

  r = {};
  r.coord.lat = doc["latitude"] .as<float>();
  r.coord.lon = doc["longitude"].as<float>();
  JsonObject h = doc["hourly"];
  for (int i = 0; i < OWM_NUM_AIR_POLLUTION; ++i)
  {
    r.dt[i]               = h["time"][i]            .as<int64_t>();
    r.components.co[i]    = h["carbon_monoxide"][i] .as<float>();
    r.components.no[i]    = 0; // o Open-Meteo não tem NO
    r.components.no2[i]   = h["nitrogen_dioxide"][i].as<float>();
    r.components.o3[i]    = h["ozone"][i]           .as<float>();
    r.components.so2[i]   = h["sulphur_dioxide"][i] .as<float>();
    r.components.pm2_5[i] = h["pm2_5"][i]           .as<float>();
    r.components.pm10[i]  = h["pm10"][i]            .as<float>();
    r.components.nh3[i]   = h["ammonia"][i]         .as<float>();
  }
  return httpResponse;
} // end getOpenMeteoAr
