/* ui.c — the interface, from the proposal ElectroNick approved (docs/CONTROLS.md): the tanpura (step 3), the tabla and the
 * taal (step 7), the reference tone (step 1). Included by app.c; talks to the engine only through engine_set_param /
 * get_param. SAVE keeps the settings (step 8): an fm1-base store object, loaded at power-on.
 *
 * Main page                                     EDIT page (rarely used settings)
 *   PLAY       tabla (the taal) on / off          SELECT     the row
 *   SEQ        tanpura on / off                   PRESETS    its value
 *   ARP        reference tone on / off            HOME       back to the main page
 *   SELECT     the taal (while playing: from the  rows: first string, register, A reference, cycle, tanpura
 *              end of the cycle; ElectroNick, step 6)        irregularity, tone swara, tabla irregularity
 *   KNOB1      tempo, 1 matra/min a step (SEL+KNOB1: 10)
 *   key        the reference tone on that key's swara while held (like an harmonium; ElectroNick, step 1)
 *   SEL + key  Sa = that key's note (cents kept)
 *   SEL + OCT-/+  Sa an octave down / up
 *   PRESETS    Sa by a cent
 *   ALGORITHM  jawari (thread position)
 *   KNOB2      tanpura volume, KNOB3 tabla volume, KNOB4 tone volume
 *   OCT- / OCT+  the tone's octave
 *   EDIT       the EDIT page
 *   SAVE       keep the settings (UNSAVED on the screen while they differ from the kept ones)
 * The keys light from Sa up: Sa and Pa bright, the other shuddha swaras dim. */

static const char *const UI_SWARA[SW_N] = {"SA", "RE KOMAL", "RE", "GA KOMAL", "GA", "MA", "MA TIVRA", "PA",
                                           "DHA KOMAL", "DHA", "NI KOMAL", "NI"};
enum { ROW_FIRST, ROW_FINE0, ROW_FINE1, ROW_FINE2, ROW_FINE3, ROW_REGISTER, ROW_CYCLE, ROW_HUMAN, ROW_TABLA_HUMAN,
       ROW_TONE, ROW_A4, NROWS };
static const char *const ROW_NAME[NROWS] = {"FIRST STRING", "FINE: 1ST STRING", "FINE: SA (2ND)", "FINE: SA (3RD)",
                                            "FINE: KHARAJ", "TANPURA REGISTER", "PLUCK CYCLE", "TANPURA IRREG.",
                                            "TABLA IRREG.", "TONE SWARA", "A REFERENCE"};
#define UI_ROWS_SHOWN 7
/* the taal's colours: sam and khali must read at a glance, whatever the theme */
#define UI_C_SAM RGB(255, 96, 64)
#define UI_C_SAM_DIM RGB(80, 34, 26)
#define UI_C_KHALI RGB(90, 170, 255)
#define UI_C_KHALI_DIM RGB(28, 44, 80)

static engine_t *ui_eng;
static uint32_t ui_btn_was, ui_keys_was, ui_frames, ui_msg_until;
static char ui_msg[2][24];
static int ui_dirty = 1, ui_page, ui_row, ui_drawn_page = -1;
static int32_t ui_drawn_matra = -1, ui_drawn_taal = -1;

static int32_t ui_get(int target, int id) { return engine_get_param(ui_eng, target, id); }
static void ui_set(int target, int id, int32_t v) { engine_set_param(ui_eng, target, id, v); ui_dirty = 1; }

/* ------------------------------------------------------------------ settings (step 8) --- */
/* what SAVE keeps (OBJ_SETTINGS, app.h); the sources' on/off are not kept: the FM-1 starts silent */
#define UI_SET_MAGIC 0x31305454u            /* "TT01" */
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    int32_t a4_dhz, sa_cents;
    int8_t first, reg, tone_swara, tone_oct, fine[TANPURA_NSTR];
    uint8_t jawari, cycle, tan_human, tabla_human, taal, vol_tanpura, vol_tabla, vol_tone;
    int16_t mpm, pad;
} ui_settings_t;
static ui_settings_t ui_kept;               /* as loaded or last saved */
static int ui_unsaved, ui_kt_key = -1;      /* the key whose tone sounds (-1 none) */
static int32_t ui_kt_saved[3];              /* the tone's on, swara, octave before a key took it */

