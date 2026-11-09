/* scen.c — see scen.h (host only) */
#include "tools/common/scen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int note_cents(const char *s, int32_t *out)
{
    static const int SEMI[7] = {9, 11, 0, 2, 4, 5, 7};   /* A B C D E F G */
    int semi, oct;
    char *end;
    long off = 0;
    if (*s < 'A' || *s > 'G')
        return -1;
    semi = SEMI[*s++ - 'A'];
    if (*s == '#') { semi++; s++; }
    else if (*s == 'b') { semi--; s++; }
    if (*s < '0' || *s > '9')
        return -1;
    oct = *s++ - '0';
    if (*s == '+' || *s == '-') {
        off = strtol(s, &end, 10);
        if (*end)
            return -1;
    } else if (*s)
        return -1;
    *out = (int32_t)((semi + 12 * (oct - 4)) * 100 + off);
    return 0;
}

static int swara_index(const char *s, int32_t *out)
{
    int k;
    for (k = 0; k < SW_N; k++)
        if (!strcmp(s, tuning_swara_name(k))) {
            *out = k;
            return 0;
        }
    return -1;
}

int scen_parse(const char *s, scen_ev_t *ev)
{
    char name[32], val[32];
    const char *colon = strchr(s, ':'), *eq = strchr(s, '=');
    char *end;
    if (!colon || !eq || eq < colon || eq - colon - 1 >= (int)sizeof name || strlen(eq + 1) >= sizeof val)
        goto bad;
    ev->t = strtod(s, &end);
    if (end != colon || ev->t < 0)
        goto bad;
    memcpy(name, colon + 1, (size_t)(eq - colon - 1));
    name[eq - colon - 1] = 0;
    strcpy(val, eq + 1);
    if (!strcmp(name, "a4")) {
        ev->target = ENG_ALL, ev->id = DSP_P_A4_DHZ, ev->v = (int32_t)lround(strtod(val, &end) * 10.0);
        if (*end) goto bad;
        return 0;
    }
    if (!strcmp(name, "sa")) {
        ev->target = ENG_ALL, ev->id = DSP_P_SA_CENTS;
        if (note_cents(val, &ev->v) == 0)
            return 0;
    } else if (!strcmp(name, "ref.swara")) {
        ev->target = ENG_REFTONE, ev->id = REFTONE_P_SWARA;
        if (swara_index(val, &ev->v) == 0)
            return 0;
    } else if (!strcmp(name, "ref.on"))  ev->target = ENG_REFTONE, ev->id = REFTONE_P_ON;
    else if (!strcmp(name, "ref.oct"))   ev->target = ENG_REFTONE, ev->id = REFTONE_P_OCTAVE;
    else if (!strcmp(name, "ref.vol"))   ev->target = ENG_REFTONE, ev->id = REFTONE_P_VOLUME;
    else if (!strcmp(name, "mix.tanpura")) ev->target = ENG_MIXER, ev->id = MIX_P_LEVEL + MIX_TANPURA;
    else if (!strcmp(name, "mix.tabla")) ev->target = ENG_MIXER, ev->id = MIX_P_LEVEL + MIX_TABLA;
    else if (!strcmp(name, "mix.ref"))   ev->target = ENG_MIXER, ev->id = MIX_P_LEVEL + MIX_REFTONE;
    else if (!strcmp(name, "master"))    ev->target = -1, ev->id = 0;
    else if (!strcmp(name, "tan.on"))    ev->target = ENG_TANPURA, ev->id = TANPURA_P_ON;
    else if (!strcmp(name, "tan.jawari")) ev->target = ENG_TANPURA, ev->id = TANPURA_P_JAWARI;
    else if (!strcmp(name, "tan.cycle")) ev->target = ENG_TANPURA, ev->id = TANPURA_P_CYCLE;
    else if (!strcmp(name, "tan.human")) ev->target = ENG_TANPURA, ev->id = TANPURA_P_HUMAN;
    else if (!strcmp(name, "tan.reg"))   ev->target = ENG_TANPURA, ev->id = TANPURA_P_REGISTER;
    else if (!strncmp(name, "tan.fine", 8) && name[8] >= '0' && name[8] <= '3' && !name[9])
        ev->target = ENG_TANPURA, ev->id = TANPURA_P_FINE + (name[8] - '0');
    else if (!strncmp(name, "tan.vol", 7) && name[7] >= '0' && name[7] <= '3' && !name[8])
        ev->target = ENG_TANPURA, ev->id = TANPURA_P_VOL + (name[7] - '0');
    else if (!strcmp(name, "tab.hit")) {      /* tab.hit=Dha or Dha/100 (dynamic 0..127, default 100) */
        char *slash = strchr(val, '/');
        long dyn = 100;
        int b;
        if (slash) {
            *slash = 0;
            dyn = strtol(slash + 1, &end, 10);
            if (*end || dyn < 0 || dyn > 127)
                goto bad;
        }
        if ((b = tabla_bol_find(val)) < 0)
            goto bad;
        ev->target = ENG_TABLA, ev->id = TABLA_P_HIT, ev->v = b | (int32_t)dyn << 8;
        return 0;
    }
    else if (!strcmp(name, "taal.taal")) {   /* taal.taal=Ektaal or 0..9 */
        int t = taal_find(val);
        ev->target = ENG_TAAL, ev->id = TAAL_P_TAAL;
        if (t >= 0) {
            ev->v = t;
            return 0;
        }
    }
    else if (!strcmp(name, "taal.on"))   ev->target = ENG_TAAL, ev->id = TAAL_P_ON;
    else if (!strcmp(name, "taal.mpm"))  ev->target = ENG_TAAL, ev->id = TAAL_P_MPM;
    else if (!strcmp(name, "taal.human")) ev->target = ENG_TAAL, ev->id = TAAL_P_HUMAN;
    else if (!strcmp(name, "tab.stop"))  ev->target = ENG_TABLA, ev->id = TABLA_P_STOP;
    else if (!strcmp(name, "tan.first")) {
        ev->target = ENG_TANPURA, ev->id = TANPURA_P_FIRST;
        if (swara_index(val, &ev->v) == 0)
            return 0;
    }
    else
        goto bad;
    ev->v = (int32_t)strtol(val, &end, 10);
    if (*end || !*val)
        goto bad;
    return 0;
bad:
    fprintf(stderr, "bad event \"%s\" (T:name=value; see tools/common/scen.h)\n", s);
    return -1;
}

