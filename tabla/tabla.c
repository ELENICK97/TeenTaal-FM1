/* tabla.c — see tabla.h */
#include "tabla/tabla.h"
#include "tabla/tabla_samples.h"
#include "tabla/tabla_bols.h"
#include "fastmath.h"

static const int8_t ADPCM_INDEX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const int16_t ADPCM_STEP[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
    796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
    20350, 22385, 24623, 27086, 29794, 32767};

/* one IMA ADPCM sample: the code at sample i of src (exactly tools/tabla_data/adpcm.py) */
static inline int32_t adpcm_next(const uint8_t *src, uint32_t i, int32_t *pred, int32_t *idx)
{
    uint32_t b = src[i >> 1];
    uint32_t code = (i & 1u) ? b >> 4 : b & 15u;
    int32_t step = ADPCM_STEP[*idx], diff = step >> 3, p = *pred, k = *idx;
    if (code & 4u) diff += step;
    if (code & 2u) diff += step >> 1;
    if (code & 1u) diff += step >> 2;
    p = (code & 8u) ? p - diff : p + diff;
    p = p < -32768 ? -32768 : p > 32767 ? 32767 : p;
    k += ADPCM_INDEX[code];
    k = k < 0 ? 0 : k > 88 ? 88 : k;
    *pred = p, *idx = k;
    return p;
}

static uint32_t rnd(tabla_t *m)
{
    uint32_t x = m->rng;
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    return m->rng = x;
}

static float rnd01(tabla_t *m) { return (float)(rnd(m) >> 8) * (1.0f / 16777216.0f); }

static void retune(tabla_t *m)
{
    float f = tuning_hz(&m->tuning, 0.0f);
    while (f >= 2.0f * TABLA_DAYAN_LO_HZ)
        f *= 0.5f;
    while (f < TABLA_DAYAN_LO_HZ)
        f *= 2.0f;
    m->dayan_hz = f;
    m->dayan_step = f * (1.0f / TABLA_DAYAN_REC_HZ);
}

void tabla_init(tabla_t *m, const tabla_config_t *cfg)
{
    int k;
    if (cfg)
        m->cfg = *cfg;
    else {
        tuning_default(&m->cfg.tuning);
        m->cfg.seed = 0x7ab1a5u;
    }
    m->tuning = m->cfg.tuning;
    m->rng = m->cfg.seed ? m->cfg.seed : 1u;
    m->age = 0, m->nq = 0, m->dropped = 0;
    for (k = 0; k < TABLA_NVOICE; k++)
        m->v[k].active = 0;
    for (k = 0; k < TABLA_NSTROKE; k++)
        m->next_take[k] = (uint8_t)(rnd(m) % TABLA_STROKE[k].ntakes);
    retune(m);
}

void tabla_set_param(tabla_t *m, int id, int32_t v)
{
    int k;
    if (tuning_set_param(&m->tuning, id, v)) {
        retune(m);
        return;
    }
    switch (id) {
    case TABLA_P_HIT:
        tabla_trigger(m, v & 255, (v >> 8) & 255, 0);
        break;
    case TABLA_P_STOP:
        m->nq = 0;
        for (k = 0; k < TABLA_NVOICE; k++)
            if (m->v[k].active && m->v[k].fade_step == 0.0f)
                m->v[k].fade_step = 1.0f / (TABLA_CHOKE_S * DSP_FS);
        break;
    default:
        break;
    }
}

int32_t tabla_get_param(const tabla_t *m, int id)
{
    if (id == DSP_P_A4_DHZ)
        return m->tuning.a4_dhz;
    if (id == DSP_P_SA_CENTS)
        return m->tuning.sa_cents;
    if (id == TABLA_P_DAYAN_DHZ)
        return (int32_t)(m->dayan_hz * 10.0f + 0.5f);
    return 0;
}

