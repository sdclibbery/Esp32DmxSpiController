#include <cmath>
#include "palettes.h"

static float limit (float x) {
  if (std::isnan(x)) { return 0.0f; }
  if (x < 0.0f) { return 0.0f; }
  if (x > 1.0f) { return 1.0f; }
  return x;
}

static Rgb blend (Rgb left, Rgb right, float lerp) {
    return Rgb(
      left.red + ((static_cast<int16_t>(right.red) - left.red) * lerp),
      left.green + ((static_cast<int16_t>(right.green) - left.green) * lerp),
      left.blue + ((static_cast<int16_t>(right.blue) - left.blue) * lerp)
    );
}

static Rgb palette3Way(const Rgb& a, const Rgb& b, const Rgb& c, float lerp) {
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
