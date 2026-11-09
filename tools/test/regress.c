/* regress — host regression tests of the audio path (tools/test/run_tests.sh), on the model of Felucca's
 * tests/regress.c:
 *   - golden: FNV-1a 64 of each scenario's samples against tools/test/golden.txt
 *   - cpu: host instructions per sample (perf_event_open) against tools/test/cpu_baseline.txt, +25 % allowed,
 *     and the estimated cost on the FM-1 against the budget (CLAUDE.md §6)
 *   - checks: the tone's frequency against an independent double-precision reference, its harmonics, no
 *     clicks (sample-to-sample step bounded by a sine's steepest slope), silence where there must be silence
 * GOLDEN_UPDATE=1 rewrites golden.txt (only for an intended change of the sound), BUDGET_UPDATE=1 the baseline. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include "tools/common/scen.h"

/* host instructions -> FM-1 cycles: target instructions ~0.8 x host (step 0 disassembly), ~1.5 cycles each
 * (an estimate until a device measurement: PROGRESS.md step 0) */
#define TARGET_CYCLES_PER_HOST_INSTR 1.2
#define FM1_CYCLES_PER_SAMPLE (360e6 / SCEN_FS_EXACT)
#define BUDGET_TAAL_REFTONE_MIXER_PCT 5.0     /* CLAUDE.md §6: taal, reftone and mixer together */
#define BUDGET_TABLA_PCT 30.0                 /* CLAUDE.md §6: the tabla, on the first core */

typedef struct {
    const char *name;
    double seconds;
    const char *ev[48];
    double f_expect;           /* the tone's frequency over [1 s, end) (0: not checked) */
    double f_max;              /* the highest frequency in the scenario, for the click bound (0: silent) */
    double silent_from;        /* from this time on, exact silence (<0: not checked) */
    int tanpura;               /* a tanpura scenario: its cost is estimated from its mode counts (see tanpura_pct) */
    int tabla;                 /* a tabla scenario: checked against the tabla's budget (with the mixer's 5 %) */
} scenario_t;

/* the expected frequencies, computed here in double from the ratios (not from tuning.c's table) */
static double ref_hz(double a4, int sa_cents, double ratio, int oct)
{
    return a4 * pow(2.0, (sa_cents - 900 + 1200.0 * log2(ratio) + 1200.0 * oct) / 1200.0);
}

