/* Step 0 probe: two test oscillators, to inspect the code clang emits for pi32v2.
 * Not part of the firmware. */
#include <stdint.h>

/* Q15 sine from a 1024-entry table, 32-bit phase, linear interpolation */
extern const int16_t sin_tab[1025];
void osc_q15(int32_t *out, int n, uint32_t *ph, uint32_t inc, int16_t amp)
{
    uint32_t p = *ph;
    for (int i = 0; i < n; i++) {
        uint32_t idx = p >> 22, frac = (p >> 7) & 0x7FFF;
        int32_t a = sin_tab[idx], b = sin_tab[idx + 1];
        int32_t s = a + (((b - a) * (int32_t)frac) >> 15);
        out[i] += (s * amp) >> 7;           /* Q15*Q15 >> 7 -> Q23 */
        p += inc;
    }
    *ph = p;
}

/* float sine via a 2-pole resonator (y[n] = k*y[n-1] - y[n-2]) */
void osc_f32(int32_t *out, int n, float *s1, float *s2, float k, float amp)
{
    float a = *s1, b = *s2;
    for (int i = 0; i < n; i++) {
        float y = k * a - b;
        b = a; a = y;
        out[i] += (int32_t)(y * amp);
    }
    *s1 = a; *s2 = b;
}

/* float biquad, the sort of thing the tanpura body filter needs */
void biquad_f32(float *x, int n, const float *c, float *z)
{
    float z1 = z[0], z2 = z[1];
    for (int i = 0; i < n; i++) {
        float in = x[i], y = c[0] * in + z1;
        z1 = c[1] * in - c[3] * y + z2;
        z2 = c[2] * in - c[4] * y;
        x[i] = y;
    }
    z[0] = z1; z[1] = z2;
}
