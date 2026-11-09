/* scen.h — host tools only: scenarios (timed parameter changes) driving the engine, and WAV output.
 *
 * An event is "T:name=value", T in seconds. Names and values:
 *   a4=440.0             reference A4 in Hz (one decimal)
 *   sa=C#3  sa=C#3+25  sa=-1100     Sa: a note name with optional cents, or cents above C4
 *   ref.on=1  ref.swara=Pa (or 0..11)  ref.oct=-1..1  ref.vol=0..100
 *   mix.tanpura= mix.tabla= mix.ref=   0..100
 *   master=0..4096       MASTER's gain as the platform passes it (default 4096)
 *   tan.on=1  tan.first=Pa (or 0..11)  tan.jawari=0..100  tan.cycle=25..80 (tenths of s)  tan.human=0..100
 *   tan.fine0..3=-50..50 (cents)  tan.vol0..3=0..100  tan.reg=0/1 (1: an octave under Sa, the default)
 *   tab.hit=Dha  tab.hit=Dha/60    a bol (data/tabla/bols.json), dynamic 0..127 (default 100)   tab.stop=1
 *   taal.on=1  taal.taal=Ektaal (or 0..9)  taal.mpm=20..600  taal.human=0..100
 * Events apply at the start of the platform block (256 frames) in which they fall, as the UI's would. */
#pragma once
#include <stdint.h>
#include "mixer/engine.h"

#define SCEN_PLAT_BLOCK 256u
#define SCEN_FS_EXACT ((double)DSP_FS_NUM / (double)DSP_FS_DEN)

typedef struct {
    double t;
    int target, id;           /* target -1: MASTER */
    int32_t v;
} scen_ev_t;

/* parse one event; 0 ok, -1 with a message on stderr */
int scen_parse(const char *s, scen_ev_t *ev);
/* frames for a duration, a whole number of platform blocks */
uint32_t scen_frames(double seconds);

typedef struct {
    engine_t eng;
    uint32_t master;
    const scen_ev_t *ev;
    int nev, next;
    uint32_t frame;
} scen_run_t;

void scen_start(scen_run_t *r, const scen_ev_t *ev, int nev);
/* apply the events due at the current frame (call before each platform block) */
void scen_apply_due(scen_run_t *r);

/* write stereo Q23 frames as a 24-bit PCM WAV. The header says 44118 Hz (an integer is required): the
 * FM-1's 44117.647 Hz played 0.008 % (0.14 cents) high on a computer. 0 ok */
int wav_write(const char *path, const int32_t *lr, uint32_t frames);
