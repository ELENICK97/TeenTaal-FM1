/* tstring_dump — host check of the float string (tanpura/tstring.c) against the double reference
 * (tools/reference/tanpura_ref.c): one string, one pluck, raw float32 frames [F_d, F_n, 0] like the reference's.
 *   build-host/tstring_dump F1 L R_MM MAT(steel|bronze) XC XE WE S0 RC MAXM FMAX DUR FS OUT.f32 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastmath.h"
#include "tanpura/tstring.h"

int main(int argc, char **argv)
{
    static tstring_t s;
    static tstring_coefs_t c;
    tstring_par_t p;
    float fs, r, rho, E, fd[64], fn[64];
    long n, N;
    FILE *fo;
    if (argc != 15) {
        fprintf(stderr, "usage: tstring_dump F1 L R_MM MAT XC XE WE S0 RC MAXM FMAX DUR FS OUT.f32\n");
        return 2;
    }
    memset(&p, 0, sizeof p);
    p.f1 = (float)atof(argv[1]); p.L = (float)atof(argv[2]); r = (float)atof(argv[3]) * 1e-3f;
    rho = strcmp(argv[4], "bronze") ? 7850.0f : 8800.0f;
    E = strcmp(argv[4], "bronze") ? 2.0e11f : 1.1e11f;
    p.rhoA = rho * FM_PI * r * r; p.EI = E * 0.25f * FM_PI * r * r * r * r;
    p.xc = (float)atof(argv[5]); p.xe = (float)atof(argv[6]); p.we = (float)atof(argv[7]);
    p.s0 = (float)atof(argv[8]); p.s1 = 6.5e-3f; p.s3 = 5.0e-6f; p.rc = (float)atof(argv[9]);
    p.xb = 10e-3f; p.wb = 2.0e-3f; p.kb = 4.39e8f; p.wc = 1.2e-3f; p.kc = 1.2e5f;
    p.maxm = atoi(argv[10]); p.fmax = (float)atof(argv[11]);
    fs = (float)atof(argv[13]);
    tstring_coefs(&c, &p, fs);
    tstring_init(&s, &c);
    tstring_pluck(&s, -0.4f, 0.05f, fs);
    fprintf(stderr, "float string: f1 %.3f Hz, T %.2f N, %d modes (top %.0f Hz)\n", p.f1, c.k.T, c.k.M, c.k.top_hz);
    fo = fopen(argv[14], "wb");
    if (!fo)
        return 1;
    N = (long)(atof(argv[12]) * fs);
    for (n = 0; n < N; n += 64) {
        int k, len = N - n < 64 ? (int)(N - n) : 64;
        memset(fd, 0, sizeof fd); memset(fn, 0, sizeof fn);
        tstring_run(&s, fd, fn, len);
        for (k = 0; k < len; k++) {
            float fr[3] = {fd[k], fn[k], 0.0f};
            fwrite(fr, sizeof fr, 1, fo);
        }
    }
    fclose(fo);
    fprintf(stderr, "deepest bridge compression %.3e m\n", s.compress_max);
    return 0;
}
