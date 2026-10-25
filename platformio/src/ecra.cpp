/* Desenho do ecrã da estação: 2/3 meteorologia + 1/3 agenda da Mariana.
 * Ver include/ecra.h. Os textos fixos estão em Latin-1 (octal), como as
 * letras do ecrã: \347 = ç, \343 = ã, \352 = ê, \267 = ·, \260 = °.
 *
 * A moldura tapa ~10 px do lado direito do ecrã (foto de 24/10/2026): nada é
 * desenhado à direita de DIR_1.
 */

#include <cmath>
#include "ecra.h"
#include "renderer.h"
#include "config.h"
#include FONT_HEADER

#ifndef ACCENT_COLOR
  #define ACCENT_COLOR GxEPD_BLACK
#endif
#define VERMELHO ACCENT_COLOR

// colunas
static const int ESQ_0  = 6;   // meteorologia
static const int ESQ_1  = 528;
static const int DIVISAO = 536;
static const int DIR_0  = 546; // agenda
static const int DIR_1  = 786;

/* Texto "a negrito": as letras só existem normais, desenha-se duas vezes
 * com 1 px de diferença.
 */
static void negrito(int x, int y, const String &s, alignment_t a,
                    uint16_t cor = GxEPD_BLACK)
{
  drawString(x, y, s, a, cor);
  drawString(x + 1, y, s, a, cor);
}

/* Corta o texto (com "...") até caber em larguraMax, com a letra já escolhida.
 */
static String caber(const String &s, int larguraMax, bool emNegrito = false)
{
  int extra = emNegrito ? 1 : 0;
  if (getStringWidth(s) + extra <= larguraMax)
  {
    return s;
  }
  String t = s;
  while (t.length() > 1 && getStringWidth(t + "...") + extra > larguraMax)
  {
    t.remove(t.length() - 1);
  }
  t.trim();
  return t + "...";
}

/* Em cima: ícone, temperatura, sensação, descrição; ao lado local, vento,
 * humidade e sol.
 */
static void desenharAgora(const dados_ecra_t &d)
{
  display.drawInvertedBitmap(ESQ_0, 14, d.icone128, 128, 128, GxEPD_BLACK);

  display.setFont(&FONT_48pt8b_temperature);
  drawString(146, 92, String(d.temperatura), LEFT);
  display.setFont(&FONT_14pt8b);
  drawString(display.getCursorX() + 4, 46, "\260C", LEFT);

  display.setFont(&FONT_12pt8b);
  drawString(146, 122, "Sensa\347\343o " + String(d.sensacao) + "\260", LEFT);
  drawString(146, 150, caber(d.descricao, ESQ_1 - 146), LEFT);

  const int xInfo = 335, xValor = 420;
  display.setFont(&FONT_14pt8b);
  negrito(xInfo, 34, caber(d.local, ESQ_1 - xInfo, true), LEFT);
  const char *nomes[4] = {"Vento", "Humidade", "Sol", "Casa"};
  const String *valores[4] = {&d.vento, &d.humidade, &d.sol, &d.casa};
  for (int i = 0; i < 4; ++i)
  {
    if (valores[i]->isEmpty())
    {
      continue; // "Casa" só com o sensor (AHT10) ligado
    }
    int y = 60 + i * 21;
    display.setFont(&FONT_9pt8b);
    drawString(xInfo, y, nomes[i], LEFT);
    negrito(xValor, y, caber(*valores[i], ESQ_1 - xValor, true), LEFT);
  }
}

/* Faixa vermelha com o aviso do IPMA (só se houver).
 */
static void desenharAviso(const dados_ecra_t &d)
{
  if (d.aviso.isEmpty())
  {
    return;
  }
  display.fillRect(ESQ_0, 160, ESQ_1 - ESQ_0, 28, VERMELHO);
  display.setFont(&FONT_11pt8b);
  if (getStringWidth(d.aviso) > ESQ_1 - ESQ_0 - 16)
  {
    display.setFont(&FONT_9pt8b);
  }
  negrito(ESQ_0 + 8, 180, caber(d.aviso, ESQ_1 - ESQ_0 - 16, true), LEFT,
          GxEPD_WHITE);
}

/* Previsão de 5 dias.
 */
