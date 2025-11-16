#include <cmath>
#include "palettes.h"
#include "hsv.h"

static const Rgb off = Rgb(0,0,0);

static float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

static Rgb blend3Hsv(const Rgb& a, const Rgb& b, const Rgb& c, float lerp) {
  if (lerp < 1.0f/2.0f) { return blendHsv(a, b, lerp*2.0f); }
  else { return blendHsv(b, c, (lerp-1.0f/2.0f)*2.0f/1.0f); }
}

static Rgb blend2 (Rgb left, Rgb right, float lerp) {
    return Rgb(
      left.red + (right.red - left.red) * lerp,
      left.green + (right.green - left.green) * lerp,
      left.blue + (right.blue - left.blue) * lerp
    );
}

static Rgb blend3(const Rgb& a, const Rgb& b, const Rgb& c, float lerp) {
  if (lerp < 1.0f/2.0f) { return blend2(a, b, lerp*2.0f); }
  else { return blend2(b, c, (lerp-1.0f/2.0f)*2.0f/1.0f); }
}

static Rgb blend4(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, float lerp) {
  if (lerp <= 1.0f/3.0f) { return blend2(a, b, lerp*3.0f); }
   else { return blend3(b, c, d, (lerp-1.0f/3.0f)*3.0f/2.0f); }
}

static Rgb blend5(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, const Rgb& e, float lerp) {
  if (lerp <= 1.0f/4.0f) { return blend2(a, b, lerp*4.0f); }
  else { return blend4(b, c, d, e, (lerp-1.0f/4.0f)*4.0f/3.0f); }
}

static Rgb blend6(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, const Rgb& e, const Rgb& f, float lerp) {
  if (lerp <= 1.0f/5.0f) { return blend2(a, b, lerp*5.0f); }
  else { return blend5(b, c, d, e, f, (lerp-1.0f/5.0f)*5.0f/4.0f); }
}

static Rgb blend7(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, const Rgb& e, const Rgb& f, const Rgb& g, float lerp) {
  if (lerp <= 1.0f/6.0f) { return blend2(a, b, lerp*6.0f); }
  else { return blend6(b, c, d, e, f, g, (lerp-1.0f/6.0f)*6.0f/5.0f); }
}

static Rgb rainbow(float lerp) {
  return Rgb(
    std::pow(lerp, 0.1f)*(0.5f + 0.5f * std::cos(lerp * 2.0f * M_PI * 0.87f)),
    std::pow(lerp, 0.1f)*(0.5f + 0.5f * std::cos(lerp * 2.0f * M_PI * 0.87f + 4.0f * M_PI / 3.0f)),
    std::pow(lerp, 0.1f)*(0.5f + 0.5f * std::cos(lerp * 2.0f * M_PI * 0.87f + 2.0f * M_PI / 3.0f))
  );
}

static Rgb blackbody(float lerp) {
  return Rgb(
    std::pow(lerp, 0.2f)*std::pow(1.07f-lerp, 0.01f),
    std::pow(lerp, 0.8f)*std::pow(1.05f-lerp, 0.01f),
    std::pow(lerp, 2.0f)
  );
}

static Rgb oil(float lerp) {
  lerp = 1.0f - lerp;
  return Rgb(
    std::pow(lerp, 0.1f)*(0.5f + 0.5f * std::cos(lerp * 17.0f)),
    std::pow(lerp, 0.1f)*(0.5f + 0.5f * std::cos(lerp * 15.5)),
    std::pow(lerp, 0.1f)*(0.5f + 0.5f * std::cos(lerp * 14.0f))
  );
}

static Rgb neon(float lerp) {
  return Rgb(
    std::pow(lerp, 0.3f)*limit(0.4f - 0.6f * std::cos(lerp * 0.75f * M_PI + 0.2f*M_PI)),
    std::pow(lerp, 0.15f)*limit(0.4f + 0.6f * std::cos(lerp * 0.75f * M_PI - 0.1f*M_PI)),
    std::pow(lerp, 0.1f)*limit(1.0f + 0.0f * std::cos(lerp * 1.9f * M_PI - 0.1f*M_PI))
  );
}

static Rgb fire(float lerp) {
  return Rgb(
    std::pow(lerp, 0.2f),
    std::pow(lerp, 0.8f)*0.9f,
    std::pow(lerp, 2.0f)*0.3f
  );
}

static Rgb heat(float lerp) {
  return Rgb(
    std::pow(lerp, 0.5f),
    std::pow(lerp, 1.5f)*0.8f,
    std::pow(lerp, 0.07f)/(1.0f+lerp)
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
    case 4: return gamma(blend3(off, fore, back, lerp));
    case 5: return gamma(blend4(back, fore, back, fore, lerp));
    case 6: return gamma(blend6(back, fore, back, fore, back, fore, lerp));
    case 7: return gamma(blend5(off, back, fore, back, fore, lerp));
    case 8: return gamma(blend7(off, back, fore, back, fore, back, fore, lerp));

    case 10: return gamma(blendHsv(back, fore, lerp));
    case 11: return gamma(blend3Hsv(off, back, fore, lerp));
    case 12: return gamma(blend3Hsv(back, fore, off, lerp));
    case 13: return gamma(blend3Hsv(back, off, fore, lerp));
    case 14: return gamma(blend3Hsv(off, fore, back, lerp));

    case 20: return gamma(rainbow(lerp));
    case 21: return gamma(blackbody(lerp));
    case 22: return gamma(oil(lerp));
    case 23: return gamma(neon(lerp));
    case 24: return gamma(fire(lerp));
    case 25: return gamma(heat(lerp));
  }
  return off;
}
