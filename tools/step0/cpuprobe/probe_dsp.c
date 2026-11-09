/* Step 0 CPU probe: the oscillators under test, compiled at -O2 as an app's DSP would be. */
#include <stdint.h>
#include "probe_dsp.h"

int16_t probe_sin[1025];

void osc_q15(int32_t *out, int n, uint32_t *ph, uint32_t inc, int16_t amp)
{
    uint32_t p = *ph;
    for (int i = 0; i < n; i++) {
        uint32_t idx = p >> 22, frac = (p >> 7) & 0x7FFF;
        int32_t a = probe_sin[idx], b = probe_sin[idx + 1];
        int32_t s = a + (((b - a) * (int32_t)frac) >> 15);
        out[i] += (s * amp) >> 7;
        p += inc;
    }
    *ph = p;
}

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

/* FPU self-test: a fixed float computation (mul, add, sub, int<->float), folded into a 32-bit
 * hash. Only operations that are exact under IEEE single precision with round-to-nearest, so the
 * FM-1 and the host must give the same number. */
uint32_t fpu_selftest(void)
{
    float a = 0.0f, b = -0.0123f, k = 1.9876f, g = 0.25f;
    uint32_t h = 2166136261u;
    for (int i = 0; i < 2000; i++) {
        float y = k * a - b + g * (float)(i & 7) * 0.001f;
        b = a; a = y;
        if (a > 1000.0f || a < -1000.0f) a *= 0.001f;
        h = (h ^ (uint32_t)(int32_t)(a * 65536.0f)) * 16777619u;
    }
    return h;
}
