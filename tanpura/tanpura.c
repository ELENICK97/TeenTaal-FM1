/* tanpura.c — see tanpura.h */
#include "tanpura/tanpura.h"
#include "tanpura/body_coefs.h"
#include "tanpura/upsample_coefs.h"
#include "fastmath.h"

static int32_t reg_offset(const tanpura_t *m);   /* (the register: below) */

#define TANPURA_LEVEL 0.151f       /* body output -> full scale: RMS about -23.5 dBFS after LEVEL_DB, the worst peak -1.4 */

/* loudness compensation (dB to add), measured by tools/reference/level_grid.py (RMS over 3 cycles) so that neither
 * Sa nor the jawari changes the loudness: rows Sa (cents above C4), columns jawari 0, 25, 50, 75, 100 */
static const float LEVEL_SA[6] = {-1800.0f, -1500.0f, -1000.0f, -300.0f, 0.0f, 700.0f};
static const float LEVEL_DB[6][5] = {
    {-0.5f, 0.9f, 2.1f, 3.7f, 5.9f},
    {-1.1f, -0.6f, 0.0f, 1.8f, 3.9f},
    {-0.4f, 0.9f, 1.6f, 2.1f, 3.1f},
    {1.5f, 1.9f, 2.3f, 2.4f, 2.8f},
    {1.8f, 2.2f, 3.0f, 3.7f, 4.0f},
    {3.0f, 4.6f, 4.5f, 4.6f, 5.3f},          /* G4, jawari 0: +3.0, not the measured +6.1: its peaks stand 22 dB over its
                                                RMS, and with the irregularity they clipped (tools/test/tanpura_stress) */
};

/* the compensation gain for the current Sa and jawari: bilinear in the table (main loop, not per sample) */
static float level_comp(const tanpura_t *m)
{
    float sa = fm_clampf((float)(m->tuning.sa_cents + reg_offset(m)), LEVEL_SA[0], LEVEL_SA[5]), jw = (float)m->jawari * 0.04f, fs, fj, db;
    int i = 0, j = (int)jw;
    while (i < 4 && sa > LEVEL_SA[i + 1])
        i++;
    if (j > 3)
        j = 3;
    fs = (sa - LEVEL_SA[i]) / (LEVEL_SA[i + 1] - LEVEL_SA[i]);
    fj = jw - (float)j;
    db = (1.0f - fs) * ((1.0f - fj) * LEVEL_DB[i][j] + fj * LEVEL_DB[i][j + 1])
         + fs * ((1.0f - fj) * LEVEL_DB[i + 1][j] + fj * LEVEL_DB[i + 1][j + 1]);
    return fm_exp2f(db * 0.166096405f);          /* 10^(db/20) */
}
#define PLUCK_AMP (-0.4f)          /* N, toward the bridge (the paper's) */
#define PLUCK_TAU 0.05f            /* s */
#define FADE_OUT_S 0.3f
static const float CYCLE_AT[TANPURA_NSTR] = {0.0f, 0.30f, 0.45f, 0.60f};

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

void tanpura_default_config(tanpura_config_t *cfg)
{
    int j;
    tuning_default(&cfg->tuning);
    cfg->fmax[0] = 6000.0f, cfg->fmax[1] = cfg->fmax[2] = 8000.0f, cfg->fmax[3] = 3000.0f;   /* step 2b: quality */
    for (j = 0; j < TANPURA_NSTR; j++)
        cfg->maxm[j] = TSTRING_MAXM;
    cfg->s0 = 0.4f;
    cfg->rc = 0.8f;
    cfg->seed = 0x2545F491u;
    cfg->eq = 0;                                 /* off: ElectroNick heard no difference (step 2b) */
}

/* the register (step 3): the tanpura an octave under the player's Sa (1, "large": the default) when that stays in
 * the range the strings cover (Sa F#2 and up), else at the Sa itself */
static int32_t reg_offset(const tanpura_t *m)
{
    return m->reg && m->tuning.sa_cents - 1200 >= TUNING_SA_MIN ? -1200 : 0;
}

