#!/usr/bin/env python3
"""Step 2b: the float tanpura (tanpura/, through build-host/tanpura_var) against the step 2a gold reference
(renders/step-2a/final/REF_sa_A2_large_2x.wav), for a series of mode truncations.

Metrics, all on the mono output over 32 s (the irregularity makes waveforms differ, so: statistics):
  - long-term spectrum in third octaves 80 Hz - 16 kHz, each normalised to its total power: RMS and max dB difference
  - spectral centroid over time (40 ms frames): mean absolute difference, Hz
  - envelope (RMS in 50 ms frames, dB): mean absolute difference of the per-cycle average envelope
    ~/.venvs/teentaal/bin/python tools/reference/compare2b.py [FMAX ...]"""
import json, os, subprocess, sys
import numpy as np
from scipy import signal
from scipy.io import wavfile
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "renders", "step-2b")
REF = os.path.join(ROOT, "renders", "step-2a", "final", "REF_sa_A2_large_2x.wav")
VAR = os.path.join(ROOT, "build-host", "tanpura_var")
FS = 24e6 / 544
CYCLE = 4.0

def load(path):
    r, x = wavfile.read(path)
    x = x.astype(np.float64)
    return x[:, 0] if x.ndim == 2 else x

def bands(x):
    f, P = signal.welch(x, FS, nperseg=8192)
    cs = 80 * 2 ** (np.arange(0, 24) / 3)
    out = np.array([np.sum(P[(f >= c / 2 ** (1 / 6)) & (f < c * 2 ** (1 / 6))]) for c in cs if c < 16000])
    return 10 * np.log10(out / out.sum() + 1e-30)

def centroid(x, n=1764, h=882):
    w = np.hanning(n)
    f = np.fft.rfftfreq(n, 1 / FS)
    c = []
    for i in range(0, len(x) - n, h):
        P = np.abs(np.fft.rfft(x[i:i + n] * w)) ** 2
        c.append(np.sum(f * P) / (np.sum(P) + 1e-30))
    return np.array(c)

def cycle_env(x):
    """the RMS envelope (dB) averaged over the cycles after the first"""
    h = int(0.05 * FS)
    e = np.array([np.sqrt(np.mean(x[i:i + h] ** 2)) for i in range(0, len(x) - h, h)])
    per = int(CYCLE / 0.05)
    k = len(e) // per
    m = e[per:k * per].reshape(k - 1, per).mean(axis=0)
    return 20 * np.log10(m / m.max() + 1e-9)

def compare(x, ref):
    bx, br = bands(x), bands(ref)
    cx, cr = centroid(x), centroid(ref)
    n = min(len(cx), len(cr))
    ex, er = cycle_env(x), cycle_env(ref)
    # the centroid averaged per position in the cycle (the irregularity averaged out)
    per = int(CYCLE * FS / 882)
    k = n // per
    ccx = cx[per:k * per].reshape(k - 1, per).mean(axis=0)
    ccr = cr[per:k * per].reshape(k - 1, per).mean(axis=0)
    return {"ltas_rms_db": round(float(np.sqrt(np.mean((bx - br) ** 2))), 2),
            "ltas_max_db": round(float(np.max(np.abs(bx - br))), 2),
            "centroid_cycle_mae_hz": round(float(np.mean(np.abs(ccx - ccr))), 0),
            "centroid_mean_hz": [round(float(cx.mean()), 0), round(float(cr.mean()), 0)],
            "envelope_cycle_mae_db": round(float(np.mean(np.abs(ex - er))), 2)}, (bx, br, ccx, ccr)

def main():
    fmaxes = sys.argv[1:] or ["6000,8000,8000,3000", "4000,5000,5000,2000", "3000,4000,4000,1500"]
    os.makedirs(OUT, exist_ok=True)
    ref = load(REF)
    res = {}
    fig, ax = plt.subplots(2, 1, figsize=(10, 8))
    for fm in fmaxes:
        tag = fm.replace(",", "_")
        wav = os.path.join(OUT, f"var_{tag}.wav")
        p = subprocess.run([VAR, fm, "160", "-1500", "32", wav], capture_output=True, text=True, check=True)
        modes = [int(v) for v in p.stdout.split("modes")[1].split("host")[0].split()]
        hips = float(p.stdout.split("host_instr_per_sample")[1])
        r, (bx, br, ccx, ccr) = compare(load(wav), ref)
        r.update({"modes": modes, "modes_total": sum(modes), "host_instr_per_sample": round(hips)})
        r["bands_below_10k"] = round(float(np.sqrt(np.mean((bx - br)[:20] ** 2))), 2)
        res[fm] = r
        print(fm, r)
        ax[0].plot(bx - br, label=f"{fm} Hz ({sum(modes)} modes, {sum(modes)/2:.0f} steps/sample)")
        ax[1].plot(np.arange(len(ccx)) * 882 / FS, ccx, lw=0.8, label=fm)
    ax[1].plot(np.arange(len(ccr)) * 882 / FS, ccr, "k", lw=1.5, label="reference 2x")
    ax[0].axhline(0, color="k", lw=0.5)
    ax[0].set_title("third-octave long-term spectrum, float model - reference (dB); bands from 80 Hz")
    ax[0].legend(fontsize=7)
    ax[1].set_title("spectral centroid, averaged over the cycles (Hz)")
    ax[1].set_xlabel("s within the 4 s cycle")
    ax[1].legend(fontsize=7)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "variants_vs_reference.png"), dpi=90)
    with open(os.path.join(OUT, "variants.json"), "w") as f:
        json.dump(res, f, indent=1)

if __name__ == "__main__":
    main()
