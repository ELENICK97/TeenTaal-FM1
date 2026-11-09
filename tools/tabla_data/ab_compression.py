#!/usr/bin/env python3
"""Step 5: A/B of the compression, every take at its own pitch: the recording as prepared for the firmware (trimmed,
cut at 1 s), then the same after IMA ADPCM (the bayan's also at half the rate). Writes OUTDIR/AB_<drum>.wav and a
listing of the order.   tools/tabla_data/ab_compression.py OUTDIR"""
import json, os, sys
import numpy as np
from scipy import signal
from scipy.io import wavfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE); sys.path.insert(0, os.path.join(HERE, "..", "tabla_proto"))
import adpcm, build_samples as B, tabla_samples as TS

def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    strokes = B.read_json("strokes.json")
    for drum in ("dayan", "bayan"):
        seq, lines = [], []
        gap = np.zeros(int(0.35 * TS.FS))
        for s, v in strokes.items():
            if s.startswith("_") or v["drum"] != drum:
                continue
            for t in v["takes"]:
                q = B.prepare(t, "dayan")                       # (at the full rate: the original)
                orig = q.astype(np.float64) / 32768
                if drum == "bayan":
                    lo = np.clip(np.round(signal.resample_poly(orig, 1, 2) * 32767), -32768, 32767).astype(np.int16)
                else:
                    lo = q
                d, idx = adpcm.encode(lo)
                dec = adpcm.decode(d, len(lo), idx).astype(np.float64) / 32768
                if drum == "bayan":
                    dec = signal.resample_poly(dec, 2, 1)[: len(orig)]
                snr = 10 * np.log10(np.sum(orig ** 2) / np.sum((orig - dec[: len(orig)]) ** 2))
                t0 = sum(len(x) for x in seq) / TS.FS
                lines.append(f"{t0:6.2f} s  {s:5s} {t:16s} A = original, B = stored (after {len(orig) / TS.FS + 0.35:.2f} s); "
                             f"error {snr:.1f} dB under the signal")
                seq += [orig * 0.7, gap, dec * 0.7, gap, gap]
        x = np.concatenate(seq)
        wavfile.write(os.path.join(out, f"AB_{drum}.wav"), 44118, (x * 32767).astype(np.int16))
        with open(os.path.join(out, f"AB_{drum}.txt"), "w") as f:
            f.write("\n".join(lines) + "\n")
        print(f"AB_{drum}.wav: {len(lines)} takes")

if __name__ == "__main__":
    main()
