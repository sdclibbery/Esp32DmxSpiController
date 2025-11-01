#include <cmath>
#include "modes.h"

float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

Rgb blend (Rgb left, Rgb right, float lerp) {
    return Rgb(
      left.red + ((static_cast<int16_t>(right.red) - left.red) * lerp),
      left.green + ((static_cast<int16_t>(right.green) - left.green) * lerp),
      left.blue + ((static_cast<int16_t>(right.blue) - left.blue) * lerp)
    );
}

Rgb palette3Way(const Rgb& a, const Rgb& b, const Rgb& c, float lerp) {
  if (lerp < 0.5) {
    return blend(a, b, lerp*2.0f);
  } else {
    return blend(b, c, (lerp-0.5f)*2.0f);
  }
}

const Rgb off = Rgb(0,0,0);
Rgb palette(uint8_t type, const Rgb& back, const Rgb& fore, float lerp) {
  lerp = limit(lerp);
  switch (type) {
    case 0: return blend(back, fore, lerp);
    case 1: return palette3Way(off, back, fore, lerp);
    case 2: return palette3Way(back, fore, off, lerp);
    case 3: return palette3Way(back, off, fore, lerp);
  }
  return off;
}

float powerSmooth (float x, float p) {
  x = limit(x);
  p = limit(p);
  if (p < 0.5f) {
    return std::pow(x, 1.0f + std::pow(2.0f*(0.5f - p),3.0f)*128.0f);
  } else {
    return std::pow(x, 1.0f / (1.0f + std::pow(2.0f*(p - 0.5f),3.0f)*128.0f));
  }
}

float gradient (float lerp, float con, float smooth) {
  if (lerp < con) { lerp = 1.0f; } // Solid section
  else { lerp = 1.0f - (lerp - con)/(1.0f - con); } // Gradient sectioon
  lerp = limit(lerp);
  lerp = powerSmooth(lerp, smooth);
  return lerp;
}

// 0-3: Solid: Blend entire strip through the palette based on control, smoothing does nothing
void modeSolid(const FixtureData& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.pixels[i] = data.control;
  }
}

// 12-15: StartBar: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
void modeStartBar(const FixtureData& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 16-19: EndBar: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
void modeEndBar(const FixtureData& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = 1.0f - lerp; // Go from end
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 20-23: MidBar: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
void modeMidBar(const FixtureData& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = std::abs(0.5f - lerp)*2.0f; // Go outwards from middle
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

void updateStrip(const FixtureData& data, PixelStrip& strip) {
  // Apply mode and calculate new pixel scalar values
  uint8_t mode = data.mode & 0xfc;
  switch (mode) {
    case 0: modeSolid(data, strip); break;
    // case 4: modeNoise(data, strip); break;
    // case 8: modeBlocks(data, strip); break;
    case 12: modeStartBar(data, strip); break;
    case 16: modeEndBar(data, strip); break;
    case 20: modeMidBar(data, strip); break;
  }
  // Apply palette and set colours
  uint8_t paletteType = data.mode & 0x03;
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.setPixel(i, palette(paletteType, data.back, data.fore, strip.pixels[i]));
  }
}

