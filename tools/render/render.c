/* render — host tool: run the engine (the same DSP as the firmware) through a scenario and write a WAV.
 *
 *   build-host/render OUT.wav SECONDS [EVENT ...]
 *   build-host/render renders/step-1/sa.wav 6 0:sa=C#3 0:ref.on=1
 *
 * Events: tools/common/scen.h. Prints the peak level, the clips and the tone's frequency. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tools/common/scen.h"

int main(int argc, char **argv)
{
    static scen_run_t r;
    scen_ev_t *ev;
    int32_t *lr;
    uint32_t frames, f;
    double secs, peak = 0;
    int i, nev;
    if (argc < 3) {
        fprintf(stderr, "usage: render OUT.wav SECONDS [T:name=value ...]   (events: tools/common/scen.h)\n");
        return 2;
    }
    secs = atof(argv[2]);
    if (secs <= 0 || secs > 600) {
        fprintf(stderr, "render: SECONDS 0..600\n");
        return 2;
    }
    nev = argc - 3;
    ev = calloc((size_t)(nev ? nev : 1), sizeof *ev);
    for (i = 0; i < nev; i++)
        if (scen_parse(argv[3 + i], &ev[i]))
            return 2;
    frames = scen_frames(secs);
    lr = malloc((size_t)frames * 2u * sizeof *lr);
    if (!ev || !lr)
        return 1;
    scen_start(&r, ev, nev);
    for (f = 0; f < frames; f += SCEN_PLAT_BLOCK) {
        r.frame = f;
        scen_apply_due(&r);
        engine_render(&r.eng, lr + 2u * f, SCEN_PLAT_BLOCK, r.master);
    }
    for (f = 0; f < 2u * frames; f++)
        if (fabs((double)lr[f]) > peak)
            peak = fabs((double)lr[f]);
    if (wav_write(argv[1], lr, frames)) {
        fprintf(stderr, "render: cannot write %s\n", argv[1]);
        return 1;
    }
    printf("%s: %.2f s, peak %.1f dBFS, %u clipped, tone at the end %.3f Hz (%s)\n", argv[1],
           (double)frames / SCEN_FS_EXACT, peak > 0 ? 20 * log10(peak / DSP_Q23_MAX) : -999.0, r.eng.mixer.clips,
           (double)reftone_hz(&r.eng.reftone), tuning_swara_name(r.eng.reftone.swara));
    free(lr);
    free(ev);
    return 0;
}
