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

static void wave(const Controls& data, PixelStrip& strip, float spring, float damp=0.01f, float bounce=0.1f) {
  spring = 1.0f + spring * 20.0f;
  for (uint16_t i=0; i<strip.length; i++ ) {
    float acc = 0.0f;
    if (i > 0) { acc += (strip.lastPixels[i-1] - strip.lastPixels[i]) * spring * 0.5f; }
    if (i < strip.length - 1) { acc += (strip.lastPixels[i+1] - strip.lastPixels[i]) * spring * 0.5f; }
    if (i > 1) { acc -= (strip.lastPixels[i-2] - 2.0f*strip.lastPixels[i-1] + strip.lastPixels[i]) * spring * 0.1f; }
    if (i < strip.length - 2) { acc -= (strip.lastPixels[i+2] - 2.0f*strip.lastPixels[i+1] + strip.lastPixels[i]) * spring * 0.1f; }
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

// 12. Droplet: plot at random pos when control has a rising edge. Smooth is blur rate.
static void dropletMode(const Controls& data, PixelStrip& strip) {
  blur(data, strip, data.smooth);
  if (data.control > 0.5f && strip.lastDropletControl <= 0.5f) {
    uint16_t dropPos = rand() % strip.length;
    strip.pixels[dropPos] = 1.0f;
  }
  strip.lastDropletControl = data.control;
}

// 13. Xor: XOR bit patterns. control is pattern y, smooth is mod
static void xorMode(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 0.2f);
  for (uint16_t i=0; i<strip.length; i++ ) {
    uint16_t mod = (uint16_t)(24.0f - data.control*19.0f);
    uint16_t value = (i ^ (uint16_t)(data.smooth*255.0f)) % mod;
    if (value == 0) { strip.pixels[i] = 1.0f; }
    if (value == mod/2) { strip.pixels[i] = 0.5f; }
  }
}

// 20: Noise: Perlin noise. Control is seed. Smoothing is scale and octaves
static void noiseMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float value = perlin_octaves(0.5f+pos*(8.0f - data.smooth*7.0f), data.control, 4, 0.5f, 2.0f);
    strip.pixels[i] = limit(value*(1.0f + data.smooth) + 0.5f);
  }
}

// 21: Sine: Sine waves. Control is phase, smoothing is wavelength
static void sineMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float value = (std::sin((pos - data.control) * (1.0f + data.smooth*8.0f) * 2.0f * 3.14159265f) + 1.0f) / 2.0f;
    strip.pixels[i] = value;
  }
}

// 22. Saw: Saw waves. Control is phase, smoothing is wavelength
static void sawMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float freq = 1.0f + data.smooth*8.0f;
    float value = fmod(pos*freq - data.control + 1.0f, 1.0f);
    strip.pixels[i] = limit(value);
  }
}

// 23. Tri: Triangle waves. Control is phase, smoothing is wavelength
static void triMode(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    float freq = 1.0f + data.smooth*8.0f;
    float value = fmod(pos*freq - data.control + 1.0f, 1.0f);
    value = (value < 0.5f) ? (value * 2.0f) : (1.0f - (value - 0.5f) * 2.0f);
    strip.pixels[i] = limit(value);
  }
}

// 50: StartGradient: solid bar rises from start of strip, control is length of bar, smooth is lerp power in rest of strip
static void startGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 51: EndGradient: solid bar falls from end of strip, control is length of bar, smooth is lerp power in rest of strip
static void endGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = 1.0f - lerp; // Go from end
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 52: MidGradient: solid bar expands from centre of strip, control is length of bar, smooth is lerp power in rest of strip
static void midGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = std::abs(0.5f - lerp)*2.0f; // Go outwards from middle
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 53. EndsGradient: solid bar expands from both ends of strip, control is length of bar, smooth is lerp power in rest of strip
static void endsGradient(const Controls& data, PixelStrip& strip) {
  for (uint16_t i=0; i<strip.length; i++ ) {
    float lerp = (float)i / (float)(strip.length-1); // Position along strip
    lerp = 1.0f - std::abs(0.5f - lerp)*2.0f; // Go inwards from ends
    lerp = gradient(lerp, data.control, data.smooth);
    strip.pixels[i] = lerp;
  }
}

