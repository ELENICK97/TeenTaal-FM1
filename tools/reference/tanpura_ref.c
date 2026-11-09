/* tanpura_ref — the DAFx-16 tanpura string model, complete and in double precision (host only).
 *
 * M. van Walstijn, J. Bridges, S. Mehes, "A Real-Time Synthesis Oriented Tanpura Model", DAFx-16
 * (docs/references/vanwalstijn-dafx16-tanpura.pdf). Equation numbers below are the paper's.
 * This is the gold reference that the reduced firmware model (tanpura/, step 2b) is compared with.
 * It is NOT firmware code: double precision, malloc, libm, all modes up to Nyquist.
 *
 * Per string: modal expansion (16)-(20) with exact modal frequency and damping (33), the vector update
 * (40)-(42), bridge and thread contact forces in closed form (43)-(48) and Fig. 4, plucking force (14)-(15),
 * nut force (22). Strings are uncoupled; the driving forces are summed (56). Output: raw float32 frames
 * [F_d, F_n, y_b of string 0] at the simulation rate; tools/reference/analyze.py does the radiation filtering.
 *
 *   tanpura_ref [options] OUT.f32
 *   -fs HZ          sample rate (default 44100)
 *   -dur S          duration (default 10)
 *   -fmax HZ        truncate the mode series here (default: Nyquist, the paper's limit)
 *   -L M            string length (default 1.0)
 *   -s F1[:MAT[:R[:FMAX]]] add a string tuned to F1 Hz; MAT steel (default) or bronze; R radius in m (default
 *                   0.14e-3); FMAX this string's own mode truncation (Hz; default -fmax)
 *   -p I:T[:A]      pluck string I at time T, A times the pluck amplitude -ae (default 1; repeatable)
 *   -cycle P        repeat every pluck each P seconds until the end
 *   -xc M -wc M -kc N/m -rc kg/s     thread (Table 1: 5e-3, 1.2e-3, 1.2e5, 1.2)
 *   -xb M -wb M -kb N/m              bridge (Table 1: 10e-3, 2e-3, 4.39e8)
 *   -ae N -taue S -betae B -xe M -we M   pluck (§4.1: -0.4, 0.05, 30, L/2, L)
 *   -s0 -s1 -s3     damping (Table 1: 0.6, 6.5e-3, 5e-6)
 *   -nostiff -nodamp -nothread -nobridge   switch model elements off (the authors' examples)
 *   -selftest       checks: g against numerical integration, energy conservation without losses,
 *                   energy decay with losses, contact solution consistency; exits 0 on success */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXS 8
#define MAXP 1024

typedef struct {
    double L, rhoA, T, EI, s0, s1, s3;
    double xb, wb, kb, hb;
    double xc, wc, kc, rc, hc;
    double xe, we;
    int bridge;
} spar_t;

typedef struct {
    spar_t p;
    int M;
    double m, xi, phic, thbb, thbc, thcc;
    double *y, *q, *a, *J, *gb, *gc, *ge, *eb, *ec, *cn, *f, *ubar;   /* f: modal frequencies, Hz; ubar: scratch */
    double yb_min_rel;            /* the deepest compression into the bridge (h_b - y_b), m */
    long contacts;                /* steps with the string in contact with the bridge */
} string_t;

typedef struct { int s; double t, a; } pluck_t;

static double ae = -0.4, taue = 0.05, betae = 30.0;

/* (20), with the limit of its footnote at beta*w = pi */
static double gfun(double beta, double x, double w)
{
    double den = M_PI * M_PI - beta * beta * w * w;
    if (fabs(den) < 1e-9 * M_PI * M_PI)
        return 0.25 * M_PI * sin(beta * x);
    return M_PI * M_PI * sin(beta * x) * cos(0.5 * beta * w) / den;
}

/* (14), (15): one pluck's force at time t since its start */
static double pluck_force(double t)
{
    double h;
    if (t < 0 || t > taue)
        return 0;
    h = 0.5 * (t + taue * sinh(betae * t / taue) / sinh(betae));
    return ae * pow(sin(M_PI / taue * h), 2);
}

