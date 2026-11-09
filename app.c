/* app.c — glue between fm1-base and the project: the audio path (mixer/engine) on two cores, and the UI (ui/).
 * No DSP here: the engine is the same code the host tools run. Compiled inside fm1-base's unity build.
 *
 * Two cores (step 3, DECISIONS.md): the second core renders the tanpura's NEXT block while the first plays this one,
 * so the first core never waits for it (one block of latency, 5.8 ms, for a drone). In each render, the first core:
 *   1. collects the job handed over in the last render (long done: the 8 ms timeout counts from this wait),
 *   2. takes the tanpura's pending starts and retunes (engine_tanpura_take) while the second core is idle,
 *   3. hands over the next block's job,
 *   4. renders the rest and mixes this block's tanpura (rendered during the last block).
 * Without the second core (it failed: cpu2_fallback), the first core renders the tanpura itself, and the screen says so. */
#include "mixer/engine.h"

/* the second core's calls, or "not there" in a build without it (fm1.toml cpu2, or FM1B_CPU2=0: one core) */
#if defined(PLAT_HOST) || FM1B_CPU2
#define app_cpu2_set plat_cpu2_set
#define app_cpu2_on plat_cpu2_on
#define app_cpu2_run plat_cpu2_run
#define app_cpu2_wait plat_cpu2_wait
#define app_cpu2_barred plat_cpu2_barred
#define app_cpu2_gave_up plat_cpu2_gave_up
#else
static int app_cpu2_set(int on) { (void)on; return -1; }
static int app_cpu2_on(void) { return 0; }
static int app_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn, (void)arg; return 0; }
static int app_cpu2_wait(void) { return 0; }
static int app_cpu2_barred(void) { return 0; }
static int app_cpu2_gave_up(void) { return 0; }
#endif

static engine_t eng;
static int32_t tanbuf[2][ENGINE_MAXN];     /* the tanpura's blocks: one played, one being rendered */
static uint32_t tan_ready;                 /* the buffer holding this block's tanpura */
static int job_out;                        /* a job was handed over and not yet collected */
static int cpu2_started;
static volatile int app_hush;              /* 1: MASTER to silence (the mixer's 20 ms ramp), before a flash write */
/* the measurement (step 3): the tanpura job's time, in 24 MHz ticks, on whichever core ran it */
static volatile uint32_t job_ticks, job_ticks_max, jobs_done, jobs_inline, renders;
#ifdef PLAT_HOST
static volatile int32_t host_peak;         /* the simulator's checks only: the loudest sample of the last ~0.5 s */
static int32_t host_pk[88];                /* (each block's: 88 blocks = 0.51 s) */
static uint32_t host_pk_i;
#endif

static void tanpura_job(uint32_t idx)
{
    uint32_t t0 = plat_ticks24(), d;
    engine_tanpura_render(&eng, tanbuf[idx], APP_BLOCK);
    d = plat_ticks24() - t0;
    job_ticks = d;
    if (d > job_ticks_max)
        job_ticks_max = d;
    jobs_done++;
}

/* the job's share of a block, in tenths of a percent: a sample is exactly 544 ticks of the 24 MHz clock
 * (44117.647 Hz = 24 MHz / 544), a block APP_BLOCK * 544 (32-bit: no 64-bit division on the FM-1) */
static uint32_t job_permille(uint32_t ticks)
{
    return ticks * 1000u / (APP_BLOCK * 544u);
}

#include "ui/ui.c"

void app_init(void)
{
    engine_init(&eng);
    ui_init_state(&eng);
    cpu2_started = app_cpu2_set(1) == 0;   /* (only here: at power-on, before the audio and the timers) */
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t next;
    renders++;
    if (app_hush)
        master_q12 = 0;
    if (n != APP_BLOCK) {                   /* (never on the FM-1: one place, as the host tools do) */
        engine_render(&eng, lr, n, master_q12);
        return;
    }
    if (job_out) {
        if (app_cpu2_wait() != 0)          /* given up (fallback): the job was dropped, do it here */
            tanpura_job(tan_ready ^ 1u), jobs_inline++;
        job_out = 0;
        tan_ready ^= 1u;                    /* the block the job rendered is this one */
    }
    engine_tanpura_take(&eng);              /* the second core is idle */
    next = tan_ready ^ 1u;
    if (app_cpu2_on() && app_cpu2_run(tanpura_job, next))
        job_out = 1;
    else {                                  /* one core: render the next block here, now */
        tanpura_job(next);
        jobs_inline++;
        tan_ready = next;
    }
    engine_render_with(&eng, lr, n, master_q12, tanbuf[tan_ready]);
#ifdef PLAT_HOST
    {
        uint32_t i;
        int32_t pk = 0;
        for (i = 0; i < 2u * n; i++)
            if (lr[i] > pk)
                pk = lr[i];
        host_pk[host_pk_i++ % 88u] = pk;
        for (i = 0, pk = 0; i < 88u; i++)
            if (host_pk[i] > pk)
                pk = host_pk[i];
        host_peak = pk;
    }
#endif
}

