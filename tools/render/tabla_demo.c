/* tabla_demo — host tool (step 5): play bols at exact instants through the engine (the firmware's audio path) and
 * write a WAV. The taal engine arrives at step 6: here the instants are given by hand.
 *
 *   build-host/tabla_demo OUT.wav SECONDS [option ...] [T:BOL[/DYN] ...]
 *     options: sa=CENTS (above C4; default -300 = A3)  tan=1 (the tanpura too)  seed=N
 *   build-host/tabla_demo x.wav 4 0:Dha/110 0.5:Na 1.0:Tin/60
 *   build-host/tabla_demo -selftest      every take decoded against the generator's hash
 *   build-host/tabla_demo -list          the bols and their strokes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tools/common/scen.h"

typedef struct { double t; int bol, dyn; } hit_t;

static int cmp(const void *a, const void *b)
{
    double d = ((const hit_t *)a)->t - ((const hit_t *)b)->t;
    return d < 0 ? -1 : d > 0;
}

int main(int argc, char **argv)
{
    static engine_t e;
    hit_t *h;
    int32_t *lr;
    uint32_t frames, f;
    int i, nh = 0, k = 0, tan = 0;
    long sa = -300, seed = 0;
    double secs, peak = 0;
    if (argc >= 2 && !strcmp(argv[1], "-selftest")) {
        int bad = tabla_selftest(), tb = taal_selftest(tabla_bol_name);
        printf("tabla selftest: %d takes differ from the generator (ADPCM %u bytes)\n", bad, tabla_data_bytes());
        printf("taal selftest: %d bols of taal/taal_tables.h differ from tabla/tabla_bols.h\n", tb);
        return bad != 0 || tb != 0;
    }
    if (argc >= 2 && !strcmp(argv[1], "-list")) {
        for (i = 0; i < tabla_bol_count(); i++) {
            int s[2], l[2], n = tabla_bol_strokes(i, s, l), j;
            printf("%-6s", tabla_bol_name(i));
            for (j = 0; j < n; j++)
                printf(" %s(%s %d%%)", tabla_stroke_name(s[j]), tabla_stroke_drum(s[j]) ? "bayan" : "dayan", l[j] * 100 / 255);
            printf("\n");
        }
        return 0;
    }
    if (argc < 3) {
        fprintf(stderr, "usage: tabla_demo OUT.wav SECONDS [sa=CENTS] [tan=1] [seed=N] [T:BOL[/DYN] ...]\n");
        return 2;
    }
    secs = atof(argv[2]);
    if (secs <= 0 || secs > 600)
        return 2;
    h = calloc((size_t)argc, sizeof *h);
    for (i = 3; i < argc; i++) {
        char name[32], *colon = strchr(argv[i], ':'), *slash, *end;
        if (!strncmp(argv[i], "sa=", 3)) { sa = strtol(argv[i] + 3, 0, 10); continue; }
        if (!strncmp(argv[i], "tan=", 4)) { tan = atoi(argv[i] + 4); continue; }
        if (!strncmp(argv[i], "seed=", 5)) { seed = strtol(argv[i] + 5, 0, 10); continue; }
        if (!colon || strlen(colon + 1) >= sizeof name)
            goto bad;
        h[nh].t = strtod(argv[i], &end);
        if (end != colon || h[nh].t < 0)
            goto bad;
        strcpy(name, colon + 1);
        h[nh].dyn = 100;
        if ((slash = strchr(name, '/'))) {
            *slash = 0;
            h[nh].dyn = (int)strtol(slash + 1, &end, 10);
            if (*end || h[nh].dyn < 0 || h[nh].dyn > 127)
                goto bad;
        }
        if ((h[nh].bol = tabla_bol_find(name)) < 0) {
            fprintf(stderr, "tabla_demo: unknown bol \"%s\" (tabla_demo -list)\n", name);
            return 2;
        }
        nh++;
    }
    qsort(h, (size_t)nh, sizeof *h, cmp);
    frames = scen_frames(secs);
    lr = malloc((size_t)frames * 2u * sizeof *lr);
    if (!lr)
        return 1;
    engine_init(&e);
    if (seed) {
        tabla_config_t c = e.tabla.cfg;
        c.seed = (uint32_t)seed;
        tabla_init(&e.tabla, &c);
    }
    engine_set_param(&e, ENG_ALL, DSP_P_SA_CENTS, (int32_t)sa);
    if (tan)
        engine_set_param(&e, ENG_TANPURA, TANPURA_P_ON, 1);
    for (f = 0; f < frames; f += SCEN_PLAT_BLOCK) {
        while (k < nh) {                         /* the hits in this block, at their sample */
            double at = h[k].t * SCEN_FS_EXACT - (double)f;
            if (at >= SCEN_PLAT_BLOCK)
                break;
            tabla_trigger(&e.tabla, h[k].bol, h[k].dyn, (int32_t)(at < 0 ? 0 : at));
            k++;
        }
        engine_render(&e, lr + 2u * f, SCEN_PLAT_BLOCK, 4096u);
    }
    for (f = 0; f < 2u * frames; f++)
        if (fabs((double)lr[f]) > peak)
            peak = fabs((double)lr[f]);
    if (wav_write(argv[1], lr, frames))
        return 1;
    printf("%s: %.2f s, %d bols, dayan %.1f Hz, peak %.1f dBFS, %u clipped, %u hits dropped\n", argv[1],
           (double)frames / SCEN_FS_EXACT, nh, tabla_get_param(&e.tabla, TABLA_P_DAYAN_DHZ) / 10.0,
           peak > 0 ? 20 * log10(peak / DSP_Q23_MAX) : -999.0, e.mixer.clips, e.tabla.dropped);
    free(lr);
    free(h);
    return 0;
bad:
    fprintf(stderr, "tabla_demo: bad argument \"%s\"\n", argv[i]);
    return 2;
}