static int string_init(string_t *s, const spar_t *p, double fs, double fmax)
{
    double dt = 1.0 / fs, wmax = 2 * M_PI * (fmax > 0 ? fmax : 0.5 * fs);
    int i, cap = 4000;
    memset(s, 0, sizeof *s);
    s->p = *p;
    s->m = 0.5 * p->rhoA * p->L;
    s->xi = dt * dt / (2 * s->m);
    s->phic = 0.5 * p->kc + p->rc / dt;
    s->y = calloc((size_t)cap, sizeof(double)); s->q = calloc((size_t)cap, sizeof(double));
    s->a = calloc((size_t)cap, sizeof(double)); s->J = calloc((size_t)cap, sizeof(double));
    s->gb = calloc((size_t)cap, sizeof(double)); s->gc = calloc((size_t)cap, sizeof(double));
    s->ge = calloc((size_t)cap, sizeof(double)); s->eb = calloc((size_t)cap, sizeof(double));
    s->ec = calloc((size_t)cap, sizeof(double)); s->cn = calloc((size_t)cap, sizeof(double));
    s->f = calloc((size_t)cap, sizeof(double)); s->ubar = calloc((size_t)cap, sizeof(double));
    for (i = 0; i < cap; i++) {
        double beta = (i + 1) * M_PI / p->L;
        double k = 0.5 * p->L * (p->EI * pow(beta, 4) + p->T * beta * beta);           /* k_i */
        double alpha = p->s0 + p->s1 * beta + p->s3 * pow(beta, 3);                    /* (18) */
        double w2 = k / s->m - alpha * alpha, w, R, Om, den, b;
        if (w2 <= 0)
            break;                                      /* (overdamped: not reached for these strings) */
        w = sqrt(w2);
        if (w >= wmax || w * dt >= M_PI)                /* truncation at Nyquist (or fmax): no mode aliasing */
            break;
        R = exp(-alpha * dt), Om = cos(w * dt), den = 1 + 2 * R * Om + R * R;
        s->a[i] = (1 - 2 * R * Om + R * R) / den;                                      /* (33) */
        b = 2 * (1 - R * R) / den;
        s->J[i] = 1 + s->a[i] + b;
        s->gb[i] = gfun(beta, p->xb, p->wb);
        s->gc[i] = gfun(beta, p->xc, p->wc);
        s->ge[i] = gfun(beta, p->xe, p->we);
        s->eb[i] = s->xi * s->gb[i] / s->J[i];
        s->ec[i] = s->xi * s->gc[i] / s->J[i];
        s->cn[i] = -((i + 1) % 2 ? -1.0 : 1.0) * (p->T * beta + p->EI * pow(beta, 3));   /* (22): v' and v''' at L */
        s->f[i] = w / (2 * M_PI);
        s->thbb += s->gb[i] * s->eb[i];
        s->thbc += s->gb[i] * s->ec[i];
        s->thcc += s->gc[i] * s->ec[i];
    }
    s->M = i;
    return s->M;
}

/* the numerical energy (49) */
static double string_energy(const string_t *s)
{
    double H = 0, yb = 0, yc = 0, d;
    int i;
    for (i = 0; i < s->M; i++) {
        H += (s->q[i] * s->q[i] + s->a[i] * s->y[i] * s->y[i]) / s->xi;
        yb += s->gb[i] * s->y[i];
        yc += s->gc[i] * s->y[i];
    }
    d = s->p.hb - yb;
    H += 0.5 * s->p.kc * (s->p.hc - yc) * (s->p.hc - yc);
    if (s->p.bridge && d > 0)
        H += 0.5 * s->p.kb * d * d;
    return H;
}

/* one step n -> n+1. fe: the external force at n+1/2 (the mean of F_e at n and n+1, §2.3).
 * Returns the bridge-side and nut forces at n+1/2; *ybo the bridge displacement y_b at n+1. */
