#!/usr/bin/env python3
"""Step 2b: IIR body radiation filters fitted to the authors' measured impulse responses (hb: bridge, hn: nut).

Method: parallel second-order sections with fixed poles on a logarithmic frequency grid, numerators fitted by
least squares in the time domain (B. Bank, "Direct design of parallel second-order filters for instrument body
modeling", ICMC 2007 — the paper's reference [24]). The bridge and nut filters SHARE the poles: the firmware's
output is mono (p_d + p_n), so one recursion per section serves both inputs:

    w_k[n] = b0_k x_d[n] + b1_k x_d[n-1] + c0_k x_n[n] + c1_k x_n[n-1] - a1_k w_k[n-1] - a2_k w_k[n-2]
    p[n]   = d_d x_d[n] + d_n x_n[n] + sum_k w_k[n]

The common onset delay (the microphone at 80 cm, 44 samples) is removed; the nut's 2 samples more are kept.

    ~/.venvs/teentaal/bin/python tools/reference/body_iir.py [NSECTIONS]   -> tanpura/body_coefs.h, figure, errors"""
import os, sys
import numpy as np
from scipy import signal
from scipy.io import wavfile
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EX = os.path.join(ROOT, "docs", "references", "dafx16a")
FS = 44100.0          # the measured responses' rate
DELAY = 44
FIT_FS = 24e6 / 544 / 2   # the rate the filter runs at: the strings' (half the FM-1's)
F_TOP = 10000.0           # fitted up to here (Nyquist at FIT_FS is 11.03 kHz)

def load(n):
    fs, h = wavfile.read(os.path.join(EX, n + ".wav"))
    return h.astype(np.float64)[DELAY:] / 32768.0

def poles(nsec, f_lo=40.0, f_hi=None):
    f = np.geomspace(f_lo, f_hi or F_TOP * 0.95, nsec)
    th = 2 * np.pi * f / FIT_FS
    dth = np.gradient(th)                         # the spacing sets each pole's bandwidth (Bank 2007)
    R = np.exp(-dth / 2)
    return R * np.exp(1j * th)

def basis(p, n):
    """impulse responses of 1/A_k and z^-1/A_k for each section, and the direct term"""
    cols = []
    imp = np.zeros(n); imp[0] = 1
    for pk in p:
        a = [1, -2 * pk.real, abs(pk) ** 2]
        u = signal.lfilter([1], a, imp)
        cols += [u, np.concatenate([[0], u[:-1]])]
    cols.append(imp)
    return np.array(cols).T

def smooth_mag(f, H, frac=6):
    """|H| smoothed over 1/frac octave (for the weights)"""
    out = np.empty(len(f))
    for i, fc in enumerate(f):
        m = (f >= fc / 2 ** (0.5 / frac)) & (f <= fc * 2 ** (0.5 / frac))
        out[i] = np.sqrt(np.mean(np.abs(H[m]) ** 2))
    return out

