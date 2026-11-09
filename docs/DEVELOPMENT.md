# Development

How TeenTaal-FM1 is built, tested and measured. The detailed, step-by-step development logs are in Italian:
[PROGRESS.md](../PROGRESS.md) (what was done and measured at each step, with ElectroNick's answers) and
[DECISIONS.md](../DECISIONS.md) (every technical decision, with its reasons and the alternatives rejected).
[CLAUDE.md](../CLAUDE.md) holds the instructions the AI assistant followed (in Italian).

## How it was made

The firmware was written by an AI coding assistant (Claude, by Anthropic) working in steps defined in CLAUDE.md, each
ending with a stop for ElectroNick to listen and approve:

| Step | Content |
|---|---|
| 0 | survey of the hardware and of the base (fm1-base), measurements in the emulator |
| 1 | skeleton: modules, mixer, host render tool, regression tests, the reference tone |
| 2a | the tanpura reference model (the DAFx-16 paper, full, double precision, host only) |
| 2b | the tanpura reduced for the FM-1 (float32, half rate, fewer modes, IIR body) with A/B listening |
| 3 | the tanpura on the FM-1, on the second core |
| 4 | tabla approach: modal synthesis rejected by ear ("sounds like a videogame"), recorded strokes chosen |
| 5 | the full stroke library and the bol table |
| 6 | the taal engine and the ten taals |
| 7 | tabla and taal on the FM-1, their screen |
| 8 | integration: saving, final controls, budget with everything on, documents |

## The hardware (measured at step 0)

| | |
|---|---|
| SoC | JieLi AC791N (WL82), pi32v2, 2 cores at 360 MHz, single-precision FPU |
| Audio | 44117.647 Hz (24 MHz / 544), stereo Q23 in int32, blocks of 256 frames |
| Flash | 1 MB; application slot 581,564 B |
| RAM | `.data+.bss` 96 KB, pool 336 KB, RAM code 32 KB |
| Panel | 240×240 LCD, 27 keys (F3–G5), 14 buttons, 7 encoders, MASTER pot |

## Layout

```
dsp/        shared DSP: constants, tuning (equal temperament, Sa in cents, A reference), sine table, ramps
tanpura/    the tanpura: modal strings with bridge contact (tstring.c), the plucking cycle and body (tanpura.c)
tabla/      the tabla: ADPCM voices, choke, round robin, dynamics, dayan retuning; generated sample and bol tables
taal/       the taal engine: exact integer position, events to the tabla; generated taal tables
reftone/    the reference tone
mixer/      the mixer and engine.c, the one audio path shared by the firmware and the host tools
ui/         screens and controls (included by app.c)
app.c/.h    the glue with fm1-base: two-core pipeline, store objects
data/       tabla strokes and bols (JSON), taals (JSON, FORMAT.md): converted to C at build time
tools/      host render tools, tests, data generators, the tanpura reference model, emulator and site scripts
platform/   fm1-base and felucca-bsp (MPL-2.0), pinned copy: FM1-BASE-VERSION.txt
```

Every audio module has the same interface: `x_init`, `x_set_param`, `x_render(m, int32_t *out, 64)` (adds mono Q23).
The same DSP sources build for the host (gcc) and the FM-1 (JieLi clang), with no `#ifdef` in the DSP; float32 only,
`-ffp-contract=off` on both, no double, no allocation, no division or transcendental function per sample.

## The sound engines

**Tanpura.** The model of van Walstijn, Bridges and Mehes (DAFx-16): each string is a sum of modes with stiffness and
frequency-dependent damping (σ0, σ1, σ3); the bridge contact (jawari) is solved in closed form each step; the cotton
thread is a spring-damper whose position is the "jawari" control. Reduced for the FM-1: strings at half rate
(22 kHz) then a half-band upsampler, modes truncated per string (6/8/8/3 kHz), the body as 24 parallel IIR sections
fitted to the paper's response, an 8.5 kHz low-pass against aliasing, loudness compensation over Sa and jawari, the
geometry following Sa. It runs on the second core: about 58 % of it at 360 MHz (estimated from the emulator).
The full reference model is in `tools/reference/tanpura_ref.c` (host only).

**Tabla.** 23 takes of 12 strokes from the Freesound pack "tabla bols" by mmiron (CC0), trimmed to about 1 s, stored
as IMA ADPCM (dayan at 44.1 kHz, bayan at 22 kHz: 220 KB in the application image). The dayan is retuned to Sa by
resampling (4-point Hermite) from the recordings' 315.2 Hz; the bayan keeps its pitch. A new stroke chokes the
previous one on the same drum in 8 ms; takes rotate; softer strokes are quieter and darker. Bols are one or two
strokes (`data/tabla/bols.json`). Mappings chosen: Ta = Na, Dhi/Di = Dhin (Ge + Tin), Ki/Kat/Kit = Ke, Tit = Te,
Tin = the pack's "na-open" (the pack has no Tin); for the pakhawaj taals Ka = Ke, Ga = Ge.

**Taal engine.** The position in a matra is an exact integer (a matra = 60 × 24,000,000 units, a sample = 544 × tempo
units): no drift; every stroke falls within one sample of its exact time at any tempo (tested at 20, 137 and 600
matras a minute for every taal). A tempo change keeps the position; a taal change waits for the end of the cycle.
The sam is accented; a small random irregularity in timing (±4 ms at most) and strength is adjustable.

**Two cores.** The second core renders the tanpura's next block while the first plays the current one (one block,
5.8 ms, of latency for the drone); tabla, taal, tone, mixer and screen run on the first core.

