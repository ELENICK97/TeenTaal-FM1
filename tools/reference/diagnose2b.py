#!/usr/bin/env python3
"""Step 2b, after ElectroNick's listening ("Q is pretty good, but the reference is less dark and has much cleaner highs"):
which reduction costs the highs? Each case changes one thing from the gold reference (double, 2x, all modes,
measured FIR body), on the double model (fast), and is compared in third octaves from 1.6 to 10 kHz.
    ~/.venvs/teentaal/bin/python tools/reference/diagnose2b.py"""
import os, sys
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(__file__))
import analyze as A, round2 as R2, compare2b as C, body_iir as B

FM1 = A.FM1
_iir = None

def iir_body(Fd, Fn):
    """the firmware's IIR body (body_iir.py's fit, 24 sections, at half the FM-1's rate)"""
    global _iir
    if _iir is None:
        _iir = B.fit(24)
    p, cb, cn = _iir[0], _iir[1], _iir[2]
    y = cb[-1] * Fd + cn[-1] * Fn
    for k, pk in enumerate(p):
        a = [1, -2 * pk.real, abs(pk) ** 2]
        y += signal.lfilter([cb[2 * k], cb[2 * k + 1]], a, Fd) + signal.lfilter([cn[2 * k], cn[2 * k + 1]], a, Fn)
    return y

def case(rate, fmax, body="fir", lp=None):
    fs = FM1 * rate
    a = R2.args(R2.A2, True, True, True, R2.FINAL)
    k = 0
    for i, x in enumerate(a):
        if x == "-s":
            a[i + 1] += f":{fmax[k]}"; k += 1
    _, Fd, Fn, _, log = A.run("dg", a, fs, 32.0)
    if rate == 0.5:
        if body == "iir":
            y = iir_body(Fd, Fn)
            if lp:
                y = signal.sosfilt(signal.butter(4, lp, fs=fs, output="sos"), y)
            return signal.resample_poly(y, 2, 1, window=("kaiser", 12.0))
        Fd = signal.resample_poly(Fd, 2, 1, window=("kaiser", 12.0))
        Fn = signal.resample_poly(Fn, 2, 1, window=("kaiser", 12.0))
        fs = FM1
    pd, pn = A.radiate(fs, Fd, Fn, FM1)
    return pd + pn

def bands(x):
    f, P = signal.welch(x, C.FS, nperseg=8192)
    cs = 1600 * 2 ** (np.arange(0, 9) / 3)
    o = np.array([np.sum(P[(f >= c / 2 ** (1 / 6)) & (f < c * 2 ** (1 / 6))]) for c in cs])
    _, Pall = signal.welch(x, C.FS, nperseg=8192)
    return cs, 10 * np.log10(o / Pall.sum())

def main():
    A.TMP = os.environ.get("TANPURA_TMP", A.TMP)
    ref = C.load(C.REF)
    cs, br = bands(ref)
    print("third-octave level relative to the reference (dB), bands " + " ".join(f"{c/1000:4.1f}k" for c in cs))
    Q = [6000, 8000, 8000, 3000]
    for label, args in (("1x, all modes, FIR body", (1, [22050] * 4)),
                        ("half rate, all modes (to 11 kHz), FIR", (0.5, [11000] * 4)),
                        ("half rate, Q modes 6/8/8/3, FIR", (0.5, Q)),
                        ("half rate, 10/10/10/10 kHz, FIR", (0.5, [10000] * 4)),
                        ("1x, Q modes 6/8/8/3, FIR", (1, Q)),
                        ("half rate, Q modes, IIR body", (0.5, Q, "iir")),
                        ("half rate, Q modes, IIR + LP 8.5k (= Q)", (0.5, Q, "iir", 8500))):
        x = case(*args)
        _, b = bands(x)
        print(f"{label:42s} " + " ".join(f"{d:+5.1f}" for d in b - br))

if __name__ == "__main__" and len(sys.argv) == 1:
    main()

def subset(strings, rate, fmax):
    """only some strings (indices into R2.A2), each pluck kept at its time and strength"""
    t = R2.A2
    a = ["-L", str(t["L"]), "-xc", "6e-3", "-xe", f"{0.35 * t['L']:.4f}", "-we", "0.02", "-s0", R2.FINAL[0], "-rc", R2.FINAL[1]]
    for j in strings:
        f, m, r = t["strings"][j]
        if j == 2:
            f = R2.cents(f, 1.0)
        a += ["-s", f"{f:.4f}:{m}:{r}:{fmax[j]}"]
    pl = R2.plucks(True)
    for i in range(0, len(pl), 2):
        s, rest = pl[i + 1].split(":", 1)
        if int(s) in strings:
            a += ["-p", f"{strings.index(int(s))}:{rest}"]
    _, Fd, Fn, _, _ = A.run("sub", a, FM1 * rate, 32.0)
    if rate == 0.5:
        Fd = signal.resample_poly(Fd, 2, 1, window=("kaiser", 12.0))
        Fn = signal.resample_poly(Fn, 2, 1, window=("kaiser", 12.0))
    return Fd, Fn

def mixed():
    A.TMP = os.environ.get("TANPURA_TMP", A.TMP)
    ref = C.load(C.REF)
    cs, br = bands(ref)
    print("mixed rates: the Sa strings at the FM-1's rate, the first string and the kharaj at half")
    for sa_f, lab in ((8000, "Sa 1x 8 kHz"), (6000, "Sa 1x 6 kHz")):
        fm = [6000, sa_f, sa_f, 3000]
        d1, n1 = subset([1, 2], 1, fm)
        d2, n2 = subset([0, 3], 0.5, fm)
        m = min(len(d1), len(d2))
        pd, pn = A.radiate(FM1, d1[:m] + d2[:m], n1[:m] + n2[:m], FM1)
        _, b = bands(pd + pn)
        print(f"{lab:42s} " + " ".join(f"{d:+5.1f}" for d in b - br))

if __name__ == "__main__" and len(sys.argv) > 1 and sys.argv[1] == "mixed":
    mixed()
