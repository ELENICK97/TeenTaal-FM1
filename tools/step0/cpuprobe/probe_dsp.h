#pragma once
#include <stdint.h>
extern int16_t probe_sin[1025];
void osc_q15(int32_t *out, int n, uint32_t *ph, uint32_t inc, int16_t amp);
void osc_f32(int32_t *out, int n, float *s1, float *s2, float k, float amp);
uint32_t fpu_selftest(void);