## Building

The firmware is built with fm1-base's tools; fm1-base's README explains the toolchain (JieLi clang for pi32v2) and
the AC79 SDK. Then, from the project's root:

```bash
make                    # host tools: build-host/render, tabla_demo, regress, ...
make test               # regression tests (hashes of the renders, CPU cost, taal timing, data checks)
tools/fw.sh             # the firmware: build/teentaal.fwsc (development identity FM-1_7700)
tools/fw.sh --host      # fm1-base's host simulator with the scripted test tools/test/fw_basic.txt
tools/web.sh            # the browser emulator at http://localhost:8790/ (needs Emscripten)
tools/emu.sh PKG MS 'KEYS' OUTDIR   # the real package in fm1-emu (headless; FM1_STRICT=1 for realistic timing)
tools/site.sh 1.0       # release build + installer page at http://localhost:8791/install/ (no m-bootloader)
```

Host tools (render a scenario to WAV): `build-host/render OUT.wav SECONDS 0:sa=A3 0:tan.on=1 0:taal.on=1 ...`
(events: `tools/common/scen.h`); `build-host/tabla_demo` plays bols at exact times.
Data: `tools/tabla_data/build_samples.py` (needs Python with numpy and scipy, and the pack's WAVs in
`docs/references/tabla_mmiron/wav/`) regenerates `tabla/tabla_samples.h`; `tools/taal_data/build_taal.py` regenerates
`taal/taal_tables.h` from `data/taal/`.

## Measurements (version 1.0, all sources on)

Measured in fm1-emu in realistic mode, which runs at 240 MHz; the FM-1 runs at 360 MHz.

| | at 240 MHz (measured) | at 360 MHz (estimated) | budget |
|---|---|---|---|
| First core: tabla, taal, tone, mixer | 5 % | ~3.3 % | 35 % |
| Second core: tanpura | 87–89 % | ~58–59 % | 85 % |
| Application image | 331 KB of 581 KB | | ≥ 150 KB free |
| RAM `.data+.bss` | 71.4 KB of 96 KB | | ≥ 25 % free |

Settings are saved in an fm1-base store object at flash offset 0xDA000/0xDB000, in the application's data area.

## Known limits and open points

- **Not tested on a real FM-1**: CPU load, speaker sound, flash timings, the install itself.
- The tanpura's highs are a little darker than the full reference model (strings at half rate). A proposal to improve
  it once the real CPU load is known is in DECISIONS.md ("Proposte rimandate").
- The tabla recordings are of one player and one instrument; some strokes have a single take; the pack has no real Tin.
- At vilambit tempos the theka is played bare, without fillers.
- In the emulator at 240 MHz the tanpura uses 87–89 % of the second core; at the FM-1's 360 MHz this is ~59 %.
- `fm1.toml` sets the package identity prefix `FM-1_77`: chosen not to collide with known firmwares at the time.

## Publishing a release

1. `tools/fw.sh --web` (the browser emulator) and the release package:
   `cd platform/fm1-base && python3 tools/build.py ../.. --release X.Y`, then copy `build/teentaal-X.Y.fwsc` to
   `release/`.
2. Run it in fm1-emu (`tools/emu.sh release/teentaal-X.Y.fwsc ...`) before anything else: only a package that ran
   in the emulator is published.
3. `tools/site.sh X.Y` builds the installer site in `build/site` (without m-bootloader) and serves it locally to try.
4. On GitHub: a Release with the `.fwsc` attached and its sha256 in the notes; the site's files on the `gh-pages`
   branch for <https://elenick97.github.io/TeenTaal-FM1/>.
