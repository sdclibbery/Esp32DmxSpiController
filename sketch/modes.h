#pragma once

struct Rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  Rgb () { red=0; green=0; blue=0; }
  Rgb (uint8_t _red, uint8_t _green, uint8_t _blue) {
    red = _red;
    green = _green;
    blue = _blue;
  }
};

struct PixelStrip {
  uint16_t length;
  void (*setPixel) (uint16_t index, Rgb color);
  PixelStrip (uint16_t _length, void (*_setPixel) (uint16_t index, Rgb color)) {
    length = _length;
    setPixel = _setPixel;
  }
};

struct FixtureData {
  uint8_t mode;
  float control;
  float smooth;
  Rgb fore;
  Rgb back;
};

void updateStrip(const FixtureData& data, PixelStrip& strip);