uint32_t scen_frames(double seconds)
{
    uint32_t f = (uint32_t)(seconds * SCEN_FS_EXACT + 0.5);
    return (f + SCEN_PLAT_BLOCK - 1u) / SCEN_PLAT_BLOCK * SCEN_PLAT_BLOCK;
}

void scen_start(scen_run_t *r, const scen_ev_t *ev, int nev)
{
    engine_init(&r->eng);
    r->master = 4096u;
    r->ev = ev, r->nev = nev, r->next = 0, r->frame = 0;
}

void scen_apply_due(scen_run_t *r)
{
    double from = (double)r->frame / SCEN_FS_EXACT, to = (double)(r->frame + SCEN_PLAT_BLOCK) / SCEN_FS_EXACT;
    int k;
    for (k = 0; k < r->nev; k++) {               /* the events falling in this block, in the list's order */
        const scen_ev_t *e = &r->ev[k];
        if (e->t < from || e->t >= to)
            continue;
        if (e->target < 0)
            r->master = (uint32_t)(e->v < 0 ? 0 : e->v > 4096 ? 4096 : e->v);
        else
            engine_set_param(&r->eng, e->target, e->id, e->v);
    }
}

static void put32(FILE *f, uint32_t v) { fputc((int)(v & 255), f); fputc((int)(v >> 8 & 255), f); fputc((int)(v >> 16 & 255), f); fputc((int)(v >> 24), f); }
static void put16(FILE *f, uint32_t v) { fputc((int)(v & 255), f); fputc((int)(v >> 8 & 255), f); }

int wav_write(const char *path, const int32_t *lr, uint32_t frames)
{
    FILE *f = fopen(path, "wb");
    uint32_t i, data = frames * 2u * 3u;
    if (!f)
        return -1;
    fwrite("RIFF", 1, 4, f); put32(f, 36u + data); fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16); put16(f, 1); put16(f, 2); put32(f, 44118u); put32(f, 44118u * 6u); put16(f, 6); put16(f, 24);
    fwrite("data", 1, 4, f); put32(f, data);
    for (i = 0; i < 2u * frames; i++) {
        uint32_t v = (uint32_t)lr[i];
        fputc((int)(v & 255), f); fputc((int)(v >> 8 & 255), f); fputc((int)(v >> 16 & 255), f);
    }
    return fclose(f) ? -1 : 0;
}
