#!/usr/bin/env python3
"""Independent check of a rendered WAV: the frequency of the left channel in time windows
(interpolated zero crossings), in Hz and in cents against a reference.
  tools/test/wavcheck.py FILE.wav REF_HZ T0:T1 [T0:T1 ...]"""
import math, struct, sys, wave
FS = 24e6 / 544                      # the FM-1's rate (the header says 44118)
def left(path):
    w = wave.open(path, 'rb'); n = w.getnframes(); raw = w.readframes(n)
    assert w.getsampwidth() == 3 and w.getnchannels() == 2
    out = []
    for i in range(0, len(raw), 6):
        v = raw[i] | raw[i+1] << 8 | raw[i+2] << 16
        out.append(v - (1 << 24) if v & 0x800000 else v)
    return out
def freq(x, t0, t1):
    a, b = int(t0 * FS), int(t1 * FS); cr = []
    for i in range(a + 1, b):
        p, c = x[i-1], x[i]
        if p < 0 <= c: cr.append(i - 1 + p / (p - c))
    return (len(cr) - 1) * FS / (cr[-1] - cr[0])
x = left(sys.argv[1]); ref = float(sys.argv[2])
for w in sys.argv[3:]:
    t0, t1 = map(float, w.split(':')); f = freq(x, t0, t1)
    print(f"{t0:5.2f}-{t1:5.2f} s  {f:9.4f} Hz  {1200*math.log2(f/ref):+9.3f} cents")
