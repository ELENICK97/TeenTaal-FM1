/* taal.c — see taal.h */
#include "taal/taal.h"
#include "taal/taal_tables.h"

/* Position in a matra: exact integers. A matra lasts 60 / mpm s = 60 * DSP_FS_NUM / (DSP_FS_DEN * mpm) samples, so with
 * a matra = TAAL_ONE units, a sample advances DSP_FS_DEN * mpm units: no rounding, no drift */
#define TAAL_ONE (60u * (uint32_t)DSP_FS_NUM)              /* 1 440 000 000 */

static uint32_t rnd(taal_t *m)
{
    uint32_t x = m->rng;
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    return m->rng = x;
}

static void set_tempo(taal_t *m, int32_t mpm)
{
    m->mpm = mpm < TAAL_MPM_MIN ? TAAL_MPM_MIN : mpm > TAAL_MPM_MAX ? TAAL_MPM_MAX : mpm;
    m->inc = (uint32_t)DSP_FS_DEN * (uint32_t)m->mpm;
}

void taal_init(taal_t *m, const taal_config_t *cfg)
{
    m->on = 0, m->taal = 0, m->human = 30, m->pending = -1, m->start = 0;
    m->matra = 0, m->part = 0, m->frac = 0, m->cycles = 0;
    m->rng = cfg && cfg->seed ? cfg->seed : 0x7aa1u;
    set_tempo(m, 120);
}

void taal_set_param(taal_t *m, int id, int32_t v)
{
    switch (id) {
    case TAAL_P_ON:
        v = v != 0;
        if (v && !m->on)
            m->start = 1;                         /* (before on: the render never sees on without it) */
        m->on = v;
        break;
    case TAAL_P_TAAL:
        if (v < 0 || v >= TAAL_N)
            break;
        if (m->on)
            m->pending = v == m->taal ? -1 : v;   /* (back to the one playing: nothing to change) */
        else
            m->taal = v, m->pending = -1;
        break;
    case TAAL_P_MPM:
        set_tempo(m, v);
        break;
    case TAAL_P_HUMAN:
        m->human = v < 0 ? 0 : v > 100 ? 100 : v;
        break;
    default:
        break;
    }
}

int32_t taal_get_param(const taal_t *m, int id)
{
    switch (id) {
    case TAAL_P_ON: return m->on;
    case TAAL_P_TAAL: return m->pending >= 0 ? m->pending : m->taal;
    case TAAL_P_MPM: return m->mpm;
    case TAAL_P_HUMAN: return m->human;
    case TAAL_P_MATRA: return (int32_t)m->matra;
    case TAAL_P_CYCLES: return (int32_t)m->cycles;
    case TAAL_P_NOW: return m->taal;
    default: return 0;
    }
}

char taal_mark(int t, int matra)
{
    const taal_def_t *d;
    int k, at = 0;
    if (t < 0 || t >= TAAL_N)
        return 0;
    d = &TAAL_DEF[t];
    for (k = 0; k < d->nvibhag; at += d->vibhag[k++])
        if (at == matra)
            return d->mark[k];
    return 0;
}

/* the event of part `part` of the current matra, at sample t */
static void emit(taal_t *m, uint8_t bol, uint32_t t, taal_event_t *e)
{
    int dyn = m->matra == 0 ? 112 : taal_mark(m->taal, (int)m->matra) ? 100 : 92;
    int h = m->human, jit = 0;
    if (m->part > 0)
        dyn = dyn * 85 / 100;
    if (h) {
        /* +-10 of dynamic and 0..8 ms late (+-4 ms around a constant 4 ms) at 100 */
        dyn += (int)(rnd(m) % (uint32_t)(2 * h / 10 + 1)) - h / 10;
        jit = (int)(rnd(m) % (uint32_t)(h * 353 / 100 + 1));     /* 353 samples = 8 ms */
    }
    e->stroke = bol;
    e->dynamic = (uint8_t)(dyn < 1 ? 1 : dyn > 127 ? 127 : dyn);
    e->reserved = 0;
    e->offset = (uint16_t)(t + (uint32_t)jit);
}

