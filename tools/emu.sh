#!/bin/bash
# Option B (step 3): the real .fwsc in fm1-emu's headless runner (joakimeriksson/fm1-emu, built from commit 1c6e80a in
# refs/fm1-emu: target/release/fm1-emu). Both cores, an interpreter: ~1/50 of real time idle, much slower while the
# tanpura computes. Keys: fm1-emu's --keys ('2500:SEQ' presses SEQ at 2.5 s for 150 ms; the boot screen lasts ~2 s).
#   tools/emu.sh PACKAGE.fwsc MS 'KEYS' OUTDIR [SNAP_MS,...]      -> OUTDIR/out.wav, OUTDIR/work/lcd_MS.bmp, OUTDIR/log.txt
# FM1_STRICT=1 in the environment: each instruction takes its measured cycles (fm1-emu's realistic mode).
set -e
EMU="$(cd "$(dirname "$0")/.." && pwd)/refs/fm1-emu/target/release/fm1-emu"
[ -x "$EMU" ] || { echo "build it: cd refs/fm1-emu && ~/.cargo/bin/cargo build --release -p fm1-emu"; exit 2; }
PKG="$(realpath "$1")"; MS="$2"; KEYS="$3"; OUT="$4"; SNAP="${5:-}"
mkdir -p "$OUT"
cd "$OUT"
mkdir -p work
"$EMU" --fwsc "$PKG" --ms "$MS" ${KEYS:+--keys "$KEYS"} --wav out.wav ${SNAP:+--snap "$SNAP"} > log.txt 2>&1 || true
grep -E "^time |audio halves|ISR|^wav:|stopped|halt|fault|cpu1" log.txt | head -12
