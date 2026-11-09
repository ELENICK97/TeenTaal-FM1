#!/usr/bin/env python3
"""Step 4: tabla stroke prototypes in modal synthesis (host, for listening and choosing the approach; the firmware
code comes at step 5). Four representative strokes — Na, Tin (dayan), Ge, Ke (bayan) — two levels of detail
("simple", "rich"), three dynamics, the compound bols Dha (Ge + Na) and Dhin (Ge + Tin), and a cycle of Teentaal.

    ~/.venvs/teentaal/bin/python tools/tabla_proto/tabla_proto.py      -> renders/step-4/

The model (sources in renders/step-4/README.md):
  - every stroke = a contact pulse (raised cosine, its length sets the brightness) exciting a bank of damped modes,
    plus a short noise burst (the skin and the finger), each mode with its own frequency, amplitude and decay;
  - dayan: Raman's near-harmonic partials 1:2:3:4:5 (the syahi's loading), then weaker inharmonic modes; tuned to Sa;
    "rich" adds the degenerate pairs split a little (slow beats), more inharmonic modes and an amplitude-dependent
    pitch dip at the attack (the head's tension rises with a hard stroke);
  - the stroke decides which modes ring: where it strikes, and where a finger damps (Na: the fundamental muted,
    a strong 3rd harmonic; Tin: an open, softer stroke on the 2nd harmonic: an assumption to judge by ear);
  - bayan: a low drum (about two octaves under the dayan), few overtones; Ge rings, optionally gliding up
    (the heel of the palm pressing); Ke is a closed slap: a thud, no sustained mode.
Each mode here is a damped sinusoid with its phase integrated (so the glide is exact); in the firmware a mode is a
two-pole resonator driven by the same pulse (identical sound, ~8 instructions a sample)."""
import os
import numpy as np
from scipy import signal
from scipy.io import wavfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "renders", "step-4")
FS = 24e6 / 544
RNG = np.random.default_rng(11)

# ------------------------------------------------------------------------------------------------ the drums
# (ratio, amplitude, decay s) per mode; the stroke multiplies amplitudes and decays per mode index
DAYAN_SIMPLE = [(1.0, 1.0, 1.2), (2.0, 0.8, 0.9), (3.0, 0.7, 0.7), (4.0, 0.5, 0.5), (5.0, 0.35, 0.4),
                (6.35, 0.15, 0.12)]
DAYAN_RICH = [(1.0, 1.0, 1.2), (2.0, 0.6, 0.9), (2.006, 0.3, 0.8), (3.0, 0.5, 0.7), (2.995, 0.3, 0.65),
              (4.0, 0.4, 0.5), (4.01, 0.2, 0.45), (5.0, 0.3, 0.4), (4.99, 0.15, 0.35),
              (5.62, 0.12, 0.15), (6.35, 0.12, 0.12), (7.21, 0.08, 0.09), (8.47, 0.06, 0.07), (9.83, 0.05, 0.05)]
BAYAN_SIMPLE = [(1.0, 1.0, 0.9), (2.1, 0.25, 0.35), (3.3, 0.1, 0.15)]
BAYAN_RICH = [(1.0, 1.0, 0.9), (1.52, 0.18, 0.4), (2.1, 0.25, 0.35), (2.68, 0.12, 0.25), (3.3, 0.1, 0.15),
              (3.9, 0.06, 0.1), (4.6, 0.04, 0.07), (5.4, 0.03, 0.05)]

# per stroke: which drum; per harmonic number (1..5, inharmonic = 6) an amplitude and a decay multiplier; the contact
# time (ms); the noise burst (level, ms, centre Hz); the glide (semitones, s) for the bayan
STROKES = {
    "Na":  dict(drum="dayan", amp={1: 0.04, 2: 0.45, 3: 1.0, 4: 0.55, 5: 0.4, 6: 1.0},
                dec={1: 0.3, 2: 0.8, 3: 1.0, 4: 0.9, 5: 0.9, 6: 1.0}, contact=0.6, noise=(0.25, 6, 3500), glide=None),
    "Tin": dict(drum="dayan", amp={1: 0.15, 2: 1.0, 3: 0.45, 4: 0.35, 5: 0.15, 6: 0.4},
                dec={1: 0.4, 2: 1.2, 3: 0.9, 4: 0.8, 5: 0.7, 6: 0.8}, contact=1.0, noise=(0.12, 5, 2500), glide=None),
    "Ge":  dict(drum="bayan", amp={}, dec={}, contact=3.0, noise=(0.08, 8, 300), glide=None),
    "Ge~": dict(drum="bayan", amp={}, dec={}, contact=3.0, noise=(0.08, 8, 300), glide=(4.0, 0.25)),
    "Ke":  dict(drum="bayan", amp={1: 0.6, 2: 0.4, 3: 0.3}, dec={1: 0.035, 2: 0.04, 3: 0.04, 4: 0.04, 5: 0.04, 6: 0.04},
                contact=2.0, noise=(0.6, 25, 500), glide=None),
}

