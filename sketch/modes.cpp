#include <cmath>
#include "modes.h"
#include "palettes.h"
#include "perlin.h"

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

static void fadePixel (const Controls& data, PixelStrip& strip, uint16_t idx, float fadeTime) {
  // if (fadeTime < 0.25f) { fadeTime = (0.25f + 2.0f*fadeTime) * (float)(rand()%100) / 100.0f; } // Fizzle fade
  strip.pixels[idx] -= strip.dt / (fadeTime + 0.001f);
  limit(strip.pixels[idx]);
}

void fadeAll(const Controls& data, PixelStrip& strip, float fadeTime) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    fadePixel(data, strip, i, fadeTime);
  }
}

static float gradient (float lerp, float con, float smooth) {
  if (lerp < con) { lerp = 1.0f; } // Solid section
  else { lerp = 1.0f - (lerp - con)/(1.0f - con); } // Gradient sectioon
  lerp = limit(lerp);
  lerp = powerSmooth(lerp, smooth);
  return lerp;
}

static void scroll(const Controls& data, PixelStrip& strip) {
  float scrollDelta = data.smooth - strip.lastScrollPos;
  if (scrollDelta > 0.5f) { scrollDelta -= 1.0f; }
  if (scrollDelta < -0.5f) { scrollDelta += 1.0f; }
  // If scrollDelta is small this frame, accumulate it
  int16_t scrollSteps = (int16_t)(scrollDelta * (strip.length-1));
  if (scrollSteps != 0) {
    for (uint16_t i=0; i<strip.length; i++ ) {
      strip.pixels[i] = strip.lastPixels[(strip.length + i - scrollSteps) % strip.length];
    }
    strip.lastScrollPos = data.smooth; // Only set when actually scrolled so that slow scrolling works
  }
}

static void drawLine(const Controls& data, PixelStrip& strip, float paletteDraw) {
  uint16_t lastDrawIdx = strip.lastDrawPos*(strip.length-1);
  uint16_t drawIdx = data.control*(strip.length-1);
  if (drawIdx < lastDrawIdx) {
    for (uint16_t i=drawIdx; i<=lastDrawIdx; i++ ) {
      strip.pixels[i] = paletteDraw;
    }
  } else {
    for (uint16_t i=lastDrawIdx; i<=drawIdx; i++ ) {
      strip.pixels[i] = paletteDraw;
    }
  }
  strip.lastDrawPos = data.control;
}

// 0: Fade: Whatever is currently showing, fade it down through the palette. Control does nothing, smooth is fade time.
static void fadeMode(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
}

// 1. Scroll: Scroll whatever is currently showing. Control does nothing, smooth is scroll pos.
static void scrollMode(const Controls& data, PixelStrip& strip) {
  scroll(data, strip);
}

// 10: Solid: Blend entire strip through the palette based on control, smoothing does nothing
static void solid(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float value = data.control;
    value += data.smooth * 16.0f * (0.03125f - std::pow(pos - 0.5f, 4.0f));
    value = limit(value);
    strip.pixels[i] = value;
  }
}

// 11: Gradient: control sets start palette position, smoothing sets end palette position, blend between the two
static void gradientMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    strip.pixels[i] = lerp(data.control, data.smooth, pos);
  }
}

// 12: Sine: Sine waves. Control is phase, smoothing is wavelength
static void sineMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float value = (std::sin((pos - data.control) * (1.0f + data.smooth*8.0f) * 2.0f * 3.14159265f) + 1.0f) / 2.0f;
    strip.pixels[i] = value;
  }
}

// 13: Noise: Perlin noise. Control is seed. Smoothing is scale and octaves
static void noiseMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float value = perlin_octaves(0.5f+pos*(8.0f - data.smooth*7.0f), data.control, 4, 0.5f, 2.0f);
    strip.pixels[i] = limit(value*(1.0f + data.smooth) + 0.5f);
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

// 15: StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
static void startFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 16: EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
static void endFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos > 1.0f - data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 17: MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time
static void midFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 30: plot: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is palette pos to plot with.
void plot(const Controls& data, PixelStrip& strip) {
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = data.smooth;
}

// 31: plotFade: Same as plot, but drawn pixels slowly fade back to back colour. Smoothing is fade time
void plotFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 32. plotScroll: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos.
void plotScroll(const Controls& data, PixelStrip& strip) {
  scroll(data, strip);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 33. plotScrollFade: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos. Fade time is fixed long.
void plotScrollFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  scroll(data, strip);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 40: line: Same as Plot, but plot all pixels between last pos and new pos
void line(const Controls& data, PixelStrip& strip) {
  drawLine(data, strip, data.smooth);
}

// 41. LineFade: Same as PlotFade but subsequent plot positions are connected not separate
void lineFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  drawLine(data, strip, 1.0f);
}

// 42. LineScroll: Same as PlotScroll, but plot all pixels between last pos and new pos
void lineScroll(const Controls& data, PixelStrip& strip) {
  scroll(data, strip);
  drawLine(data, strip, 1.0f);
}

// 43. LineScrollFade: Same as PlotScrollFade, but plot all pixels between last pos and new pos
void lineScrollFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  scroll(data, strip);
  drawLine(data, strip, 1.0f);
}

void updateStrip(const Controls& data, PixelStrip& strip, unsigned long timeNow) {
  // Timing
  strip.dt = (float)(timeNow - strip.lastUpdateTime) / 1000000.0f; // delta time in seconds
  if (strip.dt > 0.1f) { strip.dt = 0.1f; }
  strip.lastUpdateTime = timeNow;
  // Apply mode and calculate new pixel scalar values
  uint8_t mode = data.mode;
  switch (mode) {
    // Background
    case 0: fadeMode(data, strip); break;
    case 1: scrollMode(data, strip); break;
    // Full strip
    case 10: solid(data, strip); break;
    case 11: gradientMode(data, strip); break;
    case 12: sineMode(data, strip); break;
    case 13: noiseMode(data, strip); break;
    // Meter
    case 20: startGradient(data, strip); break;
    case 21: endGradient(data, strip); break;
    case 22: midGradient(data, strip); break;
    case 25: startFade(data, strip); break;
    case 26: endFade(data, strip); break;
    case 27: midFade(data, strip); break;
    // Plotting
    case 30: plot(data, strip); break;
    case 31: plotFade(data, strip); break;
    case 32: plotScroll(data, strip); break;
    case 33: plotScrollFade(data, strip); break;
    // Line drawing
    case 40: line(data, strip); break;
    case 41: lineFade(data, strip); break;
    case 42: lineScroll(data, strip); break;
    case 43: lineScrollFade(data, strip); break;
  }
  // Apply palette and set colours
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.setPixel(i, palette(data.palette, data.back, data.fore, strip.pixels[i]));
    strip.lastPixels[i] = strip.pixels[i];
  }
}

