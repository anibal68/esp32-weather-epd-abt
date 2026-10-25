/* Desenho do ecrã da estação (aprovado a 24/10/2026): 2/3 à esquerda com a
 * meteorologia, 1/3 à direita com a agenda da Mariana.
 *
 * O desenho só recebe textos já prontos (em Latin-1, como as letras do ecrã)
 * e números; quem os prepara é o preparar_ecra.cpp. Assim o ecra.cpp também
 * compila no Pi (simulador em ../simulador) para ver o resultado e procurar
 * sobreposições antes de gravar a placa.
 */

#ifndef __ECRA_H__
#define __ECRA_H__

#include <vector>
#include <Arduino.h>

#define ECRA_HORAS 24 // horas no gráfico

typedef struct evento_agenda
{
  String hora;  // "16:00" ou "Dia todo"
  String texto; // "Natação"
} evento_agenda_t;

typedef struct dia_agenda
{
  String titulo; // "Hoje · sáb 24" (o Pi prepara o texto)
  bool   hoje;
  std::vector<evento_agenda_t> eventos;
} dia_agenda_t;

typedef struct dia_previsao
{
  String         nome;    // "Hoje", "Dom", ...
  const uint8_t *icone64;
  int            max, min; // °C
} dia_previsao_t;

typedef struct dados_ecra
{
  // agora
  const uint8_t *icone128;
  int    temperatura;       // °C
  int    sensacao;          // °C
  String descricao;         // "Parcialmente nublado"
  String local;             // "Braga"
  String vento;             // "18 km/h NO"
  String humidade;          // "72 %"
  String sol;               // "07:55 - 18:39"
  String casa;              // "21\260 | 55 %" do AHT10 (vazio = sem sensor)
  // avisos do IPMA (vazio = sem avisos)
  String aviso;
  // 5 dias
  dia_previsao_t dias[5];
  // gráfico das próximas horas
  float temperaturaHora[ECRA_HORAS]; // °C
  int   chuvaHora[ECRA_HORAS];       // probabilidade, 0-100 %
  String etiquetaHora[ECRA_HORAS];   // "18", "21", ...
  // rodapé
  String estado;            // "Sensor não encontrado" (vazio = tudo bem)
  String atualizado;        // "sáb 18:19"
  String wifi;              // "Wi-Fi bom"
  bool   wifiFraco;
  String bateria;           // "Bateria 87 %" (vazio = sem medição)
  bool   bateriaFraca;
  // agenda
  bool   agendaOk;          // false = não foi possível ir buscar a agenda
  std::vector<dia_agenda_t> agenda;
} dados_ecra_t;

void desenharEcra(const dados_ecra_t &d);

#endif
