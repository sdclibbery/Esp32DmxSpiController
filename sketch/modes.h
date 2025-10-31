#pragma once

#include <NeoPixelBus.h>

typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812xMethod> PixelStrip;
// typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1X8Sk6812Method> PixelStrip;

struct FixtureData {
  uint8_t mode;
  float control;
  float smooth;
  RgbColor fore;
  RgbColor back;
};

void updateStrip(const FixtureData& data, PixelStrip& strip);
