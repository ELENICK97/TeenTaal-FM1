/* tstring.c — see tstring.h. Equation numbers are the DAFx-16 paper's. */
#include "tanpura/tstring.h"
#include "fastmath.h"

/* the pluck force shape (eqs. 14-15, beta_e = 30), normalised to a peak of 1, over its time span */
#define PLUCK_N 512
#define PLUCK_BETA 30.0f
static float pluck_tab[PLUCK_N + 2];
static int pluck_ready;

static void pluck_table(void)
{
    int i;
    float sb = 0.5f * (fm_expf(PLUCK_BETA) - fm_expf(-PLUCK_BETA)), peak = 0.0f;
    for (i = 0; i <= PLUCK_N; i++) {
        float t = (float)i / (float)PLUCK_N;                       /* (t - t_e) / tau_e */
        float sh = 0.5f * (fm_expf(PLUCK_BETA * t) - fm_expf(-PLUCK_BETA * t));
        float h = 0.5f * (t + sh / sb), s = fm_sinf(FM_PI * h);
        pluck_tab[i] = s * s;
        if (pluck_tab[i] > peak)
            peak = pluck_tab[i];
    }
    for (i = 0; i <= PLUCK_N; i++)
        pluck_tab[i] /= peak;
    pluck_tab[PLUCK_N + 1] = 0.0f;
    pluck_ready = 1;
}

/* sin(x) with a small relative error also for small x (fm_sinf's is absolute) */
static float sin_rel(float x)
{
    if (fm_fabsf(x) < 0.1f) {
        float x2 = x * x;
        return x * (1.0f - x2 * (1.0f / 6.0f) * (1.0f - x2 * (1.0f / 20.0f)));
    }
    return fm_sinf(x);
}

/* 1 - exp(-x) for x >= 0, accurate for small x */
static float one_minus_exp_neg(float x)
{
    if (x < 0.01f)
        return x * (1.0f - x * 0.5f * (1.0f - x * (1.0f / 3.0f)));
    return 1.0f - fm_expf(-x);
}

/* (20), with the limit of its footnote at beta w = pi */
static float gfun(float beta, float x, float w)
{
    float den = FM_PI * FM_PI - beta * beta * w * w;
    if (fm_fabsf(den) < 1e-4f)
        return 0.25f * FM_PI * fm_sinf(beta * x);
    return FM_PI * FM_PI * fm_sinf(beta * x) * fm_cosf(0.5f * beta * w) / den;
}

