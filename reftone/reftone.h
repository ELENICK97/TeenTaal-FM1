/* reftone.h — reference tone: a steady, clean sine on a swara, tuned like the tanpura.
 *
 * Parameters (reftone_set_param): the shared tuning (DSP_P_A4_DHZ, DSP_P_SA_CENTS) and
 *   REFTONE_P_ON      0 / 1        on fades in, off fades out (20 ms)
 *   REFTONE_P_SWARA   0..11        SW_SA .. SW_NI (dsp/tuning.h)
 *   REFTONE_P_OCTAVE  -1..+1       mandra, madhya, tar saptak
 *   REFTONE_P_VOLUME  0..100       percent, on a squared curve
 * Changes never click: the gain moves in linear ramps, the phase is continuous. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"
#include "dsp/tuning.h"

enum {
    REFTONE_P_ON = DSP_P_MODULE,
    REFTONE_P_SWARA,
    REFTONE_P_OCTAVE,
    REFTONE_P_VOLUME
};

#define REFTONE_LEVEL 0.5f             /* volume 100 % = -6 dBFS before the mixer; fades: dsp/ramp.h */

typedef struct {
    tuning_t tuning;
} reftone_config_t;

typedef struct {
    tuning_t tuning;
    int32_t on, swara, octave, volume;
    volatile uint32_t inc;             /* written by set_param (UI), read by render (audio interrupt) */
    uint32_t phase;
    float gain;                        /* the gain now, linear (0..REFTONE_LEVEL) */
} reftone_t;

void reftone_init(reftone_t *m, const reftone_config_t *cfg);   /* cfg NULL: default tuning */
void reftone_set_param(reftone_t *m, int param_id, int32_t value);
int32_t reftone_get_param(const reftone_t *m, int param_id);
void reftone_render(reftone_t *m, int32_t *out, int n_samples);   /* adds into out */
float reftone_hz(const reftone_t *m);                            /* the frequency it plays now */
