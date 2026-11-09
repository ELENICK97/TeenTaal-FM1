/* tstring.h — one tanpura string: the DAFx-16 model (van Walstijn, Bridges, Mehes) in single precision.
 *
 * Same scheme as the double-precision reference (tools/reference/tanpura_ref.c): modal string with exact modal
 * frequency and damping (eq. 33), bridge and thread contact in closed form (eqs. 43-48, Fig. 4), pluck force
 * (14-15) from a table. Per sample: no division and no transcendental (the bridge force is (s_b + u) / theta,
 * which follows from eq. 48 in every case, 1/theta a constant; the one square root is fm_sqrtf).
 * The string's driving forces at the bridge (F_d, eq. 13) and the nut (F_n, eq. 22) come out per sample;
 * the body filters are tanpura.c's.
 *
 * Coefficients are computed by tstring_coefs (outside the audio interrupt) into a tstring_coefs_t, and taken by the
 * string at the start of a block (tstring_take), so a retune never tears a block. */
#pragma once
#include <stdint.h>

#ifndef TSTRING_MAXM
#define TSTRING_MAXM 96              /* modes per string at most: step 2b quality needs <= ~80 (Sa F#2) */
#endif

typedef struct {
    float L, rhoA, EI, f1;           /* length (m), mass per length (kg/m), bending stiffness (N m^2), pitch (Hz) */
    float s0, s1, s3;                /* damping (eq. 18) */
    float xb, wb, kb;                /* bridge contact: centre, width (m), stiffness (N/m) */
    float xc, wc, kc, rc;            /* thread: centre, width (m), stiffness (N/m), damping (kg/s) */
    float xe, we;                    /* pluck: centre, width (m) */
    float fmax;                      /* truncate the mode series here (Hz; and always below Nyquist) */
    int maxm;                        /* and at this many modes (<= TSTRING_MAXM) */
} tstring_par_t;

/* the per-string constants of the update (the per-mode ones live in tmode_t) */
typedef struct {
    int M;
    float kc, phic, vden, bc, thbc, thcc, kb, ph, inv1ph, inv21ph, invth;
    float gbe, gce;                  /* sum g_b * gej, sum g_c * gej: the pluck's projections (eq. 42) */
    float T, top_hz;                 /* the tension found for f1, the highest mode kept (reports) */
} tstring_k_t;

/* a full set of coefficients, as tstring_coefs computes them (a retune is staged in one of these) */
typedef struct {
    tstring_k_t k;
    float a[TSTRING_MAXM];           /* a*_i (eq. 33) */
    float c2j[TSTRING_MAXM];         /* 2 / J_i */
    float gej[TSTRING_MAXM];         /* xi g_e,i / J_i */
    float gb[TSTRING_MAXM], gc[TSTRING_MAXM];
    float eb[TSTRING_MAXM], ec[TSTRING_MAXM];   /* xi g / J (eq. 41) */
    float cn[TSTRING_MAXM];          /* nut force weight, halved (the mean of y at n and n+1) */
} tstring_coefs_t;

/* one mode's state and coefficients together: the update reaches everything from one pointer at fixed offsets
 * (with separate arrays the pi32v2 compiler runs out of registers for their bases and rebuilds them every mode) */
typedef struct {
    float y, q, ub;                  /* state: displacement, scaled momentum, u-bar of the next step (no pluck) */
    float a, c2j, gb, gc, eb, ec, cn, gej, pad;
} tmode_t;

typedef struct {
    tstring_k_t c;                   /* the constants in force (the per-mode coefficients are in m[]) */
    tmode_t m[TSTRING_MAXM];
    float pluck_pos, pluck_step, pluck_amp;   /* the pluck table position, its step a sample, the force */
    int plucking;
    float compress_max;              /* the deepest compression into the bridge seen (m, a report) */
} tstring_t;

/* compute a string's coefficients for a sample rate (no allocation; float only; outside the audio interrupt) */
void tstring_coefs(tstring_coefs_t *c, const tstring_par_t *p, float fs);
void tstring_init(tstring_t *s, const tstring_coefs_t *c);
/* take new coefficients (a retune): the state is kept, modes beyond the new count are cleared */
void tstring_take(tstring_t *s, const tstring_coefs_t *c);
/* start a pluck: amp in newtons (negative: toward the bridge, as in the paper), tau_e seconds */
void tstring_pluck(tstring_t *s, float amp, float tau_e, float fs);
/* n samples: adds the bridge force into fd[] and the nut force into fn[] */
void tstring_run(tstring_t *s, float *fd, float *fn, int n);
/* the numerical energy (eq. 49) without the contact terms, for the tests */
float tstring_energy(const tstring_t *s, float fs);
void tstring_silence(tstring_t *s);