static void string_step(string_t *s, double fe, double *Fd, double *Fn, double *ybo)
{
    const spar_t *p = &s->p;
    double yb = 0, yc = 0, ub = 0, uc = 0, vc, bc, th, u, d, ph, sb, Fb = 0, Fc, fn = 0;
    int i;
    double *ubar = s->ubar;
    for (i = 0; i < s->M; i++) {
        yb += s->gb[i] * s->y[i];
        yc += s->gc[i] * s->y[i];
        ubar[i] = (2 * (s->q[i] - s->a[i] * s->y[i]) + s->xi * s->ge[i] * fe) / s->J[i];   /* (42) */
        ub += s->gb[i] * ubar[i];
        uc += s->gc[i] * ubar[i];
    }
    vc = (p->kc * (p->hc - yc) - s->phic * uc) / (1 + s->phic * s->thcc);                   /* (47) */
    bc = s->phic * s->thbc / (1 + s->phic * s->thcc);
    th = s->thbb - bc * s->thbc;                                                            /* (48) */
    u = -ub - s->thbc * vc;
    d = p->hb - yb;                                     /* the penetration at n (> 0: in contact) */
    if (!p->bridge) {
        sb = -u;
    } else {
        ph = 0.5 * th * p->kb;                          /* Fig. 4: phi = theta k_b / 2, Delta y^n = d */
        if (d <= 0) {
            if (u <= -d) {                              /* no contact at n nor at n+1 */
                sb = -u;
            } else {                                    /* into contact */
                sb = (2 * ph * d - u - sqrt(u * u - 4 * ph * d * (u + d))) / (2 * (1 + ph));
                Fb = -0.5 * p->kb * (d - sb) * (d - sb) / sb;            /* (30) for this case */
            }
        } else if (u >= d * (ph - 1)) {                 /* in contact at n and at n+1 */
            sb = (2 * ph * d - u) / (1 + ph);
            Fb = p->kb * (d - 0.5 * sb);
        } else {                                        /* out of contact */
            sb = -0.5 * u + 0.5 * sqrt(u * u + 4 * ph * d * d);
            Fb = 0.5 * p->kb * d * d / sb;
        }
        if (d - sb > 0) {
            s->contacts++;
            if (d - sb > s->yb_min_rel)
                s->yb_min_rel = d - sb;
        }
    }
    Fc = vc - bc * Fb;                                                                       /* (46) */
    for (i = 0; i < s->M; i++) {
        double st = ubar[i] + s->eb[i] * Fb + s->ec[i] * Fc, yn = s->y[i] + st;            /* (41), (40) */
        fn += s->cn[i] * 0.5 * (yn + s->y[i]);
        s->y[i] = yn;
        s->q[i] = st - s->q[i];
    }
    *Fd = -Fc - Fb;                                                                          /* (13) */
    *Fn = fn;
    *ybo = yb + sb;
}

static void material(const char *mat, double r, double *rhoA, double *EI)
{
    double rho = 7850, E = 2.0e11;                      /* steel */
    if (!strcmp(mat, "bronze"))
        rho = 8800, E = 1.1e11;                         /* phosphor bronze (assumed: the paper gives steel only) */
    *rhoA = rho * M_PI * r * r;
    *EI = E * 0.25 * M_PI * pow(r, 4);                  /* I = pi r^4 / 4 (§3.1) */
}

static spar_t table1(void)
{
    spar_t p = {0};
    p.L = 1.0, p.s0 = 0.6, p.s1 = 6.5e-3, p.s3 = 5.0e-6;
    p.xb = 10e-3, p.wb = 2.0e-3, p.kb = 4.39e8, p.hb = 0;
    p.xc = 5e-3, p.wc = 1.2e-3, p.kc = 1.2e5, p.rc = 1.2, p.hc = 0;
    p.bridge = 1;
    return p;
}

