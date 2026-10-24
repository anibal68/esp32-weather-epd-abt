// Simulador no Pi: imitação do ecrã (GxEPD2) que desenha numa imagem e
// regista onde ficou cada texto e cada ícone, para procurar sobreposições.
#pragma once
#include <vector>
#include "Arduino.h"
#define DISP_WIDTH  800
#define DISP_HEIGHT 480
enum { GxEPD_WHITE = 0, GxEPD_BLACK = 1, GxEPD_RED = 2 };
typedef enum alignment { LEFT, RIGHT, CENTER } alignment_t;

struct Caixa { int x0, y0, x1, y1; std::string o_que; bool texto; };

class Ecra
{
public:
  uint8_t px[DISP_HEIGHT][DISP_WIDTH] = {};
  std::vector<Caixa> caixas;
  const GFXfont *font = nullptr;
  int cx = 0, cy = 0;
  uint16_t cor = GxEPD_BLACK;
  void drawPixel(int x, int y, uint16_t c) { if (x >= 0 && y >= 0 && x < DISP_WIDTH && y < DISP_HEIGHT) px[y][x] = c; }
  void drawLine(int x0, int y0, int x1, int y1, uint16_t c);
  void fillRect(int x, int y, int w, int h, uint16_t c) { for (int j = y; j < y + h; ++j) for (int i = x; i < x + w; ++i) drawPixel(i, j, c); }
  void drawRect(int x, int y, int w, int h, uint16_t c) { drawLine(x, y, x + w - 1, y, c); drawLine(x, y + h - 1, x + w - 1, y + h - 1, c); drawLine(x, y, x, y + h - 1, c); drawLine(x + w - 1, y, x + w - 1, y + h - 1, c); }
  void drawInvertedBitmap(int x, int y, const uint8_t *b, int w, int h, uint16_t c);
  void setFont(const GFXfont *f) { font = f; }
  void setTextColor(uint16_t c) { cor = c; }
  void setCursor(int x, int y) { cx = x; cy = y; }
  int getCursorX() const { return cx; }
  void getTextBounds(const String &t, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  void print(const String &t);
};
extern Ecra display;

uint16_t getStringWidth(const String &text);
void drawString(int16_t x, int16_t y, const String &text, alignment_t alignment, uint16_t color = GxEPD_BLACK);
