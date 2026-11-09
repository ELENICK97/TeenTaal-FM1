/* app.h — the app's half of the contract with fm1-base (platform/fm1-base: device/unity.c, host/plat_host.c).
 * The project's root is fm1-base's APP_DIR: fm1.toml, app.h, app.c, words.tsv. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "TEENTAAL"
#define APP_SPLASH_RGB RGB(232, 150, 60)
#define APP_BLOCK 256u                   /* platform block (DECISIONS.md); the modules run 4 sub-blocks of 64 */
#define APP_CPU_KHZ                      /* plat_cpu_khz, shown on the provisional screen */

/* the store (step 8): the settings SAVE keeps (ui/ui.c), and the platform's panel calibration; each object a pair of
 * sectors (A/B) in the app's data area (plat.h PLAT_DATA_LO..HI): 0xDA000/0xDB000 and 0xDC000/0xDD000 */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xDA000u, 0xDC000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

#define APP_HOST_EXPECT                  /* the host simulator's checks (tools/test/fw_basic.txt) */
int app_expect(const char *what, int *got);