/* the string parameters for string j at the current settings */
static void string_par(const tanpura_t *m, int j, tstring_par_t *p)
{
    float cents = (float)(m->fine[j] + reg_offset(m)), r, rho, E;
    if (j == 0)
        cents += tuning_swara_cents(m->first) - 1200.0f;
    else if (j == 3)
        cents -= 1200.0f;
    /* the instrument follows Sa (step 2b): the large tanpura (1.1 m, steel 0.16 mm, kharaj bronze 0.25 mm) at Sa A2
     * and below, the smaller one (0.85 m, 0.13 / 0.18 mm) at Sa A3, both heard and liked by ElectroNick at step 2a;
     * in between and above (to 0.7 m at G4) by interpolation in cents, so the tension stays realistic */
    {
        float t = fm_clampf(((float)(m->tuning.sa_cents + reg_offset(m)) + 1500.0f) * (1.0f / 1200.0f), 0.0f, 1.83f);
        p->L = 1.1f - 0.25f * t;
        if (j == 3)
            r = (0.25f - 0.07f * t) * 1e-3f, rho = 8800.0f, E = 1.1e11f;   /* kharaj: bronze */
        else
            r = (0.16f - 0.03f * t) * 1e-3f, rho = 7850.0f, E = 2.0e11f;   /* steel */
    }
    p->rhoA = rho * FM_PI * r * r;
    p->EI = E * 0.25f * FM_PI * r * r * r * r;
    p->f1 = tuning_hz(&m->tuning, cents);
    p->s0 = m->cfg.s0, p->s1 = 6.5e-3f, p->s3 = 5.0e-6f;     /* Table 1, s0 from step 2a */
    p->xb = 10e-3f, p->wb = 2.0e-3f, p->kb = 4.39e8f;
    p->xc = 4e-3f + 4e-3f * (float)m->jawari * 0.01f, p->wc = 1.2e-3f, p->kc = 1.2e5f, p->rc = m->cfg.rc;
    p->xe = 0.35f * p->L, p->we = 0.02f;
    p->fmax = m->cfg.fmax[j];
    p->maxm = m->cfg.maxm[j];
}

/* prepare string j's coefficients for tanpura_take (main loop). A retune within a semitone keeps the string
 * sounding (tuning by ear while the drone plays); a larger one would change its stiffness, and so the energy its
 * state holds, too much: the render fades out, then tanpura_take silences the strings, takes it, and starts the cycle
 * again */
static void stage(tanpura_t *m, int j)
{
    tstring_par_t p;
    float r;
    string_par(m, j, &p);
    m->stage_ready[j] = 0;                       /* (the render skips a half-written stage) */
    tstring_coefs(&m->stage[j], &p, TANPURA_FS);
    r = p.f1 / (m->f1_now[j] > 0.0f ? m->f1_now[j] : p.f1);
    if (r > 1.0595f || r < 0.9439f)              /* more than a semitone */
        m->restart = 1;
    m->f1_now[j] = p.f1;
    m->comp_target = level_comp(m);
    m->stage_ready[j] = 1;
}

static uint32_t rnd(tanpura_t *m)                /* xorshift32 */
{
    uint32_t x = m->rng;
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    return m->rng = x;
}
static float rnd_pm1(tanpura_t *m) { return (float)(int32_t)(rnd(m) >> 8) * (1.0f / 8388608.0f) - 1.0f; }

static int32_t jitter(tanpura_t *m)
{
    return (int32_t)(rnd_pm1(m) * (float)m->human * 0.01f * 0.05f * TANPURA_FS);
}

static int32_t cycle_samples(const tanpura_t *m) { return (int32_t)((float)m->cycle * 0.1f * TANPURA_FS); }

static void start_cycle(tanpura_t *m)
{
    int j;
    for (j = 0; j < TANPURA_NSTR; j++) {
        m->nominal[j] = (int32_t)(CYCLE_AT[j] * (float)cycle_samples(m)) + (int32_t)(0.02f * TANPURA_FS);
        m->due[j] = m->nominal[j] + jitter(m);
    }
}

void tanpura_init(tanpura_t *m, const tanpura_config_t *cfg)
{
    int j, k;
    if (cfg)
        m->cfg = *cfg;
    else
        tanpura_default_config(&m->cfg);
    m->tuning = m->cfg.tuning;
    m->on = 0, m->first = SW_PA, m->jawari = 50, m->cycle = 40, m->human = 50, m->reg = 1, m->start_req = 0;
    for (j = 0; j < TANPURA_NSTR; j++)
        m->fine[j] = 0, m->vol[j] = 100;
    m->rng = m->cfg.seed ? m->cfg.seed : 1u;
    for (j = 0; j < TANPURA_NSTR; j++)
        m->f1_now[j] = 0.0f;
    for (j = 0; j < TANPURA_NSTR; j++) {
        stage(m, j);
        tstring_init(&m->str[j], &m->stage[j]);
        m->stage_ready[j] = 0;
    }
    for (k = 0; k < 32; k++)
        m->bw1[k] = m->bw2[k] = 0.0f;
    m->xd1 = m->xn1 = 0.0f;
    for (k = 0; k < (int)(sizeof m->up / sizeof m->up[0]); k++)
        m->up[k] = 0.0f;
    m->lp[0][0] = m->lp[0][1] = m->lp[1][0] = m->lp[1][1] = 0.0f;
    m->eqs[0] = m->eqs[1] = 0.0f;
    m->gain = 0.0f;
    m->active = 0;
    m->restart = 0;
    m->comp = m->comp_target;
    start_cycle(m);
}

