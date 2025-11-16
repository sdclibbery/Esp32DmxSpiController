#include <iostream>
#include <algorithm> // For std::clamp and std::max/min
#include <cmath>     // For fmod and basic math functions
#include "hsv.h"

/**
 * @brief Represents a color using Hue, Saturation, and Value float components.
 * * Hue (h): [0.0f, 360.0f) degrees
 * * Saturation (s): [0.0f, 1.0f]
 * * Value (v): [0.0f, 1.0f]
 */
struct Hsv {
    float h;
    float s;
    float v;
};

// --- Conversion Functions ---

/**
 * @brief Converts an Rgb color to Hsv color space.
 */
static Hsv rgb_to_hsv(const Rgb& color) {
    float r = color.red;
    float g = color.green;
    float b = color.blue;

    float M = std::max({r, g, b}); // Max component (Value)
    float m = std::min({r, g, b}); // Min component
    float C = M - m;               // Chroma

    Hsv hsv;
    hsv.v = M; // Value is the maximum component

    // Saturation calculation
    if (M == 0.0f) {
        hsv.s = 0.0f; // Black, gray, or white
    } else {
        hsv.s = C / M;
    }

    // Hue calculation
    if (C == 0.0f) {
        hsv.h = 0.0f; // Hue is undefined for achromatic colors, use 0
    } else {
        if (M == r) {
            hsv.h = (g - b) / C;
        } else if (M == g) {
            hsv.h = (b - r) / C + 2.0f;
        } else { // M == b
            hsv.h = (r - g) / C + 4.0f;
        }
        
        // Scale h to [0, 360) degrees
        hsv.h *= 60.0f;
        if (hsv.h < 0.0f) {
            hsv.h += 360.0f;
        }
    }

    return hsv;
}

/**
 * @brief Converts an Hsv color to Rgb color space.
 */
static Rgb hsv_to_rgb(const Hsv& hsv) {
    float h = hsv.h;
    float s = hsv.s;
    float v = hsv.v;

    // Based on standard HSV to RGB algorithm
    float C = v * s;              // Chroma
    float H_prime = std::fmod(h / 60.0f, 6.0f); // H' in [0, 6)
    float X = C * (1.0f - std::abs(std::fmod(H_prime, 2.0f) - 1.0f));
    float m = v - C;              // Mismatch value

    float r_temp, g_temp, b_temp;

    if (H_prime >= 0.0f && H_prime < 1.0f) {
        r_temp = C; g_temp = X; b_temp = 0.0f;
    } else if (H_prime >= 1.0f && H_prime < 2.0f) {
        r_temp = X; g_temp = C; b_temp = 0.0f;
    } else if (H_prime >= 2.0f && H_prime < 3.0f) {
        r_temp = 0.0f; g_temp = C; b_temp = X;
    } else if (H_prime >= 3.0f && H_prime < 4.0f) {
        r_temp = 0.0f; g_temp = X; b_temp = C;
    } else if (H_prime >= 4.0f && H_prime < 5.0f) {
        r_temp = X; g_temp = 0.0f; b_temp = C;
    } else { // H_prime >= 5.0f and H_prime < 6.0f
        r_temp = C; g_temp = 0.0f; b_temp = X;
    }

    // Add m to each component
    return Rgb(r_temp + m, g_temp + m, b_temp + m);
}

// --- HSV LERP with Shortest Hue Path ---

/**
 * @brief Performs linear interpolation (LERP) between two Rgb colors by 
 * converting them to Hsv space, blending, and converting back.
 * * The Hue blending uses the shortest path around the color wheel.
 * @param start The starting color (t=0.0f).
 * @param end The ending color (t=1.0f).
 * @param t The interpolation factor, clamped to the range [0.0f, 1.0f].
 * @return The blended Rgb color.
 */
Rgb blendHsv(const Rgb& left, const Rgb& right, float lerp) {
  // Clamp 't' to the range [0.0f, 1.0f]
  float t = std::max(std::min(lerp, 1.0f), 0.0f);

  // 1. Convert Rgb to Hsv
  Hsv hsv_start = rgb_to_hsv(left);
  Hsv hsv_end = rgb_to_hsv(right);
  Hsv hsv_result;

  // 2. Perform LERP on S and V components (simple linear interpolation)
  hsv_result.s = hsv_start.s + t * (hsv_end.s - hsv_start.s);
  hsv_result.v = hsv_start.v + t * (hsv_end.v - hsv_start.v);

  // 3. Perform LERP on Hue (H) component (circular shortest path with achromatic fix)
  
  // Use a small epsilon for floating-point comparison with zero saturation.
  const float S_EPSILON = 1e-6f;
  
  // If Start is achromatic (S1~0) and End is chromatic (S2>0), 
  // use End's hue for Start to maintain hue constancy as saturation increases.
  if (hsv_start.s < S_EPSILON && hsv_end.s > S_EPSILON) {
      hsv_start.h = hsv_end.h;
  }
  // If End is achromatic (S2~0) and Start is chromatic (S1>0), 
  // use Start's hue for End to maintain hue constancy as saturation decreases.
  else if (hsv_end.s < S_EPSILON && hsv_start.s > S_EPSILON) {
      hsv_end.h = hsv_start.h;
  }
  // If both are achromatic (S1~0 and S2~0), hue LERP is unnecessary 
  // but proceeds safely (hue_diff will be near zero).

  // Now perform the shortest path LERP
  float hue_diff = hsv_end.h - hsv_start.h;

  // Adjust difference to take the shortest path around the 360-degree circle
  if (hue_diff > 180.0f) {
      hue_diff -= 360.0f; // Go backward
  } else if (hue_diff < -180.0f) {
      hue_diff += 360.0f; // Go forward
  }

  // LERP the hue
  hsv_result.h = hsv_start.h + t * hue_diff;

  // Wrap the result back into [0, 360)
  hsv_result.h = std::fmod(hsv_result.h, 360.0f);
  if (hsv_result.h < 0.0f) {
      hsv_result.h += 360.0f;
  }

  // 4. Convert Hsv result back to Rgb
  return hsv_to_rgb(hsv_result);
}
