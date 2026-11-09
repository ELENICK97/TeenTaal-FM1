/* mixer.c — see mixer.h */
#include "mixer/mixer.h"
#include "dsp/ramp.h"

static float level_gain(int32_t pct)
{
    float v = (float)pct * 0.01f;
    return v * v;
}

void mixer_init(mixer_t *m)
{
    int k;
    for (k = 0; k < MIX_NSRC; k++) {
        m->level[k] = 100;
        m->gain[k] = 1.0f;
    }
    m->master = 0.0f;              /* MASTER ramps up from silence at start */
    m->clips = 0;
}

void mixer_set_param(mixer_t *m, int id, int32_t v)
{
    if (id >= MIX_P_LEVEL && id < MIX_P_LEVEL + MIX_NSRC)
        m->level[id - MIX_P_LEVEL] = v < 0 ? 0 : v > 100 ? 100 : v;
}

void mixer_render(mixer_t *m, int32_t *const src[MIX_NSRC], int32_t *lr, int n, uint32_t master_q12)
{
    float g0[MIX_NSRC], dg[MIX_NSRC], inv_n = 1.0f / (float)n;
    float mtarget = (float)(master_q12 > 4096u ? 4096u : master_q12) * (1.0f / 4096.0f);
    float m0 = m->master, mst1 = dsp_slew(m0, mtarget, n), dm = (mst1 - m0) * inv_n;
    int i, k;
    for (k = 0; k < MIX_NSRC; k++) {      /* every gain: 20 ms a full sweep at most (dsp/ramp.h) */
        float g1 = dsp_slew(m->gain[k], level_gain(m->level[k]), n);
        g0[k] = m->gain[k];
        dg[k] = (g1 - g0[k]) * inv_n;
        m->gain[k] = g1;
    }
    m->master = mst1;
    for (i = 0; i < n; i++) {
        float t = (float)(i + 1), acc = 0.0f, v;
        for (k = 0; k < MIX_NSRC; k++)
            acc += (float)src[k][i] * (g0[k] + dg[k] * t);
        v = acc * (m0 + dm * t);
        if (v > DSP_Q23_F) {
            v = DSP_Q23_F;
            m->clips++;
        } else if (v < -DSP_Q23_F) {
            v = -DSP_Q23_F;
            m->clips++;
        }
        lr[2 * i] = lr[2 * i + 1] = (int32_t)v;
    }
}
