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
  strip.pixels[idx] -= strip.dt / (fadeTime + 0.001f);
  strip.pixels[idx] = limit(strip.pixels[idx]);
}
void fadeAll(const Controls& data, PixelStrip& strip, float fadeTime) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    fadePixel(data, strip, i, 2.0f*fadeTime);
  }
}

static void fizzlePixel (const Controls& data, PixelStrip& strip, uint16_t idx, float fizzleTime) {
  fizzleTime = (0.25f + 2.0f*fizzleTime) * (float)(rand()%1000) / 1000.0f;
  strip.pixels[idx] -= strip.dt / (fizzleTime + 0.001f);
  strip.pixels[idx] = limit(strip.pixels[idx]);
}
void fizzleAll(const Controls& data, PixelStrip& strip, float fizzleTime) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    fizzlePixel(data, strip, i, 2.0f*fizzleTime);
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

static void blur(const Controls& data, PixelStrip& strip, float blurRate) {
  float blurFactor = (blurRate + 0.02f) * strip.dt * 15.0f;
  for (uint16_t i=0; i<strip.length; i++ ) {
    float left = strip.lastPixels[(i < 1) ? 0 : i - 1];
    float right = strip.lastPixels[(i+1 >= strip.length) ? strip.length-1 : i + 1];
    float center = strip.lastPixels[i];
    float lDiff = left - center;
    float rDiff = right - center;
    strip.pixels[i] += lDiff * blurFactor / 2.0f;
    strip.pixels[i] += rDiff * blurFactor / 2.0f;
    strip.pixels[i] = limit(strip.pixels[i]*(1.0f - strip.dt*0.1f));
  }
}

static void wave(const Controls& data, PixelStrip& strip, float spring, float damp=0.1f, float bounce=0.0f) {
  spring = 1.0f + spring * 20.0f;
  for (uint16_t i=0; i<strip.length; i++ ) {
    float acc = 0.0f;
    if (i > 0) { acc += (strip.lastPixels[i-1] - strip.lastPixels[i]) * spring * 0.5f; }
    if (i < strip.length - 1) { acc += (strip.lastPixels[i+1] - strip.lastPixels[i]) * spring * 0.5f; }
// Need a second order term to try and straighten/smooth the rope?
    acc += -strip.pixelVel[i] * damp;
    strip.pixelVel[i] += acc * strip.dt;
  }
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.pixels[i] += strip.pixelVel[i] * strip.dt;
    if (strip.pixels[i] < 0.0f) {
      strip.pixels[i] = 0.0f;
      strip.pixelVel[i] *= -bounce;
    }
    if (strip.pixels[i] > 1.0f) {
      strip.pixels[i] = 1.0f;
      strip.pixelVel[i] *= -bounce;
    }
  }
}