void tstring_coefs(tstring_coefs_t *c, const tstring_par_t *p, float fs)
{
    float dt = 1.0f / fs, m = 0.5f * p->rhoA * p->L, xi = dt * dt / (2.0f * m);
    float b1 = FM_PI / p->L, a1 = p->s0 + p->s1 * b1 + p->s3 * b1 * b1 * b1, w1 = FM_TWO_PI * p->f1;
    float k1, T, wmax = FM_TWO_PI * (p->fmax < 0.5f * fs ? p->fmax : 0.5f * fs);
    float thbb = 0.0f, thbc = 0.0f, thcc = 0.0f, gbe = 0.0f, gce = 0.0f, th;
    int i, maxm = p->maxm < TSTRING_MAXM ? p->maxm : TSTRING_MAXM;
    if (!pluck_ready)
        pluck_table();
    /* the tension that puts mode 1 exactly at f1, stiffness and damping included: k_1 = m (w1^2 + a1^2) */
    k1 = m * (w1 * w1 + a1 * a1);
    T = (2.0f * k1 / p->L - p->EI * b1 * b1 * b1 * b1) / (b1 * b1);
    c->k.T = T;
    c->k.top_hz = 0.0f;
    for (i = 0; i < maxm; i++) {
        float beta = (float)(i + 1) * b1, b2 = beta * beta;
        float k = 0.5f * p->L * (p->EI * b2 * b2 + T * b2);
        float alpha = p->s0 + p->s1 * beta + p->s3 * b2 * beta;
        float w2 = k / m - alpha * alpha, w, R, omR, s, omO, den, b;
        if (w2 <= 0.0f)
            break;
        w = fm_sqrtf(w2);
        if (w >= wmax || w * dt >= FM_PI)
            break;
        /* eq. 33 without cancellation: 1 - 2 R cos + R^2 = (1 - R)^2 + 2 R (1 - cos), 1 - cos = 2 sin^2(w dt / 2) */
        R = fm_expf(-alpha * dt);
        omR = one_minus_exp_neg(alpha * dt);
        s = sin_rel(0.5f * w * dt);
        omO = 2.0f * s * s;
        den = (1.0f + R) * (1.0f + R) - 2.0f * R * omO;
        c->a[i] = (omR * omR + 2.0f * R * omO) / den;
        b = 2.0f * omR * (1.0f + R) / den;
        c->c2j[i] = 2.0f / (1.0f + c->a[i] + b);
        c->gb[i] = gfun(beta, p->xb, p->wb);
        c->gc[i] = gfun(beta, p->xc, p->wc);
        c->gej[i] = xi * gfun(beta, p->xe, p->we) * 0.5f * c->c2j[i];
        c->eb[i] = xi * c->gb[i] * 0.5f * c->c2j[i];
        c->ec[i] = xi * c->gc[i] * 0.5f * c->c2j[i];
        c->cn[i] = 0.5f * ((i & 1) ? -1.0f : 1.0f) * (T * beta + p->EI * b2 * beta);   /* eq. 22: -(-1)^n, mode n = i+1; halved */
        thbb += c->gb[i] * c->eb[i];
        thbc += c->gb[i] * c->ec[i];
        thcc += c->gc[i] * c->ec[i];
        gbe += c->gb[i] * c->gej[i];
        gce += c->gc[i] * c->gej[i];
        c->k.top_hz = w / FM_TWO_PI;
    }
    c->k.M = i;
    c->k.kc = p->kc;
    c->k.phic = 0.5f * p->kc + p->rc * fs;
    c->k.vden = 1.0f / (1.0f + c->k.phic * thcc);
    c->k.bc = c->k.phic * thbc * c->k.vden;
    c->k.thbc = thbc;
    c->k.thcc = thcc;
    c->k.gbe = gbe;
    c->k.gce = gce;
    th = thbb - c->k.bc * thbc;
    c->k.kb = p->kb;
    c->k.ph = 0.5f * th * p->kb;
    c->k.inv1ph = 1.0f / (1.0f + c->k.ph);
    c->k.inv21ph = 0.5f * c->k.inv1ph;
    c->k.invth = 1.0f / th;
}

void tstring_silence(tstring_t *s)
{
    int i;
    for (i = 0; i < TSTRING_MAXM; i++)
        s->m[i].y = s->m[i].q = s->m[i].ub = 0.0f;
    s->plucking = 0;
    s->pluck_pos = 0.0f;
}

void tstring_take(tstring_t *s, const tstring_coefs_t *c)
{
    int i;
    s->c = c->k;
    for (i = 0; i < TSTRING_MAXM; i++) {
        tmode_t *m = &s->m[i];
        if (i < c->k.M) {
            m->a = c->a[i], m->c2j = c->c2j[i], m->gb = c->gb[i], m->gc = c->gc[i];
            m->eb = c->eb[i], m->ec = c->ec[i], m->cn = c->cn[i], m->gej = c->gej[i];
        } else {
            m->y = m->q = m->ub = 0.0f;
            m->a = m->c2j = m->gb = m->gc = m->eb = m->ec = m->cn = m->gej = 0.0f;
        }
        m->pad = 0.0f;
    }
}

void tstring_init(tstring_t *s, const tstring_coefs_t *c)
{
    tstring_silence(s);
    tstring_take(s, c);
    s->compress_max = 0.0f;
}

void tstring_pluck(tstring_t *s, float amp, float tau_e, float fs)
{
    s->pluck_amp = amp;
    s->pluck_pos = 0.0f;
    s->pluck_step = (float)PLUCK_N / (tau_e * fs);
    s->plucking = 1;
}

/* the pluck force at n + 1/2 (the table read half a sample on) */
static float pluck_next(tstring_t *s)
{
    float x = s->pluck_pos + 0.5f * s->pluck_step, fr, v;
    int i;
    if (x >= (float)PLUCK_N) {
        s->plucking = 0;
        return 0.0f;
    }
    i = (int)x;
    fr = x - (float)i;
    v = pluck_tab[i] + (pluck_tab[i + 1] - pluck_tab[i]) * fr;
    s->pluck_pos += s->pluck_step;
    return v * s->pluck_amp;
}

/* One merged loop per step: it finishes step n (eqs. 41, 40, the nut force) and prepares step n+1's u-bar (eq. 42)
 * with its projections on the bridge and the thread. The bridge and thread displacements are tracked from the
 * contact solution (y_b += s_b, y_c += s_c: exact in exact arithmetic) and recomputed from the modes once per call,
 * so float rounding cannot accumulate in them. The pluck's part of u-bar is added through its precomputed
 * projections (gbe, gce) and, while a pluck lasts, in the update itself. */