def harmonic_index(ratio):
    """which of Raman's harmonics a mode belongs to (1..5), or 6: inharmonic"""
    h = int(round(ratio))
    return h if 1 <= h <= 5 and abs(ratio - h) < 0.05 else 6

def stroke(name, f0, vel=1.0, level="rich", dur=2.0):
    """one stroke: f0 the drum's fundamental (Hz), vel 0..1 (dynamics)"""
    st = STROKES[name]
    modes = (DAYAN_RICH if level == "rich" else DAYAN_SIMPLE) if st["drum"] == "dayan" else \
            (BAYAN_RICH if level == "rich" else BAYAN_SIMPLE)
    n = int(dur * FS)
    t = np.arange(n) / FS
    # a harder stroke is brighter: a shorter contact; and (rich) bends the pitch down at the attack
    contact = st["contact"] * 1e-3 * (1.3 - 0.5 * vel)
    y = np.zeros(n)
    bend = np.zeros(n)
    if level == "rich":
        bend = -0.35 * vel * np.exp(-t / 0.06) * (1 if st["drum"] == "dayan" else 2.0)   # semitones
    glide = np.zeros(n)
    if st["glide"]:
        semis, tg = st["glide"]
        glide = semis * (1 - np.exp(-np.maximum(t - 0.03, 0) / tg))
    for ratio, a, d in modes:
        h = harmonic_index(ratio) if st["drum"] == "dayan" else min(int(round(ratio)), 6)
        a *= st["amp"].get(h, 1.0)
        d *= st["dec"].get(h, 1.0)
        f = f0 * ratio * 2 ** ((bend + glide) / 12)
        if f[0] >= FS / 2 or a <= 0:
            continue
        # the contact's spectrum at this mode: a smooth roll-off past ~1/T (a finger is no exact raised cosine:
        # that one's spectral zeros fell on single harmonics, Tin's 5th at -65 dB)
        x = f[0] * contact
        pulse = 1.0 / (1.0 + (x / 0.6) ** 2)
        ph = 2 * np.pi * np.cumsum(f) / FS
        y += a * pulse * np.exp(-t / d) * np.sin(ph)
    # the attack: the modes start over the contact time, not at once
    y *= 1 - np.exp(-t / max(contact * 0.4, 1e-4))
    # the noise burst
    lvl, ms, fc = st["noise"]
    nb = RNG.standard_normal(n) * np.exp(-t / (ms * 1e-3))
    if level == "rich":
        b, a = signal.butter(2, [fc / 2.0, min(fc * 2.0, FS / 2 - 100)], btype="band", fs=FS)
    else:
        b, a = signal.butter(1, fc, btype="low", fs=FS)
    y += lvl * (0.5 + 0.5 * vel) * signal.lfilter(b, a, nb) * 3
    return y * (0.25 + 0.75 * vel)

# ------------------------------------------------------------------------------------------------ rendering
def dayan_f0(sa_hz):
    """the dayan on Sa, in the octave a medium dayan plays: 250..500 Hz"""
    f = sa_hz
    while f < 250:
        f *= 2
    while f >= 500:
        f /= 2
    return f

def bayan_f0(dayan):
    return dayan / 4 if dayan / 4 >= 70 else dayan / 2

def place(buf, y, at):
    i = int(at * FS)
    m = min(len(y), len(buf) - i)
    buf[i:i + m] += y[:m]

BOLS = {"Na": ["Na"], "Ta": ["Na"], "Tin": ["Tin"], "Ge": ["Ge"], "Ke": ["Ke"],
        "Dha": ["Ge", "Na"], "Dhin": ["Ge", "Tin"]}

