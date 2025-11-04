#include <cmath>
#include "modes.h"
#include "palettes.h"

static float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

static float lerp (float a, float b, float lerp) {
  return a + (b - a) * limit(lerp);
}

static float powerSmooth (float x, float p) {
  x = limit(x);
  p = limit(p);
  if (p < 0.5f) {
    return std::pow(x, 1.0f + std::pow(2.0f*(0.5f - p),3.0f)*128.0f);
  } else {
    return std::pow(x, 1.0f / (1.0f + std::pow(2.0f*(p - 0.5f),3.0f)*128.0f));
  }
}

static void fadePixel (const Controls& data, PixelStrip& strip, uint16_t idx) {
  strip.pixels[idx] -= strip.dt / (data.smooth + 0.001f);
  limit(strip.pixels[idx]);
}

void fadeAll(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    fadePixel(data, strip, i);
  }
}

static float gradient (float lerp, float con, float smooth) {
  if (lerp < con) { lerp = 1.0f; } // Solid section
  else { lerp = 1.0f - (lerp - con)/(1.0f - con); } // Gradient sectioon
  lerp = limit(lerp);
  lerp = powerSmooth(lerp, smooth);
  return lerp;
}

// 0: Fade: Whatever is currently showing, fade it down through the palette. Control does nothing, smooth is fade time.
static void fadeMode(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip);
}

// 1: Solid: Blend entire strip through the palette based on control, smoothing does nothing
static void solid(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float value = data.control;
    value += data.smooth * 16.0f * (0.03125f - std::pow(pos - 0.5f, 4.0f));
    value = limit(value);
    strip.pixels[i] = value;
  }
}

// 2: Gradient: control sets start palette position, smoothing sets end palette position, blend between the two
static void gradientMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    strip.pixels[i] = lerp(data.control, data.smooth, pos);
  }
}

// 10: StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
static void startGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 11: EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
static void endGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = 1.0f - lerp; // Go from end
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 12: MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
static void midGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = std::abs(0.5f - lerp)*2.0f; // Go outwards from middle
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 22: DrawFade: Same as Draw, but drawn pixels slowly fade back to back colour. Smoothing is fade time
void drawFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip);
  // Set pixel to be drawn
  uint16_t paddleIdx = data.control*strip.length;
  strip.pixels[paddleIdx] = 1.0f;
}

void updateStrip(const Controls& data, PixelStrip& strip, unsigned long timeNow) {
  // Timing
  strip.dt = (float)(timeNow - strip.lastUpdateTime) / 1000000.0f; // delta time in seconds
  if (strip.dt > 0.1f) { strip.dt = 0.1f; }
  strip.lastUpdateTime = timeNow;
  // Apply mode and calculate new pixel scalar values
  uint8_t mode = data.mode;
  switch (mode) {
    case 0: fadeMode(data, strip); break;
    case 1: solid(data, strip); break;
    case 2: gradientMode(data, strip); break;
    case 10: startGradient(data, strip); break;
    case 11: endGradient(data, strip); break;
    case 12: midGradient(data, strip); break;
    case 21: drawFade(data, strip); break;
  }
  // Apply palette and set colours
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.setPixel(i, palette(data.palette, data.back, data.fore, strip.pixels[i]));
  }
  strip.lastControlValue = data.control;
}