void tanpura_set_param(tanpura_t *m, int id, int32_t v)
{
    int j;
    if (tuning_set_param(&m->tuning, id, v)) {
        for (j = 0; j < TANPURA_NSTR; j++)
            stage(m, j);
        return;
    }
    if (id >= TANPURA_P_FINE && id < TANPURA_P_FINE + TANPURA_NSTR) {
        m->fine[id - TANPURA_P_FINE] = clampi(v, -50, 50);
        stage(m, id - TANPURA_P_FINE);
        return;
    }
    if (id >= TANPURA_P_VOL && id < TANPURA_P_VOL + TANPURA_NSTR) {
        m->vol[id - TANPURA_P_VOL] = clampi(v, 0, 100);
        return;
    }
    switch (id) {
    case TANPURA_P_ON:
        v = v != 0;
        if (v && !m->on)
            m->start_req = 1;                    /* (tanpura_take starts the cycle: the render may be running) */
        m->on = v;
        break;
    case TANPURA_P_REGISTER:
        m->reg = v != 0;
        for (j = 0; j < TANPURA_NSTR; j++)
            stage(m, j);
        break;
    case TANPURA_P_FIRST:  m->first = clampi(v, 0, SW_N - 1); stage(m, 0); break;
    case TANPURA_P_JAWARI:
        m->jawari = clampi(v, 0, 100);
        for (j = 0; j < TANPURA_NSTR; j++)
            stage(m, j);
        break;
    case TANPURA_P_CYCLE:  m->cycle = clampi(v, 25, 80); break;
    case TANPURA_P_HUMAN:  m->human = clampi(v, 0, 100); break;
    default: break;
    }
}

int32_t tanpura_get_param(const tanpura_t *m, int id)
{
    if (id == DSP_P_A4_DHZ) return m->tuning.a4_dhz;
    if (id == DSP_P_SA_CENTS) return m->tuning.sa_cents;
    if (id >= TANPURA_P_FINE && id < TANPURA_P_FINE + TANPURA_NSTR) return m->fine[id - TANPURA_P_FINE];
    if (id >= TANPURA_P_VOL && id < TANPURA_P_VOL + TANPURA_NSTR) return m->vol[id - TANPURA_P_VOL];
    switch (id) {
    case TANPURA_P_ON:     return m->on;
    case TANPURA_P_FIRST:  return m->first;
    case TANPURA_P_JAWARI: return m->jawari;
    case TANPURA_P_CYCLE:  return m->cycle;
    case TANPURA_P_HUMAN:  return m->human;
    case TANPURA_P_REGISTER: return m->reg;
    case TANPURA_P_SA_NOW: return m->tuning.sa_cents + reg_offset(m);
    default:               return 0;
    }
}

void tanpura_take(tanpura_t *m)
{
    int j;
    if (m->restart && (m->gain == 0.0f || !m->active)) {   /* a large retune, faded out (or silent): afresh */
        for (j = 0; j < TANPURA_NSTR; j++)
            tstring_silence(&m->str[j]);
        start_cycle(m);
        m->restart = 0;
    }
    if (!m->restart)
        for (j = 0; j < TANPURA_NSTR; j++)
            if (m->stage_ready[j]) {
                tstring_take(&m->str[j], &m->stage[j]);
                m->stage_ready[j] = 0;
            }
    if (m->start_req) {
        start_cycle(m);
        m->active = 1;
        m->start_req = 0;
    }
}

/* string j for n samples from offset `at` of the block, its plucks at their sample */
static void run_string(tanpura_t *m, int j, int n)
{
    tstring_t *s = &m->str[j];
    int at = 0;
    while (at < n) {
        int len = n - at;
        if (m->on && m->due[j] < len)
            len = m->due[j] < 0 ? 0 : m->due[j];
        if (len > 0) {
            tstring_run(s, m->fd + at, m->fn + at, len);
            at += len;
            m->due[j] -= len;
            m->nominal[j] -= len;
        }
        if (m->on && m->due[j] <= 0 && at < n) {
            float a = PLUCK_AMP * (float)m->vol[j] * 0.01f * (1.0f + 0.003f * (float)m->human * rnd_pm1(m));
            tstring_pluck(s, a, PLUCK_TAU, TANPURA_FS);
            m->nominal[j] += cycle_samples(m);
            m->due[j] = m->nominal[j] + jitter(m);
        }
    }
}