static void ui_settings_now(ui_settings_t *p)
{
    int j;
    for (j = 0; j < (int)sizeof *p; j++)
        ((uint8_t *)p)[j] = 0;
    p->magic = UI_SET_MAGIC, p->version = 1, p->size = (uint16_t)sizeof *p;
    p->a4_dhz = ui_get(ENG_ALL, DSP_P_A4_DHZ);
    p->sa_cents = ui_get(ENG_ALL, DSP_P_SA_CENTS);
    p->first = (int8_t)ui_get(ENG_TANPURA, TANPURA_P_FIRST);
    p->reg = (int8_t)ui_get(ENG_TANPURA, TANPURA_P_REGISTER);
    for (j = 0; j < TANPURA_NSTR; j++)
        p->fine[j] = (int8_t)ui_get(ENG_TANPURA, TANPURA_P_FINE + j);
    p->jawari = (uint8_t)ui_get(ENG_TANPURA, TANPURA_P_JAWARI);
    p->cycle = (uint8_t)ui_get(ENG_TANPURA, TANPURA_P_CYCLE);
    p->tan_human = (uint8_t)ui_get(ENG_TANPURA, TANPURA_P_HUMAN);
    p->tabla_human = (uint8_t)ui_get(ENG_TAAL, TAAL_P_HUMAN);
    p->taal = (uint8_t)ui_get(ENG_TAAL, TAAL_P_TAAL);
    p->mpm = (int16_t)ui_get(ENG_TAAL, TAAL_P_MPM);
    if (ui_kt_key >= 0)                     /* (a key's tone is not a setting: what it will go back to) */
        p->tone_swara = (int8_t)ui_kt_saved[1], p->tone_oct = (int8_t)ui_kt_saved[2];
    else
        p->tone_swara = (int8_t)ui_get(ENG_REFTONE, REFTONE_P_SWARA), p->tone_oct = (int8_t)ui_get(ENG_REFTONE, REFTONE_P_OCTAVE);
    p->vol_tanpura = (uint8_t)ui_get(ENG_MIXER, MIX_P_LEVEL + MIX_TANPURA);
    p->vol_tabla = (uint8_t)ui_get(ENG_MIXER, MIX_P_LEVEL + MIX_TABLA);
    p->vol_tone = (uint8_t)ui_get(ENG_REFTONE, REFTONE_P_VOLUME);
}

/* a loaded record: every field inside its range, or the record is not used (defaults) */
static int ui_settings_valid(const ui_settings_t *p)
{
    int j;
    if (p->magic != UI_SET_MAGIC || p->version != 1 || p->size != sizeof *p)
        return 0;
    if (p->a4_dhz < TUNING_A4_DHZ_MIN || p->a4_dhz > TUNING_A4_DHZ_MAX || p->sa_cents < TUNING_SA_MIN
        || p->sa_cents > TUNING_SA_MAX || p->first < 0 || p->first >= SW_N || p->reg < 0 || p->reg > 1
        || p->jawari > 100 || p->cycle < 25 || p->cycle > 80 || p->tan_human > 100 || p->tabla_human > 100
        || p->taal >= taal_count() || p->mpm < TAAL_MPM_MIN || p->mpm > TAAL_MPM_MAX || p->tone_swara < 0
        || p->tone_swara >= SW_N || p->tone_oct < -1 || p->tone_oct > 1 || p->vol_tanpura > 100 || p->vol_tabla > 100
        || p->vol_tone > 100)
        return 0;
    for (j = 0; j < TANPURA_NSTR; j++)
        if (p->fine[j] < -50 || p->fine[j] > 50)
            return 0;
    return 1;
}

static void ui_settings_apply(const ui_settings_t *p)
{
    int j;
    engine_set_param(ui_eng, ENG_ALL, DSP_P_A4_DHZ, p->a4_dhz);
    engine_set_param(ui_eng, ENG_ALL, DSP_P_SA_CENTS, p->sa_cents);
    engine_set_param(ui_eng, ENG_TANPURA, TANPURA_P_FIRST, p->first);
    engine_set_param(ui_eng, ENG_TANPURA, TANPURA_P_REGISTER, p->reg);
    for (j = 0; j < TANPURA_NSTR; j++)
        engine_set_param(ui_eng, ENG_TANPURA, TANPURA_P_FINE + j, p->fine[j]);
    engine_set_param(ui_eng, ENG_TANPURA, TANPURA_P_JAWARI, p->jawari);
    engine_set_param(ui_eng, ENG_TANPURA, TANPURA_P_CYCLE, p->cycle);
    engine_set_param(ui_eng, ENG_TANPURA, TANPURA_P_HUMAN, p->tan_human);
    engine_set_param(ui_eng, ENG_TAAL, TAAL_P_HUMAN, p->tabla_human);
    engine_set_param(ui_eng, ENG_TAAL, TAAL_P_TAAL, p->taal);
    engine_set_param(ui_eng, ENG_TAAL, TAAL_P_MPM, p->mpm);
    engine_set_param(ui_eng, ENG_REFTONE, REFTONE_P_SWARA, p->tone_swara);
    engine_set_param(ui_eng, ENG_REFTONE, REFTONE_P_OCTAVE, p->tone_oct);
    engine_set_param(ui_eng, ENG_MIXER, MIX_P_LEVEL + MIX_TANPURA, p->vol_tanpura);
    engine_set_param(ui_eng, ENG_MIXER, MIX_P_LEVEL + MIX_TABLA, p->vol_tabla);
    engine_set_param(ui_eng, ENG_REFTONE, REFTONE_P_VOLUME, p->vol_tone);
}

