/* tanpura_stress — step 2b robustness of the float tanpura (tanpura/), host:
 *   1. every Sa from F#2 to G4 (6 values) x jawari 0, 50, 100 x first string Pa, Ma, Ni: 40 s each, the output finite,
 *      no clipping, the loudness steady (the last 10 s within 2x the peak of the first 10 s: nothing grows)
 *   2. one string, one pluck, 60 s, at both ends of the Sa range and of the thread: the modal energy sampled every
 *      0.1 s after the pluck never rises (beyond float noise) and ends below 1e-6 of its start
 *   3. retuning while it sounds (Sa, the first string, the thread, the fine tuning, every 0.4 s): finite, no clipping,
 *      no jump between samples larger than half full scale
 *   4. ten minutes at the defaults: finite, no clipping, the peak of the last minute within 2x of the first
 * Exit 0 when everything passes.   build-host/tanpura_stress [quick] */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "tanpura/tanpura.h"

static int fails;
static void check(const char *what, int ok, const char *detail)
{
    printf("  %-4s %-58s %s\n", ok ? "ok" : "FAIL", what, detail);
    if (!ok)
        fails++;
}

typedef struct { double peak_first, peak_last, maxstep; long clipped, nonfinite; } stats_t;

/* render seconds of the tanpura into stats; t_first/t_last: the windows for the peaks */
static void run(tanpura_t *t, double secs, double t_first, double t_last, stats_t *st,
                void (*every)(tanpura_t *, long), long every_n)
{
    int32_t out[DSP_BLOCK];
    long n, N = (long)(secs * DSP_FS) / DSP_BLOCK, k;
    int32_t prev = 0;
    memset(st, 0, sizeof *st);
    for (n = 0; n < N; n++) {
        double t0 = (double)n * DSP_BLOCK / DSP_FS;
        if (every && n % every_n == 0)
            every(t, n / every_n);
        memset(out, 0, sizeof out);
        tanpura_take(t);
        tanpura_render(t, out, DSP_BLOCK);
        for (k = 0; k < DSP_BLOCK; k++) {
            double v = fabs((double)out[k]), step = fabs((double)out[k] - (double)prev);
            if (v >= DSP_Q23_MAX)
                st->clipped++;
            if (t0 < t_first && v > st->peak_first)
                st->peak_first = v;
            if (t0 >= t_last && v > st->peak_last)
                st->peak_last = v;
            if (step > st->maxstep)
                st->maxstep = step;
            prev = out[k];
        }
    }
    {
        int j, i;
        for (j = 0; j < TANPURA_NSTR; j++)
            for (i = 0; i < t->str[j].c.M; i++)
                if (!isfinite(t->str[j].m[i].y) || !isfinite(t->str[j].m[i].q))
                    st->nonfinite++;
    }
}

static int retune_step;
static void retune(tanpura_t *t, long k)
{
    static const int32_t SA[] = {-1800, -300, -1500, 700, -1000, 0};
    tanpura_set_param(t, DSP_P_SA_CENTS, SA[k % 6]);
    tanpura_set_param(t, TANPURA_P_FIRST, (int32_t)(k * 5 % 12));
    tanpura_set_param(t, TANPURA_P_JAWARI, (int32_t)(k * 37 % 101));
    tanpura_set_param(t, TANPURA_P_FINE + (int)(k % 4), (int32_t)((k * 13) % 101) - 50);
    retune_step++;
}

