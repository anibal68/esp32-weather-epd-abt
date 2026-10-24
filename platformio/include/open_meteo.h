/* Open-Meteo (sem chave) em vez do OpenWeatherMap.
 *
 * Os dados são postos nas mesmas estruturas owm_* que o resto do programa usa,
 * nas unidades "standard" do OWM (kelvin, m/s, hPa), e o código de tempo WMO é
 * convertido no id do OWM para os ícones continuarem a funcionar.
 */
#ifndef __OPEN_METEO_H__
#define __OPEN_METEO_H__

#include <WiFiClientSecure.h>
#include "api_response.h"

int getOpenMeteo(WiFiClientSecure &client, owm_resp_onecall_t &r);
int getOpenMeteoAr(WiFiClientSecure &client, owm_resp_air_pollution_t &r);
int getIpmaAvisos(WiFiClientSecure &client, std::vector<owm_alerts_t> &alerts);

int pedirJson(WiFiClientSecure &client, const char *host, const String &uri,
              JsonDocument &doc, JsonDocument *filtro = nullptr);

#endif
