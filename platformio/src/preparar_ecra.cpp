/* Prepara os textos e números do desenho novo (ecra.cpp) a partir dos dados
 * do Open-Meteo e do IPMA (estruturas owm_*, em kelvin e m/s).
 * Textos em Latin-1, como as letras do ecrã.
 */

#include <cmath>
#include "preparar_ecra.h"
#include "_strftime.h"
#include "config.h"
#include "conversions.h"
#include "display_utils.h"

static int graus(float kelvin)
{
  return static_cast<int>(std::round(kelvin_to_celsius(kelvin)));
}

static String hora(int64_t t, const char *formato)
{
  time_t ts = t;
  char buf[32] = {};
  _strftime(buf, sizeof(buf), formato, localtime(&ts));
  return buf;
}

/* Descrição do tempo pelo código (os do wmoParaOwm, em open_meteo.cpp).
 */
static String descricao(int id)
{
  switch (id)
  {
  case 800: return "C\351u limpo";
  case 801: return "Pouco nublado";
  case 802: return "Parcialmente nublado";
  case 804: return "Muito nublado";
  case 741: return "Nevoeiro";
  case 300: return "Chuvisco fraco";
  case 301: return "Chuvisco";
  case 302: return "Chuvisco forte";
  case 511: return "Chuva gelada";
  case 500: return "Chuva fraca";
  case 501: return "Chuva moderada";
  case 502: return "Chuva forte";
  case 600: return "Neve fraca";
  case 601: return "Neve";
  case 602: return "Neve forte";
  case 520: return "Aguaceiros fracos";
  case 521: return "Aguaceiros";
  case 522: return "Aguaceiros fortes";
  case 620:
  case 621: return "Aguaceiros de neve";
  case 211: return "Trovoada";
  case 201:
  case 202: return "Trovoada com granizo";
  default:  return "";
  }
}

/* O aviso mais grave (vermelho > laranja > amarelo), com início e fim;
 * "(+N)" se houver mais.
 */
static String aviso(const std::vector<owm_alerts_t> &alerts)
{
  if (alerts.empty())
  {
    return "";
  }
  const char *cores[3] = {"vermelho", "laranja", "amarelo"};
  const owm_alerts_t *pior = &alerts[0];
  bool achou = false;
  for (int c = 0; c < 3 && !achou; ++c)
  {
    for (const owm_alerts_t &a : alerts)
    {
      if (a.event.indexOf(cores[c]) >= 0)
      {
        pior = &a;
        achou = true;
        break;
      }
    }
  }
  String s = pior->event + "  " + hora(pior->start, "%a %H:%M")
             + " - " + hora(pior->end, "%a %H:%M");
  if (alerts.size() > 1)
  {
    s += " (+" + String(alerts.size() - 1) + ")";
  }
  return s;
}

void prepararEcra(dados_ecra_t &d, const owm_resp_onecall_t &r,
                  const String &estado, int rssi, uint32_t batVoltage,
                  float inTemp, float inHumidity)
{
  const owm_current_t &c = r.current;
  d.icone128    = getCurrentConditionsBitmap128(c, r.daily[0]);
  d.temperatura = graus(c.temp);
  d.sensacao    = graus(c.feels_like);
  d.descricao   = descricao(c.weather.id);
  d.local       = CITY_STRING;
  d.vento       = String(static_cast<int>(std::round(
                    meterspersecond_to_kilometersperhour(c.wind_speed))))
                  + " km/h " + getCompassPointNotation(c.wind_deg);
  d.humidade    = String(c.humidity) + " %";
  d.sol         = hora(c.sunrise, "%H:%M") + " - " + hora(c.sunset, "%H:%M");
  d.aviso       = aviso(r.alerts);
  if (std::isnan(inTemp) || std::isnan(inHumidity))
  {
    d.casa = ""; // BME280 não ligado ou leitura falhada
  }
  else
  {
    d.casa = String(static_cast<int>(std::round(inTemp))) + "\260 | "
             + String(static_cast<int>(std::round(inHumidity))) + " %";
  }

  for (int i = 0; i < 5; ++i)
  {
    const owm_daily_t &dia = r.daily[i];
    d.dias[i].nome    = (i == 0) ? String("Hoje") : hora(dia.dt, "%a");
    d.dias[i].icone64 = getDailyForecastBitmap64(dia);
    d.dias[i].max     = graus(dia.temp.max);
    d.dias[i].min     = graus(dia.temp.min);
  }

  for (int i = 0; i < ECRA_HORAS; ++i)
  {
    d.temperaturaHora[i] = kelvin_to_celsius(r.hourly[i].temp);
    d.chuvaHora[i]       = static_cast<int>(std::round(r.hourly[i].pop * 100));
    d.etiquetaHora[i]    = hora(r.hourly[i].dt, "%H");
  }

  d.estado     = estado;
  d.atualizado = hora(time(nullptr), "%a %H:%M");
  String w = getWiFidesc(rssi);
  w.toLowerCase();
  d.wifi       = "Wi-Fi " + w;
  d.wifiFraco  = rssi < -70;
  if (batVoltage == UINT32_MAX)
  {
    d.bateria = ""; // sem divisor no GPIO0: não há medição
    d.bateriaFraca = false;
  }
  else
  {
    d.bateria = "Bateria " + String(calcBatPercent(batVoltage,
                                                   MIN_BATTERY_VOLTAGE,
                                                   MAX_BATTERY_VOLTAGE)) + " %";
    d.bateriaFraca = batVoltage < WARN_BATTERY_VOLTAGE;
  }

  // a agenda da Mariana vem do Pi (passo 3); por agora fica indisponível
  d.agendaOk = false;
  d.agenda.clear();
}