int tabla_trigger(tabla_t *m, int bol, int dyn, int32_t at)
{
    int k;
    if (bol < 0 || bol >= TABLA_NBOL || at < 0 || m->nq >= TABLA_QUEUE) {
        m->dropped++;
        return -1;
    }
    for (k = m->nq; k > 0 && m->q[k - 1].at > at; k--)      /* keep the queue in time order (stable) */
        m->q[k] = m->q[k - 1];
    m->q[k].bol = (uint8_t)bol;
    m->q[k].dyn = (uint8_t)(dyn < 0 ? 0 : dyn > 127 ? 127 : dyn);
    m->q[k].at = at;
    m->nq++;
    return 0;
}

static void start_stroke(tabla_t *m, int s, float level, int dyn)
{
    const tabla_stroke_t *st = &TABLA_STROKE[s];
    const tabla_take_t *tk = &TABLA_TAKE[st->first + m->next_take[s]];
    tabla_voice_t *v = 0;
    float d = (float)dyn * (1.0f / 127.0f), fc;
    int k;
    m->next_take[s] = (uint8_t)((m->next_take[s] + 1u) % st->ntakes);
    for (k = 0; k < TABLA_NVOICE; k++) {          /* the drum's ringing stroke: choked (the hand on the head) */
        tabla_voice_t *o = &m->v[k];
        if (o->active && o->drum == st->drum && o->fade_step == 0.0f)
            o->fade_step = o->fade * (1.0f / (TABLA_CHOKE_S * DSP_FS));
    }
    for (k = 0; k < TABLA_NVOICE && !v; k++)      /* a free voice, else the oldest */
        if (!m->v[k].active)
            v = &m->v[k];
    if (!v) {
        v = &m->v[0];
        for (k = 1; k < TABLA_NVOICE; k++)
            if (m->v[k].age < v->age)
                v = &m->v[k];
    }
    v->src = TABLA_ADPCM + tk->off;
    v->n = tk->n;
    v->pred = 0, v->idx = tk->idx;
    v->h[0] = 0.0f;
    for (k = 1; k < 4; k++)                       /* h = {0, x0, x1, x2}: the first output is x0 */
        v->h[k] = (uint32_t)(k - 1) < v->n ?(float)adpcm_next(v->src, (uint32_t)(k - 1), &v->pred, &v->idx) : 0.0f;
    v->i = 3;
    v->frac = 0.0f;
    v->rate = st->drum == TABLA_DAYAN ? 1.0f : 0.5f;
    v->gain = st->gain * level * d * (0.35f + 0.65f * d) * (0.95f + 0.05f * rnd01(m))
              * TABLA_LEVEL * (DSP_Q23_F / 32768.0f);
    fc = 2000.0f * fm_exp2f(3.5f * d);            /* 2 kHz at 0, 6.7 kHz at 64, 22.6 kHz at 127 */
    v->lpa = fm_expf(-6.2831853f * fc * (1.0f / DSP_FS));
    if (v->lpa < 0.05f)
        v->lpa = 0.0f;
    v->lp = 0.0f;
    v->fade = 1.0f, v->fade_step = 0.0f;
    v->age = ++m->age;
    v->drum = st->drum;
    v->active = 1;
}

static void play_bol(tabla_t *m, int bol, int dyn)
{
    const tabla_bol_t *b = &TABLA_BOL[bol];
    int k;
    for (k = 0; k < b->n; k++)
        start_stroke(m, b->stroke[k], (float)b->level[k] * (1.0f / 255.0f), dyn);
}

