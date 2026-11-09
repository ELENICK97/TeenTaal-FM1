#!/usr/bin/env python3
"""Step 2a: run the reference tanpura model (build-host/tanpura_ref), radiate it through the measured body
responses as the paper does, and compare with the paper and with the authors' sound examples.

    ~/.venvs/teentaal/bin/python tools/reference/analyze.py          # everything -> renders/step-2a/

Radiation: p_d = F_d * h_b, p_n = F_n * h_n with the authors' measured impulse responses (docs/references/dafx16a,
2048 taps at 44.1 kHz). Stereo WAVs carry p_d left and p_n right, as in the paper's sound examples (§4.4).
An oversampled run is decimated to the base rate before the radiation filters (the responses are 44.1 kHz).
All WAVs of this step share one gain, so their loudness can be compared."""
import json, os, subprocess, sys
import numpy as np
from scipy import signal
from scipy.io import wavfile
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
REF = os.path.join(ROOT, "build-host", "tanpura_ref")
EX = os.path.join(ROOT, "docs", "references", "dafx16a")
OUT = os.path.join(ROOT, "renders", "step-2a")
FIG = os.path.join(OUT, "fig")
TMP = os.environ.get("TANPURA_TMP", "/tmp")
FM1 = 24e6 / 544
GAIN = None          # set from the first C3 render (peak at -3 dBFS), then the same for every file
results = {}

def run(name, args, fs=44100.0, dur=10.0):
    """run the model; returns (fs, Fd, Fn, yb) and its log"""
    path = os.path.join(TMP, name + ".f32")
    p = subprocess.run([REF, "-fs", repr(fs), "-dur", repr(dur), *args, path], capture_output=True, text=True)
    if p.returncode:
        sys.exit(p.stderr)
    x = np.fromfile(path, dtype=np.float32).reshape(-1, 3).astype(np.float64)
    os.remove(path)
    return fs, x[:, 0], x[:, 1], x[:, 2], p.stderr.strip()

def load_ir(n):
    fs, h = wavfile.read(os.path.join(EX, n + ".wav"))
    return h.astype(np.float64) / 32768.0

HB, HN = load_ir("hb"), load_ir("hn")

def radiate(fs, Fd, Fn, base):
    """decimate to the base rate if oversampled, then the measured body responses"""
    k = int(round(fs / base))
    if k > 1:
        Fd = signal.resample_poly(Fd, 1, k, window=("kaiser", 12.0))
        Fn = signal.resample_poly(Fn, 1, k, window=("kaiser", 12.0))
    pd = signal.fftconvolve(Fd, HB)[: len(Fd)]
    pn = signal.fftconvolve(Fn, HN)[: len(Fn)]
    return pd, pn

def write(name, pd, pn, rate):
    global GAIN
    if GAIN is None:
        GAIN = 10 ** (-3 / 20) / max(np.max(np.abs(pd)), np.max(np.abs(pn)))
    st = np.stack([pd, pn], axis=1) * GAIN
    peak = np.max(np.abs(st))
    if peak > 1:
        print(f"  warning: {name} clips at the shared gain (peak {20*np.log10(peak):+.1f} dBFS): scaled down")
        st /= peak
    wavfile.write(os.path.join(OUT, name + ".wav"), int(round(rate)), (st * 32767).astype(np.int16))
    return 20 * np.log10(max(peak, 1e-12))

def author(n):
    fs, x = wavfile.read(os.path.join(EX, n + ".wav"))
    x = x.astype(np.float64) / 32768.0
    return fs, (x[:, 0], x[:, 1]) if x.ndim == 2 else (x, None)

def centroid(x, fs, hop=0.02, win=0.04):
    """spectral centroid over time (Hz), frames win long every hop"""
    n, h = int(win * fs), int(hop * fs)
    w = np.hanning(n)
    t, c = [], []
    for i in range(0, len(x) - n, h):
        X = np.abs(np.fft.rfft(x[i:i + n] * w))
        f = np.fft.rfftfreq(n, 1 / fs)
        e = np.sum(X ** 2)
        t.append((i + n / 2) / fs)
        c.append(np.sum(f * X ** 2) / e if e > 0 else 0)
    return np.array(t), np.array(c)

