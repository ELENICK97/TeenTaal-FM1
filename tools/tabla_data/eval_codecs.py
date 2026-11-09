#!/usr/bin/env python3
"""Step 5: which storage for the tabla recordings? For every take: the recording (trimmed as for the firmware) against
its stored-and-decoded version, in four formats. Error = signal-to-error ratio over the whole take and over its
attack (the first 20 ms), and the error's level in the tail; size in KB.
    ~/.venvs/teentaal/bin/python tools/tabla_data/eval_codecs.py"""
import os, sys
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tabla_proto"))
import adpcm
import tabla_samples as TS

FS = TS.FS

def mulaw(x):
    mu = 255.0
    y = np.sign(x) * np.log1p(mu * np.abs(x)) / np.log1p(mu)
    q = np.round(y * 127) / 127
    return np.sign(q) * ((1 + mu) ** np.abs(q) - 1) / mu

def snr(ref, x):
    return 10 * np.log10(np.sum(ref ** 2) / (np.sum((ref - x) ** 2) + 1e-20))

def store(x, fmt):
    """x at FS, float -1..1 -> (decoded at FS, bytes)"""
    if fmt.endswith("22"):
        lo = signal.resample_poly(x, 1, 2)
    else:
        lo = x.copy()
    if fmt.startswith("adpcm"):
        q = np.clip(np.round(lo * 32767), -32768, 32767).astype(np.int16)
        data, idx = adpcm.encode(q)
        y = adpcm.decode(data, len(q), idx).astype(np.float64) / 32767
        size = len(data) + 4
    elif fmt.startswith("mulaw"):
        y = mulaw(lo)
        size = len(lo)
    else:
        y = np.round(lo * 32767) / 32767
        size = 2 * len(lo)
    if fmt.endswith("22"):
        y = signal.resample_poly(y, 2, 1)[: len(x)]
    return y, size

def main():
    names = sorted({nm for _, names in TS.STROKES.values() for nm in names} | {"dhec", "ghe", "ghe_8", "tas"})
    fmts = ["pcm16_22", "adpcm_22", "adpcm_44", "mulaw_22"]
    tot = {f: [0, []] for f in fmts}
    print(f"{'take':16s} {'s':>5s}  " + "  ".join(f"{f:>21s}" for f in fmts))
    print(f"{'':16s} {'':>5s}  " + "  ".join(f"{'SNR all/attack, KB':>21s}" for f in fmts))
    for nm in names:
        x = TS.load(nm)
        x = x[: int(1.05 * FS)] if len(x) > 1.05 * FS else x          # (the long ones: cut at ~1 s, as agreed)
        if len(x) > int(1.0 * FS):
            f = int(0.05 * FS); x = x.copy(); x[int(1.0 * FS) - f:int(1.0 * FS)] *= np.linspace(1, 0, f); x = x[: int(1.0 * FS)]
        x = x / np.max(np.abs(x)) * 0.9
        a = int(0.02 * FS)
        # the reference for the 22 kHz formats is the band-limited original (the rate's own loss is shown by pcm16_22)
        row = []
        for f in fmts:
            y, size = store(x, f)
            row.append(f"{snr(x, y):5.1f}/{snr(x[:a], y[:a]):5.1f} dB {size/1024:5.1f}")
            tot[f][0] += size
            tot[f][1].append(snr(x, y))
        print(f"{nm:16s} {len(x)/FS:5.2f}  " + "  ".join(f"{r:>21s}" for r in row))
    print("total KB:  " + "  ".join(f"{f}: {tot[f][0]/1024:.0f} (median SNR {np.median(tot[f][1]):.1f} dB)" for f in fmts))

if __name__ == "__main__":
    main()
