#pragma once

struct Rgb {
  float red;
  float green;
  float blue;
  Rgb () { red=0.0f; green=0.0f; blue=0.0f; }
  Rgb (float _red, float _green, float _blue) {
    red = _red;
    green = _green;
    blue = _blue;
  }
};

struct PixelStrip {
  uint16_t length;
  float* pixels;
  float* lastPixels;
  float* pixelVel;
  unsigned long lastUpdateTime;
  float dt;
  uint8_t lastMode;
  float lastScrollPos;
  float lastDrawPos;
  float lastDropletControl;
  void (*setPixel) (uint16_t index, Rgb colour);
  PixelStrip (uint16_t _length, void (*_setPixel) (uint16_t index, Rgb colour)) {
    length = _length;
    pixels = new float[length] {0.0f};
    lastPixels = new float[length] {0.0f};
    pixelVel = new float[length] {0.0f};
    setPixel = _setPixel;
    lastUpdateTime = 0;
    dt = 0.0f;
    lastMode = 0;
    lastScrollPos = 0.0f;
    lastDrawPos = 0.0f;
    lastDropletControl = 0.0f;
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
