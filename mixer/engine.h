/* engine.h — every audio module and the mixer together: the one audio path the firmware (app.c) and the
 * host tools (tools/render, tools/test) both run, so a host render is what the FM-1 plays.
 *
 * engine_render takes the platform's block (any multiple of DSP_BLOCK; 256 on the FM-1) and runs the
 * modules in sub-blocks of DSP_BLOCK. engine_set_param is the only way in from the UI. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"
#include "tanpura/tanpura.h"
#include "tabla/tabla.h"
#include "taal/taal.h"
#include "reftone/reftone.h"
#include "mixer/mixer.h"

#define ENGINE_MAXN 256                /* frames in one engine_render at most (the platform's block) */

/* where a parameter goes */
enum { ENG_ALL, ENG_TANPURA, ENG_TABLA, ENG_REFTONE, ENG_MIXER, ENG_TAAL };

typedef struct {
    tanpura_t tanpura;
    tabla_t tabla;
    taal_t taal;                       /* its events go to the tabla, sub-block by sub-block */
    reftone_t reftone;
    mixer_t mixer;
    volatile int32_t muted;        /* engine_panic: silence at once */
    int32_t buf[MIX_NSRC][DSP_BLOCK];
    int32_t tan[ENGINE_MAXN];          /* engine_render's own tanpura block */
} engine_t;

void engine_init(engine_t *e);
/* ENG_ALL sends the shared tuning (DSP_P_A4_DHZ, DSP_P_SA_CENTS) to every module */
void engine_set_param(engine_t *e, int target, int param_id, int32_t value);
int32_t engine_get_param(const engine_t *e, int target, int param_id);
/* lr: n stereo frames (Q23), overwritten; n a multiple of DSP_BLOCK, <= ENGINE_MAXN; master_q12: MASTER's gain
 * 0..4096. Everything in one place: the host tools, and the firmware when it runs on one core */
void engine_render(engine_t *e, int32_t *lr, uint32_t n, uint32_t master_q12);

/* The same, split for two cores (step 3). The first core: engine_tanpura_take while the second core is idle, then
 * engine_render_with with a tanpura buffer the second core rendered earlier. The second core's job:
 * engine_tanpura_render (mono Q23, n frames, overwritten). engine_render is exactly these three in a row. */
void engine_tanpura_take(engine_t *e);
void engine_tanpura_render(engine_t *e, int32_t *tan, uint32_t n);
void engine_render_with(engine_t *e, int32_t *lr, uint32_t n, uint32_t master_q12, const int32_t *tan);
void engine_panic(engine_t *e);    /* silence now (before a flash write): until engine_init */
