/* mixer.h — sums the modules' mono Q23 buffers with their levels and MASTER, saturates at full scale,
 * and writes the platform's stereo frames (the same signal on L and R: the FM-1's sound is mono).
 *
 * Parameters (mixer_set_param):
 *   MIX_P_LEVEL + MIX_TANPURA / MIX_TABLA / MIX_REFTONE   0..100 percent, on a squared curve (default 100)
 * Levels and MASTER move in linear ramps across each block: no zipper noise. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"

enum { MIX_TANPURA, MIX_TABLA, MIX_REFTONE, MIX_NSRC };
enum { MIX_P_LEVEL = DSP_P_MODULE };

typedef struct {
    int32_t level[MIX_NSRC];
    float gain[MIX_NSRC];          /* the gains now, after the last block */
    float master;                  /* MASTER now, 0..1 */
    uint32_t clips;                /* samples clipped at full scale since init */
} mixer_t;

void mixer_init(mixer_t *m);
void mixer_set_param(mixer_t *m, int param_id, int32_t value);
/* src[MIX_NSRC] mono Q23 buffers of n samples; lr: n stereo frames, overwritten; master_q12: 0..4096 */
void mixer_render(mixer_t *m, int32_t *const src[MIX_NSRC], int32_t *lr, int n, uint32_t master_q12);
