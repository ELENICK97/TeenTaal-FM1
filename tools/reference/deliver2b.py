#!/usr/bin/env python3
"""Step 2b deliverables: 32 s WAVs of the firmware tanpura (build-host/tanpura_var: tanpura/ exactly as on the FM-1),
an A/B file against the gold reference, side-by-side spectrograms.
    ~/.venvs/teentaal/bin/python tools/reference/deliver2b.py"""
import os, subprocess
import numpy as np
from scipy.io import wavfile
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import signal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "renders", "step-2b")
VAR = os.path.join(ROOT, "build-host", "tanpura_var")
REF = os.path.join(ROOT, "renders", "step-2a", "final", "REF_sa_A2_large_2x.wav")
FS = 24e6 / 544
Q, M, E = "6000,8000,8000,3000", "4000,5000,5000,2000", "3000,4000,4000,1500"
SA = {"F#2": -1800, "A2": -1500, "D3": -1000, "A3": -300}
SW = {"Pa": 7, "Ma": 5, "Ni": 11}
FILES = [  # name, setting, Sa, jawari, first
    ("01_Q_quality_saA2_Pa_jawari50", Q, "A2", 50, "Pa"),
    ("02_M_middle_saA2_Pa_jawari50", M, "A2", 50, "Pa"),
    ("03_E_economy_saA2_Pa_jawari50", E, "A2", 50, "Pa"),
    ("04_Q_saA2_Ma", Q, "A2", 50, "Ma"),
    ("05_Q_saA2_Ni", Q, "A2", 50, "Ni"),
    ("06_Q_saF#2_Pa", Q, "F#2", 50, "Pa"),
    ("07_Q_saD3_Pa", Q, "D3", 50, "Pa"),
    ("08_Q_saA3_Pa", Q, "A3", 50, "Pa"),
    ("09_Q_saA2_jawari_light_0", Q, "A2", 0, "Pa"),
    ("10_Q_saA2_jawari_strong_100", Q, "A2", 100, "Pa"),
]

def load(p):
    r, x = wavfile.read(p)
    x = x.astype(np.float64)
    x = x[:, 0] if x.ndim == 2 else x
    return x / (2.0 ** 31 if x.dtype == np.int32 or np.abs(x).max() > 40000 else 32768.0)

def main():
    os.makedirs(OUT, exist_ok=True)
    for name, cfg, sa, jw, first in FILES:
        p = subprocess.run([VAR, cfg, "96", str(SA[sa]), "32", os.path.join(OUT, name + ".wav"), str(jw), str(SW[first])],
                           capture_output=True, text=True, check=True)
        print(name, p.stdout.strip())
    # Q without the brightness correction, for the comparison ElectroNick asked about
    noeq = os.path.join(OUT, "_Q_without_eq.wav")
    subprocess.run([VAR, Q, "96", str(SA["A2"]), "32", noeq, "50", "7", "0"], capture_output=True, check=True)
    # A/B: 8 s of the reference, 8 s of the firmware tanpura (quality, corrected), twice, matched in RMS
    ref, q, e = load(REF), load(os.path.join(OUT, FILES[0][0] + ".wav")), load(os.path.join(OUT, FILES[2][0] + ".wav"))
    q0 = load(noeq)
    os.remove(noeq)
    witheq = os.path.join(OUT, "_Q_with_eq.wav")
    subprocess.run([VAR, Q, "96", str(SA["A2"]), "32", witheq, "50", "7", "1"], capture_output=True, check=True)
    q1 = load(witheq)
    os.remove(witheq)
    q1 *= np.sqrt(np.mean(ref ** 2)) / np.sqrt(np.mean(q1 ** 2))
    n8 = int(8 * FS)
    rms = lambda x: np.sqrt(np.mean(x ** 2))
    q *= rms(ref) / rms(q)
    q0 *= rms(ref) / rms(q0)
    e *= rms(ref) / rms(e)
    g = 10 ** (-1 / 20) / max(np.max(np.abs(ref)), np.max(np.abs(q)))
    gap = np.zeros(int(0.5 * FS))
    def seg(x, k):                                   # segment k, faded in and out over 20 ms
        y = x[k * n8:(k + 1) * n8]
        i = np.arange(len(y))
        return y * np.minimum(1, np.minimum(i, i[::-1]) / (0.02 * FS))
    ab = np.concatenate([seg(ref, 0), gap, seg(q, 1), gap, seg(ref, 2), gap, seg(q, 3)]) * g
    for old_name in ("AB_reference_then_Q_every_8s.wav",):
        if os.path.exists(os.path.join(OUT, old_name)):
            os.remove(os.path.join(OUT, old_name))
    wavfile.write(os.path.join(OUT, "AB_ref_Q_ref_Q_8s_each_with_pauses.wav"), int(round(FS)), (ab * 32767).astype(np.int16))
    # the two whole, separately, at the same gain: to switch between them by hand
    wavfile.write(os.path.join(OUT, "AB_1_reference.wav"), int(round(FS)), (ref * g * 32767).astype(np.int16))
    for old_name in ("AB_2_Q_firmware.wav",):
        if os.path.exists(os.path.join(OUT, old_name)):
            os.remove(os.path.join(OUT, old_name))
    wavfile.write(os.path.join(OUT, "AB_2_Q_as_before.wav"), int(round(FS)), (q0 * g * 32767).astype(np.int16))
    wavfile.write(os.path.join(OUT, "AB_3_Q_brighter.wav"), int(round(FS)), (q1 * g * 32767).astype(np.int16))
    fig, ax = plt.subplots(3, 1, figsize=(12, 9), sharex=True)
    for a, x, t in zip(ax, (ref, q, e), ("gold reference (step 2a, double, 2x, measured body)",
                                         "Q: firmware tanpura, quality (the default: no brightness correction)",
                                         "E: firmware tanpura, economy (3/4/4/1.5 kHz)")):
        f, tt, S = signal.spectrogram(x[: int(12 * FS)], FS, nperseg=2048, noverlap=1536)
        S = 10 * np.log10(S + 1e-20)
        a.pcolormesh(tt, f / 1000, S, vmin=S.max() - 90, vmax=S.max(), shading="auto", cmap="magma")
        a.set_ylim(0, 12); a.set_title(t, fontsize=9); a.set_ylabel("kHz")
    ax[-1].set_xlabel("s")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "spectrograms_reference_Q_E.png"), dpi=80)

if __name__ == "__main__":
    main()
