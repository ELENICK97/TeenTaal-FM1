/* taal.h — the taal engine: sequencing, laya, position in the cycle. It makes no sound: it emits events (bol, dynamic,
 * sample offset) that tabla/ plays (tabla_trigger). The taals are data: data/taal/ (JSON), converted at build time into
 * taal/taal_tables.h by tools/taal_data/build_taal.py (format: data/taal/FORMAT.md).
 *
 * Position: the matra in the cycle and an exact integer fraction of it (taal.c: no drift), advanced per sample, so a
 * tempo change moves on from where the cycle is: no jump. A divided matra ("Ti-Ra-Ki-Ta") plays its parts at even
 * fractions of the matra. Events fall on their exact sample (plus the irregularity).
 * Dynamics: the sam 112, the first matra of another vibhag 100, the rest 92; the later parts of a divided matra 85 %
 * of that. The irregularity ("human", 0..100): at 100, +-4 ms and +-10 of dynamic, at random.
 *
 * Parameters (taal_set_param):
 *   TAAL_P_ON      0 / 1        1 starts at the sam; 0 stops (the strokes still ringing ring out)
 *   TAAL_P_TAAL    0..TAAL_N-1  the taal; while playing, the change waits for the end of the cycle (ElectroNick, step 6)
 *   TAAL_P_MPM     20..600      tempo, matras a minute (default 120)
 *   TAAL_P_HUMAN   0..100       irregularity (default 30)
 *   (read only) TAAL_P_MATRA    the matra now, 0-based;  TAAL_P_CYCLES  cycles completed since start;
 *               TAAL_P_NOW      the taal playing now (TAAL_P_TAAL gives the one chosen, maybe still to come)
 * Threads (step 7): taal_set_param in the main loop, taal_advance in the audio path. A start is a request the next
 * taal_advance takes (it resets the position there); the other parameters are single words. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"

#define TAAL_MPM_MIN 20
#define TAAL_MPM_MAX 600

enum {
    TAAL_P_ON = DSP_P_MODULE,
    TAAL_P_TAAL,
    TAAL_P_MPM,
    TAAL_P_HUMAN,
    TAAL_P_MATRA,
    TAAL_P_CYCLES,
    TAAL_P_NOW,
    TAAL_P_END
};

typedef struct {
    uint16_t stroke;       /* the bol (tabla_bol_find / tabla/tabla_bols.h) */
    uint8_t dynamic;       /* 0..127 */
    uint8_t reserved;
    uint16_t offset;       /* samples from the start of the block being rendered (may pass its end: tabla carries it) */
} taal_event_t;

typedef struct {
    uint32_t seed;
} taal_config_t;

typedef struct {
    volatile int32_t on, taal, mpm, human;
    volatile int32_t pending;   /* a taal to switch to at the end of the cycle; -1 none */
    volatile int32_t start;     /* a start requested: the next taal_advance resets the position */
    uint32_t matra, part;  /* the matra now; its next part to play */
    volatile uint32_t inc;
    uint32_t frac;         /* position in the matra (0..TAAL_ONE, taal.c), increment per sample */
    uint32_t cycles, rng;
} taal_t;

void taal_init(taal_t *m, const taal_config_t *cfg);   /* cfg NULL: defaults */
void taal_set_param(taal_t *m, int param_id, int32_t value);
int32_t taal_get_param(const taal_t *m, int param_id);
/* the events falling in the next n samples, at most max; returns how many */
int taal_advance(taal_t *m, int n_samples, taal_event_t *events, int max);

/* the taal table */
int taal_count(void);
const char *taal_name(int taal);
int taal_matras(int taal);
int taal_tempo(int taal);                      /* the suggested tempo */
int taal_find(const char *name);               /* ignoring case; -1 if none */
/* the mark of the vibhag starting at this matra (0-based): 'X', '0', '1'..'9'; 0 inside a vibhag */
char taal_mark(int taal, int matra);
int taal_matra_parts(int taal, int matra);     /* parts in a matra (1 = undivided) */
int taal_part_bol(int taal, int matra, int part);   /* the bol of a part; -1 for a rest */
/* the generated table's bol names against the tabla's (bol_name: tabla_bol_name): mismatches, 0 expected */
int taal_selftest(const char *(*bol_name)(int));