def minphase_smooth(h, w, frac=6, nfft=32768):
    """the target: the measured magnitude power-smoothed over 1/frac octave (the dense high-frequency resonances
    have random phases: a complex fit averages them away and loses their power), made minimum phase (cepstrum)"""
    H = np.fft.rfft(h, nfft)
    f = np.fft.rfftfreq(nfft, 1 / FS)
    P = np.abs(H) ** 2
    c = np.cumsum(P)
    lo = np.searchsorted(f, f / 2 ** (0.5 / frac)); hi = np.searchsorted(f, f * 2 ** (0.5 / frac), side="right")
    hi = np.maximum(hi, lo + 1)
    Ps = (c[np.minimum(hi, len(c)) - 1] - np.where(lo > 0, c[lo - 1], 0)) / (hi - lo)
    logm = 0.5 * np.log(np.maximum(Ps, 1e-16))
    full = np.concatenate([logm, logm[-2:0:-1]])           # even, length nfft
    cep = np.fft.ifft(full).real
    fold = np.zeros(nfft); fold[0] = cep[0]; fold[1:nfft // 2] = 2 * cep[1:nfft // 2]; fold[nfft // 2] = cep[nfft // 2]
    Hm = np.exp(np.fft.fft(fold))
    hmin = np.fft.ifft(Hm).real[: nfft // 2]
    return signal.freqz(hmin, worN=w)[1]

def fit(nsec):
    """weighted least squares on the complex response, on a logarithmic grid 30 Hz - 20 kHz, weights 1/|H| (smoothed):
    every band fitted in relative terms, as the ear hears it"""
    hb, hn = load("hb"), load("hn")
    n = len(hb)
    p = poles(nsec)
    B = basis(p, n)                                       # (for the time-domain responses of the result)
    f = np.geomspace(30, F_TOP, 3000)
    w = 2 * np.pi * f / FIT_FS                            # the model's own frequency axis
    z1 = np.exp(-1j * w)
    cols = []
    for pk in p:
        A = 1 - 2 * pk.real * z1 + abs(pk) ** 2 * z1 ** 2
        cols += [1 / A, z1 / A]
    cols.append(np.ones_like(z1))
    M = np.array(cols).T
    res = []
    for h in (hb, hn):
        H = minphase_smooth(h, 2 * np.pi * f / FS)          # the target, read at the same physical frequencies
        W = 1 / np.maximum(np.abs(H), 1e-6)
        Mr = np.vstack([(M * W[:, None]).real, (M * W[:, None]).imag])
        Hr = np.concatenate([(H * W).real, (H * W).imag])
        res.append(np.linalg.lstsq(Mr, Hr, rcond=None)[0])
    return p, res[0], res[1], hb, hn, B

def third_oct_err(h, g):
    f, H = signal.freqz(h, worN=16384, fs=FS)
    _, G = signal.freqz(g, worN=16384, fs=FS)
    cs = 50 * 2 ** (np.arange(0, 28) / 3)
    e = []
    for fc in cs:
        m = (f >= fc / 2 ** (1 / 6)) & (f < fc * 2 ** (1 / 6))
        if fc > 16000 or not m.any():
            continue
        e.append(10 * np.log10(np.mean(np.abs(G[m]) ** 2) / np.mean(np.abs(H[m]) ** 2)))
    return np.array(e)

def iir_response(p, c, w):
    z1 = np.exp(-1j * w)
    G = np.full(len(w), c[-1], dtype=complex)
    for k, pk in enumerate(p):
        A = 1 - 2 * pk.real * z1 + abs(pk) ** 2 * z1 ** 2
        G += (c[2 * k] + c[2 * k + 1] * z1) / A
    return G

def third_oct_err_iir(h, p, c):
    f, H = signal.freqz(h, worN=16384, fs=FS)
    m = f < F_TOP
    f, H = f[m], H[m]
    G = iir_response(p, c, 2 * np.pi * f / FIT_FS)
    cs = 50 * 2 ** (np.arange(0, 28) / 3)
    e = []
    for fc in cs:
        m = (f >= fc / 2 ** (1 / 6)) & (f < fc * 2 ** (1 / 6))
        if fc * 2 ** (1 / 6) > F_TOP or not m.any():
            continue
        e.append(10 * np.log10(np.mean(np.abs(G[m]) ** 2) / np.mean(np.abs(H[m]) ** 2)))
    return np.array(e)

def main():
    nsec = int(sys.argv[1]) if len(sys.argv) > 1 else 24
    report = []
    for k in (12, 16, 20, 24):
        p, cb, cn, hb, hn, B = fit(k)
        eb, en = third_oct_err_iir(hb, p, cb), third_oct_err_iir(hn, p, cn)
        report.append((k, np.sqrt(np.mean(eb ** 2)), np.abs(eb).max(), np.sqrt(np.mean(en ** 2)), np.abs(en).max()))
    for r in report:
        print(f"{r[0]:2d} sections: bridge {r[1]:.2f} dB rms / {r[2]:.2f} max, nut {r[3]:.2f} dB rms / {r[4]:.2f} max"
              f" (third octaves 50 Hz-{F_TOP/1000:.0f} kHz, filter at {FIT_FS:.1f} Hz)")
    p, cb, cn, hb, hn, B = fit(nsec)
    # figure
    fig, ax = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
    for a, h, c, nm in ((ax[0], hb, cb, "bridge (hb)"), (ax[1], hn, cn, "nut (hn)")):
        f, H = signal.freqz(h, worN=8192, fs=FS)
        G = iir_response(p, c, 2 * np.pi * f / FIT_FS)
        a.semilogx(f, 20 * np.log10(np.abs(H) + 1e-9), lw=0.6, color="gray", label="measured FIR (2048)")
        a.semilogx(f, 20 * np.log10(np.abs(G) + 1e-9), lw=1.0, label=f"IIR, {nsec} shared sections")
        a.set_title(nm); a.set_ylabel("dB"); a.legend(); a.set_xlim(30, FIT_FS / 2)
    ax[1].set_xlabel("Hz")
    fig.tight_layout()
    os.makedirs(os.path.join(ROOT, "renders", "step-2b"), exist_ok=True)
    fig.savefig(os.path.join(ROOT, "renders", "step-2b", "body_iir_fit.png"), dpi=90)
    # the header
    a1 = -2 * p.real
    a2 = np.abs(p) ** 2
    b0b, b1b, b0n, b1n = cb[0:-1:2], cb[1:-1:2], cn[0:-1:2], cn[1:-1:2]
    hdr = os.path.join(ROOT, "tanpura", "body_coefs.h")
    with open(hdr, "w") as f:
        f.write("/* body_coefs.h — GENERATED by tools/reference/body_iir.py: do not edit.\n"
                f" * Parallel second-order body filters, {nsec} sections with poles shared by the bridge and nut\n"
                " * responses (measured by van Walstijn et al., DAFx-16; fitted after B. Bank, ICMC 2007).\n"
                f" * Runs at the strings' rate, {FIT_FS:.3f} Hz (half the FM-1's); fitted up to {F_TOP:.0f} Hz. */\n")
        f.write("#pragma once\n\n")
        f.write(f"#define BODY_NSEC {nsec}\n")
        def arr(name, v):
            f.write(f"static const float {name}[BODY_NSEC] = {{\n")
            for i in range(0, len(v), 4):
                f.write("    " + ", ".join(f"{x:.9e}f" for x in v[i:i + 4]) + ",\n")
            f.write("};\n")
        arr("BODY_A1", a1); arr("BODY_A2", a2)
        arr("BODY_B0_BRIDGE", b0b); arr("BODY_B1_BRIDGE", b1b)
        arr("BODY_B0_NUT", b0n); arr("BODY_B1_NUT", b1n)
        f.write(f"#define BODY_D_BRIDGE {cb[-1]:.9e}f\n#define BODY_D_NUT {cn[-1]:.9e}f\n")
    print("wrote", hdr)

if __name__ == "__main__":
    main()
