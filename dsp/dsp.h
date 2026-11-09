/* dsp.h — constants shared by every audio module.
 *
 * The same code builds for the host (tools/render, tools/test) and for the FM-1: no #ifdef here.
 * Float DSP: single precision only, compiled with -ffp-contract=off on both targets (DECISIONS.md). */
#pragma once
#include <stdint.h>

/* the FM-1's sample rate: 24 MHz / 544 = 44117.647 Hz (felucca-bsp boards/fm1/fm1_audio.h) */
#define DSP_FS_NUM 24000000
#define DSP_FS_DEN 544
#define DSP_FS 44117.647f

/* modules render in sub-blocks of this many samples (the platform's block is 256: 4 sub-blocks) */
#define DSP_BLOCK 64

/* module output: mono, Q23 in int32 (the platform's scale: full scale +-8388607) */
#define DSP_Q23_MAX 8388607
#define DSP_Q23_F 8388607.0f

/* parameter ids shared by every module (the tuning); module ids start at DSP_P_MODULE */
enum {
    DSP_P_A4_DHZ = 1,        /* reference A4 in tenths of a hertz (4400 = 440.0 Hz) */
    DSP_P_SA_CENTS = 2,      /* Sa (middle octave) in cents above C4 at that reference (-1100 = C#3) */
    DSP_P_MODULE = 16
};