/* ---------------------------------------------------------------- self-test --- */
static int selftest(void)
{
    int fails = 0, i, n;
    double fs = 44100;
    string_t s;
    spar_t p = table1();
    material("steel", 0.14e-3, &p.rhoA, &p.EI);
    p.T = pow(2 * p.L * 130.81, 2) * p.rhoA;
    p.xe = 0.5 * p.L, p.we = p.L;
    /* 1. g (20) against numerical integration of (19) */
    {
        double worst = 0;
        int k;
        for (k = 1; k < 400; k += 7) {
            double beta = k * M_PI / p.L, num = 0, x0 = p.xb - p.wb / 2, hstep = p.wb / 20000;
            for (n = 0; n < 20000; n++) {
                double x = x0 + (n + 0.5) * hstep;
                num += sin(beta * x) * M_PI / (2 * p.wb) * cos(M_PI / p.wb * (x - p.xb)) * hstep;
            }
            if (fabs(num - gfun(beta, p.xb, p.wb)) > worst)
                worst = fabs(num - gfun(beta, p.xb, p.wb));
        }
        printf("  %-4s g (20) vs numerical integral: worst |diff| %.2e\n", worst < 1e-8 ? "ok" : "FAIL", worst);
        fails += worst >= 1e-8;
    }
    /* 2. lossless (no damping, no thread damping): energy constant after the pluck, through bridge contact */
    {
        spar_t q = p;
        double H0 = 0, worst = 0, Fd, Fn, yb;
        q.s0 = q.s1 = q.s3 = 0, q.rc = 0;
        string_init(&s, &q, fs, 0);
        for (n = 0; n < (int)(0.6 * fs); n++) {
            double t = n / fs;
            string_step(&s, 0.5 * (pluck_force(t) + pluck_force(t + 1 / fs)), &Fd, &Fn, &yb);
            if (t > taue + 2 / fs) {
                double H = string_energy(&s);
                if (H0 == 0)
                    H0 = H;
                if (fabs(H - H0) / H0 > worst)
                    worst = fabs(H - H0) / H0;
            }
        }
        printf("  %-4s lossless energy drift after the pluck: %.2e (relative), %ld contact steps\n",
               worst < 1e-9 && s.contacts > 100 ? "ok" : "FAIL", worst, s.contacts);
        fails += !(worst < 1e-9 && s.contacts > 100);
    }
    /* 3. lossy (Table 1): energy never increases after the pluck */
    {
        double Hp = 0, worst = 0, Fd, Fn, yb, ybs = 0, wcons = 0;
        string_init(&s, &p, fs, 0);
        for (n = 0; n < (int)(1.0 * fs); n++) {
            double t = n / fs, H, chk = 0;
            string_step(&s, 0.5 * (pluck_force(t) + pluck_force(t + 1 / fs)), &Fd, &Fn, &yb);
            for (i = 0; i < s.M; i++)                   /* 4. y_b^{n+1} from the contact solution = g_b^T y^{n+1} */
                chk += s.gb[i] * s.y[i];
            if (fabs(chk - yb) > wcons)
                wcons = fabs(chk - yb);
            if (fabs(yb) > ybs)
                ybs = fabs(yb);
            H = string_energy(&s);
            if (t > taue + 2 / fs && Hp > 0 && (H - Hp) / Hp > worst)
                worst = (H - Hp) / Hp;
            Hp = H;
        }
        printf("  %-4s lossy energy: largest step increase %.2e (relative)\n", worst < 1e-12 ? "ok" : "FAIL", worst);
        fails += worst >= 1e-12;
        printf("  %-4s y_b from the contact solution vs g_b^T y: worst %.2e m (of %.2e m)\n",
               wcons < 1e-9 * ybs ? "ok" : "FAIL", wcons, ybs);
        fails += !(wcons < 1e-9 * ybs);
    }
    printf("%s\n", fails ? "SELFTEST FAILED" : "selftest ok");
    return fails ? 1 : 0;
}

