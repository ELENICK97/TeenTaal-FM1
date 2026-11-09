/* tuning.c — see tuning.h */
#include "dsp/tuning.h"
#include "dsp/dsp.h"
#include "fastmath.h"

/* Swara intervals above Sa: equal temperament, 100 cents a semitone (ElectroNick, 2026-11-06; DECISIONS.md).
 * Just intonation was tried at step 1 (renders/step-1 history in DECISIONS.md) and set aside. */
static const char *const SWARA_NAME[SW_N] = {"Sa", "re", "Re", "ga", "Ga", "Ma", "Ma'", "Pa", "dha", "Dha", "ni", "Ni"};

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

void tuning_default(tuning_t *t)
{
    t->a4_dhz = TUNING_A4_DHZ_DEFAULT;
    t->sa_cents = TUNING_SA_DEFAULT;
}

int tuning_set_param(tuning_t *t, int id, int32_t v)
{
    if (id == DSP_P_A4_DHZ)
        t->a4_dhz = clampi(v, TUNING_A4_DHZ_MIN, TUNING_A4_DHZ_MAX);
    else if (id == DSP_P_SA_CENTS)
        t->sa_cents = clampi(v, TUNING_SA_MIN, TUNING_SA_MAX);
    else
        return 0;
    return 1;
}

float tuning_swara_cents(int swara)
{
    return 100.0f * (float)(swara < 0 ? 0 : swara >= SW_N ? SW_N - 1 : swara);
}

const char *tuning_swara_name(int swara)
{
    return SWARA_NAME[swara < 0 ? 0 : swara >= SW_N ? SW_N - 1 : swara];
}

float tuning_hz(const tuning_t *t, float cents)
{
    float a4 = (float)t->a4_dhz * 0.1f;
    return a4 * fm_exp2f(((float)(t->sa_cents - 900) + cents) * (1.0f / 1200.0f));
}

uint32_t tuning_phase_inc(float hz)
{
    float inc = hz * (4294967296.0f / DSP_FS);
    if (!(inc > 0.0f) || inc >= 2147483520.0f)  /* at or above Nyquist (or nonsense): silence */
        return 0;
    return (uint32_t)(int32_t)inc;              /* < 2^31: the signed conversion every FPU has */
}