static scenario_t SC[] = {
    {"idle", 3, {0}, 0, 0, 0, 0, 0},
    {"ref_sa_c4", 6, {"0:sa=C4", "0:ref.vol=100", "0:ref.on=1"}, 0, 0, -1, 0, 0},
    {"ref_sa_cs3", 6, {"0:sa=C#3", "0:ref.vol=100", "0:ref.on=1"}, 0, 0, -1, 0, 0},
    {"ref_sa_cs3_plus37", 6, {"0:sa=C#3+37", "0:ref.vol=100", "0:ref.on=1"}, 0, 0, -1, 0, 0},
    {"ref_a432_sa_d3", 6, {"0:a4=432", "0:sa=D3", "0:ref.vol=100", "0:ref.on=1"}, 0, 0, -1, 0, 0},
    {"ref_pa_tar", 6, {"0:sa=C#3", "0:ref.swara=Pa", "0:ref.oct=1", "0:ref.vol=100", "0:ref.on=1"}, 0, 0, -1, 0, 0},
    {"ref_ni_k_mandra", 6, {"0:sa=G2", "0:ref.swara=ni", "0:ref.oct=-1", "0:ref.vol=100", "0:ref.on=1"}, 0, 0, -1, 0, 0},
    {"ref_onoff", 4, {"0:sa=C#3", "0:ref.vol=100", "0.1:ref.on=1", "0.6:ref.on=0", "0.913:ref.on=1",
                      "1.37:ref.on=0", "1.8:ref.on=1", "2.2:ref.vol=10", "2.6:ref.vol=100", "2.9:master=500",
                      "3.1:master=4096", "3.3:ref.on=0"}, 0, 0, 3.4, 0, 0},
    {"ref_changes", 6, {"0:sa=C#3", "0:ref.vol=100", "0:ref.on=1", "0.5:ref.swara=Pa", "1.0:ref.oct=1",
                        "1.5:ref.swara=Ni", "2.0:ref.vol=30", "2.5:sa=E3", "3.0:ref.vol=100", "3.5:ref.on=0",
                        "3.7:ref.on=1", "4.0:master=1000", "4.5:mix.ref=50", "5.0:ref.on=0"}, 0, 0, 5.1, 0, 0},
    /* the tanpura (step 2b) */
    {"tan_A2_default", 8, {"0:sa=A2", "0:tan.on=1"}, 0, 0, -1, 1, 0},
    {"tan_Fs2_j0_Ma", 6, {"0:sa=F#2", "0:tan.jawari=0", "0:tan.first=Ma", "0:tan.on=1"}, 0, 0, -1, 1, 0},
    {"tan_G4_j100_Ni", 6, {"0:sa=G4", "0:tan.reg=0", "0:tan.jawari=100", "0:tan.first=Ni", "0:tan.on=1"}, 0, 0, -1, 1, 0},
    {"tan_retune", 6, {"0:sa=A2", "0:tan.on=1", "1:sa=A2+30", "1.5:tan.jawari=10", "2:tan.first=Ma", "2.5:sa=D3",
                       "3:tan.fine1=-20", "3.5:tan.jawari=90", "4:tan.cycle=25", "4.5:tan.human=0"}, 0, 0, -1, 1, 0},
    {"tan_onoff", 5, {"0:sa=A2", "0:tan.on=1", "3:tan.on=0"}, 0, 0, 3.4, 1, 0},
    /* the tabla (step 5) */
    {"tab_bols_A3", 9, {"0.1:tab.hit=Dha/127", "0.6:tab.hit=Dhin", "1.1:tab.hit=Tin/90", "1.6:tab.hit=Na/60",
                        "2.1:tab.hit=Tun/30", "2.6:tab.hit=Ge/110", "3.1:tab.hit=Ke", "3.4:tab.hit=Ge", "3.6:tab.hit=Te",
                        "3.7:tab.hit=Re", "3.8:tab.hit=Ti", "3.9:tab.hit=Ne", "4.2:tab.hit=Dhet", "4.7:tab.hit=Tas",
                        "5.2:tab.hit=Dha/127", "5.3:tab.hit=Dha/127", "5.4:tab.hit=Dha/127", "6:tab.hit=Ge/127",
                        "6.2:tab.hit=Na/127", "6.4:tab.hit=Tin/127"}, 0, 0, -1, 0, 1},
    /* the dense worst case: a stroke every 46 ms on both drums, the dayan at its highest step (Sa F#4: 370 Hz) */
    {"tab_drut_Fs4", 3, {"0:sa=F#4", "0.05:tab.hit=Dha/127", "0.1:tab.hit=Tin", "0.15:tab.hit=Dhin/127",
                         "0.2:tab.hit=Na", "0.25:tab.hit=Dha", "0.3:tab.hit=Tun", "0.35:tab.hit=Dha/127",
                         "0.4:tab.hit=Tin", "0.45:tab.hit=Dhin", "0.5:tab.hit=Ge", "0.55:tab.hit=Dha/127",
                         "0.6:tab.hit=Tin", "0.65:tab.hit=Dhin/127", "0.7:tab.hit=Na", "0.75:tab.hit=Dha",
                         "0.8:tab.hit=Tun", "0.85:tab.hit=Dha/127", "0.9:tab.hit=Tin", "0.95:tab.hit=Dhin",
                         "1.0:tab.hit=Dha/127", "1.05:tab.hit=Dhin"}, 0, 0, -1, 0, 1},
    {"tab_stop_retune", 4, {"0.1:tab.hit=Dhin/127", "0.5:sa=C4", "0.6:tab.hit=Tin", "0.8:sa=F#2+40",
                            "0.9:tab.hit=Dha", "1.0:tab.stop=1"}, 0, 0, 1.05, 0, 1},
    /* the taal engine driving the tabla (step 6) */
    {"taal_teentaal_160", 8, {"0:taal.mpm=160", "0.05:taal.on=1"}, 0, 0, -1, 0, 1},
    {"taal_switch_tempo", 12, {"0:taal.taal=Jhaptaal", "0:taal.mpm=90", "0.05:taal.on=1", "3:taal.mpm=200",
                               "4.3:taal.taal=Ektaal", "6:taal.mpm=60", "7:taal.mpm=61", "7.5:taal.mpm=300",
                               "8.1:taal.taal=Rupak", "9:taal.human=100", "10:taal.on=0"}, 0, 0, -1, 0, 1},
    {"taal_drut_ektaal_600", 4, {"0:sa=F#4", "0:taal.taal=Ektaal", "0:taal.mpm=600", "0.05:taal.on=1",
                                 "3:taal.on=0", "3.2:tab.stop=1"}, 0, 0, 3.3, 0, 1},
    {"taal_teentaal_tanpura", 8, {"0:sa=A3", "0:tan.on=1", "0:taal.mpm=120", "0.05:taal.on=1"}, 0, 0, -1, 1, 0},
};