static void render_voice(tabla_t *m, tabla_voice_t *v, int32_t *out, int n)
{
    const float step = v->drum == TABLA_DAYAN ? m->dayan_step : v->rate;
    float h0 = v->h[0], h1 = v->h[1], h2 = v->h[2], h3 = v->h[3], fr = v->frac, lp = v->lp, fade = v->fade;
    const float a = v->lpa, g = v->gain, fs = v->fade_step;
    int32_t pred = v->pred, idx = v->idx;
    uint32_t i = v->i;
    int j;
    for (j = 0; j < n; j++) {
        /* 4-point Hermite between h1 and h2 */
        float c1 = 0.5f * (h2 - h0), c2 = h0 - 2.5f * h1 + 2.0f * h2 - 0.5f * h3, c3 = 0.5f * (h3 - h0) + 1.5f * (h1 - h2);
        float y = ((c3 * fr + c2) * fr + c1) * fr + h1;
        lp = y + a * (lp - y);
        out[j] += (int32_t)(lp * g * fade);
        fade -= fs;
        if (fade <= 0.0f) {
            v->active = 0;
            return;
        }
        fr += step;
        while (fr >= 1.0f) {
            fr -= 1.0f;
            h0 = h1, h1 = h2, h2 = h3;
            h3 = i < v->n ? (float)adpcm_next(v->src, i, &pred, &idx) : 0.0f;
            i++;
        }
        if (i > v->n + 3u) {                      /* the take is over (its last sample has passed h1) */
            v->active = 0;
            return;
        }
    }
    v->h[0] = h0, v->h[1] = h1, v->h[2] = h2, v->h[3] = h3;
    v->frac = fr, v->lp = fm_flush(lp), v->fade = fade;
    v->pred = pred, v->idx = idx, v->i = i;
}

static void render_span(tabla_t *m, int32_t *out, int n)
{
    int k;
    if (n <= 0)
        return;
    for (k = 0; k < TABLA_NVOICE; k++)
        if (m->v[k].active)
            render_voice(m, &m->v[k], out, n);
}

void tabla_render(tabla_t *m, int32_t *out, int n)
{
    int done = 0, k, used = 0;
    while (used < m->nq && m->q[used].at < n) {
        int at = (int)m->q[used].at;
        render_span(m, out + done, at - done);
        done = at;
        play_bol(m, m->q[used].bol, m->q[used].dyn);
        used++;
    }
    render_span(m, out + done, n - done);
    for (k = used; k < m->nq; k++) {             /* the later hits: carried to the next render */
        m->q[k - used] = m->q[k];
        m->q[k - used].at -= n;
    }
    m->nq -= used;
}

int tabla_bol_count(void) { return TABLA_NBOL; }

const char *tabla_bol_name(int bol) { return bol >= 0 && bol < TABLA_NBOL ? TABLA_BOL[bol].name : "?"; }

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }

int tabla_bol_find(const char *name)
{
    int b;
    for (b = 0; b < TABLA_NBOL; b++) {
        const char *p = TABLA_BOL[b].name, *q = name;
        while (*p && lower(*p) == lower(*q))
            p++, q++;
        if (!*p && !*q)
            return b;
    }
    return -1;
}

int tabla_bol_strokes(int bol, int *stroke, int *level)
{
    int k;
    if (bol < 0 || bol >= TABLA_NBOL)
        return 0;
    for (k = 0; k < TABLA_BOL[bol].n; k++)
        stroke[k] = TABLA_BOL[bol].stroke[k], level[k] = TABLA_BOL[bol].level[k];
    return TABLA_BOL[bol].n;
}

const char *tabla_stroke_name(int s) { return s >= 0 && s < TABLA_NSTROKE ? TABLA_STROKE[s].name : "?"; }

int tabla_stroke_drum(int s) { return s >= 0 && s < TABLA_NSTROKE ? TABLA_STROKE[s].drum : -1; }

int tabla_selftest(void)
{
    int t, bad = 0;
    for (t = 0; t < TABLA_NTAKE; t++) {
        const tabla_take_t *tk = &TABLA_TAKE[t];
        int32_t pred = 0, idx = tk->idx;
        uint32_t i, h = 2166136261u;
        for (i = 0; i < tk->n; i++) {
            int32_t s = adpcm_next(TABLA_ADPCM + tk->off, i, &pred, &idx);
            h = (h ^ ((uint32_t)s & 0xFFFFu)) * 16777619u;
        }
        bad += h != tk->hash;
    }
    return bad;
}

uint32_t tabla_data_bytes(void) { return (uint32_t)sizeof TABLA_ADPCM; }
