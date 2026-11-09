/* reftone.c — see reftone.h */
#include "reftone/reftone.h"
#include "dsp/sine.h"
#include "dsp/ramp.h"

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

float reftone_hz(const reftone_t *m)
{
    return tuning_hz(&m->tuning, tuning_swara_cents(m->swara) + 1200.0f * (float)m->octave);
}

static void retune(reftone_t *m)
{
    m->inc = tuning_phase_inc(reftone_hz(m));
}

void reftone_init(reftone_t *m, const reftone_config_t *cfg)
{
    if (cfg)
        m->tuning = cfg->tuning;
    else
        tuning_default(&m->tuning);
    m->on = 0;
    m->swara = SW_SA;
    m->octave = 0;
    m->volume = 70;
    m->phase = 0;
    m->gain = 0.0f;
    retune(m);
}

void reftone_set_param(reftone_t *m, int id, int32_t v)
{
    if (tuning_set_param(&m->tuning, id, v)) {
        retune(m);
        return;
    }
    switch (id) {
    case REFTONE_P_ON:     m->on = v != 0; break;
    case REFTONE_P_SWARA:  m->swara = clampi(v, 0, SW_N - 1); retune(m); break;
    case REFTONE_P_OCTAVE: m->octave = clampi(v, -1, 1); retune(m); break;
    case REFTONE_P_VOLUME: m->volume = clampi(v, 0, 100); break;
    default: break;
    }
}

int32_t reftone_get_param(const reftone_t *m, int id)
{
    switch (id) {
    case DSP_P_A4_DHZ:     return m->tuning.a4_dhz;
    case DSP_P_SA_CENTS:   return m->tuning.sa_cents;
    case REFTONE_P_ON:     return m->on;
    case REFTONE_P_SWARA:  return m->swara;
    case REFTONE_P_OCTAVE: return m->octave;
    case REFTONE_P_VOLUME: return m->volume;
    default:               return 0;
    }
}

void reftone_render(reftone_t *m, int32_t *out, int n)
{
    float v = (float)m->volume * 0.01f;
    float target = m->on ? v * v * REFTONE_LEVEL : 0.0f;
    float g0 = m->gain, g1 = dsp_slew(g0, target, n), dg;   /* linear across the block, 20 ms a full sweep */
    uint32_t ph = m->phase, inc = m->inc;
    int i;
    m->gain = g1;
    if (g0 == 0.0f && g1 == 0.0f) {       /* silent: keep the phase moving, add nothing */
        m->phase = ph + inc * (uint32_t)n;
        return;
    }
    dg = (g1 - g0) / (float)n;            /* (one division a block) */
    for (i = 0; i < n; i++) {
        float g = g0 + dg * (float)(i + 1);
        out[i] += (int32_t)(sine_at(ph) * g * DSP_Q23_F);
        ph += inc;
    }
    m->phase = ph;
}
