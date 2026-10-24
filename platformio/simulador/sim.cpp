/* Simulador do ecrã no Pi: corre o src/ecra.cpp verdadeiro com dados de
 * exemplo, grava uma imagem (PPM) e avisa de sobreposições.
 * Uso: ./simular.sh
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "renderer.h"
#include "ecra.h"
#include "icons/128x128/wi_day_cloudy_128x128.h"
#include "icons/64x64/wi_day_cloudy_64x64.h"
#include "icons/64x64/wi_rain_64x64.h"
#include "icons/64x64/wi_day_sunny_64x64.h"
#include "icons/64x64/wi_fog_64x64.h"

Ecra display;

void Ecra::drawLine(int x0, int y0, int x1, int y1, uint16_t c)
{
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
  for (;;) { drawPixel(x0, y0, c); if (x0 == x1 && y0 == y1) break; int e2 = 2 * e; if (e2 >= dy) { e += dy; x0 += sx; } if (e2 <= dx) { e += dx; y0 += sy; } }
}

void Ecra::drawInvertedBitmap(int x, int y, const uint8_t *b, int w, int h, uint16_t c)
{
  // a caixa do ícone é só a parte desenhada (os ícones têm margens em branco)
  int bw = (w + 7) / 8, x0 = 9999, y0 = 9999, x1 = -1, y1 = -1;
  for (int j = 0; j < h; ++j)
    for (int i = 0; i < w; ++i)
      if (!(b[j * bw + i / 8] & (0x80 >> (i % 8))))
      {
        drawPixel(x + i, y + j, c);
        x0 = std::min(x0, x + i); y0 = std::min(y0, y + j);
        x1 = std::max(x1, x + i); y1 = std::max(y1, y + j);
      }
  if (x1 >= 0) caixas.push_back({x0, y0, x1, y1, "icone", false});
}

void Ecra::getTextBounds(const String &t, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h)
{
  int minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;
  for (unsigned char c : t.s)
  {
    if (c < font->first || c > font->last) continue;
    const GFXglyph &g = font->glyph[c - font->first];
    if (g.width && g.height)
    {
      int a = x + g.xOffset, b = y + g.yOffset;
      if (a < minx) minx = a;
      if (b < miny) miny = b;
      if (a + g.width - 1 > maxx) maxx = a + g.width - 1;
      if (b + g.height - 1 > maxy) maxy = b + g.height - 1;
    }
    x += g.xAdvance;
  }
  if (maxx >= minx) { *x1 = minx; *w = maxx - minx + 1; } else { *x1 = x; *w = 0; }
  if (maxy >= miny) { *y1 = miny; *h = maxy - miny + 1; } else { *y1 = y; *h = 0; }
}

void Ecra::print(const String &t)
{
  int16_t bx, by; uint16_t bw, bh;
  getTextBounds(t, cx, cy, &bx, &by, &bw, &bh);
  if (bw) caixas.push_back({bx, by, bx + bw - 1, by + bh - 1, t.s, true});
  for (unsigned char c : t.s)
  {
    if (c < font->first || c > font->last) { fprintf(stderr, "LETRA EM FALTA: 0x%02x em '%s'\n", c, t.c_str()); continue; }
    const GFXglyph &g = font->glyph[c - font->first];
    const uint8_t *bm = font->bitmap + g.bitmapOffset;
    unsigned bit = 0;
    for (int j = 0; j < g.height; ++j)
      for (int i = 0; i < g.width; ++i, ++bit)
        if (bm[bit / 8] & (0x80 >> (bit % 8))) drawPixel(cx + g.xOffset + i, cy + g.yOffset + j, cor);
    cx += g.xAdvance;
  }
}

// iguais às do renderer.cpp
uint16_t getStringWidth(const String &text)
{
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

void drawString(int16_t x, int16_t y, const String &text, alignment_t alignment, uint16_t color)
{
  int16_t x1, y1; uint16_t w, h;
  display.setTextColor(color);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT) x = x - w;
  if (alignment == CENTER) x = x - w / 2;
  display.setCursor(x, y);
  display.print(text);
}

static void exemplo(dados_ecra_t &d, bool extremo)
{
  d.icone128 = wi_day_cloudy_128x128;
  d.temperatura = extremo ? -12 : 15;
  d.sensacao = extremo ? -18 : 13;
  d.descricao = extremo ? "Trovoada com granizo e aguaceiros fortes" : "Parcialmente nublado";
  d.local = "Braga";
  d.vento = extremo ? "105 km/h NO" : "7 km/h NO";
  d.humidade = extremo ? "100 %" : "65 %";
  d.sol = "07:55 - 18:39";
  d.casa = extremo ? "-5\260 | 100 %" : "21\260 | 55 %";
  d.aviso = extremo ? "Aviso vermelho: Precipita\347\343o  s\341b 18:00 - dom 06:00 (+2)"
                    : "Aviso amarelo: Precipita\347\343o  s\341b 18:00 - dom 06:00";
  const char *nomes[5] = {"Hoje", "Dom", "Seg", "Ter", "Qua"};
  const uint8_t *ic[5] = {wi_day_cloudy_64x64, wi_rain_64x64, wi_rain_64x64, wi_fog_64x64, wi_day_sunny_64x64};
  for (int i = 0; i < 5; ++i)
    d.dias[i] = {nomes[i], ic[i], (extremo ? -2 : 19) + i, (extremo ? -15 : 10) - i};
  float t[24] = {14,13,13,12,11,11,10,10,10,11,12,14,17,19,19,18,17,16,15,15,14,14,13,13};
  int c[24] = {0,0,0,0,0,0,0,0,0,0,0,10,20,40,70,90,90,80,60,40,20,10,0,0};
  for (int i = 0; i < 24; ++i)
  {
    d.temperaturaHora[i] = extremo ? t[i] * 2 - 40 : t[i];
    d.chuvaHora[i] = extremo ? 100 : c[i];
    char b[4]; snprintf(b, sizeof b, "%02d", (18 + i) % 24); d.etiquetaHora[i] = b;
  }
  d.estado = extremo ? "BME n\343o encontrado e outra mensagem comprida" : "BME n\343o encontrado";
  d.atualizado = "s\341b 18:19";
  d.wifi = extremo ? "Wi-Fi fraco" : "Wi-Fi bom";
  d.wifiFraco = extremo;
  d.bateria = extremo ? "Bateria 8 %" : "Bateria 87 %";
  d.bateriaFraca = extremo;
  d.agendaOk = true;
  auto dia = [&](const char *ti, bool h, std::vector<evento_agenda_t> e) { d.agenda.push_back({ti, h, e}); };
  dia("Hoje \267 s\341b 24", true, {{"16:00", "Nata\347\343o"}, {"19:30", "Jantar av\363s"}});
  dia("Amanh\343 \267 dom 25", false, {{"Dia todo", "Feira medieval"}, {"11:00", "Missa"}});
  dia("Seg 26", false, {{"08:30", "Visita de estudo ao museu de Serralves"}, {"17:30", "Ingl\352s"}});
  dia("Ter 27", false, {{"18:00", "Dentista"}});
  dia("Qua 28", false, {{"17:30", "Ingl\352s"}, {"18:30", "Nata\347\343o"}});
  dia("Qui 29", false, {{"09:00", "Teste de Matem\341tica"}});
  dia("Sex 30", false, {{"17:00", "Anivers\341rio da In\352s"}, {"20:00", "Cinema"}});
  if (extremo)
    for (auto &x : d.agenda) x.eventos.push_back({"Dia todo", "Mais um evento muito comprido"});
}

int main(int argc, char **argv)
{
  bool extremo = argc > 2 && !strcmp(argv[2], "extremo");
  dados_ecra_t d;
  exemplo(d, extremo);
  desenharEcra(d);

  int problemas = 0;
  auto &cx = display.caixas;
  for (size_t i = 0; i < cx.size(); ++i)
  {
    const Caixa &a = cx[i];
    if (a.x0 < 0 || a.y0 < 0 || a.x1 >= 786 || a.y1 >= DISP_HEIGHT)
    { printf("FORA DA ZONA VISIVEL: '%s'\n", a.o_que.c_str()); ++problemas; }
    if (a.x0 <= 537 && a.x1 >= 536)
    { printf("PASSA A DIVISORIA: '%s'\n", a.o_que.c_str()); ++problemas; }
    for (size_t j = i + 1; j < cx.size(); ++j)
    {
      const Caixa &b = cx[j];
      bool negrito = a.texto && b.texto && a.o_que == b.o_que && abs(a.x0 - b.x0) <= 1 && a.y0 == b.y0;
      if (negrito) continue;
      const int F = 3; // folga mínima entre textos/ícones
      if (a.x0 <= b.x1 + F && b.x0 <= a.x1 + F && a.y0 <= b.y1 + F && b.y0 <= a.y1 + F)
      { printf("SOBREPOSTOS: '%s' (%d,%d-%d,%d) e '%s' (%d,%d-%d,%d)\n", a.o_que.c_str(), a.x0, a.y0, a.x1, a.y1, b.o_que.c_str(), b.x0, b.y0, b.x1, b.y1); ++problemas; }
    }
  }

  FILE *f = fopen(argv[1], "wb");
  fprintf(f, "P6 %d %d 255\n", DISP_WIDTH, DISP_HEIGHT);
  for (int y = 0; y < DISP_HEIGHT; ++y)
    for (int x = 0; x < DISP_WIDTH; ++x)
    {
      uint8_t c = display.px[y][x];
      uint8_t rgb[3] = {255, 255, 255};
      if (c == GxEPD_BLACK) rgb[0] = rgb[1] = rgb[2] = 0;
      if (c == GxEPD_RED) { rgb[0] = 200; rgb[1] = rgb[2] = 0; }
      fwrite(rgb, 1, 3, f);
    }
  fclose(f);
  printf("%d problema(s)\n", problemas);
  return problemas ? 1 : 0;
}
