#!/usr/bin/env python3
"""Step 2b: does the string survive a lower simulation rate, and does a low-pass on the driving forces rescue a
truncated mode series? Runs the double reference (build-host/tanpura_ref) with the step 2a final settings (Sa A2
large tanpura) and compares with REF_sa_A2_large_2x through compare2b's metrics.
    ~/.venvs/teentaal/bin/python tools/reference/rate_test.py"""
import os, sys
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(__file__))
import analyze as A, round2 as R2, compare2b as C

FM1 = A.FM1

def run(rate_factor, fmax, lp=None, dur=32.0):
    fs = FM1 * rate_factor
    args = R2.args(R2.A2, True, True, True, R2.FINAL) + ["-fmax", str(fmax)]
    fs_, Fd, Fn, _, log = A.run("rt", args, fs, dur)
    if lp:                                    # the forces low-passed at lp Hz before anything else
        sos = signal.butter(8, lp, fs=fs, output="sos")
        Fd, Fn = signal.sosfilt(sos, Fd), signal.sosfilt(sos, Fn)
    if rate_factor == 0.5:                    # back up to the FM-1's rate
        Fd = signal.resample_poly(Fd, 2, 1, window=("kaiser", 12.0))
        Fn = signal.resample_poly(Fn, 2, 1, window=("kaiser", 12.0))
        fs = FM1
    pd, pn = A.radiate(fs, Fd, Fn, FM1)
    modes = sum(int(l.split(" modes")[0].split()[-1]) for l in log.splitlines() if " modes" in l)
    return pd + pn, modes

def main():
    A.TMP = os.environ.get("TANPURA_TMP", A.TMP)
    ref = C.load(C.REF)
    for rf, fm, lp, label in ((1, 22050, None, "1x full (the float model's ceiling)"),
                              (1, 8000, None, "1x, 8 kHz"),
                              (0.5, 8000, None, "half rate, 8 kHz"),
                              (0.5, 10000, None, "half rate, 10 kHz"),
                              (1, 4000, None, "1x, 4 kHz"),
                              (1, 4000, 4500, "1x, 4 kHz + low-pass 4.5 kHz"),
                              (0.5, 5000, None, "half rate, 5 kHz"),
                              (0.5, 5000, 5500, "half rate, 5 kHz + low-pass 5.5 kHz"),
                              (0.5, 6000, 6500, "half rate, 6 kHz + low-pass 6.5 kHz")):
        x, m = run(rf, fm, lp)
        r, _ = C.compare(x / np.max(np.abs(x)), ref)
        cost = m * rf
        print(f"{label:40s} modes {m:3d}  mode-steps/sample {cost:5.0f}  LTAS {r['ltas_rms_db']:5.2f} dB rms "
              f"(max {r['ltas_max_db']:5.2f})  centroid {r['centroid_cycle_mae_hz']:5.0f} Hz  env {r['envelope_cycle_mae_db']:.2f} dB")

if __name__ == "__main__":
    main()