def ltas_bands(x, fs, t0=0.3, t1=3.0):
    """long-term spectrum in third-octave bands from 100 Hz to 16 kHz, dB re the loudest band"""
    seg = x[int(t0 * fs):int(t1 * fs)]
    f, P = signal.welch(seg, fs, nperseg=8192)
    centres = 100 * 2 ** (np.arange(0, 22) / 3)
    out = []
    for fc in centres:
        m = (f >= fc / 2 ** (1 / 6)) & (f < fc * 2 ** (1 / 6))
        out.append(10 * np.log10(np.sum(P[m]) + 1e-30))
    out = np.array(out)
    return centres, out - out.max()

def spec(ax, x, fs, title, fmax=12000, dur=None):
    if dur:
        x = x[: int(dur * fs)]
    f, t, S = signal.spectrogram(x, fs, nperseg=2048, noverlap=1536)
    S = 10 * np.log10(S + 1e-20)
    ax.pcolormesh(t, f / 1000, S, vmin=S.max() - 90, vmax=S.max(), shading="auto", cmap="magma")
    ax.set_ylim(0, fmax / 1000)
    ax.set_title(title, fontsize=9)
    ax.set_ylabel("kHz")

# ------------------------------------------------------------------------------------------------ cases
def c3_cases():
    """the C3 string of Table 1 against the authors' examples"""
    cases = [  # name, our options, author's file, description
        ("c3_xc5_normal", ["-xc", "5e-3"], "ex1", "normal, thread at 5 mm"),
        ("c3_no_stiffness", ["-nostiff"], "ex2", "no stiffness (EI = 0)"),
        ("c3_no_damping", ["-nodamp"], "ex3", "no damping (string and thread)"),
        ("c3_no_thread", ["-nothread"], "ex4", "no thread (k_c = r_c = 0; the authors' recipe is unknown)"),
        ("c3_xc4", ["-xc", "4e-3"], "ex5a", "thread at 4 mm"),
        ("c3_xc8", ["-xc", "8e-3"], "ex5c", "thread at 8 mm"),
        ("c3_xc6", ["-xc", "6e-3"], None, "thread at 6 mm"),
    ]
    fig, axes = plt.subplots(len(cases), 2, figsize=(12, 2.2 * len(cases)), sharex=True)
    figc, axc = plt.subplots(1, 1, figsize=(9, 5))
    figl, axl = plt.subplots(1, 1, figsize=(9, 5))
    for k, (name, opts, ex, desc) in enumerate(cases):
        fs, Fd, Fn, yb, log = run(name, ["-s", "130.81", *opts], 44100.0, 10.0)
        pd, pn = radiate(fs, Fd, Fn, 44100.0)
        lvl = write("01_" + name, pd, pn, 44100)
        r = {"desc": desc, "log": log, "peak_dbfs": round(lvl, 1)}
        mix = pd + pn
        spec(axes[k, 0], pd, fs, f"ours: {desc} (p_d)", dur=6)
        t, c = centroid(pd, fs)
        axc.plot(t, c, label=f"ours {desc}")
        if ex:
            afs, (al, ar) = author(ex)
            spec(axes[k, 1], al, afs, f"authors {ex}: {desc} (left)", dur=6)
            ta, ca = centroid(al, afs)
            axc.plot(ta, ca, "--", label=f"authors {ex}")
            # third-octave long-term spectra of the left channel, ours vs theirs
            cf, lo = ltas_bands(pd, fs)
            _, la = ltas_bands(al, afs)
            axl.semilogx(cf, lo - la, label=desc)
            r["ltas_diff_db"] = {"max_abs": round(float(np.max(np.abs(lo - la))), 1),
                                 "rms": round(float(np.sqrt(np.mean((lo - la) ** 2))), 1)}
            # the centroid's descent: time for the left channel's centroid to fall to 60 % of its 0.2-0.5 s value
            def descent(t, c):
                m0 = np.mean(c[(t > 0.2) & (t < 0.5)])
                i = np.where((t > 0.5) & (c < 0.6 * m0))[0]
                return (round(float(m0)), round(float(t[i[0]]), 2) if len(i) else None)
            r["centroid_ours"] = descent(t, c)
            r["centroid_authors"] = descent(ta, ca)
        else:
            axes[k, 1].axis("off")
        results[name] = r
    for a in axes[-1]:
        a.set_xlabel("s")
    fig.tight_layout()
    fig.savefig(os.path.join(FIG, "c3_spectrograms_ours_vs_authors.png"), dpi=90)
    axc.set_xlim(0, 6); axc.set_xlabel("s"); axc.set_ylabel("spectral centroid of p_d (Hz)")
    axc.legend(fontsize=7); axc.set_title("Spectral centroid: ours (solid) vs the authors' examples (dashed)")
    figc.savefig(os.path.join(FIG, "c3_centroid.png"), dpi=90)
    axl.axhline(0, color="k", lw=0.5); axl.set_xlabel("Hz"); axl.set_ylabel("dB, ours - authors")
    axl.set_title("Long-term spectrum 0.3-3 s, third octaves, each normalised to its loudest band")
    axl.legend(fontsize=8)
    figl.savefig(os.path.join(FIG, "c3_ltas_difference.png"), dpi=90)
    plt.close("all")