void tstring_run(tstring_t *s, float *fd, float *fn, int n)
{
    const tstring_k_t *c = &s->c;
    const int M = c->M;
    tmode_t *restrict md = s->m;
    float yb = 0.0f, yc = 0.0f, ubs = 0.0f, ucs = 0.0f;
    int k, i;
    /* the exact state at the start of the call */
    for (i = 0; i < M; i++) {
        tmode_t *m = &md[i];
        float v = (m->q - m->a * m->y) * m->c2j;
        yb += m->gb * m->y;
        yc += m->gc * m->y;
        m->ub = v;
        ubs += m->gb * v;
        ucs += m->gc * v;
    }
    for (k = 0; k < n; k++) {
        float fe = s->plucking ? pluck_next(s) : 0.0f;
        float ubt = ubs + fe * c->gbe, uct = ucs + fe * c->gce, vc, u, d, sb, Fb, Fc, nut = 0.0f, nb = 0.0f, nc = 0.0f;
        /* the contact forces (eqs. 46-48, Fig. 4), h_b = h_c = 0 */
        vc = (-c->kc * yc - c->phic * uct) * c->vden;
        u = -ubt - c->thbc * vc;
        d = -yb;                                    /* the penetration at n */
        if (d <= 0.0f) {
            if (u <= -d) {
                sb = -u;                            /* no contact at n nor n+1 */
                Fb = 0.0f;
            } else {                                /* into contact */
                sb = (2.0f * c->ph * d - u - fm_sqrtf(u * u - 4.0f * c->ph * d * (u + d))) * c->inv21ph;
                Fb = (sb + u) * c->invth;
            }
        } else if (u >= d * (c->ph - 1.0f)) {       /* in contact at n and n+1 */
            sb = (2.0f * c->ph * d - u) * c->inv1ph;
            Fb = c->kb * (d - 0.5f * sb);
        } else {                                    /* out of contact */
            sb = -0.5f * u + 0.5f * fm_sqrtf(u * u + 4.0f * c->ph * d * d);
            Fb = (sb + u) * c->invth;
        }
        if (d - sb > s->compress_max)
            s->compress_max = d - sb;
        Fc = vc - c->bc * Fb;
        yb += sb;                                   /* y_b^{n+1} */
        yc += uct + c->thbc * Fb + c->thcc * Fc;    /* y_c^{n+1}: g_c^T s */
        if (fe != 0.0f) {
            for (i = 0; i < M; i++) {
                tmode_t *m = &md[i];
                float st = m->ub + m->gej * fe + m->eb * Fb + m->ec * Fc, yo = m->y, yn = yo + st, qn = st - m->q, v;
                nut += m->cn * (yn + yo);
                m->y = yn;
                m->q = qn;
                v = (qn - m->a * yn) * m->c2j;
                m->ub = v;
                nb += m->gb * v;
                nc += m->gc * v;
            }
        } else {
            for (i = 0; i < M; i++) {
                tmode_t *m = &md[i];
                float st = m->ub + m->eb * Fb + m->ec * Fc, yo = m->y, yn = yo + st, qn = st - m->q, v;
                nut += m->cn * (yn + yo);
                m->y = yn;
                m->q = qn;
                v = (qn - m->a * yn) * m->c2j;
                m->ub = v;
                nb += m->gb * v;
                nc += m->gc * v;
            }
        }
        ubs = nb, ucs = nc;
        fd[k] += -Fc - Fb;
        fn[k] += nut;
    }
}

float tstring_energy(const tstring_t *s, float fs)
{
    /* (q^T q + y^T A y) / xi, with xi recovered from eb / (g_b / J): the modal part of eq. 49, for the tests */
    float H = 0.0f, xi = 0.0f;
    int i;
    for (i = 0; i < s->c.M; i++)
        if (s->m[i].gb != 0.0f) {
            xi = s->m[i].eb / (s->m[i].gb * 0.5f * s->m[i].c2j);
            break;
        }
    (void)fs;
    if (xi == 0.0f)
        return 0.0f;
    for (i = 0; i < s->c.M; i++)
        H += s->m[i].q * s->m[i].q + s->m[i].a * s->m[i].y * s->m[i].y;
    return H / xi;
}
