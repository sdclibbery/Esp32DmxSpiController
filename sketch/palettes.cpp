#include <cmath>
#include <algorithm>
#include "palettes.h"
#include "hsv.h"

typedef Rgb (*BlendFunc)(const Rgb&, const Rgb&, float);

static const Rgb off = Rgb(0,0,0);

static float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

static Rgb blend3(const Rgb& a, const Rgb& b, const Rgb& c, float lerp, BlendFunc blend);

static Rgb blendRgb (const Rgb& left, const Rgb& right, float lerp) {
    return Rgb(
      left.red + (right.red - left.red) * lerp,
      left.green + (right.green - left.green) * lerp,
      left.blue + (right.blue - left.blue) * lerp
    );
}

static Rgb blendScaledSum(const Rgb& left, const Rgb& right, float lerp) {
  float sumRed = left.red + right.red;
  float sumGreen = left.green + right.green;
  float sumBlue = left.blue + right.blue;
  float sumMax = std::max(sumRed, std::max(sumGreen, sumBlue));
  float scale = sumMax > 1.0f ? 1.0f / sumMax : 1.0f;
  return blend3(left, Rgb(sumRed * scale, sumGreen * scale, sumBlue * scale), right, lerp, blendRgb);
}

static Rgb blendSub(const Rgb& left, const Rgb& right, float lerp) {
  Rgb sub = Rgb(
    limit(right.red - left.red),
    limit(right.green - left.green),
    limit(right.blue - left.blue)
  );
  return blend3(left, sub, right, lerp, blendRgb);
}

static Rgb blend3(const Rgb& a, const Rgb& b, const Rgb& c, float lerp, BlendFunc blend) {
  if (lerp < 1.0f/2.0f) { return blend(a, b, lerp*2.0f); }
  else { return blend(b, c, (lerp-1.0f/2.0f)*2.0f/1.0f); }
}

static Rgb blend4(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, float lerp, BlendFunc blend) {
  if (lerp <= 1.0f/3.0f) { return blend(a, b, lerp*3.0f); }
   else { return blend3(b, c, d, (lerp-1.0f/3.0f)*3.0f/2.0f, blend); }
}

static Rgb blend5(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, const Rgb& e, float lerp, BlendFunc blend) {
  if (lerp <= 1.0f/4.0f) { return blend(a, b, lerp*4.0f); }
  else { return blend4(b, c, d, e, (lerp-1.0f/4.0f)*4.0f/3.0f, blend); }
}

static Rgb blend6(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, const Rgb& e, const Rgb& f, float lerp, BlendFunc blend) {
  if (lerp <= 1.0f/5.0f) { return blend(a, b, lerp*5.0f); }
  else { return blend5(b, c, d, e, f, (lerp-1.0f/5.0f)*5.0f/4.0f, blend); }
}