static void biScroll(const Controls& data, PixelStrip& strip, float direction=1.0f) {
  float scrollDelta = (data.smooth - strip.lastScrollPos)*direction;
  if (scrollDelta > 0.5f) { scrollDelta -= 1.0f; }
  if (scrollDelta < -0.5f) { scrollDelta += 1.0f; }
  // If scrollDelta is small this frame, accumulate it
  int16_t scrollSteps = (int16_t)(scrollDelta * (strip.length-1));
  if (scrollSteps != 0) {
    for (uint16_t i=0; i<strip.length; i++ ) {
      if (i < strip.length/2) {
        strip.pixels[i] = strip.lastPixels[(strip.length + i - scrollSteps) % strip.length];
      } else {
        strip.pixels[i] = strip.lastPixels[(strip.length + i + scrollSteps) % strip.length];
      }
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

// 1: Fizzle: Whatever is currently showing, fizzle it down through the palette. Control does nothing, smooth is fade time.
static void fizzleMode(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
}

// 2. Scroll: Scroll whatever is currently showing. Control does nothing, smooth is scroll pos.
static void scrollMode(const Controls& data, PixelStrip& strip) {
  scroll(data, strip);
}

// 3. Blur: Blur whatever is currently showing with a slight fade. Control does nothing, smooth is blur rate.
static void blurMode(const Controls& data, PixelStrip& strip) {
  blur(data, strip, data.smooth);
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

// 14. Droplet: plot at random pos when control has a rising edge. Smooth is blur rate.
static void dropletMode(const Controls& data, PixelStrip& strip) {
  blur(data, strip, data.smooth);
  if (data.control > 0.5f && strip.lastDropletControl <= 0.5f) {
    uint16_t dropPos = rand() % strip.length;
    strip.pixels[dropPos] = 1.0f;    
  }
  strip.lastDropletControl = data.control;
}

// 20: StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
static void startGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 21: EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
static void endGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = 1.0f - lerp; // Go from end
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 22: MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
static void midGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = std::abs(0.5f - lerp)*2.0f; // Go outwards from middle
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 23. EndsGradient: solid bar expands from both ends of strip, control is length of bar, smooth is lerp power in rest of strip
static void endsGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = 1.0f - std::abs(0.5f - lerp)*2.0f; // Go inwards from ends
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 30: StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
static void startFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 31: EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
static void endFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos > 1.0f - data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 32: MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time
static void midFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 33. EndsFade: solid bar expands from both ends of strip, control is length of bar, smooth is fade time
static void endsFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (1.0f - std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 40: StartFizzle: solid bar rises from start of strip, control is length of bar, smooth is fizzle time
static void startFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 41: EndFizzle: solid bar falls from end of strip, control is length of bar, smooth is fizzle time
static void endFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos > 1.0f - data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 42: MidFizzle: solid bar expands from centre of strip, control is length of bar, smooth is fizzle time
static void midFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 43. Endsfizzle: solid bar expands from both ends of strip, control is length of bar, smooth is fizzle time
static void endsFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (1.0f - std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 50. Plot: Control sets plot pos. Smoothing is palette pos to plot at the plot pos.
void plot(const Controls& data, PixelStrip& strip) {
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = data.smooth;
}

// 51: plotFade: Same as plot, but drawn pixels slowly fade back to back colour. Smoothing is fade time
void plotFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 52. plotScrollFade: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos. Fade time is fixed long.
void plotScrollFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  scroll(data, strip);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 53: plotFizzle: Same as plot, but drawn pixels slowly fizzle back to back colour. Smoothing is fizzle time
void plotFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 60: line: Same as Plot, but plot all pixels between last pos and new pos
void line(const Controls& data, PixelStrip& strip) {
  drawLine(data, strip, data.smooth);
}

// 61. LineFade: Same as PlotFade but subsequent plot positions are connected not separate
void lineFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  drawLine(data, strip, 1.0f);
}

// 62. LineScrollFade: Same as PlotScrollFade, but plot all pixels between last pos and new pos
void lineScrollFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  scroll(data, strip);
  drawLine(data, strip, 1.0f);
}

// 63. LineFizzle: Same as PlotFizzle but subsequent plot positions are connected not separate
void lineFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  drawLine(data, strip, 1.0f);
}

// 70. StartTicker: Control sets palette entry to draw at start of strip. Smoothing is scroll pos
static void startTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[0] = data.control;
  scroll(data, strip);
}

// 71. EndTicker: Control sets palette entry to draw at end of strip. Smoothing is scroll pos
static void endTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[strip.length-1] = data.control;
  scroll(data, strip);
}

// 72. MidTicker: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways
static void midTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[strip.length/2] = data.control;
  biScroll(data, strip, -1.0f);
}

// 73. EndsTicker: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways
static void endsTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[0] = data.control;
  strip.pixels[strip.length-1] = data.control;
  biScroll(data, strip);
}

// 80. StartTickerFade: Control sets palette entry to draw at start of strip. Smoothing is scroll pos. A fixed slow fade is applied
static void startTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[0] = data.control;
  scroll(data, strip);
}

// 81. EndTickerFade: Control sets palette entry to draw at end of strip. Smoothing is scroll pos. A fixed slow fade is applied
static void endTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[strip.length-1] = data.control;
  scroll(data, strip);
}

// 82. MidTickerFade: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways. A fixed slow fade is applied
static void midTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[strip.length/2] = data.control;
  biScroll(data, strip, -1.0f);
}

// 83. EndsTickerFade: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways. A fixed slow fade is applied
static void endsTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[0] = data.control;
  strip.pixels[strip.length-1] = data.control;
  biScroll(data, strip);
}