static void desenharDias(const dados_ecra_t &d)
{
  const int largura = (ESQ_1 - ESQ_0) / 5;
  for (int i = 0; i < 5; ++i)
  {
    const dia_previsao_t &dia = d.dias[i];
    int cx = ESQ_0 + i * largura + largura / 2;
    display.setFont(&FONT_11pt8b);
    negrito(cx, 214, dia.nome, CENTER);
    display.drawInvertedBitmap(cx - 32, 226, dia.icone64, 64, 64, GxEPD_BLACK);
    display.setFont(&FONT_10pt8b);
    drawString(cx, 306, String(dia.max) + "\260 | " + String(dia.min) + "\260",
               CENTER);
  }
}

/* Gráfico das próximas horas: temperatura (linha preta) e probabilidade de
 * chuva (barras vermelhas).
 */
static void desenharGrafico(const dados_ecra_t &d)
{
  const int x0 = 44, x1 = 490, y0 = 322, y1 = 440;

  float tMin = d.temperaturaHora[0], tMax = tMin;
  for (int i = 1; i < ECRA_HORAS; ++i)
  {
    tMin = std::min(tMin, d.temperaturaHora[i]);
    tMax = std::max(tMax, d.temperaturaHora[i]);
  }
  int passo = (tMax - tMin > 20) ? 10 : 5;
  // 1 grau de folga para a linha não encostar às bordas
  int limMin = static_cast<int>(std::floor((tMin - 1) / passo)) * passo;
  int limMax = static_cast<int>(std::ceil((tMax + 1) / passo)) * passo;
  if (limMax - limMin < 2 * passo)
  {
    limMax = limMin + 2 * passo;
  }
  auto yTemp = [&](float t) {
    return static_cast<int>(std::round(
      y1 - (t - limMin) * (y1 - y0) / static_cast<float>(limMax - limMin)));
  };

  const float colX = (x1 - x0) / static_cast<float>(ECRA_HORAS);

  // barras de chuva por baixo de tudo
  for (int i = 0; i < ECRA_HORAS; ++i)
  {
    if (d.chuvaHora[i] <= 0)
    {
      continue;
    }
    int h = d.chuvaHora[i] * (y1 - y0) / 100;
    int bx = static_cast<int>(std::round(x0 + i * colX)) + 2;
    int bw = std::max(2, static_cast<int>(colX) - 4);
    display.fillRect(bx, y1 - h, bw, h, VERMELHO);
  }

  // eixos, linhas tracejadas e escalas
  display.drawRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, GxEPD_BLACK);
  display.setFont(&FONT_8pt8b);
  for (int t = limMin; t <= limMax; t += passo)
  {
    int y = yTemp(t);
    if (t != limMin && t != limMax)
    {
      for (int x = x0 + 2; x < x1; x += 4)
      {
        display.drawPixel(x, y, GxEPD_BLACK);
      }
    }
    drawString(x0 - 5, y + 5, String(t) + "\260", RIGHT);
  }
  for (int p = 0; p <= 100; p += 50)
  {
    int y = y1 - p * (y1 - y0) / 100;
    drawString(x1 + 5, y + 5, String(p) + "%", LEFT, VERMELHO);
  }

  // temperatura
  for (int i = 1; i < ECRA_HORAS; ++i)
  {
    int xa = static_cast<int>(std::round(x0 + (i - 0.5f) * colX));
    int xb = static_cast<int>(std::round(x0 + (i + 0.5f) * colX));
    int ya = yTemp(d.temperaturaHora[i - 1]);
    int yb = yTemp(d.temperaturaHora[i]);
    display.drawLine(xa, ya,     xb, yb,     GxEPD_BLACK);
    display.drawLine(xa, ya + 1, xb, yb + 1, GxEPD_BLACK);
    display.drawLine(xa, ya - 1, xb, yb - 1, GxEPD_BLACK);
  }

  // horas, de 6 em 6 (a 1.ª encostada à esquerda para não tocar na escala)
  for (int i = 0; i < ECRA_HORAS; i += 6)
  {
    int x = static_cast<int>(std::round(x0 + i * colX));
    display.drawLine(x, y1, x, y1 + 3, GxEPD_BLACK);
    drawString(i == 0 ? x + 2 : x, y1 + 16, d.etiquetaHora[i] + "h",
               i == 0 ? LEFT : CENTER);
  }
}

