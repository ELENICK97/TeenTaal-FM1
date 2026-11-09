#!/usr/bin/env python3
"""Step 2a, second listening round (ElectroNick, 2026-11-06: "sounds like a tanpura, but like a Minimoog cutoff sweep on
a sawtooth; decay a bit fast; thread 6 mm; deeper and richer; the oversampled one is richer").

Every file: 2x oversampled (the quality target), thread at 6 mm, 4 s cycle, 32 s, MONO (p_d + p_n: the FM-1 is
mono), one shared gain. Each variant adds one change to the one before, so each step can be heard on its own:
  A  as round 1 (the paper's pluck: the whole string, first mode only), thread 6 mm, mono
  B  + finger pluck: 2 cm wide at 35 % of the length (excites many modes from the start)
  C  + longer sound: string damping sigma0 0.6 -> 0.3, thread damping r_c 1.2 -> 0.6
  D  + human: plucks +-25 ms and +-15 % in strength, the second Sa string +1 cent (a slow beat)
  E  = D on a large tanpura, Sa A2 (two octaves below the A flute), strings 1.1 m
    ~/.venvs/teentaal/bin/python tools/reference/round2.py"""
import os, sys
import numpy as np
from scipy.io import wavfile
sys.path.insert(0, os.path.dirname(__file__))
import analyze as A
import matplotlib.pyplot as plt

OUT = os.path.join(A.OUT, "round2")
FM1 = A.FM1
CYCLE, DUR = 4.0, 32.0
STARTS = [0.0, 1.2, 1.8, 2.4]                     # Pa, Sa, Sa, kharaj within a cycle

def plucks(human, seed=7):
    rng = np.random.default_rng(seed)
    out = []
    k = 0
    while k * CYCLE < DUR:
        for s, t0 in enumerate(STARTS):
            t, a = k * CYCLE + t0, 1.0
            if human:
                t += rng.uniform(-0.025, 0.025)
                a *= rng.uniform(0.85, 1.15)
            if 0 <= t < DUR:
                out += ["-p", f"{s}:{t:.4f}:{a:.3f}"]
        k += 1
    return out

def cents(f, c):
    return f * 2 ** (c / 1200)

A3 = dict(L=0.85, strings=[(164.81, "steel", 0.13e-3), (220.0, "steel", 0.13e-3), (220.0, "steel", 0.13e-3),
                           (110.0, "bronze", 0.18e-3)])
A2 = dict(L=1.10, strings=[(82.41, "steel", 0.16e-3), (110.0, "steel", 0.16e-3), (110.0, "steel", 0.16e-3),
                           (55.0, "bronze", 0.25e-3)])

def args(t, finger, longer, human, damping=("0.3", "0.6")):
    a = ["-L", str(t["L"]), "-xc", "6e-3"]
    for j, (f, m, r) in enumerate(t["strings"]):
        if human and j == 2:
            f = cents(f, 1.0)
        a += ["-s", f"{f:.4f}:{m}:{r}"]
    if finger:
        a += ["-xe", f"{0.35 * t['L']:.4f}", "-we", "0.02"]
    if longer:
        a += ["-s0", damping[0], "-rc", damping[1]]
    return a + plucks(human)

VARIANTS = [
    ("A_as_round1_mono", A3, False, False, False),
    ("B_finger_pluck", A3, True, False, False),
    ("C_finger_longer", A3, True, True, False),
    ("D_finger_longer_human", A3, True, True, True),
    ("E_large_tanpura_sa_A2", A2, True, True, True),
]

def main():
    os.makedirs(OUT, exist_ok=True)
    mono, logs = {}, {}
    for name, t, fi, lo, hu in VARIANTS:
        fs, Fd, Fn, _, log = A.run(name, args(t, fi, lo, hu), 2 * FM1, DUR)
        pd, pn = A.radiate(fs, Fd, Fn, FM1)
        mono[name], logs[name] = pd + pn, log
        print(name, "\n  " + log.replace("\n", "\n  "))
    g = 10 ** (-1 / 20) / max(np.max(np.abs(x)) for x in mono.values())
    fig, ax = plt.subplots(len(mono), 1, figsize=(12, 2.4 * len(mono)), sharex=True)
    for k, (name, x) in enumerate(mono.items()):
        wavfile.write(os.path.join(OUT, name + ".wav"), int(round(FM1)), (x * g * 32767).astype(np.int16))
        A.spec(ax[k], x, FM1, name, fmax=12000, dur=12)
        lv = 20 * np.log10(np.sqrt(np.mean((x * g) ** 2)))
        print(f"{name}: RMS {lv:.1f} dBFS")
    ax[-1].set_xlabel("s")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "round2_spectrograms.png"), dpi=80)

FINAL = ("0.4", "0.8")      # third round: between B and C (ElectroNick: C "too loud for too much")

def final():
    """the reference chosen after the second round: large tanpura Sa A2 (and A3 to compare), finger pluck,
    damping between B and C, human; at 2x (the quality target) and at the FM-1's rate (the 2b baseline)"""
    out = os.path.join(A.OUT, "final")
    os.makedirs(out, exist_ok=True)
    runs = [("REF_sa_A2_large_2x", A2, 2), ("REF_sa_A2_large_1x", A2, 1), ("REF_sa_A3_2x", A3, 2)]
    mono = {}
    for name, t, k in runs:
        fs, Fd, Fn, _, log = A.run(name, args(t, True, True, True, FINAL), k * FM1, DUR)
        pd, pn = A.radiate(fs, Fd, Fn, FM1)
        mono[name] = pd + pn
    g = 10 ** (-1 / 20) / max(np.max(np.abs(x)) for x in mono.values())
    fig, ax = plt.subplots(len(mono), 1, figsize=(12, 2.6 * len(mono)), sharex=True)
    for k, (name, x) in enumerate(mono.items()):
        wavfile.write(os.path.join(out, name + ".wav"), int(round(FM1)), (x * g * 32767).astype(np.int16))
        np.save(os.path.join(A.TMP, name + ".npy"), x * g)
        A.spec(ax[k], x, FM1, name, fmax=16000, dur=12)
        print(f"{name}: RMS {20*np.log10(np.sqrt(np.mean((x*g)**2))):.1f} dBFS")
    ax[-1].set_xlabel("s")
    fig.tight_layout()
    fig.savefig(os.path.join(out, "final_spectrograms.png"), dpi=80)

if __name__ == "__main__":
    A.TMP = os.environ.get("TANPURA_TMP", A.TMP)
    final() if sys.argv[1:] == ["final"] else main()
