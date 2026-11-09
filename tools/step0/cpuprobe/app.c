/* Step 0 CPU probe: K1 sets how many oscillators run (0..128), SEL switches Q15 table / float resonator.
 * The screen shows plat_cpu_pct (the render's share of its block time) and the clock. */
#include "probe_dsp.h"

#define MAXOSC 128
static volatile int32_t nosc = 8, mode;          /* mode 0 = Q15, 1 = float */
static uint32_t ph[MAXOSC], inc[MAXOSC];
static float s1[MAXOSC], s2[MAXOSC], kk[MAXOSC];
static int32_t mono[APP_BLOCK];
static uint32_t btn_was, frames;
static volatile uint32_t renders, fpu_isr;
static volatile int32_t peak;
static uint32_t fpu_main, rps, last_ms, last_renders;

void app_init(void)
{
    int i;
    /* sine table by a float resonator at init (no libm needed) */
    float a = 0.0f, b = -0.00613588f, k = 1.99996235f;      /* 2cos(2pi/1024) */
    for (i = 0; i <= 1024; i++) {
        float y = k * a - b;
        probe_sin[i] = (int16_t)(a * 32767.0f / 1.0f);
        b = a; a = y;
    }
    fpu_main = fpu_selftest();
    for (i = 0; i < MAXOSC; i++) {
        inc[i] = 42836000u;                                  /* 440 Hz, all in phase: a clean tone */
        s1[i] = 0.0f; s2[i] = 0.01f; kk[i] = 1.999f;
    }
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i;
    int32_t j, no = nosc, m = mode;
    (void)master_q12;
    if (!fpu_isr)
        fpu_isr = fpu_selftest();          /* once, inside the audio interrupt */
    renders++;
    for (i = 0; i < n; i++)
        mono[i] = 0;
    for (j = 0; j < no; j++) {
        if (m == 0)
            osc_q15(mono, (int)n, &ph[j], inc[j], 64);
        else
            osc_f32(mono, (int)n, &s1[j], &s2[j], kk[j], 50000.0f);
    }
    {                                            /* a test tone around -20 dBFS whatever the count */
        int32_t g = no > 0 ? 64 / (no < 64 ? no : 64) : 0, pk = 0;
        for (i = 0; i < n; i++) {
            int32_t v = clamp(mono[i] * g, -8388607, 8388607);
            lr[2 * i] = v;
            lr[2 * i + 1] = v;
            if (v > pk) pk = v;
        }
        peak = pk;
    }
}

void app_panic(void) { nosc = 0; }
void app_ui_init(void) { lcd_fill(0, 0, 240, 240, C_BLACK); }

static void num(char *b, int32_t v)
{
    char t[12];
    int i = 0, k = 0;
    if (v < 0) { b[k++] = '-'; v = -v; }
    do { t[i++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (i) b[k++] = t[--i];
    b[k] = 0;
}

static void hex(char *b, uint32_t v)
{
    int i;
    for (i = 0; i < 8; i++)
        b[i] = "0123456789ABCDEF"[(v >> (28 - 4 * i)) & 15u];
    b[8] = 0;
}

void app_frame(void)
{
    char t[16];
    uint32_t btn = plat_buttons();
    int32_t d = plat_enc(EN_K1);
    frames++;
    if (d)
        nosc = clamp(nosc + d, 0, MAXOSC);
    if ((btn & (1u << B_SEL)) && !(btn_was & (1u << B_SEL)))
        mode = !mode;
    btn_was = btn;
    if (plat_ms() - last_ms >= 1000u) {
        uint32_t r = renders;
        rps = (r - last_renders) * 1000u / (plat_ms() - last_ms);
        last_renders = r;
        last_ms = plat_ms();
    }
    if (frames % 10u)
        return;
    cv_begin(240, 120, C_BLACK);
    cv_text(10, 6, &FONT_B, mode ? "FLOAT RESONATOR" : "Q15 TABLE+LERP", C_WHITE);
    num(t, nosc);   cv_text(10, 30, &FONT_B, "OSC", C_GRAY);  cv_text(130, 30, &FONT_B, t, C_HI);
    num(t, (int32_t)plat_cpu_pct()); cv_text(10, 54, &FONT_B, "CPU %", C_GRAY); cv_text(130, 54, &FONT_B, t, C_HI);
    num(t, (int32_t)(plat_cpu_khz() / 1000u)); cv_text(10, 78, &FONT_B, "MHZ", C_GRAY); cv_text(130, 78, &FONT_B, t, C_HI);
    num(t, APP_BLOCK); cv_text(10, 102, &FONT_XS, "BLOCK", C_GRAY); cv_text(130, 102, &FONT_XS, t, C_WHITE);
    cv_blit(0, 20);
    cv_begin(240, 100, C_BLACK);
    num(t, (int32_t)rps); cv_text(10, 0, &FONT_B, "RENDERS/S", C_GRAY); cv_text(130, 0, &FONT_B, t, C_HI);
    num(t, (int32_t)renders); cv_text(10, 22, &FONT_B, "RENDERS", C_GRAY); cv_text(130, 22, &FONT_B, t, C_HI);
    hex(t, fpu_main); cv_text(10, 44, &FONT_B, "FPU MAIN", C_GRAY); cv_text(130, 44, &FONT_B, t, C_HI);
    hex(t, fpu_isr);  cv_text(10, 66, &FONT_B, "FPU AUDIO", C_GRAY); cv_text(130, 66, &FONT_B, t, C_HI);
    cv_blit(0, 140);
}

void app_input_only(void) {}
void app_say(const char *a, const char *b) { (void)a; (void)b; }

int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "nosc")) *got = (int)nosc;
    else if (!strcmp(what, "frames")) *got = (int)frames;
    else if (!strcmp(what, "fpu_ok")) *got = fpu_main == fpu_isr && fpu_main != 0;
    else if (!strcmp(what, "rps")) *got = (int)rps;
    else if (!strcmp(what, "peak")) *got = (int)peak;
    else if (!strcmp(what, "mode")) *got = (int)mode;
    else return 0;
    return 1;
}