void app_panic(void) { engine_panic(&eng); }
void app_ui_init(void) { ui_draw_all(); }
void app_frame(void) { ui_frame(); }
void app_input_only(void) {}
void app_say(const char *a, const char *b) { ui_say(a, b); }

#ifdef PLAT_HOST                           /* the host simulator's checks only (strcmp: not on the FM-1) */
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "ref_on")) *got = (int)engine_get_param(&eng, ENG_REFTONE, REFTONE_P_ON);
    else if (!strcmp(what, "swara")) *got = (int)engine_get_param(&eng, ENG_REFTONE, REFTONE_P_SWARA);
    else if (!strcmp(what, "octave")) *got = (int)engine_get_param(&eng, ENG_REFTONE, REFTONE_P_OCTAVE);
    else if (!strcmp(what, "volume")) *got = (int)engine_get_param(&eng, ENG_REFTONE, REFTONE_P_VOLUME);
    else if (!strcmp(what, "sa_cents")) *got = (int)engine_get_param(&eng, ENG_ALL, DSP_P_SA_CENTS);
    else if (!strcmp(what, "a4_dhz")) *got = (int)engine_get_param(&eng, ENG_ALL, DSP_P_A4_DHZ);
    else if (!strcmp(what, "tan_on")) *got = (int)engine_get_param(&eng, ENG_TANPURA, TANPURA_P_ON);
    else if (!strcmp(what, "tan_sa")) *got = (int)engine_get_param(&eng, ENG_TANPURA, TANPURA_P_SA_NOW);
    else if (!strcmp(what, "jawari")) *got = (int)engine_get_param(&eng, ENG_TANPURA, TANPURA_P_JAWARI);
    else if (!strcmp(what, "first")) *got = (int)engine_get_param(&eng, ENG_TANPURA, TANPURA_P_FIRST);
    else if (!strcmp(what, "register")) *got = (int)engine_get_param(&eng, ENG_TANPURA, TANPURA_P_REGISTER);
    else if (!strcmp(what, "tan_vol")) *got = (int)engine_get_param(&eng, ENG_MIXER, MIX_P_LEVEL + MIX_TANPURA);
    else if (!strcmp(what, "taal_on")) *got = (int)engine_get_param(&eng, ENG_TAAL, TAAL_P_ON);
    else if (!strcmp(what, "taal")) *got = (int)engine_get_param(&eng, ENG_TAAL, TAAL_P_TAAL);
    else if (!strcmp(what, "taal_now")) *got = (int)engine_get_param(&eng, ENG_TAAL, TAAL_P_NOW);
    else if (!strcmp(what, "mpm")) *got = (int)engine_get_param(&eng, ENG_TAAL, TAAL_P_MPM);
    else if (!strcmp(what, "matra")) *got = (int)engine_get_param(&eng, ENG_TAAL, TAAL_P_MATRA);
    else if (!strcmp(what, "tabla_human")) *got = (int)engine_get_param(&eng, ENG_TAAL, TAAL_P_HUMAN);
    else if (!strcmp(what, "tab_vol")) *got = (int)engine_get_param(&eng, ENG_MIXER, MIX_P_LEVEL + MIX_TABLA);
    else if (!strcmp(what, "unsaved")) *got = ui_unsaved;
    else if (!strcmp(what, "loaded")) *got = ui_loaded;
    else if (!strcmp(what, "fine0")) *got = (int)engine_get_param(&eng, ENG_TANPURA, TANPURA_P_FINE);
    else if (!strcmp(what, "kt_key")) *got = ui_kt_key;
    else if (!strcmp(what, "page")) *got = ui_page;
    else if (!strcmp(what, "cpu2")) *got = cpu2_started && app_cpu2_on();
    else if (!strcmp(what, "jobs")) *got = (int)jobs_done;
#ifdef PLAT_HOST
    else if (!strcmp(what, "peak")) *got = (int)host_peak;
#endif
    else return 0;
    return 1;
}
#endif