/* ---------------------------------------------------------------- main --- */
int main(int argc, char **argv)
{
    static string_t st[MAXS];
    pluck_t pl[MAXP];
    spar_t base = table1();
    double fs = 44100, dur = 10, fmax = 0, cyc = 0, xe = -1, we = -1, r[MAXS], f1[MAXS], sfmax[MAXS];
    char mat[MAXS][16];
    int ns = 0, np = 0, nostiff = 0, nodamp = 0, nothread = 0, i, j;
    long n, N;
    const char *out = NULL;
    FILE *fo;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : "0";
#define OPT(name, var) else if (!strcmp(a, name)) { var = atof(v); i++; }
        if (!strcmp(a, "-selftest"))
            return selftest();
        OPT("-fs", fs) OPT("-dur", dur) OPT("-fmax", fmax) OPT("-L", base.L) OPT("-cycle", cyc)
        OPT("-xc", base.xc) OPT("-wc", base.wc) OPT("-kc", base.kc) OPT("-rc", base.rc)
        OPT("-xb", base.xb) OPT("-wb", base.wb) OPT("-kb", base.kb)
        OPT("-ae", ae) OPT("-taue", taue) OPT("-betae", betae) OPT("-xe", xe) OPT("-we", we)
        OPT("-s0", base.s0) OPT("-s1", base.s1) OPT("-s3", base.s3)
        else if (!strcmp(a, "-nostiff")) nostiff = 1;
        else if (!strcmp(a, "-nodamp")) nodamp = 1;
        else if (!strcmp(a, "-nothread")) nothread = 1;
        else if (!strcmp(a, "-nobridge")) base.bridge = 0;
        else if (!strcmp(a, "-s") && ns < MAXS) {
            char buf[64], *c1, *c2, *c3;
            snprintf(buf, sizeof buf, "%s", v);
            c1 = strchr(buf, ':');
            if (c1) *c1++ = 0;
            c2 = c1 ? strchr(c1, ':') : NULL;
            if (c2) *c2++ = 0;
            c3 = c2 ? strchr(c2, ':') : NULL;
            if (c3) *c3++ = 0;
            sfmax[ns] = c3 ? atof(c3) : 0;
            f1[ns] = atof(buf);
            snprintf(mat[ns], sizeof mat[ns], "%s", c1 && *c1 ? c1 : "steel");
            r[ns] = c2 ? atof(c2) : 0.14e-3;
            ns++, i++;
        } else if (!strcmp(a, "-p") && np < MAXP) {
            pl[np].s = atoi(v);
            pl[np].t = atof(strchr(v, ':') ? strchr(v, ':') + 1 : "0");
            pl[np].a = strchr(v, ':') && strchr(strchr(v, ':') + 1, ':') ? atof(strchr(strchr(v, ':') + 1, ':') + 1) : 1.0;
            np++, i++;
        } else if (a[0] != '-' && !out)
            out = a;
        else {
            fprintf(stderr, "tanpura_ref: unknown option %s (see the header of tools/reference/tanpura_ref.c)\n", a);
            return 2;
        }
    }
    if (!out || !ns) {
        fprintf(stderr, "usage: tanpura_ref [-s F1[:MAT[:R]] ...] [-p I:T ...] [options] OUT.f32 | -selftest\n");
        return 2;
    }
    if (!np)
        for (j = 0; j < ns; j++)
            pl[np].s = j, pl[np].t = 0, pl[np].a = 1.0, np++;
    if (nothread)
        base.kc = 0, base.rc = 0;
    if (nodamp)
        base.s0 = base.s1 = base.s3 = 0, base.rc = 0;    /* string and thread damping: matches the authors' ex3 (0.8 dB) */
    for (j = 0; j < ns; j++) {
        spar_t p = base;
        double t;
        material(mat[j], r[j], &p.rhoA, &p.EI);
        if (nostiff)
            p.EI = 0;
        p.T = pow(2 * p.L * f1[j], 2) * p.rhoA;         /* the tension for f1 (the stiffness's few cents ignored) */
        p.xe = xe >= 0 ? xe : 0.5 * p.L;
        p.we = we >= 0 ? we : p.L;
        string_init(&st[j], &p, fs, sfmax[j] > 0 ? sfmax[j] : fmax);
        t = st[j].M ? st[j].f[0] : 0;
        fprintf(stderr, "string %d: f1 %.3f Hz (target %.3f), %s r %.3g mm, T %.2f N, EI %.3g, %d modes (top %.0f Hz)\n",
                j, t, f1[j], mat[j], r[j] * 1e3, p.T, p.EI, st[j].M, st[j].M ? st[j].f[st[j].M - 1] : 0);
    }
    if (cyc > 0) {                                      /* the cycle repeated to the end */
        int n0 = np, k;
        for (k = 1; np < MAXP; k++) {
            int added = 0;
            for (j = 0; j < n0 && np < MAXP; j++)
                if (pl[j].t + k * cyc < dur)
                    pl[np].s = pl[j].s, pl[np].t = pl[j].t + k * cyc, pl[np].a = pl[j].a, np++, added++;
            if (!added)
                break;
        }
    }
    fo = fopen(out, "wb");
    if (!fo)
        return 1;
    N = (long)(dur * fs);
    for (n = 0; n < N; n++) {
        double t = n / fs, Fd = 0, Fn = 0, yb0 = 0;
        float fr[3];
        for (j = 0; j < ns; j++) {
            double fe = 0, d, nf, y;
            int k;
            for (k = 0; k < np; k++)
                if (pl[k].s == j)
                    fe += pl[k].a * 0.5 * (pluck_force(t - pl[k].t) + pluck_force(t + 1 / fs - pl[k].t));
            string_step(&st[j], fe, &d, &nf, &y);
            Fd += d, Fn += nf;                                                              /* (56) */
            if (j == 0)
                yb0 = y;
        }
        fr[0] = (float)Fd, fr[1] = (float)Fn, fr[2] = (float)yb0;
        fwrite(fr, sizeof fr, 1, fo);
    }
    fclose(fo);
    for (j = 0; j < ns; j++)
        fprintf(stderr, "string %d: deepest bridge compression %.3e m, in contact %.1f %% of the time\n", j,
                st[j].yb_min_rel, 100.0 * st[j].contacts / (double)N);
    return 0;
}
