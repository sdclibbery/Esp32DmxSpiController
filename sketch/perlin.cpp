#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// --- Perlin Noise Implementation ---

// The permutation table. This is a lookup table of random values.
// We duplicate it to 512 to avoid having to use the modulo operator
// (which is slow) when indexing.
static int p[512];
static bool inited = false;

/**
 * @brief Initializes the permutation table with random values.
 * This should be called once at the beginning of your program.
 */
static void perlin_init() {
    // Fill the first 256 entries with 0-255
    for (int i = 0; i < 256; i++) {
        p[i] = i;
    }

    // Seed the random number generator
    srand(25432); // Fixed seed for reproducibility

    // Shuffle the array using Fisher-Yates shuffle
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = p[i];
        p[i] = p[j];
        p[j] = temp;
    }

    // Duplicate the table to the second 256 entries
    for (int i = 0; i < 256; i++) {
        p[i + 256] = p[i];
    }
}

/**
 * @brief The "fade" function. This is a quintic polynomial 
 *        (6t^5 - 15t^4 + 10t^3) which has 0 first and second
 *        derivatives at t=0 and t=1, creating smoother transitions.
 */
static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/**
 * @brief Linear interpolation.
 */
static float lerp_perlin(float t, float a, float b) {
    return a + t * (b - a);
}

/**
 * @brief Calculates the dot product of a gradient vector and the
 *        distance vector from a grid point to the sample point.
 * @param hash The hash value from the permutation table.
 * @param x Fractional x.
 * @param y Fractional y.
 */
static float grad(int hash, float x, float y) {
    // Get the lower 3 bits of the hash
    int h = hash & 7; 
    
    // Select one of 8 gradient vectors
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;

    // (1,1), (-1,1), (1,-1), (-1,-1), (1,0), (-1,0), (0,1), (0,-1)
    // The last bit (h&1) determines the sign of u
    // The second to last bit (h&2) determines the sign of v
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

/**
 * @brief Calculates a single octave of 2D Perlin noise.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return The noise value, in the range [-1.0, 1.0].
 */
static float perlin(float x, float y) {
    // Find the integer grid cell coordinates (X, Y)
    // We use floorf() for floats
    int xi = (int)floorf(x);
    int yi = (int)floorf(y);

    // Get the fractional part of x and y (xf, yf)
    float xf = x - (float)xi;
    float yf = y - (float)yi;

    // We must bitwise-AND with 255 to keep indices within 0-255
    int X = xi & 255;
    int Y = yi & 255;

    // Calculate the fade-curved values for interpolation
    float u = fade(xf);
    float v = fade(yf);

    // Get the hash values for the 4 corners of the grid cell
    // p[p[X] + Y]     -> top-left
    // p[p[X+1] + Y]   -> top-right
    // p[p[X] + Y+1]   -> bottom-left
    // p[p[X+1] + Y+1] -> bottom-right
    // We add 1 to Y/X for the bottom/right corners
    int aa = p[p[X] + Y];
    int ba = p[p[X + 1] + Y];
    int ab = p[p[X] + Y + 1];
    int bb = p[p[X + 1] + Y + 1];

    // Calculate the dot products (influences)
    // xf and yf are the distances from the top-left corner
    float grad_aa = grad(aa, xf, yf);
    float grad_ba = grad(ba, xf - 1.0f, yf);       // xf-1 for right edge
    float grad_ab = grad(ab, xf, yf - 1.0f);       // yf-1 for bottom edge
    float grad_bb = grad(bb, xf - 1.0f, yf - 1.0f); // xf-1, yf-1 for bottom-right

    // Interpolate along x
    float lerp_top = lerp_perlin(u, grad_aa, grad_ba);
    float lerp_bottom = lerp_perlin(u, grad_ab, grad_bb);

    // Interpolate along y
    float noise = lerp_perlin(v, lerp_top, lerp_bottom);
    
    // The result is *approximately* in [-1, 1], but we can
    // scale it slightly to ensure it's fully in that range.
    // 0.707 is sqrt(2)/2, which is the max possible value
    // for 2D. Let's use a slightly more generous factor.
    return noise * 1.4f;
}

/**
 * @brief Generates 2D Perlin noise by summing multiple octaves.
 *
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param octaves The number of octaves to sum (e.g., 4).
 * @param persistence The persistence value (e.g., 0.5). This is the
 *                    factor by which the amplitude of each successive
 *                    octave is multiplied.
 * @param lacunarity The lacunarity value (e.g., 2.0). This is the
 *                   factor by which the frequency of each successive
 *                   octave is multiplied.
 * @return The resulting multi-octave noise value.
 */
float perlin_octaves(float x, float y, int octaves, float persistence, float lacunarity) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float max_value = 0.0f; // Used for normalization

    if (!inited) {
        perlin_init();
        inited = true;
    }

    for (int i = 0; i < octaves; i++) {
        // Get the noise value for this octave
        total += perlin(x * frequency, y * frequency) * amplitude;

        // Add to the maximum possible value
        max_value += amplitude;

        // Modify amplitude and frequency for the next octave
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // Normalize the result to be in the range [-1.0, 1.0]
    return total / max_value;
}
