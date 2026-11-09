/* sine.h — sine from a 1024-point float table with linear interpolation, 32-bit phase.
 * Interpolation error < 5e-6 of full scale (harmonics below -106 dB). sine_init() once at start. */
#pragma once
#include <stdint.h>

#define SINE_BITS 10
#define SINE_N (1 << SINE_BITS)

extern float sine_table[SINE_N + 1];

void sine_init(void);

/* phase: a full turn is 2^32 */
static inline float sine_at(uint32_t phase)
{
    uint32_t i = phase >> (32 - SINE_BITS);
    float fr = (float)(int32_t)(phase & ((1u << (32 - SINE_BITS)) - 1u)) * (1.0f / (float)(1u << (32 - SINE_BITS)));
    float a = sine_table[i];
    return a + (sine_table[i + 1] - a) * fr;
}
