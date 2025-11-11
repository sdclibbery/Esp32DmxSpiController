#include <cmath>
#include "palettes.h"

static const Rgb off = Rgb(0,0,0);

static float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

static Rgb blend2 (Rgb left, Rgb right, float lerp) {
    return Rgb(
      left.red + (right.red - left.red) * lerp,
      left.green + (right.green - left.green) * lerp,
      left.blue + (right.blue - left.blue) * lerp
    );
}

static Rgb blend3(const Rgb& a, const Rgb& b, const Rgb& c, float lerp) {
  if (lerp < 0.5) {
    return blend2(a, b, lerp*2.0f);
  } else {
    return blend2(b, c, (lerp-0.5f)*2.0f);
  }
}
static Rgb rainbow(float lerp) {
  return Rgb(
    0.5f + 0.5f * std::cos(lerp * 2.0f * M_PI * 5.0f/6.0f),
    0.5f + 0.5f * std::cos(lerp * 2.0f * M_PI * 5.0f/6.0f + 4.0f * M_PI / 3.0f),
    0.5f + 0.5f * std::cos(lerp * 2.0f * M_PI * 5.0f/6.0f + 2.0f * M_PI / 3.0f)
  );
}

static Rgb oil(float lerp) {
  lerp = 1.0f - lerp;
  return Rgb(
    0.5f + 0.5f * std::cos(lerp * 17.0f),
    0.5f + 0.5f * std::cos(lerp * 15.5),
    0.5f + 0.5f * std::cos(lerp * 14.0f)
  );
}

static Rgb neon(float lerp) {
  return Rgb(
    limit(0.4f - 0.6f * std::cos(lerp * 0.9f * M_PI + 0.1f * M_PI)),
    limit(0.4f + 0.6f * std::cos(lerp * 0.9f * M_PI)),
    limit(1.0f + 0.2f * std::cos(lerp * 2.0f * M_PI))
  );
}

static float gammaChannel(float x) { return limit(x*x); } // Gamma approximation. Gives much better visual linearity
static Rgb gamma(const Rgb& colour) {
  return Rgb( gammaChannel(colour.red), gammaChannel(colour.green), gammaChannel(colour.blue) );
}

Rgb palette(uint8_t type, const Rgb& back, const Rgb& fore, float lerp) {
  lerp = limit(lerp);
  switch (type) {
    case 0: return gamma(blend2(back, fore, lerp));
    case 1: return gamma(blend3(off, back, fore, lerp));
    case 2: return gamma(blend3(back, fore, off, lerp));
    case 3: return gamma(blend3(back, off, fore, lerp));
    case 20: return gamma(rainbow(lerp));
    case 31: return gamma(oil(lerp));
    case 32: return gamma(neon(lerp));
  }
  return off;
}
