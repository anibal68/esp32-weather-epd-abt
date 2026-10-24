/* Prepara os dados do desenho novo (ecra.h) a partir do Open-Meteo/IPMA.
 */

#ifndef __PREPARAR_ECRA_H__
#define __PREPARAR_ECRA_H__

#include "api_response.h"
#include "ecra.h"

void prepararEcra(dados_ecra_t &d, const owm_resp_onecall_t &r,
                  const String &estado, int rssi, uint32_t batVoltage,
                  float inTemp, float inHumidity);

#endif
