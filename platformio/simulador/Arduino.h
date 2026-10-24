// Simulador no Pi: imitação mínima do Arduino.h (String e tipos das letras).
#pragma once
#include <cstdint>
#include <string>
#define PROGMEM

typedef struct { uint16_t bitmapOffset; uint8_t width, height, xAdvance; int8_t xOffset, yOffset; } GFXglyph;
typedef struct { uint8_t *bitmap; GFXglyph *glyph; uint16_t first, last; uint8_t yAdvance; } GFXfont;

class String
{
public:
  std::string s;
  String() {}
  String(const char *c) : s(c) {}
  String(const std::string &c) : s(c) {}
  String(char c) : s(1, c) {}
  String(int n) : s(std::to_string(n)) {}
  unsigned length() const { return s.size(); }
  bool isEmpty() const { return s.empty(); }
  const char *c_str() const { return s.c_str(); }
  void remove(unsigned i) { s.erase(i); }
  void trim()
  {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(0, 1);
  }
  String &operator+=(const String &o) { s += o.s; return *this; }
  friend String operator+(const String &a, const String &b) { return String(a.s + b.s); }
  friend String operator+(const String &a, const char *b) { return String(a.s + b); }
  friend String operator+(const char *a, const String &b) { return String(a + b.s); }
  bool operator==(const String &o) const { return s == o.s; }
};