// 90. StartBlur: pixel drawn at start of strip, control is palette entry of pixel, smooth is blur rate
static void startBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 3.0f * data.smooth);
  strip.pixels[0] = data.control;
}

// 91. EndBlur: pixel drawn at end of strip, control is palette entry of pixel, smooth is blur rate
static void endBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 3.0f * data.smooth);
  strip.pixels[strip.length-1] = data.control;
}

// 92. MidBlur: pixel drawn at centre of strip, control is palette entry of pixel, smooth is blur rate
static void midBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 2.0f * data.smooth);
  strip.pixels[strip.length/2] = data.control;
}

// 93. EndsBlur: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is blur rate
static void endsBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 2.0f * data.smooth);
  strip.pixels[0] = data.control;
  strip.pixels[strip.length-1] = data.control;
}

// 100. StartWave: pixel drawn at start of strip, control is palette entry of pixel, smooth is spring
static void startWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[0] = data.control;
  strip.pixelVel[0] = 0.0f;
}

// 101. EndWave: pixel drawn at end of strip, control is palette entry of pixel, smooth is spring
static void endWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[strip.length-1] = data.control;
  strip.pixelVel[strip.length-1] = 0.0f;
}

// 102. MidWave: pixel drawn at centre of strip, control is palette entry of pixel, smooth is spring
static void midWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[strip.length/2] = data.control;
  strip.pixelVel[strip.length/2] = 0.0f;
}

// 103. EndsWave: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is spring
static void endsWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[0] = data.control;
  strip.pixelVel[0] = 0.0f;
  strip.pixels[strip.length-1] = data.control;
  strip.pixelVel[strip.length-1] = 0.0f;
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
    case 1: fizzleMode(data, strip); break;
    case 2: scrollMode(data, strip); break;
    case 3: blurMode(data, strip); break;
    // Full strip
    case 10: solid(data, strip); break;
    case 11: gradientMode(data, strip); break;
    case 12: sineMode(data, strip); break;
    case 13: noiseMode(data, strip); break;
    case 14: dropletMode(data, strip); break;
    // Meter gradient
    case 20: startGradient(data, strip); break;
    case 21: endGradient(data, strip); break;
    case 22: midGradient(data, strip); break;
    case 23: endsGradient(data, strip); break;
    // Meter fade
    case 30: startFade(data, strip); break;
    case 31: endFade(data, strip); break;
    case 32: midFade(data, strip); break;
    case 33: endsFade(data, strip); break;
    // Meter fizzle
    case 40: startFizzle(data, strip); break;
    case 41: endFizzle(data, strip); break;
    case 42: midFizzle(data, strip); break;
    case 43: endsFizzle(data, strip); break;
    // Plotting
    case 50: plot(data, strip); break;
    case 51: plotFade(data, strip); break;
    case 52: plotScrollFade(data, strip); break;
    case 53: plotFizzle(data, strip); break;
    // Line drawing
    case 60: line(data, strip); break;
    case 61: lineFade(data, strip); break;
    case 62: lineScrollFade(data, strip); break;
    case 63: lineFizzle(data, strip); break;
    // Ticker
    case 70: startTicker(data, strip); break;
    case 71: endTicker(data, strip); break;
    case 72: midTicker(data, strip); break;
    case 73: endsTicker(data, strip); break;
    // TickerFade
    case 80: startTickerFade(data, strip); break;
    case 81: endTickerFade(data, strip); break;
    case 82: midTickerFade(data, strip); break;
    case 83: endsTickerFade(data, strip); break;
    // MeterBlur
    case 90: startBlur(data, strip); break;
    case 91: endBlur(data, strip); break;
    case 92: midBlur(data, strip); break;
    case 93: endsBlur(data, strip); break;
    // MeterWave
    case 100: startWave(data, strip); break;
    case 101: endWave(data, strip); break;
    case 102: midWave(data, strip); break;
    case 103: endsWave(data, strip); break;
  }
  // Apply palette and set colours
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.setPixel(i, palette(data.palette, data.back, data.fore, strip.pixels[i], strip.dt));
    strip.lastPixels[i] = strip.pixels[i];
  }
}