int main(int argc, char **argv)
{
    static tanpura_t t;
    static const int32_t SA[] = {-1800, -1500, -1000, -300, 0, 700};
    static const char *const SAN[] = {"F#2", "A2", "D3", "A3", "C4", "G4"};
    static const int32_t JW[] = {0, 50, 100}, FIRST[] = {SW_PA, SW_MA, SW_NI};
    int quick = argc > 1 && !strcmp(argv[1], "quick");
    char buf[160];
    int a, b, c;
    stats_t st;
    /* 1 */
    printf("1. range x thread x first string, %d s each\n", quick ? 12 : 40);
    for (a = 0; a < 6; a++)
        for (b = 0; b < 3; b++)
            for (c = 0; c < 3; c++) {
                double secs = quick ? 12 : 40;
                tanpura_init(&t, NULL);
                tanpura_set_param(&t, DSP_P_SA_CENTS, SA[a]);
                tanpura_set_param(&t, TANPURA_P_JAWARI, JW[b]);
                tanpura_set_param(&t, TANPURA_P_FIRST, FIRST[c]);
                tanpura_set_param(&t, TANPURA_P_ON, 1);
                run(&t, secs, secs / 4, secs * 3 / 4, &st, NULL, 1);
                snprintf(buf, sizeof buf, "peak %.1f dBFS first, %.1f dBFS last; %ld clipped, %ld non-finite",
                         20 * log10(st.peak_first / DSP_Q23_MAX + 1e-12), 20 * log10(st.peak_last / DSP_Q23_MAX + 1e-12),
                         st.clipped, st.nonfinite);
                {
                    char w[96];
                    snprintf(w, sizeof w, "Sa %s, jawari %d, first %s", SAN[a], JW[b], tuning_swara_name(FIRST[c]));
                    check(w, st.clipped == 0 && st.nonfinite == 0 && st.peak_last < 2 * st.peak_first && st.peak_first > 0, buf);
                }
            }
    /* 2 */
    printf("2. one string, one pluck, 60 s: the energy never rises\n");
    for (a = 0; a < 6; a += 5)
        for (b = 0; b < 3; b += 2) {
            static tstring_t s;
            static tstring_coefs_t co;
            tstring_par_t p;
            float fd[32], fn[32];
            double e0 = 0, prev = 0, worst = 0, last = 0;
            long n, N = (long)(60.0 * TANPURA_FS) / 32;
            tanpura_init(&t, NULL);
            tanpura_set_param(&t, DSP_P_SA_CENTS, SA[a]);
            tanpura_set_param(&t, TANPURA_P_JAWARI, JW[b]);
            p = (tstring_par_t){0};
            /* the Sa string as the tanpura builds it */
            p.L = 1.1f, p.rhoA = 7850.0f * 3.14159265f * 0.16e-3f * 0.16e-3f;
            p.EI = 2.0e11f * 0.25f * 3.14159265f * 0.16e-3f * 0.16e-3f * 0.16e-3f * 0.16e-3f;
            p.f1 = tuning_hz(&t.tuning, 0.0f), p.s0 = 0.4f, p.s1 = 6.5e-3f, p.s3 = 5.0e-6f;
            p.xb = 10e-3f, p.wb = 2e-3f, p.kb = 4.39e8f, p.xc = 4e-3f + 4e-3f * (float)JW[b] * 0.01f, p.wc = 1.2e-3f;
            p.kc = 1.2e5f, p.rc = 0.8f, p.xe = 0.385f, p.we = 0.02f, p.fmax = 8000.0f, p.maxm = TSTRING_MAXM;
            tstring_coefs(&co, &p, TANPURA_FS);
            tstring_init(&s, &co);
            tstring_pluck(&s, -0.4f, 0.05f, TANPURA_FS);
            for (n = 0; n < N; n++) {
                memset(fd, 0, sizeof fd); memset(fn, 0, sizeof fn);
                tstring_run(&s, fd, fn, 32);
                if ((n * 32) % (long)(0.1 * TANPURA_FS) < 32 && n * 32 > (long)(0.06 * TANPURA_FS)) {
                    double e = tstring_energy(&s, TANPURA_FS);
                    if (e0 == 0)
                        e0 = e;
                    if (prev > 0 && (e - prev) / e0 > worst)
                        worst = (e - prev) / e0;
                    prev = e, last = e;
                }
            }
            snprintf(buf, sizeof buf, "largest rise %.1e of the start; end %.1e of the start", worst, last / e0);
            {
                char w[96];
                snprintf(w, sizeof w, "Sa %s, jawari %d", SAN[a], JW[b]);
                check(w, isfinite(last) && worst < 1e-4 && last / e0 < 1e-6, buf);
            }
        }
    /* 3 */
    printf("3. retuning every 0.4 s while it sounds\n");
    tanpura_init(&t, NULL);
    tanpura_set_param(&t, TANPURA_P_ON, 1);
    retune_step = 0;
    run(&t, quick ? 10 : 30, 1, 1e9, &st, retune, (long)(0.4 * DSP_FS / DSP_BLOCK));
    snprintf(buf, sizeof buf, "%d retunes; %ld clipped, %ld non-finite; largest step %.2f of full scale",
             retune_step, st.clipped, st.nonfinite, st.maxstep / DSP_Q23_MAX);
    check("retune", st.clipped == 0 && st.nonfinite == 0 && st.maxstep < 0.5 * DSP_Q23_MAX, buf);
    /* 4 */
    if (!quick) {
        printf("4. ten minutes at the defaults (Sa A2)\n");
        tanpura_init(&t, NULL);
        tanpura_set_param(&t, DSP_P_SA_CENTS, -1500);
        tanpura_set_param(&t, TANPURA_P_ON, 1);
        run(&t, 600, 60, 540, &st, NULL, 1);
        snprintf(buf, sizeof buf, "peak %.1f dBFS first minute, %.1f dBFS last; %ld clipped, %ld non-finite",
                 20 * log10(st.peak_first / DSP_Q23_MAX), 20 * log10(st.peak_last / DSP_Q23_MAX), st.clipped, st.nonfinite);
        check("10 minutes", st.clipped == 0 && st.nonfinite == 0 && st.peak_last < 2 * st.peak_first, buf);
    }
    printf("%s (%d failed)\n", fails ? "STRESS FAILED" : "stress ok", fails);
    return fails ? 1 : 0;
}