static int ui_same(const ui_settings_t *a, const ui_settings_t *b)
{
    const uint8_t *x = (const uint8_t *)a, *y = (const uint8_t *)b;
    uint32_t j;
    for (j = 0; j < sizeof *a; j++)
        if (x[j] != y[j])
            return 0;
    return 1;
}

static int ui_loaded;                       /* 1: the settings came from the flash; 0: defaults (none, or bad) */
static void ui_init_state(engine_t *e)
{
    ui_settings_t s;
    ui_eng = e;
    engine_set_param(e, ENG_ALL, DSP_P_A4_DHZ, TUNING_A4_DHZ_DEFAULT);
    engine_set_param(e, ENG_ALL, DSP_P_SA_CENTS, TUNING_SA_DEFAULT);
    if (plat_store_load(OBJ_SETTINGS, &s, sizeof s) == (int)sizeof s && ui_settings_valid(&s)) {
        ui_settings_apply(&s);
        ui_loaded = 1;
    }
    ui_settings_now(&ui_kept);              /* (defaults count as kept: UNSAVED only after a change) */
}

static void ui_cat(char *d, const char *s) { while (*d) d++; while ((*d++ = *s++)) ; }
static void ui_num(char *b, int32_t v, int plus)
{
    char t[12];
    int i = 0, k = 0;
    if (v < 0) { b[k++] = '-'; v = -v; } else if (plus) b[k++] = '+';
    do { t[i++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (i) b[k++] = t[--i];
    b[k] = 0;
}
static void ui_dec1(char *b, int32_t v)           /* tenths: 4400 -> "440.0" */
{
    char u[12];
    b[0] = 0;
    ui_num(b, v / 10, 0);
    ui_cat(b, ".");
    ui_num(u, v % 10, 0);
    ui_cat(b, u);
}

/* a pitch in cents above C4 as the nearest note and its offset: -1063 -> "C#3 +37" */
static void ui_note(char *b, int32_t c, int cents)
{
    static const char *const N[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int32_t semi = (c >= 0 ? c + 50 : c - 49) / 100, off = c - 100 * semi, n = semi + 48;   /* 48: C4 */
    char t[12];
    b[0] = 0;
    ui_cat(b, N[(n % 12 + 12) % 12]);
    ui_num(t, n / 12, 0);
    ui_cat(b, t);
    if (cents && off) {
        ui_cat(b, " ");
        ui_num(t, off, 1);
        ui_cat(b, t);
    }
}

static void ui_say(const char *a, const char *b)
{
    ui_msg[0][0] = ui_msg[1][0] = 0;
    ui_cat(ui_msg[0], a);
    ui_cat(ui_msg[1], b);
    ui_msg_until = plat_ms() + 1500u;
    ui_dirty = 1;
}

static void ui_header(void)
{
    cv_begin(240, 30, C_BLACK);
    if (plat_ms() < ui_msg_until) {
        int32_t x = cv_text(10, 7, &FONT_B, ui_msg[0], C_WHITE);
        cv_text(x + 8, 7, &FONT_B, ui_msg[1], C_HI);
    } else {
        cv_text(10, 7, &FONT_B, ui_page ? "SETTINGS" : APP_NAME, C_WHITE);
        if (!(cpu2_started && app_cpu2_on()))
            cv_text(118, 9, &FONT_XS, "1 CORE", C_HI);
        if (ui_unsaved)
            cv_text(172, 9, &FONT_XS, "UNSAVED", C_HI);
    }
    cv_blit(0, 0);
}

/* the CPU line: the first core's render, the tanpura job's mean (last) and worst, in tenths of a percent */
static void ui_cpu(char *t)
{
    char u[12];
    uint32_t a = job_permille(job_ticks), w = job_permille(job_ticks_max);
    t[0] = 0;
    ui_cat(t, "CPU0 ");
    ui_num(u, (int32_t)plat_cpu_pct(), 0); ui_cat(t, u); ui_cat(t, "%  TANPURA ");
    ui_num(u, (int32_t)(a / 10), 0); ui_cat(t, u); ui_cat(t, "% MAX ");
    ui_num(u, (int32_t)(w / 10), 0); ui_cat(t, u); ui_cat(t, "%");
}

/* the taal: name (and the next one, chosen while playing), tempo, the matra now, the cycle in vibhags */
static void ui_taal(void)
{
    char t[40], u[16];
    int32_t on = ui_get(ENG_TAAL, TAAL_P_ON), tl = ui_get(ENG_TAAL, TAAL_P_NOW), next = ui_get(ENG_TAAL, TAAL_P_TAAL);
    int32_t mt = on ? ui_get(ENG_TAAL, TAAL_P_MATRA) : -1, nm = taal_matras(tl), x, k, v, w, gap = 4;
    int32_t vstart = 0, vk = 0, khali = 0;
    char mark = 0;
    cv_begin(240, 100, C_BLACK);
    t[0] = 0;
    ui_cat(t, taal_name(tl));
    x = cv_text(10, 0, &FONT_B, t, C_WHITE);
    if (next != tl) {
        t[0] = 0; ui_cat(t, "> "); ui_cat(t, taal_name(next));
        cv_text(x + 8, 0, &FONT_B, t, C_HI);
    }
    t[0] = 0; ui_num(t, ui_get(ENG_TAAL, TAAL_P_MPM), 0); ui_cat(t, " /MIN");
    cv_text(178, 2, &FONT_S, t, C_WHITE);
    /* the matra now: big, with "/16", SAM or KHALI */
    for (k = 0; k <= (mt < 0 ? 0 : mt); k++)        /* the vibhag holding matra mt */
        if (taal_mark(tl, k))
            mark = taal_mark(tl, k);
    if (on) {
        ui_num(t, mt + 1, 0);
        x = cv_text(10, 24, &FONT_L, t, mt == 0 ? UI_C_SAM : mark == '0' ? UI_C_KHALI : C_HI);
        t[0] = 0; ui_cat(t, "/"); ui_num(u, nm, 0); ui_cat(t, u);
        cv_text(x + 4, 44, &FONT_S, t, C_GRAY);
        if (mt == 0)
            cv_text(120, 34, &FONT_B, "SAM", UI_C_SAM);
        else if (mark == '0')
            cv_text(120, 34, &FONT_B, "KHALI", UI_C_KHALI);
    } else {
        cv_text(10, 34, &FONT_B, "TABLA OFF", C_DIM);
        cv_text(120, 38, &FONT_XS, "PLAY TO START", C_DIM);
    }
    /* the cycle: a cell a matra, vibhags apart, the mark over each vibhag */
    for (k = 1, v = 0; k < nm; k++)
        v += taal_mark(tl, k) != 0;                  /* (vibhags - 1: the gaps) */
    w = (220 - gap * v) / nm;
    if (w > 24)
        w = 24;
    x = 10;
    for (k = 0; k < nm; k++) {
        uint16_t c;
        if (taal_mark(tl, k)) {
            if (k)
                x += gap;
            vstart = k, vk = taal_mark(tl, k), khali = vk == '0';
            u[0] = (char)vk, u[1] = 0;
            cv_text(x + 1, 66, &FONT_XS, u, k == 0 && vk != '0' ? UI_C_SAM : khali ? UI_C_KHALI : C_GRAY);
        }
        (void)vstart;
        if (k == mt)
            c = k == 0 ? UI_C_SAM : khali ? UI_C_KHALI : C_HI;
        else
            c = k == 0 ? UI_C_SAM_DIM : khali ? UI_C_KHALI_DIM : C_LINE;
        cv_rect(x, 82, w - 2, 16, c);
        x += w;
    }
    cv_blit(0, 60);
    ui_drawn_matra = mt, ui_drawn_taal = tl * 64 + next;
}

static void ui_main(void)
{
    char t[48], u[24];
    int32_t sa = ui_get(ENG_ALL, DSP_P_SA_CENTS), tsa = ui_get(ENG_TANPURA, TANPURA_P_SA_NOW);
    int32_t ton = ui_get(ENG_TANPURA, TANPURA_P_ON), jw = ui_get(ENG_TANPURA, TANPURA_P_JAWARI);
    int32_t on = ui_get(ENG_REFTONE, REFTONE_P_ON), sw = ui_get(ENG_REFTONE, REFTONE_P_SWARA);
    int32_t oct = ui_get(ENG_REFTONE, REFTONE_P_OCTAVE), x;
    /* Sa */
    cv_begin(240, 26, C_BLACK);
    x = cv_text(10, 6, &FONT_XS, "SA", C_GRAY);
    ui_note(t, sa, 1);
    cv_text(x + 6, 0, &FONT_B, t, C_HI);
    x = cv_text(130, 6, &FONT_XS, "TANPURA", C_GRAY);
    ui_note(t, tsa, 0);
    cv_text(x + 6, 0, &FONT_B, t, C_WHITE);
    cv_blit(0, 32);
    ui_taal();
    /* the tanpura, the tone, the tabla's volume: a line each */
    cv_begin(240, 46, C_BLACK);
    cv_text(10, 2, &FONT_S, "TANPURA", C_GRAY);
    cv_text(84, 2, &FONT_S, ton ? "ON" : "OFF", ton ? C_HI : C_DIM);
    t[0] = 0;
    ui_cat(t, UI_SWARA[ui_get(ENG_TANPURA, TANPURA_P_FIRST)]);
    ui_cat(t, "  J"); ui_num(u, jw, 0); ui_cat(t, u);
    ui_cat(t, "  V"); ui_num(u, ui_get(ENG_MIXER, MIX_P_LEVEL + MIX_TANPURA), 0); ui_cat(t, u);
    cv_text(120, 4, &FONT_XS, t, C_WHITE);
    cv_text(10, 24, &FONT_S, "TONE", C_GRAY);
    cv_text(84, 24, &FONT_S, on ? "ON" : "OFF", on ? C_HI : C_DIM);
    t[0] = 0;
    ui_cat(t, UI_SWARA[sw < 0 || sw >= SW_N ? 0 : sw]);
    ui_cat(t, oct < 0 ? " MANDRA" : oct > 0 ? " TAR" : "");
    ui_cat(t, "  V"); ui_num(u, ui_get(ENG_REFTONE, REFTONE_P_VOLUME), 0); ui_cat(t, u);
    cv_text(120, 26, &FONT_XS, t, C_WHITE);
    cv_blit(0, 162);
    /* the tabla's volume, CPU, help */
    cv_begin(240, 30, C_BLACK);
    ui_cpu(t);
    ui_cat(t, "  TABLA V");
    ui_num(u, ui_get(ENG_MIXER, MIX_P_LEVEL + MIX_TABLA), 0);
    ui_cat(t, u);
    cv_text(10, 2, &FONT_XS, t, C_GRAY);
    cv_text(10, 16, &FONT_XS, "PLAY TABLA  SEQ TANPURA  ARP TONE", C_DIM);
    cv_blit(0, 210);
}

static void ui_row_value(char *t, int row)
{
    char u[16];
    t[0] = 0;
    switch (row) {
    case ROW_FIRST:    ui_cat(t, UI_SWARA[ui_get(ENG_TANPURA, TANPURA_P_FIRST)]); break;
    case ROW_REGISTER: ui_cat(t, ui_get(ENG_TANPURA, TANPURA_P_REGISTER) ? "OCTAVE BELOW" : "AT SA"); break;
    case ROW_A4:       ui_dec1(t, ui_get(ENG_ALL, DSP_P_A4_DHZ)); ui_cat(t, " HZ"); break;
    case ROW_CYCLE:    ui_dec1(t, ui_get(ENG_TANPURA, TANPURA_P_CYCLE)); ui_cat(t, " S"); break;
    case ROW_HUMAN:    ui_num(u, ui_get(ENG_TANPURA, TANPURA_P_HUMAN), 0); ui_cat(t, u); ui_cat(t, " %"); break;
    case ROW_TONE:     ui_cat(t, UI_SWARA[ui_get(ENG_REFTONE, REFTONE_P_SWARA)]); break;
    case ROW_TABLA_HUMAN: ui_num(u, ui_get(ENG_TAAL, TAAL_P_HUMAN), 0); ui_cat(t, u); ui_cat(t, " %"); break;
    case ROW_FINE0: case ROW_FINE1: case ROW_FINE2: case ROW_FINE3:
        ui_num(u, ui_get(ENG_TANPURA, TANPURA_P_FINE + row - ROW_FINE0), 1); ui_cat(t, u); ui_cat(t, " CENTS"); break;
    default: break;
    }
}

static void ui_edit(void)
{
    char t[24];
    int r, part, top = clamp(ui_row - UI_ROWS_SHOWN / 2, 0, NROWS - UI_ROWS_SHOWN), half = (UI_ROWS_SHOWN + 1) / 2;
    for (part = 0; part < 2; part++) {               /* (two strips: a canvas holds at most 144 rows) */
        int r0 = top + (part ? half : 0), r1 = top + (part ? UI_ROWS_SHOWN : half);
        cv_begin(240, (uint32_t)(24 * (r1 - r0)), C_BLACK);
        for (r = r0; r < r1; r++) {
            int sel = r == ui_row, y = (r - r0) * 24;
            if (sel)
                cv_rect(4, y, 232, 22, C_DIM);
            cv_text(10, y + 4, &FONT_S, ROW_NAME[r], sel ? C_WHITE : C_GRAY);
            ui_row_value(t, r);
            cv_text(150, y + 4, &FONT_S, t, sel ? C_HI : C_WHITE);
        }
        cv_blit(0, (uint32_t)(34 + 24 * (r0 - top)));
    }
    cv_begin(240, 30, C_BLACK);
    {   /* where the list is: a row of dots */
        int32_t x = 10;
        for (r = 0; r < NROWS; r++, x += 6)
            cv_rect(x, 2, 4, 4, r == ui_row ? C_HI : r >= top && r < top + UI_ROWS_SHOWN ? C_GRAY : C_DIM);
    }
    cv_text(10, 12, &FONT_XS, "SELECT ROW   PRESET VALUE   HOME BACK", C_DIM);
    cv_blit(0, 206);
}

static void ui_draw_all(void)
{
    if (ui_page != ui_drawn_page) {         /* a new page: the whole screen black first (its strips leave gaps) */
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui_drawn_page = ui_page;
    }
    ui_header();
    if (ui_page)
        ui_edit();
    else
        ui_main();
    ui_dirty = 0;
}

static void ui_edit_step(int row, int32_t d)
{
    switch (row) {
    case ROW_FIRST:
        ui_set(ENG_TANPURA, TANPURA_P_FIRST, clamp(ui_get(ENG_TANPURA, TANPURA_P_FIRST) + d, 0, SW_N - 1));
        break;
    case ROW_REGISTER: ui_set(ENG_TANPURA, TANPURA_P_REGISTER, d > 0); break;
    case ROW_A4:       ui_set(ENG_ALL, DSP_P_A4_DHZ, ui_get(ENG_ALL, DSP_P_A4_DHZ) + d); break;
    case ROW_CYCLE:    ui_set(ENG_TANPURA, TANPURA_P_CYCLE, ui_get(ENG_TANPURA, TANPURA_P_CYCLE) + d); break;
    case ROW_HUMAN:    ui_set(ENG_TANPURA, TANPURA_P_HUMAN, ui_get(ENG_TANPURA, TANPURA_P_HUMAN) + 5 * d); break;
    case ROW_TONE:
        ui_set(ENG_REFTONE, REFTONE_P_SWARA, clamp(ui_get(ENG_REFTONE, REFTONE_P_SWARA) + d, 0, SW_N - 1));
        break;
    case ROW_TABLA_HUMAN: ui_set(ENG_TAAL, TAAL_P_HUMAN, ui_get(ENG_TAAL, TAAL_P_HUMAN) + 5 * d); break;
    case ROW_FINE0: case ROW_FINE1: case ROW_FINE2: case ROW_FINE3:
        ui_set(ENG_TANPURA, TANPURA_P_FINE + row - ROW_FINE0, clamp(ui_get(ENG_TANPURA, TANPURA_P_FINE + row - ROW_FINE0) + d, -50, 50));
        break;
    default: break;
    }
}

/* SEL + key: Sa = the key's note (key 0 = F3), the cents above the semitone kept, inside Sa's range */
static void ui_sa_key(int k)
{
    int32_t sa = ui_get(ENG_ALL, DSP_P_SA_CENTS), semi = (sa >= 0 ? sa + 50 : sa - 49) / 100;
    int32_t off = sa - 100 * semi, c = (k - 7) * 100 + off;     /* key 7 = C4 */
    while (c > TUNING_SA_MAX)
        c -= 1200;
    ui_set(ENG_ALL, DSP_P_SA_CENTS, c);
}

/* the semitone of Sa (middle), nearest */
static int32_t ui_sa_semi(void)
{
    int32_t sa = ui_get(ENG_ALL, DSP_P_SA_CENTS);
    return (sa >= 0 ? sa + 50 : sa - 49) / 100;
}

/* a key alone: the tone on its swara (and octave, from the middle Sa) while it is held; then the tone as it was */
static void ui_key_tone(int k)
{
    int32_t rel = (k - 7) - ui_sa_semi(), sw = (rel % 12 + 12) % 12, oct = (rel - sw) / 12;   /* key 7 = C4 */
    if (ui_kt_key < 0) {
        ui_kt_saved[0] = ui_get(ENG_REFTONE, REFTONE_P_ON);
        ui_kt_saved[1] = ui_get(ENG_REFTONE, REFTONE_P_SWARA);
        ui_kt_saved[2] = ui_get(ENG_REFTONE, REFTONE_P_OCTAVE);
    }
    ui_kt_key = k;
    ui_set(ENG_REFTONE, REFTONE_P_SWARA, sw);
    ui_set(ENG_REFTONE, REFTONE_P_OCTAVE, clamp(oct, -1, 1));
    ui_set(ENG_REFTONE, REFTONE_P_ON, 1);
}

static void ui_key_tone_end(void)
{
    ui_set(ENG_REFTONE, REFTONE_P_ON, ui_kt_saved[0]);
    ui_set(ENG_REFTONE, REFTONE_P_SWARA, ui_kt_saved[1]);
    ui_set(ENG_REFTONE, REFTONE_P_OCTAVE, ui_kt_saved[2]);
    ui_kt_key = -1;
}

/* the keys from Sa up: Sa and Pa lit, Re Ga Ma Dha Ni dim; a key held lit */
static void ui_lights(uint32_t buttons, uint32_t held)
{
    uint32_t lit = held, dim = 0;
    int32_t sa = ui_sa_semi(), k;
    for (k = 0; k < NKEYS; k++) {
        int32_t rel = (k - 7) - sa, sw = (rel % 12 + 12) % 12;
        if (rel < 0)
            continue;
        if (sw == SW_SA || sw == SW_PA)
            lit |= 1u << k;
        else if (sw == SW_RE || sw == SW_GA || sw == SW_MA || sw == SW_DHA || sw == SW_NI)
            dim |= 1u << k;
    }
    plat_lights(buttons, lit, 0u, dim, 1);
}

/* SAVE: the sound fades out first (the flash write stops it for ~12 ms: a click otherwise), then the write, then
 * the sound fades back in */
static uint32_t ui_save_at;                 /* when the fade has ended: write then (0: no save pending) */
static void ui_save_request(void)
{
    app_hush = 1;
    ui_save_at = plat_ms() + 30u;
    if (!ui_save_at)
        ui_save_at = 1;
}

static void ui_save(void)
{
    ui_settings_t s;
    ui_save_at = 0;
    ui_settings_now(&s);
    if (plat_store_save(OBJ_SETTINGS, &s, sizeof s) == 0) {
        ui_kept = s;
        ui_say("SAVED", "");
    } else {
        ui_say("SAVE", "FAILED");
    }
    app_hush = 0;
}

static void ui_frame(void)
{
    uint32_t btn = plat_buttons(), pressed = btn & ~ui_btn_was, keys = plat_keys(), kp = keys & ~ui_keys_was;
    int sel = (btn >> B_SEL) & 1u, k;
    int32_t d;
    ui_btn_was = btn;
    ui_keys_was = keys;
    ui_frames++;
    if (pressed & (1u << B_EDIT))
        ui_page = 1, ui_dirty = 1;
    if (pressed & (1u << B_HOME))
        ui_page = 0, ui_dirty = 1;
    if ((pressed & (1u << B_SAVE)) && !ui_save_at)
        ui_save_request();
    if (ui_save_at && (int32_t)(plat_ms() - ui_save_at) >= 0)
        ui_save();
    if (pressed & (1u << B_PLAY))
        ui_set(ENG_TAAL, TAAL_P_ON, !ui_get(ENG_TAAL, TAAL_P_ON));
    if (pressed & (1u << B_SEQ))
        ui_set(ENG_TANPURA, TANPURA_P_ON, !ui_get(ENG_TANPURA, TANPURA_P_ON));
    if (pressed & (1u << B_ARP))
        ui_set(ENG_REFTONE, REFTONE_P_ON, !ui_get(ENG_REFTONE, REFTONE_P_ON));
    if (pressed & (1u << B_OCTDN)) {
        if (sel)
            ui_set(ENG_ALL, DSP_P_SA_CENTS, ui_get(ENG_ALL, DSP_P_SA_CENTS) - 1200);
        else
            ui_set(ENG_REFTONE, REFTONE_P_OCTAVE, ui_get(ENG_REFTONE, REFTONE_P_OCTAVE) - 1);
    }
    if (pressed & (1u << B_OCTUP)) {
        if (sel)
            ui_set(ENG_ALL, DSP_P_SA_CENTS, ui_get(ENG_ALL, DSP_P_SA_CENTS) + 1200);
        else
            ui_set(ENG_REFTONE, REFTONE_P_OCTAVE, ui_get(ENG_REFTONE, REFTONE_P_OCTAVE) + 1);
    }
    if (sel && kp)
        for (k = 0; k < NKEYS; k++)
            if (kp >> k & 1u) {
                ui_sa_key(k);
                break;
            }
    if (!sel && kp) {                            /* a key alone: the tone there (the last key pressed wins) */
        for (k = NKEYS - 1; k >= 0; k--)
            if (kp >> k & 1u)
                break;
        ui_key_tone(k);
    } else if (ui_kt_key >= 0 && !(keys >> ui_kt_key & 1u)) {
        if (keys) {                              /* released, another still held: the tone goes there */
            for (k = 0; k < NKEYS && !(keys >> k & 1u); k++)
                ;
            ui_key_tone(k);
        } else
            ui_key_tone_end();
    }
    if (ui_page) {
        if ((d = plat_enc(EN_SELECT)) != 0)
            ui_row = clamp(ui_row + d, 0, NROWS - 1), ui_dirty = 1;
        if ((d = plat_enc(EN_PRESET)) != 0)
            ui_edit_step(ui_row, d);
    } else {
        if ((d = plat_enc(EN_SELECT)) != 0)     /* the taal, round the list */
            ui_set(ENG_TAAL, TAAL_P_TAAL, ((ui_get(ENG_TAAL, TAAL_P_TAAL) + d) % taal_count() + taal_count()) % taal_count());
        if ((d = plat_enc(EN_PRESET)) != 0)
            ui_set(ENG_ALL, DSP_P_SA_CENTS, ui_get(ENG_ALL, DSP_P_SA_CENTS) + d);
    }
    if ((d = plat_enc(EN_ALGO)) != 0)
        ui_set(ENG_TANPURA, TANPURA_P_JAWARI, ui_get(ENG_TANPURA, TANPURA_P_JAWARI) + 2 * d);
    if ((d = plat_enc(EN_K2)) != 0)
        ui_set(ENG_MIXER, MIX_P_LEVEL + MIX_TANPURA, clamp(ui_get(ENG_MIXER, MIX_P_LEVEL + MIX_TANPURA) + 5 * d, 0, 100));
    if ((d = plat_enc(EN_K4)) != 0)
        ui_set(ENG_REFTONE, REFTONE_P_VOLUME, ui_get(ENG_REFTONE, REFTONE_P_VOLUME) + 5 * d);
    if ((d = plat_enc(EN_K1)) != 0)
        ui_set(ENG_TAAL, TAAL_P_MPM, ui_get(ENG_TAAL, TAAL_P_MPM) + (sel ? 10 : 1) * d);
    if ((d = plat_enc(EN_K3)) != 0)
        ui_set(ENG_MIXER, MIX_P_LEVEL + MIX_TABLA, clamp(ui_get(ENG_MIXER, MIX_P_LEVEL + MIX_TABLA) + 5 * d, 0, 100));
    {
        ui_settings_t now;
        int u;
        ui_settings_now(&now);
        u = !ui_same(&now, &ui_kept);
        if (u != ui_unsaved)
            ui_unsaved = u, ui_dirty = 1;
    }
    ui_lights((ui_get(ENG_TANPURA, TANPURA_P_ON) ? 1u << B_SEQ : 0u)
              | (ui_get(ENG_REFTONE, REFTONE_P_ON) && ui_kt_key < 0 ? 1u << B_ARP : 0u)
              | (ui_get(ENG_TAAL, TAAL_P_ON) ? LED_PLAY_GREEN : 0u) | (ui_page ? 1u << B_EDIT : 0u)
              | (ui_unsaved ? 1u << B_SAVE : 0u), keys);
    if (ui_dirty || ui_frames % 30u == 0 || (ui_msg_until && plat_ms() >= ui_msg_until)) {
        if (ui_msg_until && plat_ms() >= ui_msg_until)
            ui_msg_until = 0;
        ui_draw_all();
    } else if (!ui_page && (ui_get(ENG_TAAL, TAAL_P_ON) ? ui_get(ENG_TAAL, TAAL_P_MATRA) : -1) != ui_drawn_matra) {
        ui_taal();                               /* a new matra: the taal's strip alone, at once */
    }
}