/* Rodapé: hora da atualização, Wi-Fi, bateria e problemas (a vermelho).
 */
static void desenharRodape(const dados_ecra_t &d)
{
  const int y = 474, espaco = 14;
  display.setFont(&FONT_7pt8b);
  drawString(ESQ_0, y, "Atualizado " + d.atualizado, LEFT);
  int x = display.getCursorX() + espaco;
  drawString(x, y, d.wifi, LEFT, d.wifiFraco ? VERMELHO : GxEPD_BLACK);
  x = display.getCursorX() + espaco;
  if (!d.bateria.isEmpty())
  {
    drawString(x, y, d.bateria, LEFT,
               d.bateriaFraca ? VERMELHO : GxEPD_BLACK);
    x = display.getCursorX() + espaco;
  }
  if (!d.estado.isEmpty())
  {
    drawString(ESQ_1, y, caber(d.estado, ESQ_1 - x), RIGHT, VERMELHO);
  }
}

/* Agenda: enche a altura toda; o que não couber fica em "+ N eventos".
 */
static void desenharAgenda(const dados_ecra_t &d)
{
  display.setFont(&FONT_12pt8b);
  negrito(DIR_0, 30, "Agenda da Mariana", LEFT);

  display.setFont(&FONT_10pt8b);
  if (!d.agendaOk)
  {
    drawString(DIR_0, 64, "Agenda indispon\355vel", LEFT, VERMELHO);
    return;
  }
  if (d.agenda.empty())
  {
    drawString(DIR_0, 64, "Sem eventos esta semana", LEFT);
    return;
  }

  const int ALTURA_DIA = 30, ALTURA_EVENTO = 22, LIMITE = 450;
  const int xTexto = DIR_0 + 80;
  int y = 46;     // topo da linha seguinte
  int faltam = 0; // eventos que não couberam
  for (const dia_agenda_t &dia : d.agenda)
  {
    int n = dia.eventos.size();
    // o dia só entra se couber com pelo menos um evento
    if (faltam > 0 || y + ALTURA_DIA + ALTURA_EVENTO > LIMITE)
    {
      faltam += n;
      continue;
    }
    display.setFont(&FONT_11pt8b);
    if (dia.hoje)
    {
      display.fillRect(DIR_0 - 4, y, DIR_1 - DIR_0 + 4, 24, VERMELHO);
      negrito(DIR_0, y + 18, caber(dia.titulo, DIR_1 - DIR_0, true), LEFT,
              GxEPD_WHITE);
    }
    else
    {
      negrito(DIR_0, y + 18, caber(dia.titulo, DIR_1 - DIR_0, true), LEFT);
      display.drawLine(DIR_0, y + 24, DIR_1, y + 24, GxEPD_BLACK);
    }
    y += ALTURA_DIA;

    display.setFont(&FONT_10pt8b);
    for (const evento_agenda_t &ev : dia.eventos)
    {
      if (y + ALTURA_EVENTO > LIMITE)
      {
        ++faltam;
        continue;
      }
      negrito(DIR_0, y + 16, caber(ev.hora, xTexto - DIR_0 - 6, true), LEFT);
      drawString(xTexto, y + 16, caber(ev.texto, DIR_1 - xTexto), LEFT);
      y += ALTURA_EVENTO;
    }
    y += 8;
  }

  if (faltam > 0)
  {
    display.setFont(&FONT_9pt8b);
    drawString(DIR_0, 472, "+ " + String(faltam)
               + (faltam == 1 ? " evento esta semana" : " eventos esta semana"),
               LEFT);
  }
}

void desenharEcra(const dados_ecra_t &d)
{
  desenharAgora(d);
  desenharAviso(d);
  desenharDias(d);
  desenharGrafico(d);
  desenharRodape(d);
  display.drawLine(DIVISAO,     0, DIVISAO,     DISP_HEIGHT - 1, GxEPD_BLACK);
  display.drawLine(DIVISAO + 1, 0, DIVISAO + 1, DISP_HEIGHT - 1, GxEPD_BLACK);
  desenharAgenda(d);
}
