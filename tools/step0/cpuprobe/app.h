/* Step 0 CPU probe (not a musical function): N test oscillators in the render, the platform's
 * render-time percentage on screen. Built on fm1-base (MPL-2.0). */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "CPUPROBE"
#define APP_SPLASH_RGB RGB(200, 140, 60)
#define APP_BLOCK 256u                   /* platform block decided at step 0 (ElectroNick, 2026-11-06) */
#define APP_CPU_KHZ
#define APP_HOST_EXPECT

enum { OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
int app_expect(const char *what, int *got);
