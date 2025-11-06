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
  float* pixels;
  float* lastPixels;
  unsigned long lastUpdateTime;
  float dt;
  float lastScrollPos;
  void (*setPixel) (uint16_t index, Rgb color);
  PixelStrip (uint16_t _length, void (*_setPixel) (uint16_t index, Rgb color)) {
    length = _length;
    pixels = new float[length] {};
    lastPixels = new float[length] {};
    setPixel = _setPixel;
    lastUpdateTime = 0;
    dt = 0.0f;
    lastScrollPos = 0.0f;
  }
};

struct Controls {
  uint8_t mode;
  uint8_t palette;
  float control;
  float smooth;
  Rgb back;
  Rgb fore;
  Controls (Rgb _back, Rgb _fore) {
    mode = 0;
    palette = 0;
    control = 0;
    smooth = 0;
    back = _back;
    fore = _fore;
  }
};

void updateStrip(const Controls& data, PixelStrip& strip, unsigned long timeNow);