/* the tanpura's cost on the FM-1, estimated from what it computes: 31.6 pi32v2 instructions per mode and string
 * step (the merged loop, 31, and the per-call setup, 20 / 32, from the disassembly), the strings at half rate, plus
 * the per-step contact solution (~90 a string), the body (24 sections, ~14 each) and the upsampler (~40), all at
 * 1.0..1.3 cycles an instruction (not measured yet: step 3) */
static void tanpura_pct(const tanpura_t *t, double *lo, double *hi)
{
    double steps = 0, instr;
    int j;
    for (j = 0; j < TANPURA_NSTR; j++)
        steps += t->str[j].c.M;
    steps /= TANPURA_OS;
    instr = steps * 31.6 + (4 * 90 + 24 * 14) / (double)TANPURA_OS + 40;
    *lo = instr * 1.0 / FM1_CYCLES_PER_SAMPLE * 100;
    *hi = instr * 1.3 / FM1_CYCLES_PER_SAMPLE * 100;
}
#define BUDGET_TANPURA_PCT 85.0                 /* CLAUDE.md §6: of the second core, all of it the tanpura's (step 2b) */
#define NSC ((int)(sizeof SC / sizeof SC[0]))

static int fails;
static void check(const char *sc, const char *what, int ok, const char *detail)
{
    printf("  %-4s %-20s %-34s %s\n", ok ? "ok" : "FAIL", sc, what, detail ? detail : "");
    if (!ok)
        fails++;
}

static uint64_t fnv1a(const int32_t *x, size_t n)
{
    uint64_t h = 1469598103934665603ull;
    size_t i;
    int b;
    for (i = 0; i < n; i++)
        for (b = 0; b < 4; b++)
            h = (h ^ (uint8_t)((uint32_t)x[i] >> (8 * b))) * 1099511628211ull;
    return h;
}

/* positive-going zero crossings of the left channel in [t0, t1), interpolated: the mean frequency */
static double zc_freq(const int32_t *lr, uint32_t frames, double t0, double t1)
{
    uint32_t a = (uint32_t)(t0 * SCEN_FS_EXACT), b = (uint32_t)(t1 * SCEN_FS_EXACT), i;
    double first = -1, last = -1;
    long n = 0;
    if (b > frames)
        b = frames;
    for (i = a + 1; i < b; i++) {
        double p = lr[2 * (i - 1)], c = lr[2 * i];
        if (p < 0 && c >= 0) {
            double t = (i - 1) + p / (p - c);
            if (first < 0)
                first = t;
            last = t;
            n++;
        }
    }
    return n > 1 ? (n - 1) * SCEN_FS_EXACT / (last - first) : 0;
}

/* power of frequency f in [t0, t0 + 1 s) of the left channel, Hann window (Goertzel at any frequency) */
static double tone_power(const int32_t *lr, double t0, double f)
{
    uint32_t a = (uint32_t)(t0 * SCEN_FS_EXACT), n = (uint32_t)SCEN_FS_EXACT, i;
    double w = 2 * M_PI * f / SCEN_FS_EXACT, re = 0, im = 0;
    for (i = 0; i < n; i++) {
        double win = 0.5 - 0.5 * cos(2 * M_PI * i / n), x = lr[2 * (a + i)] * win;
        re += x * cos(w * i);
        im -= x * sin(w * i);
    }
    return re * re + im * im;
}

static int perf_fd(void)
{
    struct perf_event_attr a;
    memset(&a, 0, sizeof a);
    a.type = PERF_TYPE_HARDWARE, a.size = sizeof a, a.config = PERF_COUNT_HW_INSTRUCTIONS;
    a.disabled = 1, a.exclude_kernel = 1, a.exclude_hv = 1;
    return (int)syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);
}

