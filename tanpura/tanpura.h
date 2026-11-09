/* tanpura.h — the tanpura: four strings (DAFx-16 model, tanpura/tstring.c), the plucking cycle, the body.
 *
 * Strings: 0 the first string (a swara in the mandra octave: Pa by default), 1 and 2 Sa, 3 kharaj (Sa an octave
 * below, bronze). The instrument follows Sa: the large tanpura of step 2a (1.1 m, steel 0.16 mm, kharaj bronze
 * 0.25 mm) at Sa A2 and below, the smaller one (0.85 m, 0.13 / 0.18 mm) at A3, interpolated (0.7 m at G4).
 * Finger pluck at 35 % of the string, 2 cm wide.
 * Cycle: first string, Sa, Sa, kharaj at 0, 30, 45 and 60 % of the cycle, then a pause.
 * Rates (step 2b): the strings and the body run at half the FM-1's rate (22058.8 Hz; above 11 kHz the reference
 * holds -44 dB of its energy), then a half-band filter brings the output up to 44117.6 Hz.
 *
 * Parameters (tanpura_set_param), besides the shared tuning (DSP_P_A4_DHZ, DSP_P_SA_CENTS):
 *   TANPURA_P_ON        0 / 1      1 starts the cycle; 0 stops plucking and fades out in 0.3 s
 *   TANPURA_P_FIRST     0..11      the first string's swara (SW_PA default; SW_MA, SW_NI, SW_NI_K, any)
 *   TANPURA_P_JAWARI    0..100     the thread position: 0 = 4 mm (short jvari) .. 100 = 8 mm (long); 50 = 6 mm
 *   TANPURA_P_CYCLE     25..80     the cycle in tenths of a second (default 40: 4 s)
 *   TANPURA_P_HUMAN     0..100     irregularity: 100 = plucks +-50 ms and +-30 % (default 50)
 *   TANPURA_P_FINE + j  -50..50    string j's fine tuning in cents (j = 0..3)
 *   TANPURA_P_VOL + j   0..100     string j's volume (taken at its next pluck: the model is exactly linear in it)
 *   TANPURA_P_REGISTER  0 / 1      1 (default): the tanpura an octave under the player's Sa ("large"), when that stays
 *                                  in its range (Sa F#3 and up); 0: at the Sa itself
 *   (read only) TANPURA_P_SA_NOW   the tanpura's own Sa in cents above C4 (the register applied)
 * Threads (step 3): tanpura_set_param in the main loop; tanpura_take where no render runs (the first core between two
 * of the second core's jobs; the engine before each render on the host); tanpura_render anywhere else. Only
 * tanpura_take changes the strings' state from outside the render.
 * The first string's swara, the fine tuning, the Sa and the jawari retune the strings: the new coefficients are
 * computed in tanpura_set_param (the main loop) and taken by the render at its next sub-block. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"
#include "dsp/tuning.h"
#include "tanpura/tstring.h"

#define TANPURA_NSTR 4
#define TANPURA_OS 2                             /* the strings run at DSP_FS / TANPURA_OS */
#define TANPURA_FS (DSP_FS / (float)TANPURA_OS)

enum {
    TANPURA_P_ON = DSP_P_MODULE,
    TANPURA_P_FIRST,
    TANPURA_P_JAWARI,
    TANPURA_P_CYCLE,
    TANPURA_P_HUMAN,
    TANPURA_P_FINE,                              /* + string 0..3 */
    TANPURA_P_VOL = TANPURA_P_FINE + TANPURA_NSTR,   /* + string 0..3 */
    TANPURA_P_REGISTER = TANPURA_P_VOL + TANPURA_NSTR,
    TANPURA_P_SA_NOW,
    TANPURA_P_END
};

typedef struct {
    tuning_t tuning;
    float fmax[TANPURA_NSTR];                    /* each string's mode truncation, Hz (step 2b's cost knob) */
    int maxm[TANPURA_NSTR];                      /* and at most this many modes per string */
    float s0, rc;                                /* damping (step 2a final: 0.4 s^-1, 0.8 kg/s) */
    uint32_t seed;                               /* the irregularity's random sequence */
    int eq;                                      /* the brightness correction (UPS_EQ): 1 on; off by default (step 2b) */
} tanpura_config_t;

typedef struct {
    tanpura_config_t cfg;
    tuning_t tuning;
    int32_t on, first, jawari, cycle, human, reg, fine[TANPURA_NSTR], vol[TANPURA_NSTR];
    volatile int32_t start_req;                  /* switched on: tanpura_take starts the cycle */
    tstring_t str[TANPURA_NSTR];
    tstring_coefs_t stage[TANPURA_NSTR];         /* retunes prepared by set_param, taken by render */
    volatile int32_t stage_ready[TANPURA_NSTR];
    int32_t due[TANPURA_NSTR], nominal[TANPURA_NSTR];   /* samples to the next pluck: actual, on the grid */
    uint32_t rng;
    float gain;                                  /* output fade (on/off, large retunes) */
    float f1_now[TANPURA_NSTR];                  /* each string's pitch as last staged */
    volatile int32_t restart;                    /* a retune beyond a semitone: fade, silence, start the cycle again */
    float comp, comp_target;                     /* loudness compensation for Sa and jawari (now, wanted) */
    int32_t active;                              /* the strings are being computed */
    float bw1[32], bw2[32], xd1, xn1;            /* body filter state (BODY_NSEC <= 32) */
    float fd[DSP_BLOCK / TANPURA_OS], fn[DSP_BLOCK / TANPURA_OS];
    float up[DSP_BLOCK / TANPURA_OS + 64];       /* the upsampler's input: its history, then this block's */
    float lp[2][2];                              /* the 8.5 kHz low-pass's state (UPS_LP_N = 2 biquads) */
    float eqs[2];                                /* the brightness correction's state */
} tanpura_t;

void tanpura_default_config(tanpura_config_t *cfg);
void tanpura_init(tanpura_t *m, const tanpura_config_t *cfg);   /* cfg NULL: the defaults */
void tanpura_set_param(tanpura_t *m, int param_id, int32_t value);
int32_t tanpura_get_param(const tanpura_t *m, int param_id);
void tanpura_take(tanpura_t *m);                /* pending starts and retunes in: where no render runs (see above) */
void tanpura_render(tanpura_t *m, int32_t *out, int n_samples);   /* adds into out; n even, <= DSP_BLOCK */