static Rgb blend7(const Rgb& a, const Rgb& b, const Rgb& c, const Rgb& d, const Rgb& e, const Rgb& f, const Rgb& g, float lerp, BlendFunc blend) {
  if (lerp <= 1.0f/6.0f) { return blend(a, b, lerp*6.0f); }
  else { return blend6(b, c, d, e, f, g, (lerp-1.0f/6.0f)*6.0f/5.0f, blend); }
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

static float nonLinear(float x) {
  return limit((x*x + x)/2.0f);
}

static float fizzle(float x) {
  float rnd = (float)(rand()%1000) / 1000.0f;
  return limit(x + (std::pow(rnd, 7.0f) - 0.5f) * 0.4f);
}

Rgb palette(uint8_t type, const Rgb& back, const Rgb& fore, float lerp) {
  lerp = limit(lerp);
  switch (type) {
    case 0: return gamma(blendRgb(back, fore, lerp));
    case 1: return gamma(blend3(off, back, fore, lerp, &blendRgb));
    case 2: return gamma(blend3(back, fore, off, lerp, &blendRgb));
    case 3: return gamma(blend3(back, off, fore, lerp, &blendRgb));
    case 4: return gamma(blend3(off, fore, back, lerp, &blendRgb));
    case 5: return gamma(blend4(back, fore, back, fore, lerp, &blendRgb));
    case 6: return gamma(blend6(back, fore, back, fore, back, fore, lerp, &blendRgb));
    case 7: return gamma(blend5(off, back, fore, back, fore, lerp, &blendRgb));
    case 8: return gamma(blend7(off, back, fore, back, fore, back, fore, lerp, &blendRgb));

    case 10: return gamma(blendHsv(back, fore, lerp));
    case 11: return gamma(blend3(off, back, fore, lerp, &blendHsv));
    case 12: return gamma(blend3(back, fore, off, lerp, &blendHsv));
    case 13: return gamma(blend3(back, off, fore, lerp, &blendHsv));
    case 14: return gamma(blend3(off, fore, back, lerp, &blendHsv));
    case 15: return gamma(blend4(back, fore, back, fore, lerp, &blendHsv));
    case 16: return gamma(blend6(back, fore, back, fore, back, fore, lerp, &blendHsv));
    case 17: return gamma(blend5(off, back, fore, back, fore, lerp, &blendHsv));
    case 18: return gamma(blend7(off, back, fore, back, fore, back, fore, lerp, &blendHsv));

    case 20: return gamma(blendHsv(back, fore, nonLinear(lerp)));
    case 21: return gamma(blend3(off, back, fore, nonLinear(lerp), &blendHsv));
    case 22: return gamma(blend3(back, fore, off, nonLinear(lerp), &blendHsv));
    case 23: return gamma(blend3(back, off, fore, nonLinear(lerp), &blendHsv));
    case 24: return gamma(blend3(off, fore, back, nonLinear(lerp), &blendHsv));
    case 25: return gamma(blend4(back, fore, back, fore, nonLinear(lerp), &blendHsv));
    case 26: return gamma(blend6(back, fore, back, fore, back, fore, nonLinear(lerp), &blendHsv));
    case 27: return gamma(blend5(off, back, fore, back, fore, nonLinear(lerp), &blendHsv));
    case 28: return gamma(blend7(off, back, fore, back, fore, back, fore, nonLinear(lerp), &blendHsv));

    case 30: return gamma(blendHsv(back, fore, fizzle(lerp)));
    case 31: return gamma(blend3(off, back, fore, fizzle(lerp), &blendHsv));
    case 32: return gamma(blend3(back, fore, off, fizzle(lerp), &blendHsv));
    case 33: return gamma(blend3(back, off, fore, fizzle(lerp), &blendHsv));
    case 34: return gamma(blend3(off, fore, back, fizzle(lerp), &blendHsv));
    case 35: return gamma(blend4(back, fore, back, fore, fizzle(lerp), &blendHsv));
    case 36: return gamma(blend6(back, fore, back, fore, back, fore, fizzle(lerp), &blendHsv));
    case 37: return gamma(blend5(off, back, fore, back, fore, fizzle(lerp), &blendHsv));
    case 38: return gamma(blend7(off, back, fore, back, fore, back, fore, fizzle(lerp), &blendHsv));

    case 40: return gamma(blendScaledSum(back, fore, lerp));
    case 41: return gamma(blend3(off, back, fore, lerp, &blendScaledSum));
    case 42: return gamma(blend3(back, fore, off, lerp, &blendScaledSum));
    case 43: return gamma(blend3(back, off, fore, lerp, &blendScaledSum));
    case 44: return gamma(blend3(off, fore, back, lerp, &blendScaledSum));
    case 45: return gamma(blend4(back, fore, back, fore, lerp, &blendScaledSum));
    case 46: return gamma(blend6(back, fore, back, fore, back, fore, lerp, &blendScaledSum));
    case 47: return gamma(blend5(off, back, fore, back, fore, lerp, &blendScaledSum));
    case 48: return gamma(blend7(off, back, fore, back, fore, back, fore, lerp, &blendScaledSum));

    case 50: return gamma(blendSub(back, fore, lerp));
    case 51: return gamma(blend3(off, back, fore, lerp, &blendSub));
    case 52: return gamma(blend3(back, fore, off, lerp, &blendSub));
    case 53: return gamma(blend3(back, off, fore, lerp, &blendSub));
    case 54: return gamma(blend3(off, fore, back, lerp, &blendSub));
    case 55: return gamma(blend4(back, fore, back, fore, lerp, &blendSub));
    case 56: return gamma(blend6(back, fore, back, fore, back, fore, lerp, &blendSub));
    case 57: return gamma(blend5(off, back, fore, back, fore, lerp, &blendSub));
    case 58: return gamma(blend7(off, back, fore, back, fore, back, fore, lerp, &blendSub));

    case 240: return gamma(rainbow(lerp));
    case 241: return gamma(blackbody(lerp));
    case 242: return gamma(oil(lerp));
    case 243: return gamma(neon(lerp));
    case 244: return gamma(fire(lerp));
    case 245: return gamma(heat(lerp));
  }
  return off;
}
