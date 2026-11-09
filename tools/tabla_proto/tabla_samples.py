#!/usr/bin/env python3
"""Step 4, second approach (ElectroNick, 2026-11-08: the modal prototypes "sound like a videogame"): tabla strokes from
real recordings — Freesound pack "tabla bols" by mmiron (CC0, docs/references/tabla_mmiron/wav/).

  - the dayan's strokes are retuned to Sa by resampling (all modes together, the ring a little shorter or longer:
    roughly what a smaller or larger dayan does); the recordings' dayan sits at ~312 Hz (Tun's fundamental, D#4);
  - the bayan keeps its own pitch (it is not tuned to Sa precisely);
  - several takes of a stroke are rotated, with a little variation in level: no machine-gun repetition;
  - a new stroke on a drum chokes the one still ringing on that drum (the hand on the head), in 8 ms; except Ge,
    whose bayan keeps ringing under the dayan's strokes (the other hand);
  - Tin is missing from the pack: Dhin is rendered with two stand-ins, na-open and tun, for ElectroNick's ear.

    ~/.venvs/teentaal/bin/python tools/tabla_proto/tabla_samples.py      -> renders/step-4/samples/"""
import glob, os
import numpy as np
from scipy import signal
from scipy.io import wavfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC = os.path.join(ROOT, "docs", "references", "tabla_mmiron", "wav")
OUT = os.path.join(ROOT, "renders", "step-4", "samples")
FS = 24e6 / 544
DAYAN_HZ = 315.2                     # the recordings' dayan: Tun's fundamental, 3 takes 314.7-315.6 Hz
RNG = np.random.default_rng(3)

def load(name):
    p = glob.glob(os.path.join(SRC, f"*__mmiron__{name}.wav"))[0]
    r, x = wavfile.read(p)
    x = x.astype(np.float64) / 32768.0
    x = x.mean(axis=1) if x.ndim == 2 else x
    i0 = max(int(np.argmax(np.abs(x) > 0.02 * np.abs(x).max())) - 16, 0)   # the onset, a few samples early
    x = x[i0:]
    # the tail: cut where it falls 60 dB under the peak, with a 20 ms fade
    e = np.convolve(x ** 2, np.ones(441) / 441, "same")
    j = np.where(e < e.max() * 1e-6)[0]
    j = j[j > np.argmax(e)]
    n = j[0] if len(j) else len(x)
    x = x[:n].copy()
    f = min(int(0.02 * r), n)
    x[n - f:] *= np.linspace(1, 0, f)
    return signal.resample_poly(x, 44118, 44100) if r == 44100 else x   # (to the FM-1's rate, closely)

# the strokes: which drum, the takes (file names in the pack)
STROKES = {
    "Na":   ("dayan", ["na", "na_sharp"]),
    "Ta":   ("dayan", ["tas", "tas_2", "tas_3"]),
    "Tun":  ("dayan", ["tun", "tun_2", "tun_3"]),
    "Te":   ("dayan", ["te", "te_2", "te_ne", "te_middlefinger"]),
    "Re":   ("dayan", ["re"]),
    "TinA": ("dayan", ["na-open"]),                 # stand-in A for Tin
    "TinB": ("dayan", ["tun_2"]),                   # stand-in B for Tin
    "Ge":   ("bayan", ["ghe_2", "ghe_3", "ghe_7", "ghe_4"]),
    "GeL":  ("bayan", ["ghe_5", "ghe_6"]),           # the long, gliding ones
    "Ke":   ("bayan", ["ke", "ke_2", "ke_3"]),
}
CACHE = {}
TAKE = {}

def take(stroke):
    drum, names = STROKES[stroke]
    k = TAKE.get(stroke, RNG.integers(len(names)))
    TAKE[stroke] = (k + 1) % len(names)
    nm = names[k]
    if nm not in CACHE:
        CACHE[nm] = load(nm)
    return drum, CACHE[nm]

def retune(x, ratio):
    """play the recording ratio times faster (ratio > 1: higher and shorter)"""
    if abs(ratio - 1) < 1e-4:
        return x
    n = int(len(x) / ratio)
    return np.interp(np.arange(n) * ratio, np.arange(len(x)), x)

BOLS = {"Na": ["Na"], "Ta": ["Ta"], "Tun": ["Tun"], "Te": ["Te"], "Re": ["Re"], "Ke": ["Ke"], "Ge": ["Ge"],
        "Dha": ["Ge", "Na"], "DhinA": ["Ge", "TinA"], "DhinB": ["Ge", "TinB"], "TinA": ["TinA"], "TinB": ["TinB"],
        "Ti": ["Te"], "Ka": ["Ke"], "GeL": ["GeL"]}