int taal_advance(taal_t *m, int n_samples, taal_event_t *ev, int max)
{
    const uint32_t n = (uint32_t)n_samples;
    uint32_t t = 0;
    int cnt = 0;
    if (!m->on || n_samples <= 0)
        return 0;
    if (m->start) {                               /* from the sam (of the taal chosen last) */
        if (m->pending >= 0)
            m->taal = m->pending, m->pending = -1;
        m->matra = 0, m->part = 0, m->frac = 0, m->cycles = 0;
        m->start = 0;
    }
    for (;;) {
        const taal_def_t *d = &TAAL_DEF[m->taal];
        const taal_matra_t *mt = &TAAL_MATRA[d->matra0 + m->matra];
        if (m->part < mt->n) {
            /* the part's place in the matra: part / n of it */
            uint32_t target = m->part * (TAAL_ONE / mt->n), need, s;
            if (m->frac < target) {
                need = target - m->frac;
                s = (need - 1u) / m->inc + 1u;            /* samples until the position reaches it */
                if (t + s >= n) {
                    m->frac += (n - t) * m->inc;
                    return cnt;
                }
                t += s;
                m->frac += s * m->inc;
            } else if (t >= n) {
                return cnt;                               /* (exactly on the next block's first sample) */
            }
            if (TAAL_STEP[mt->first + m->part] != TAAL_REST && cnt < max)
                emit(m, TAAL_STEP[mt->first + m->part], t, &ev[cnt++]);
            m->part++;
        } else {
            /* to the end of the matra */
            uint32_t s = (TAAL_ONE - m->frac - 1u) / m->inc + 1u;   /* ceil((ONE - frac) / inc) */
            if (t + s > n) {
                m->frac += (n - t) * m->inc;
                return cnt;
            }
            t += s;
            m->frac = m->frac + s * m->inc - TAAL_ONE;    /* the position in the next matra */
            m->part = 0;
            if (++m->matra >= d->matras) {
                m->matra = 0, m->cycles++;
                if (m->pending >= 0)                      /* a new taal: at the end of the cycle, from its sam */
                    m->taal = m->pending, m->pending = -1;
            }
        }
    }
}

int taal_count(void) { return TAAL_N; }

const char *taal_name(int t) { return t >= 0 && t < TAAL_N ? TAAL_DEF[t].name : "?"; }

int taal_matras(int t) { return t >= 0 && t < TAAL_N ? TAAL_DEF[t].matras : 0; }

int taal_tempo(int t) { return t >= 0 && t < TAAL_N ? TAAL_DEF[t].tempo : 120; }

int taal_matra_parts(int t, int matra)
{
    if (t < 0 || t >= TAAL_N || matra < 0 || matra >= TAAL_DEF[t].matras)
        return 0;
    return TAAL_MATRA[TAAL_DEF[t].matra0 + matra].n;
}

int taal_part_bol(int t, int matra, int part)
{
    const taal_matra_t *mt;
    if (part < 0 || part >= taal_matra_parts(t, matra))
        return -1;
    mt = &TAAL_MATRA[TAAL_DEF[t].matra0 + matra];
    return TAAL_STEP[mt->first + part] == TAAL_REST ? -1 : TAAL_STEP[mt->first + part];
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }

static int same(const char *a, const char *b)
{
    while (*a && lower(*a) == lower(*b))
        a++, b++;
    return !*a && !*b;
}

int taal_find(const char *name)
{
    int t;
    for (t = 0; t < TAAL_N; t++)
        if (same(TAAL_DEF[t].name, name))
            return t;
    return -1;
}

int taal_selftest(const char *(*bol_name)(int))
{
    int k, bad = 0;
    for (k = 0; k < (int)(sizeof TAAL_BOLCHECK / sizeof TAAL_BOLCHECK[0]); k++)
        bad += !same(bol_name(TAAL_BOLCHECK[k].id), TAAL_BOLCHECK[k].name);
    return bad;
}