// 60: StartFade: solid bar rises from start of strip, control is length of bar, smooth is fade time
static void startFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 61: EndFade: solid bar falls from end of strip, control is length of bar, smooth is fade time
static void endFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos > 1.0f - data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 62: MidFade: solid bar expands from centre of strip, control is length of bar, smooth is fade time
static void midFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 63. EndsFade: solid bar expands from both ends of strip, control is length of bar, smooth is fade time
static void endsFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (1.0f - std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 70: StartFizzle: solid bar rises from start of strip, control is length of bar, smooth is fizzle time
static void startFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 71: EndFizzle: solid bar falls from end of strip, control is length of bar, smooth is fizzle time
static void endFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (pos > 1.0f - data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 72: MidFizzle: solid bar expands from centre of strip, control is length of bar, smooth is fizzle time
static void midFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 73. Endsfizzle: solid bar expands from both ends of strip, control is length of bar, smooth is fizzle time
static void endsFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  for (uint16_t i=0; i<strip.length; i++ ) {
    float pos = (float)i / (float)(strip.length-1);
    if (1.0f - std::abs(0.5f - pos)*2.0f <= data.control) {
      strip.pixels[i] = 1.0f;
    }
  }
}

// 80. StartBlur: pixel drawn at start of strip, control is palette entry of pixel, smooth is blur rate
static void startBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 3.0f * data.smooth);
  strip.pixels[0] = data.control;
}

// 81. EndBlur: pixel drawn at end of strip, control is palette entry of pixel, smooth is blur rate
static void endBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 3.0f * data.smooth);
  strip.pixels[strip.length-1] = data.control;
}

// 82. MidBlur: pixel drawn at centre of strip, control is palette entry of pixel, smooth is blur rate
static void midBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 2.0f * data.smooth);
  strip.pixels[strip.length/2] = data.control;
}

// 83. EndsBlur: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is blur rate
static void endsBlur(const Controls& data, PixelStrip& strip) {
  blur(data, strip, 0.5f + 2.0f * data.smooth);
  strip.pixels[0] = data.control;
  strip.pixels[strip.length-1] = data.control;
}

// 90. StartWave: pixel drawn at start of strip, control is palette entry of pixel, smooth is spring
static void startWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[0] = data.control;
  strip.pixelVel[0] = 0.0f;
}

// 91. EndWave: pixel drawn at end of strip, control is palette entry of pixel, smooth is spring
static void endWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[strip.length-1] = data.control;
  strip.pixelVel[strip.length-1] = 0.0f;
}

// 92. MidWave: pixel drawn at centre of strip, control is palette entry of pixel, smooth is spring
static void midWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[strip.length/2] = data.control;
  strip.pixelVel[strip.length/2] = 0.0f;
}

// 93. EndsWave: pixels drawn at both ends of strip, control is palette entry of pixel, smooth is spring
static void endsWave(const Controls& data, PixelStrip& strip) {
  wave(data, strip, data.smooth);
  strip.pixels[0] = data.control;
  strip.pixelVel[0] = 0.0f;
  strip.pixels[strip.length-1] = data.control;
  strip.pixelVel[strip.length-1] = 0.0f;
}

// 100. StartTicker: Control sets palette entry to draw at start of strip. Smoothing is scroll pos
static void startTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[0] = data.control;
  scroll(data, strip);
}

// 101. EndTicker: Control sets palette entry to draw at end of strip. Smoothing is scroll pos
static void endTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[strip.length-1] = data.control;
  scroll(data, strip);
}

// 102. MidTicker: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways
static void midTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[strip.length/2] = data.control;
  biScroll(data, strip, -1.0f);
}

// 103. EndsTicker: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways
static void endsTicker(const Controls& data, PixelStrip& strip) {
  strip.pixels[0] = data.control;
  strip.pixels[strip.length-1] = data.control;
  biScroll(data, strip);
}

// 110. StartTickerFade: Control sets palette entry to draw at start of strip. Smoothing is scroll pos. A fixed slow fade is applied
static void startTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[0] = data.control;
  scroll(data, strip);
}

// 111. EndTickerFade: Control sets palette entry to draw at end of strip. Smoothing is scroll pos. A fixed slow fade is applied
static void endTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[strip.length-1] = data.control;
  scroll(data, strip);
}

// 112. MidTickerFade: Control sets palette entry to draw at mid of strip. Smoothing is scroll pos, but moving out both ways. A fixed slow fade is applied
static void midTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[strip.length/2] = data.control;
  biScroll(data, strip, -1.0f);
}