void tanpura_render(tanpura_t *m, int32_t *out, int n)
{
    float g0 = m->gain, g1, dg, *x = m->up + 2 * UPS_K;   /* x[-2K..-1]: the history, x[0..ns-1]: this block */
    float c0 = m->comp, c1 = m->comp_target, dc;
    int i, j, k, ns = n / TANPURA_OS;
    dc = (c1 - c0) / (float)ns;
    if (!m->active)
        return;
    for (i = 0; i < ns; i++)
        m->fd[i] = m->fn[i] = 0.0f;
    for (j = 0; j < TANPURA_NSTR; j++)
        run_string(m, j, ns);
    /* the body at the strings' rate: parallel sections with shared poles, the bridge and nut forces in */
    for (i = 0; i < ns; i++) {
        float xd = m->fd[i], xn = m->fn[i], p = BODY_D_BRIDGE * xd + BODY_D_NUT * xn;
        for (k = 0; k < BODY_NSEC; k++) {
            float w = BODY_B0_BRIDGE[k] * xd + BODY_B1_BRIDGE[k] * m->xd1 + BODY_B0_NUT[k] * xn
                      + BODY_B1_NUT[k] * m->xn1 - BODY_A1[k] * m->bw1[k] - BODY_A2[k] * m->bw2[k];
            m->bw2[k] = m->bw1[k];
            m->bw1[k] = w;
            p += w;
        }
        m->xd1 = xd, m->xn1 = xn;
        if (m->cfg.eq) {                         /* the brightness correction (upsample_coefs.h) */
            float yv = UPS_EQ[0] * p + m->eqs[0];
            m->eqs[0] = UPS_EQ[1] * p - UPS_EQ[3] * yv + m->eqs[1];
            m->eqs[1] = UPS_EQ[2] * p - UPS_EQ[4] * yv;
            p = yv;
        }
        for (k = 0; k < UPS_LP_N; k++) {          /* the low-pass at 8.5 kHz (upsample_coefs.h) */
            float yv = UPS_LP[k][0] * p + m->lp[k][0];
            m->lp[k][0] = UPS_LP[k][1] * p - UPS_LP[k][3] * yv + m->lp[k][1];
            m->lp[k][1] = UPS_LP[k][2] * p - UPS_LP[k][4] * yv;
            p = yv;
        }
        x[i] = p * TANPURA_LEVEL * (c0 + dc * (float)(i + 1));
    }
    m->comp = c1;
    /* up to the FM-1's rate (upsample_coefs.h), with the on/off fade */
    if (m->restart)                              /* a large retune: out in 20 ms */
        g1 = g0 - (float)n / (0.02f * DSP_FS) < 0.0f ? 0.0f : g0 - (float)n / (0.02f * DSP_FS);
    else if (m->on)
        g1 = g0 + (float)n / (0.02f * DSP_FS) > 1.0f ? 1.0f : g0 + (float)n / (0.02f * DSP_FS);
    else
        g1 = g0 - (float)n / (FADE_OUT_S * DSP_FS) < 0.0f ? 0.0f : g0 - (float)n / (FADE_OUT_S * DSP_FS);
    dg = (g1 - g0) / (float)n;
    for (i = 0; i < ns; i++) {
        const float *c = x + i - UPS_K;           /* the centre: out[2i] = c[0] */
        float odd = 0.0f, e = c[0], o;
        for (k = 0; k < UPS_K; k++)
            odd += UPS_H[k] * (c[-k] + c[1 + k]);
        o = odd;
        e = fm_clampf(e * (g0 + dg * (float)(2 * i + 1)), -1.0f, 1.0f);
        o = fm_clampf(o * (g0 + dg * (float)(2 * i + 2)), -1.0f, 1.0f);
        out[2 * i] += (int32_t)(e * DSP_Q23_F);
        out[2 * i + 1] += (int32_t)(o * DSP_Q23_F);
    }
    for (k = 0; k < 2 * UPS_K; k++)              /* keep the last 2K inputs as the next block's history */
        m->up[k] = x[ns - 2 * UPS_K + k];
    m->gain = g1;
    if (!m->on && g1 == 0.0f) {                  /* faded out: silence and stop computing */
        for (j = 0; j < TANPURA_NSTR; j++)
            tstring_silence(&m->str[j]);
        for (k = 0; k < 32; k++)
            m->bw1[k] = m->bw2[k] = 0.0f;
        m->xd1 = m->xn1 = 0.0f;
        for (k = 0; k < (int)(sizeof m->up / sizeof m->up[0]); k++)
            m->up[k] = 0.0f;
        m->lp[0][0] = m->lp[0][1] = m->lp[1][0] = m->lp[1][1] = 0.0f;
        m->eqs[0] = m->eqs[1] = 0.0f;
        m->active = 0;
    }
}
