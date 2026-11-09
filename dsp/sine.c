/* sine.c — see sine.h */
#include "dsp/sine.h"
#include "fastmath.h"

float sine_table[SINE_N + 1];

void sine_init(void)
{
    int i;
    for (i = 0; i <= SINE_N; i++)
        sine_table[i] = fm_sinf(FM_TWO_PI * (float)i / (float)SINE_N);
}