def bol(name, f_d, f_b, vel, level, dur=2.0):
    y = np.zeros(int(dur * FS))
    for s in BOLS[name]:
        y += stroke(s, f_d if STROKES[s]["drum"] == "dayan" else f_b, vel, level, dur)
    return y

def write(name, x, peak_db=-3.0):
    x = x / (np.max(np.abs(x)) + 1e-12) * 10 ** (peak_db / 20)
    wavfile.write(os.path.join(OUT, name + ".wav"), int(round(FS)), (x * 32767).astype(np.int16))

def theka(f_d, f_b, level, mpm=120, cycles=2):
    """Teentaal (the theka of CLAUDE.md §7), mpm matras a minute; Ta played as Na here; sam a little louder"""
    tk = ["Dha", "Dhin", "Dhin", "Dha", "Dha", "Dhin", "Dhin", "Dha", "Dha", "Tin", "Tin", "Ta", "Ta", "Dhin", "Dhin", "Dha"]
    beat = 60.0 / mpm
    total = cycles * 16 * beat + 2.0
    out = np.zeros(int(total * FS))
    rng = np.random.default_rng(5)
    for c in range(cycles):
        for i, b in enumerate(tk):
            vel = (0.95 if i == 0 else 0.75) * rng.uniform(0.92, 1.0)
            # khali (matras 9-12): no bayan resonance; Tin and Ta stand alone there
            place(out, bol(b, f_d, f_b, vel, level), (c * 16 + i) * beat + rng.uniform(-0.008, 0.008))
    return out

def main():
    os.makedirs(OUT, exist_ok=True)
    sa = 220.0                                   # ElectroNick's Sa A3 (tanpura A2, flute A4)
    f_d = dayan_f0(sa)                           # 440 Hz
    f_b = bayan_f0(f_d)                          # 110 Hz
    print(f"dayan {f_d:.1f} Hz, bayan {f_b:.1f} Hz")
    # each stroke: three dynamics (soft, medium, hard), simple and rich
    for k, s in enumerate(["Na", "Tin", "Ge", "Ge~", "Ke"]):
        for level in ("simple", "rich"):
            buf = np.zeros(int(4.0 * FS))
            for j, v in enumerate((0.35, 0.65, 1.0)):
                y = stroke(s, f_d if STROKES[s]["drum"] == "dayan" else f_b, v, level, 1.3)
                place(buf, y, j * 1.3)
            nm = s.replace("~", "_glide")
            write(f"{k + 1:02d}_{nm}_{level}_soft_medium_hard", buf)
    # the compound bols
    for k, b in enumerate(["Dha", "Dhin"]):
        for level in ("simple", "rich"):
            buf = np.zeros(int(4.0 * FS))
            for j, v in enumerate((0.35, 0.65, 1.0)):
                place(buf, bol(b, f_d, f_b, v, level, 1.3), j * 1.3)
            write(f"{k + 6:02d}_{b}_{level}_soft_medium_hard", buf)
    # a phrase with Ke in it
    for level in ("simple", "rich"):
        seq = ["Dha", "Ke", "Dha", "Ge", "Na", "Ke", "Tin", "Na"]
        buf = np.zeros(int(6.0 * FS))
        for i, b in enumerate(seq):
            place(buf, bol(b, f_d, f_b, 0.8, level), i * 0.35)
        write(f"08_phrase_with_Ke_{level}", buf)
    # Teentaal, two cycles at 120 matras a minute, alone and over the step 2b tanpura (Sa A2)
    tan_path = os.path.join(ROOT, "renders", "step-2b", "01_Q_quality_saA2_Pa_jawari50.wav")
    r, tan = wavfile.read(tan_path)
    tan = tan[:, 0].astype(np.float64) / 2 ** 31
    for level in ("simple", "rich"):
        x = theka(f_d, f_b, level)
        write(f"09_teentaal_120_{level}", x)
        tb = x / np.max(np.abs(x)) * 0.5
        n = min(len(tb), len(tan))
        mix = tb[:n] + tan[:n] / np.max(np.abs(tan[:n])) * 0.45
        write(f"10_teentaal_120_{level}_with_tanpura", mix)

if __name__ == "__main__":
    main()