def paper_figures():
    """Fig. 7 (y_b windows), Fig. 8 (forces and radiation), Fig. 9 (44.1 kHz vs oversampled), no stiffness"""
    fs, Fd, Fn, yb, log = run("fig78", ["-s", "130.81", "-xc", "5e-3"], 44100.0, 2.0)
    wins = [(74, 78), (210, 214), (346, 350), (482, 486), (618, 622), (754, 760), (890, 896), (1026, 1032), (1164, 1168)]
    fig, ax = plt.subplots(3, 3, figsize=(10, 7))
    for a, (t0, t1) in zip(ax.flat, wins):
        i0, i1 = int(t0 / 1000 * fs), int(t1 / 1000 * fs)
        a.plot(np.arange(i0, i1) / fs * 1000, yb[i0:i1] * 1e6)
        a.set_ylim(-2, 22); a.set_title(f"{t0}-{t1} ms", fontsize=8)
    fig.suptitle("y_b (µm) at the bridge: compare with the paper's Fig. 7")
    fig.tight_layout(); fig.savefig(os.path.join(FIG, "paper_fig7_yb.png"), dpi=90)
    pd, pn = radiate(fs, Fd, Fn, 44100.0)
    fig, ax = plt.subplots(2, 1, figsize=(10, 6), sharex=True)
    t = np.arange(len(Fd)) / fs
    ax[0].plot(t, Fd, lw=0.5, label="F_d"); ax[0].plot(t, Fn, lw=0.5, label="F_n"); ax[0].set_ylabel("N"); ax[0].legend()
    ax[1].plot(t, pd, lw=0.5, label="p_d"); ax[1].plot(t, pn, lw=0.5, label="p_n"); ax[1].legend()
    ax[1].set_xlabel("s"); fig.suptitle("Driving forces and radiation: compare with the paper's Fig. 8")
    fig.savefig(os.path.join(FIG, "paper_fig8_forces.png"), dpi=90)
    results["fig78"] = {"log": log, "Fd_min_N": round(float(Fd.min()), 3), "Fd_max_N": round(float(Fd.max()), 3),
                        "yb_max_um": round(float(yb.max() * 1e6), 1),
                        "yb_min_um": round(float(yb.min() * 1e6), 4)}
    # Fig. 9: spectral envelopes at 44.1 kHz and at 8x (352.8 kHz), 133 modes (20 kHz) in both
    envs = {}
    for k in (1, 2, 8):
        f2, Fd2, Fn2, _, lg = run(f"fig9_{k}", ["-s", "130.81", "-fmax", "20000"], 44100.0 * k, 2.0)
        pd2, pn2 = radiate(f2, Fd2, Fn2, 44100.0)
        envs[k] = [signal.welch(x, 44100.0, nperseg=4096) for x in (pd2, pn2)]
        results[f"fig9_x{k}"] = {"log": lg}
    fig, ax = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
    for j, nm in enumerate(("p_d (bridge)", "p_n (nut)")):
        for k, sty in ((8, "lightgray"), (2, "tab:blue"), (1, "tab:red")):
            f, P = envs[k][j]
            ax[j].plot(f / 1000, 10 * np.log10(P + 1e-30), color=sty, lw=0.8, label=f"{44.1*k:.1f} kHz")
        ax[j].set_title(nm); ax[j].set_ylabel("dB"); ax[j].legend()
    ax[1].set_xlabel("kHz"); fig.suptitle("Fig. 9: 44.1 kHz vs oversampled, 133 modes in all")
    fig.savefig(os.path.join(FIG, "paper_fig9_oversampling.png"), dpi=90)
    # the high band (15-20 kHz) excess of 44.1 kHz over 8x, as in the paper's text
    for k in (1, 2):
        d = []
        for j in range(2):
            f, P = envs[k][j]; f8, P8 = envs[8][j]
            m = (f > 15000) & (f < 20000)
            d.append(round(float(10 * np.log10(np.sum(P[m]) / np.sum(P8[m]))), 1))
        results[f"fig9_x{k}"]["band_15_20k_vs_x8_db"] = {"p_d": d[0], "p_n": d[1]}
    plt.close("all")

