#!/usr/bin/env python3
"""Step 2b: the tanpura's loudness (RMS over 3 cycles after the first) on a grid of Sa x jawari, through the render
tool, for the level compensation table in tanpura/tanpura.c.   ~/.venvs/teentaal/bin/python tools/reference/level_grid.py"""
import os, subprocess, tempfile
import numpy as np
from scipy.io import wavfile
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SA = [-1800, -1500, -1000, -300, 0, 700]
JW = [0, 25, 50, 75, 100]
def measure(sa, jw):
    with tempfile.TemporaryDirectory() as d:
        w = os.path.join(d, "x.wav")
        subprocess.run([os.path.join(ROOT, "build-host", "render"), w, "16", f"0:sa={sa}", f"0:tan.jawari={jw}",
                        "0:tan.on=1"], check=True, capture_output=True)
        r, x = wavfile.read(w)
    x = x[:, 0].astype(np.float64) / 2.0 ** 31        # (scipy reads 24-bit PCM as int32, full scale 2^31)
    seg = x[int(4.0 * 44117.6):int(16.0 * 44117.6)]
    return 20 * np.log10(np.sqrt(np.mean(seg ** 2)) + 1e-12), 20 * np.log10(np.max(np.abs(seg)) + 1e-12)
rms = np.zeros((len(SA), len(JW))); pk = np.zeros_like(rms)
for i, sa in enumerate(SA):
    for j, jw in enumerate(JW):
        rms[i, j], pk[i, j] = measure(sa, jw)
ref = rms[SA.index(-1500), JW.index(50)]
print("RMS relative to Sa A2, jawari 50 (dB); rows Sa", SA, "columns jawari", JW)
for i, sa in enumerate(SA):
    print(f"{sa:6d}: " + " ".join(f"{rms[i, j] - ref:6.1f}" for j in range(len(JW))) + "   peak-RMS: " + " ".join(f"{pk[i, j] - rms[i, j]:5.1f}" for j in range(len(JW))))
print("C table (dB to ADD):")
print("static const float LEVEL_DB[%d][%d] = {" % (len(SA), len(JW)))
for i in range(len(SA)):
    print("    {" + ", ".join(f"{ref - rms[i, j]:.1f}f" for j in range(len(JW))) + "},")
print("};")