class Player:
    def __init__(self, dayan_hz, total_s):
        self.ratio = dayan_hz / DAYAN_HZ
        self.out = np.zeros(int(total_s * FS))
        self.ringing = {"dayan": None, "bayan": None}   # (start sample, its signal) of the stroke still sounding

    def _choke(self, drum, at):
        r = self.ringing[drum]
        if r is None:
            return
        s0, y = r
        k = at - s0
        if 0 < k < len(y):
            f = min(int(0.008 * FS), len(y) - k)
            tail = y[k:k + f] * np.linspace(1, 0, f)
            self.out[at:at + len(y) - k] -= y[k:]
            self.out[at:at + f] += tail

    def play(self, bol, t, vel=0.8):
        at = max(int(t * FS), 0)
        for st in BOLS[bol]:
            drum, x = take(st)
            if drum == "dayan":
                x = retune(x, self.ratio)
            g = vel * RNG.uniform(0.9, 1.0)
            if vel < 0.6:                              # a soft stroke is also a little darker
                b, a = signal.butter(1, 2500 + 6000 * vel, fs=FS)
                x = signal.lfilter(b, a, x)
            y = x * g
            self._choke(drum, at)                      # (the same drum only: a Ge keeps ringing under the dayan)
            m = min(len(y), len(self.out) - at)
            self.out[at:at + m] += y[:m]
            self.ringing[drum] = (at, y[:m])

def write(name, x, peak_db=-3.0):
    x = x / (np.max(np.abs(x)) + 1e-12) * 10 ** (peak_db / 20)
    wavfile.write(os.path.join(OUT, name + ".wav"), int(round(FS)), (x * 32767).astype(np.int16))

def teentaal(dayan_hz, tin, mpm=120, cycles=2):
    tk = ["Dha", "Dhin", "Dhin", "Dha", "Dha", "Dhin", "Dhin", "Dha", "Dha", "Tin", "Tin", "Ta", "Ta", "Dhin", "Dhin", "Dha"]
    beat = 60.0 / mpm
    p = Player(dayan_hz, cycles * 16 * beat + 3)
    for c in range(cycles):
        for i, b in enumerate(tk):
            b = {"Dhin": "Dhin" + tin, "Tin": "Tin" + tin, "Ta": "Na"}.get(b, b)
            p.play(b, (c * 16 + i) * beat + RNG.uniform(-0.006, 0.006), 0.95 if i == 0 else RNG.uniform(0.72, 0.82))
    return p.out

def main():
    os.makedirs(OUT, exist_ok=True)
    sa = 220.0
    for label, dhz in (("dayanA4", 2 * sa), ("dayanA3", sa)):
        # each stroke: all its takes, then soft, medium, hard on one take
        for k, st in enumerate(["Na", "Ta", "Tun", "TinA", "TinB", "Te", "Re", "Ge", "GeL", "Ke"]):
            if STROKES[st][0] == "bayan" and label == "dayanA3":
                continue                                   # (the bayan is the same in both)
            n = len(STROKES[st][1])
            p = Player(dhz, 1.2 * (n + 3) + 3)
            for j in range(n):
                p.play(st, j * 1.2, 0.85)
            for j, v in enumerate((0.4, 0.65, 0.95)):
                p.play(st, (n + j) * 1.2, v)
            nm = {"TinA": "Tin_standin_A_na-open", "TinB": "Tin_standin_B_tun"}.get(st, st)
            suffix = "" if STROKES[st][0] == "bayan" else "_" + label
            write(f"{k + 1:02d}_{nm}{suffix}", p.out)
        for k, b in enumerate(["Dha", "DhinA", "DhinB"]):
            p = Player(dhz, 6)
            for j, v in enumerate((0.5, 0.75, 0.95)):
                p.play(b, j * 1.3, v)
            nm = {"DhinA": "Dhin_with_Tin_A", "DhinB": "Dhin_with_Tin_B"}.get(b, b)
            write(f"{k + 11:02d}_{nm}_{label}", p.out)
        p = Player(dhz, 6)
        for i, b in enumerate(["Dha", "Ke", "Dha", "Ge", "Na", "Ke", "TinA", "Na", "Dha", "Ti", "Re", "Ke", "Te", "Dha"]):
            p.play(b, i * 0.3, 0.8)
        write(f"14_phrase_{label}", p.out)
        r, tan = wavfile.read(os.path.join(ROOT, "renders", "step-2b", "01_Q_quality_saA2_Pa_jawari50.wav"))
        tan = tan[:, 0].astype(np.float64) / 2 ** 31
        for tin in ("A", "B"):
            x = teentaal(dhz, tin)
            write(f"15_teentaal_120_TinStandin{tin}_{label}", x)
            n = min(len(x), len(tan))
            mix = x[:n] / np.max(np.abs(x)) * 0.55 + tan[:n] / np.max(np.abs(tan[:n])) * 0.4
            write(f"16_teentaal_120_TinStandin{tin}_{label}_with_tanpura", mix)

if __name__ == "__main__":
    main()