/* "name value" lines; returns the value for name, or -1 */
static double file_lookup(const char *path, const char *name, char *sval, size_t svn)
{
    FILE *f = fopen(path, "r");
    char line[256], n[128], v[64];
    double r = -1;
    if (!f)
        return -1;
    while (fgets(line, sizeof line, f))
        if (line[0] != '#' && sscanf(line, "%127s %63s", n, v) == 2 && !strcmp(n, name)) {
            if (sval)
                snprintf(sval, svn, "%s", v);
            r = atof(v);
        }
    fclose(f);
    return r;
}


/* the taal engine's timing (step 6), on its own: human 0, every taal at slow, medium and fast tempo; each event against
 * its exact instant in double (matra k, part j of n: (k + j / n) * 60 / mpm s); then a tempo change in the middle
 * of a matra: the position must go on from where it was (the next event at the time the two tempos give) */
static void taal_timing_checks(void)
{
    static const int MPM[3] = {20, 137, 600};
    char buf[160];
    int t, q;
    for (t = 0; t < taal_count(); t++)
        for (q = 0; q < 3; q++) {
            taal_t m;
            taal_event_t ev[16];
            double worst = 0, spm = 60.0 / MPM[q] * SCEN_FS_EXACT;     /* samples a matra */
            uint32_t at = 0, nev = 0;
            int matra = 0, part = 0, k, cycles = 0;
            taal_init(&m, 0);
            taal_set_param(&m, TAAL_P_HUMAN, 0);
            taal_set_param(&m, TAAL_P_TAAL, t);
            taal_set_param(&m, TAAL_P_MPM, MPM[q]);
            taal_set_param(&m, TAAL_P_ON, 1);
            /* the expected sequence: walk the matras with the same table the engine reads (taal_matra_parts) */
            while (cycles < 2) {
                int ne = taal_advance(&m, DSP_BLOCK, ev, 16);
                for (k = 0; k < ne; k++) {
                    double exact, d;
                    int n;
                    /* skip rests: advance (matra, part) to the next sounding part */
                    for (;;) {
                        n = taal_matra_parts(t, matra);
                        if (taal_part_bol(t, matra, part) >= 0)
                            break;
                        if (++part >= n) part = 0, matra = (matra + 1) % taal_matras(t), cycles += matra == 0;
                    }
                    exact = (cycles * taal_matras(t) + matra + (double)part / n) * spm;
                    d = fabs((double)(at + ev[k].offset) - exact);
                    if (d > worst) worst = d;
                    if (ev[k].stroke != taal_part_bol(t, matra, part)) worst = 1e9;
                    nev++;
                    if (++part >= n) part = 0, matra = (matra + 1) % taal_matras(t), cycles += matra == 0;
                }
                at += DSP_BLOCK;
                if (at > 4000000u) break;
            }
            snprintf(buf, sizeof buf, "%u events, worst %.2f samples off", nev, worst);
            if (q == 1 || worst > 1.0) {
                char nm[40];
                snprintf(nm, sizeof nm, "taal_%s_%d", taal_name(t), MPM[q]);
                check(nm, "timing within 1 sample", worst <= 1.0, buf);
            }
        }
    {   /* tempo change at 0.3 of matra 5 of Teentaal: 120 -> 240 */
        taal_t m;
        taal_event_t ev[16];
        uint32_t at = 0, change = (uint32_t)(5.3 * 0.5 * SCEN_FS_EXACT) / DSP_BLOCK * DSP_BLOCK, got = 0;
        double expect;
        taal_init(&m, 0);
        taal_set_param(&m, TAAL_P_HUMAN, 0);
        taal_set_param(&m, TAAL_P_ON, 1);
        while (at < 10u * (uint32_t)SCEN_FS_EXACT && !got) {
            int ne, k;
            if (at == change)
                taal_set_param(&m, TAAL_P_MPM, 240);
            ne = taal_advance(&m, DSP_BLOCK, ev, 16);
            for (k = 0; k < ne; k++)
                if (at > change && !got)
                    got = at + ev[k].offset;
            at += DSP_BLOCK;
        }
        /* matra 6 starts after the rest of matra 5 at the new tempo */
        expect = change + (6.0 - change / (0.5 * SCEN_FS_EXACT)) * 0.25 * SCEN_FS_EXACT;
        snprintf(buf, sizeof buf, "next event at %u, expected %.1f", got, expect);
        check("taal_tempo_change", "continuous position", fabs(got - expect) <= 1.0, buf);
    }
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "tools/test";
    char golden[512], baseline[512];
    int upd_golden = getenv("GOLDEN_UPDATE") && atoi(getenv("GOLDEN_UPDATE"));
    int upd_budget = getenv("BUDGET_UPDATE") && atoi(getenv("BUDGET_UPDATE"));
    FILE *gout = NULL, *bout = NULL;
    int fd = perf_fd(), s;
    double worst_pct = 0;
    snprintf(golden, sizeof golden, "%s/golden.txt", dir);
    snprintf(baseline, sizeof baseline, "%s/cpu_baseline.txt", dir);
    if (fd < 0)
        printf("  (perf_event_open unavailable: no CPU checks)\n");
    if (upd_golden && (gout = fopen(golden, "w")))
        fprintf(gout, "# golden renders (tools/test/regress.c): name, FNV-1a 64 of the samples.\n"
                      "# Rewritten by GOLDEN_UPDATE=1 tools/test/run_tests.sh -- only for an intended change of the sound.\n");
    if (upd_budget && fd >= 0 && (bout = fopen(baseline, "w")))
        fprintf(bout, "# host instructions per sample of engine_render (tools/test/regress.c), gcc -O2 under WSL2.\n"
                      "# The check allows +25 %%. Rewritten by BUDGET_UPDATE=1 tools/test/run_tests.sh.\n");

    /* the expected frequencies (independent of tuning.c) */
    SC[1].f_expect = ref_hz(440, 0, 1, 0);
    SC[2].f_expect = ref_hz(440, -1100, 1, 0);
    SC[3].f_expect = ref_hz(440, -1063, 1, 0);
    SC[4].f_expect = ref_hz(432, -1000, 1, 0);
    SC[5].f_expect = ref_hz(440, -1100, pow(2, 7 / 12.0), 1);
    SC[6].f_expect = ref_hz(440, -1700, pow(2, 10 / 12.0), -1);
    for (s = 1; s <= 6; s++)
        SC[s].f_max = SC[s].f_expect;
    SC[7].f_max = SC[2].f_expect;                   /* Sa C#3 throughout */
    SC[8].f_max = ref_hz(440, -800, pow(2, 11 / 12.0), 1);   /* Ni, tar, Sa = E3: the highest the scenario reaches */

    for (s = 0; s < NSC; s++) {
        static scen_run_t r;
        scen_ev_t ev[48];
        const scenario_t *sc = &SC[s];
        uint32_t frames = scen_frames(sc->seconds), f;
        int32_t *lr = malloc((size_t)frames * 2u * sizeof *lr);
        int nev = 0, i;
        long long instr = 0;
        char buf[160], name[128], gv[64];
        uint64_t h;
        while (nev < 48 && sc->ev[nev]) {
            if (scen_parse(sc->ev[nev], &ev[nev]))
                return 2;
            nev++;
        }
        scen_start(&r, ev, nev);
        if (fd >= 0) {
            ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        }
        for (f = 0; f < frames; f += SCEN_PLAT_BLOCK) {
            r.frame = f;
            scen_apply_due(&r);
            if (fd >= 0) ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
            engine_render(&r.eng, lr + 2u * f, SCEN_PLAT_BLOCK, r.master);
            if (fd >= 0) ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        }
        if (fd >= 0 && read(fd, &instr, sizeof instr) != sizeof instr)
            instr = 0;

        /* golden */
        h = fnv1a(lr, 2u * (size_t)frames);
        snprintf(name, sizeof name, "render/%s", sc->name);
        if (gout) {
            fprintf(gout, "%s %016llx\n", name, (unsigned long long)h);
        } else {
            gv[0] = 0;
            file_lookup(golden, name, gv, sizeof gv);
            snprintf(buf, sizeof buf, "%016llx%s%s", (unsigned long long)h, gv[0] ? " vs " : " (no golden)", gv);
            check(sc->name, "golden hash", gv[0] && strtoull(gv, 0, 16) == h, buf);
        }

        /* cpu */
        if (fd >= 0 && instr > 0) {
            double ips = (double)instr / frames, base, pct = ips * TARGET_CYCLES_PER_HOST_INSTR / FM1_CYCLES_PER_SAMPLE * 100;
            snprintf(name, sizeof name, "cpu/%s", sc->name);
            if (bout)
                fprintf(bout, "%s %.1f\n", name, ips);
            base = file_lookup(baseline, name, 0, 0);
            snprintf(buf, sizeof buf, "%.1f instr/sample (baseline %.1f), FM-1 est. %.2f %%", ips, base, pct);
            if (!bout)
                check(sc->name, "cpu vs baseline +25 %", base > 0 && ips <= base * 1.25, buf);
            if (sc->tanpura) {
                double lo, hi;
                tanpura_pct(&r.eng.tanpura, &lo, &hi);
                snprintf(buf, sizeof buf, "est. %.0f-%.0f %% of the second core (budget %.0f %%)", lo, hi,
                         BUDGET_TANPURA_PCT);
                check(sc->name, "tanpura cpu vs budget", hi <= BUDGET_TANPURA_PCT, buf);
            } else if (sc->tabla) {
                snprintf(buf, sizeof buf, "est. %.2f %% of a core with the mixer (budget %.0f + %.0f %%)", pct,
                         BUDGET_TABLA_PCT, BUDGET_TAAL_REFTONE_MIXER_PCT);
                check(sc->name, "tabla cpu vs budget", pct <= BUDGET_TABLA_PCT + BUDGET_TAAL_REFTONE_MIXER_PCT, buf);
                if (r.eng.tabla.dropped)
                    check(sc->name, "no hits dropped", 0, 0);
            } else {
                snprintf(buf, sizeof buf, "est. %.2f %% of a core (budget %.0f %%)", pct, BUDGET_TAAL_REFTONE_MIXER_PCT);
                check(sc->name, "cpu vs budget", pct <= BUDGET_TAAL_REFTONE_MIXER_PCT, buf);
                if (pct > worst_pct)
                    worst_pct = pct;
            }
        }

        /* frequency, harmonics */
        if (sc->f_expect > 0) {
            double fm = zc_freq(lr, frames, 1.0, sc->seconds - 0.05);
            double cents = 1200 * log2(fm / sc->f_expect), p1 = tone_power(lr, 2.0, sc->f_expect), worst = -999;
            int k;
            snprintf(buf, sizeof buf, "%.4f Hz vs %.4f Hz (%+.4f cents)", fm, sc->f_expect, cents);
            check(sc->name, "frequency within 0.02 cents", fabs(cents) < 0.02, buf);
            for (k = 2; k <= 6; k++)
                if (k * sc->f_expect < SCEN_FS_EXACT / 2) {
                    double d = 10 * log10(tone_power(lr, 2.0, k * sc->f_expect) / p1 + 1e-30);
                    if (d > worst)
                        worst = d;
                }
            snprintf(buf, sizeof buf, "worst of harmonics 2..6: %.1f dB", worst);
            check(sc->name, "harmonics below -90 dB", worst < -90, buf);
        }

        /* clicks: no step larger than the steepest sine at f_max and the scenario's peak allows (+1 %, +2 LSB) */
        if (sc->f_max > 0) {
            double peak = 0, bound, worst = 0;
            uint32_t at = 0;
            for (f = 0; f < 2u * frames; f++)
                if (fabs((double)lr[f]) > peak)
                    peak = fabs((double)lr[f]);
            bound = 2 * M_PI * sc->f_max / SCEN_FS_EXACT * peak * 1.01 + 2;
            for (f = 1; f < frames; f++) {
                double d = fabs((double)lr[2 * f] - lr[2 * (f - 1)]);
                if (d > worst)
                    worst = d, at = f;
            }
            snprintf(buf, sizeof buf, "largest step %.0f at %.3f s, bound %.0f", worst, at / SCEN_FS_EXACT, bound);
            check(sc->name, "no clicks", worst <= bound, buf);
        }

        /* silence */
        if (sc->silent_from >= 0) {
            uint32_t a = (uint32_t)(sc->silent_from * SCEN_FS_EXACT), bad = 0;
            for (f = 2u * a; f < 2u * frames; f++)
                bad += lr[f] != 0;
            snprintf(buf, sizeof buf, "from %.2f s: %u non-zero samples", sc->silent_from, bad);
            check(sc->name, "silence", bad == 0, buf);
        }
        snprintf(buf, sizeof buf, "%u", r.eng.mixer.clips);
        check(sc->name, "no clipping", r.eng.mixer.clips == 0, buf);
        (void)i;
        free(lr);
    }
    taal_timing_checks();
    if (gout)
        fclose(gout), printf("  golden rewritten: %s\n", golden);
    if (bout)
        fclose(bout), printf("  cpu baseline rewritten: %s\n", baseline);
    printf("%s (%d failed); worst estimated cost %.2f %% of a core\n", fails ? "REGRESS FAILED" : "regress ok", fails, worst_pct);
    return fails ? 1 : 0;
}
