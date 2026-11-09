/* ramp.h — gain changes without clicks: a gain moves toward its target at most a full sweep (0 to 1)
 * per DSP_FADE_S, as a linear ramp across each block. Used by every module and the mixer. */
#pragma once
#include "dsp/dsp.h"

#define DSP_FADE_S 0.020f              /* 0 -> 1 takes 20 ms (882 samples) */

/* the gain at the end of a block of n samples, moving from cur toward target */
static inline float dsp_slew(float cur, float target, int n)
{
    float maxstep = (float)n * (1.0f / (DSP_FADE_S * DSP_FS));
    if (target > cur)
        return target - cur > maxstep ? cur + maxstep : target;
    return cur - target > maxstep ? cur - maxstep : target;
}
