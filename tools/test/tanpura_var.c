/* tanpura_var — host tool for step 2b: the tanpura module (tanpura/) with a mode limit, its drone as a WAV, and the
 * host instructions per sample of tanpura_render (perf_event_open).
 *   build-host/tanpura_var FMAX[,F1,F2,F3] MAXM SA_CENTS DUR OUT.wav [JAWARI [FIRST_SWARA [EQ]]]
 *   (one FMAX for every string, or one per string: first, Sa, Sa, kharaj) */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include "tanpura/tanpura.h"
#include "tools/common/scen.h"

static int perf_fd(void)
{
    struct perf_event_attr a;
    memset(&a, 0, sizeof a);
    a.type = PERF_TYPE_HARDWARE, a.size = sizeof a, a.config = PERF_COUNT_HW_INSTRUCTIONS;
    a.disabled = 1, a.exclude_kernel = 1, a.exclude_hv = 1;
    return (int)syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);
}

int main(int argc, char **argv)
{
    static tanpura_t t;
    tanpura_config_t cfg;
    int32_t *lr, mono[DSP_BLOCK];
    long n, N, k;
    long long instr = 0;
    int j, fd = perf_fd();
    if (argc < 6) {
        fprintf(stderr, "usage: tanpura_var FMAX MAXM SA_CENTS DUR OUT.wav [JAWARI [FIRST_SWARA]]\n");
        return 2;
    }
    tanpura_default_config(&cfg);
    {
        char *q = argv[1];
        for (j = 0; j < TANPURA_NSTR; j++) {
            cfg.fmax[j] = (float)strtod(q, &q);
            if (*q == ',') q++;
            else if (j == 0) { int k; for (k = 1; k < TANPURA_NSTR; k++) cfg.fmax[k] = cfg.fmax[0]; break; }
        }
    }
    for (j = 0; j < TANPURA_NSTR; j++)
        cfg.maxm[j] = atoi(argv[2]);
    if (argc > 8)
        cfg.eq = atoi(argv[8]);
    tanpura_init(&t, &cfg);
    tanpura_set_param(&t, DSP_P_SA_CENTS, atoi(argv[3]));
    if (argc > 6) tanpura_set_param(&t, TANPURA_P_JAWARI, atoi(argv[6]));
    if (argc > 7) tanpura_set_param(&t, TANPURA_P_FIRST, atoi(argv[7]));
    tanpura_set_param(&t, TANPURA_P_ON, 1);
    N = (long)(atof(argv[4]) * DSP_FS) / DSP_BLOCK * DSP_BLOCK;
    lr = malloc((size_t)N * 2 * sizeof *lr);
    if (fd >= 0) ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    for (n = 0; n < N; n += DSP_BLOCK) {
        memset(mono, 0, sizeof mono);
        if (fd >= 0) ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        tanpura_take(&t);
        tanpura_render(&t, mono, DSP_BLOCK);
        if (fd >= 0) ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        for (k = 0; k < DSP_BLOCK; k++)
            lr[2 * (n + k)] = lr[2 * (n + k) + 1] = mono[k];
    }
    if (fd >= 0 && read(fd, &instr, sizeof instr) != sizeof instr)
        instr = 0;
    wav_write(argv[5], lr, (uint32_t)N);
    printf("modes");
    for (j = 0; j < TANPURA_NSTR; j++)
        printf(" %d", t.str[j].c.M);
    printf("  host_instr_per_sample %.1f\n", (double)instr / (double)N);
    return 0;
}