def drones():
    """4-string patterns: the paper's G2-C3-C3-C2 (Fig. 11), and one on ElectroNick's Sa (A3: Pa E3, Sa, Sa, kharaj A2)"""
    plucks = ["-p", "0:0", "-p", "1:1.2", "-p", "2:1.8", "-p", "3:2.4"]
    paper = ["-s", "98.0", "-s", "130.81", "-s", "130.81", "-s", "65.41:bronze:0.2e-3", *plucks]
    a3 = ["-L", "0.85", "-s", "164.81:steel:0.13e-3", "-s", "220.0:steel:0.13e-3", "-s", "220.0:steel:0.13e-3",
          "-s", "110.0:bronze:0.18e-3", *plucks]
    # Fig. 11: the bridge force spectrogram of one cycle
    fs, Fd, Fn, yb, log = run("fig11", paper, 44100.0, 4.0)
    fig, ax = plt.subplots(1, 1, figsize=(10, 4))
    spec(ax, Fd, fs, "total bridge force F_d, G2-C3-C3-C2, plucks at 0, 1.2, 1.8, 2.4 s: compare with Fig. 11",
         fmax=10000)
    ax.set_xlabel("s"); fig.tight_layout(); fig.savefig(os.path.join(FIG, "paper_fig11_drone.png"), dpi=90)
    results["fig11"] = {"log": log}
    for name, opts, base in (("02_drone_paper_G2_C3_C3_C2", paper, 44100.0),
                             ("03_drone_sa_A3_Pa", a3, FM1)):
        for k in (1, 2):
            fs, Fd, Fn, _, log = run(name, [*opts, "-cycle", "4.0"], base * k, 32.0)
            pd, pn = radiate(fs, Fd, Fn, base)
            nm = name + ("" if k == 1 else "_2x_oversampled")
            lvl = write(nm, pd, pn, base)
            results[nm] = {"log": log, "peak_dbfs": round(lvl, 1)}
            if k == 1:
                fig, ax = plt.subplots(1, 1, figsize=(12, 4))
                spec(ax, pd, base, f"{nm}: p_d", fmax=12000, dur=12)
                ax.set_xlabel("s"); fig.tight_layout(); fig.savefig(os.path.join(FIG, nm + "_spectrogram.png"), dpi=90)
                plt.close("all")
    # A/B: the A3 drone at 1x and 2x, side by side
    fig, ax = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
    for j, (nm, k) in enumerate((("03_drone_sa_A3_Pa", 1), ("03_drone_sa_A3_Pa_2x_oversampled", 2))):
        r, x = wavfile.read(os.path.join(OUT, nm + ".wav"))
        spec(ax[j], x[:, 0].astype(float), r, nm, fmax=22000, dur=8)
    ax[1].set_xlabel("s"); fig.tight_layout(); fig.savefig(os.path.join(FIG, "03_drone_1x_vs_2x.png"), dpi=90)
    plt.close("all")

def main():
    os.makedirs(FIG, exist_ok=True)
    if not os.path.exists(REF):
        sys.exit("build-host/tanpura_ref missing: gcc -std=c99 -O2 -o build-host/tanpura_ref tools/reference/tanpura_ref.c -lm")
    c3_cases()
    paper_figures()
    drones()
    with open(os.path.join(OUT, "results.json"), "w") as f:
        json.dump(results, f, indent=1)
    print(json.dumps(results, indent=1))

if __name__ == "__main__":
    main()
