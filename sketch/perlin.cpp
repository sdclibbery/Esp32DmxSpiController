#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

// --- Constants ---
#define P_SIZE 512 // Must be a power of 2, used for the permutation array size (256 * 2)
#define P_MASK 255 // Mask for modulo 256 operation

// Global permutation array (stores random indices/hashes)
static int p[P_SIZE]; 
static int p_inited = 0;

// --- Core Helper Functions ---

/**
 * @brief Smoothstep function (6t^5 - 15t^4 + 10t^3) for interpolation.
 * @param t Interpolation factor (0.0 to 1.0).
 * @return Smoothed value.
 */
static inline float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/**
 * @brief Linear interpolation.
 * @param a Start value.
 * @param b End value.
 * @param t Interpolation factor (0.0 to 1.0).
 * @return Interpolated value.
 */
static inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

/**
 * @brief 1D Gradient function (Dot product of coordinate offset and random gradient vector).
 * * In 1D, the gradient is simply -1 or 1. We use the hash value to determine the sign.
 * @param hash A value from the permutation table (0-255).
 * @param x The offset from the cell corner (x - X).
 * @return The result of the dot product (either x or -x).
 */
static inline float grad(int hash, float x) {
    // Check the least significant bit of the hash
    return (hash & 1) == 0 ? -x : x;
}

// --- Initialization ---

/**
 * @brief Initializes the permutation array with a given seed.
 * * Uses Fisher-Yates shuffle algorithm and doubles the array to P_SIZE=512 
 * to simplify boundary checks in the main noise function.
 * @param seed The starting seed for the random number generator.
 */
static void init_perlin(unsigned int seed) {
    int i;
    // 1. Initialize the first 256 elements
    for (i = 0; i < 256; i++) {
        p[i] = i;
    }

    // 2. Seed and shuffle (Fisher-Yates)
    srand(seed);
    for (i = 0; i < 256; i++) {
        // Generate a random index j between i and 255
        int j = rand() % 256;
        
        // Swap p[i] and p[j]
        int temp = p[i];
        p[i] = p[j];
        p[j] = temp;
    }

    // 3. Duplicate the array to avoid modulo operations later (P_SIZE=512)
    for (i = 0; i < 256; i++) {
        p[i + 256] = p[i];
    }
}

// --- Single Octave Noise Function ---

/**
 * @brief Generates a single octave of Perlin noise for a given coordinate.
 * @param x The 1D coordinate.
 * @return The noise value, typically in the range [-1.0, 1.0].
 */
static float perlin_noise_single(float x) {
    // 1. Determine unit cell coordinates and fractional part (t)
    long X = (long)floorf(x);
    float xf = x - X; // Fractional part (t)

    // 2. Hash coordinate to get permutation index (using P_MASK == 255)
    int X_mod = (int)(X & P_MASK);

    // 3. Look up gradient hashes for the two endpoints (X and X+1)
    int g0 = p[X_mod];     // Hash for X
    int g1 = p[X_mod + 1]; // Hash for X+1

    // 4. Interpolate factor (u) using the smoothstep function
    float u = fade(xf);

    // 5. Calculate influence from the two end gradients
    float influence_0 = grad(g0, xf);
    float influence_1 = grad(g1, xf - 1.0f); // Offset coordinate for the right side

    // 6. Interpolate the influences
    return lerp(influence_0, influence_1, u);
}


// --- Fractal (Multi-Octave) Noise Function ---

/**
 * @brief Generates fractal Perlin noise using multiple octaves.
 * @param x The 1D coordinate input.
 * @param scale The frequency multiplier (how zoomed in the noise is).
 * @param octaves The number of noise layers to combine for detail.
 * @return The normalized multi-octave noise value, typically in the range [-1.0, 1.0].
 */
float perlin_noise_octaves(float x, float scale, int octaves) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float max_value = 0.0f; // Used for normalization

    if (!p_inited) { init_perlin(42); }

    if (scale <= FLT_MIN) scale = 1.0f; // Prevent division by zero/near zero

    for (int i = 0; i < octaves; i++) {
        // The coordinate is scaled by frequency and the base 'scale'
        total += perlin_noise_single(x * frequency / scale) * amplitude;
        
        // Accumulate maximum possible value for normalization
        max_value += amplitude;
        
        // Increase frequency for the next octave (more detail)
        frequency *= 2.0f;
        
        // Decrease amplitude for the next octave (less influence)
        amplitude *= 0.5f;
    }
    
    // Normalize the result to ensure it stays in the range [-1.0, 1.0]
    return total / max_value;
}