// 113. EndsTickerFade: Control sets palette entry to draw at both ends of strip. Smoothing is scroll pos, but moving in both ways. A fixed slow fade is applied
static void endsTickerFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  strip.pixels[0] = data.control;
  strip.pixels[strip.length-1] = data.control;
  biScroll(data, strip);
}

// 150. Plot: Control sets plot pos. Smoothing is palette pos to plot at the plot pos.
void plot(const Controls& data, PixelStrip& strip) {
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = data.smooth;
}

// 151: plotFade: Same as plot, but drawn pixels slowly fade back to back colour. Smoothing is fade time
void plotFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 152. plotScrollFade: Control sets plot pos. Fore is drawn into the strip at plot pos. Smoothing is scroll pos. Fade time is fixed long.
void plotScrollFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  scroll(data, strip);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 153: plotFizzle: Same as plot, but drawn pixels slowly fizzle back to back colour. Smoothing is fizzle time
void plotFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
  uint16_t plotIdx = data.control*(strip.length-1);
  strip.pixels[plotIdx] = 1.0f;
}

// 160: line: Same as Plot, but plot all pixels between last pos and new pos
void line(const Controls& data, PixelStrip& strip) {
  drawLine(data, strip, data.smooth);
}

// 161. LineFade: Same as PlotFade but subsequent plot positions are connected not separate
void lineFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, data.smooth);
  drawLine(data, strip, 1.0f);
}

// 162. LineScrollFade: Same as PlotScrollFade, but plot all pixels between last pos and new pos
void lineScrollFade(const Controls& data, PixelStrip& strip) {
  fadeAll(data, strip, 1.0f);
  scroll(data, strip);
  drawLine(data, strip, 1.0f);
}

// 163. LineFizzle: Same as PlotFizzle but subsequent plot positions are connected not separate
void lineFizzle(const Controls& data, PixelStrip& strip) {
  fizzleAll(data, strip, data.smooth);
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
    case 1: fizzleMode(data, strip); break;
    case 2: scrollMode(data, strip); break;
    case 3: blurMode(data, strip); break;
    // Full strip
    case 10: solid(data, strip); break;
    case 11: gradientMode(data, strip); break;
    case 12: dropletMode(data, strip); break;
    case 13: xorMode(data, strip); break;
    // Waveforms
    case 20: noiseMode(data, strip); break;
    case 21: sineMode(data, strip); break;
    case 22: sawMode(data, strip); break;
    case 23: triMode(data, strip); break;
    // Meter gradient
    case 50: startGradient(data, strip); break;
    case 51: endGradient(data, strip); break;
    case 52: midGradient(data, strip); break;
    case 53: endsGradient(data, strip); break;
    // Meter fade
    case 60: startFade(data, strip); break;
    case 61: endFade(data, strip); break;
    case 62: midFade(data, strip); break;
    case 63: endsFade(data, strip); break;
    // Meter fizzle
    case 70: startFizzle(data, strip); break;
    case 71: endFizzle(data, strip); break;
    case 72: midFizzle(data, strip); break;
    case 73: endsFizzle(data, strip); break;
    // MeterBlur
    case 80: startBlur(data, strip); break;
    case 81: endBlur(data, strip); break;
    case 82: midBlur(data, strip); break;
    case 83: endsBlur(data, strip); break;
    // MeterWave
    case 90: startWave(data, strip); break;
    case 91: endWave(data, strip); break;
    case 92: midWave(data, strip); break;
    case 93: endsWave(data, strip); break;
    // Ticker
    case 100: startTicker(data, strip); break;
    case 101: endTicker(data, strip); break;
    case 102: midTicker(data, strip); break;
    case 103: endsTicker(data, strip); break;
    // TickerFade
    case 110: startTickerFade(data, strip); break;
    case 111: endTickerFade(data, strip); break;
    case 112: midTickerFade(data, strip); break;
    case 113: endsTickerFade(data, strip); break;
    // Plotting
    case 150: plot(data, strip); break;
    case 151: plotFade(data, strip); break;
    case 152: plotScrollFade(data, strip); break;
    case 153: plotFizzle(data, strip); break;
    // Line drawing
    case 160: line(data, strip); break;
    case 161: lineFade(data, strip); break;
    case 162: lineScrollFade(data, strip); break;
    case 163: lineFizzle(data, strip); break;
  }
  // Apply palette and set colours
  for (uint16_t i=0; i<strip.length; i++ ) {
    strip.setPixel(i, palette(data.palette, data.back, data.fore, strip.pixels[i], strip.dt));
    strip.lastPixels[i] = strip.pixels[i];
  }
}

