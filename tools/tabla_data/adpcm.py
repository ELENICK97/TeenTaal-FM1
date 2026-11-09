#!/usr/bin/env python3
"""IMA ADPCM, 4 bits a sample (the standard tables), encoder and decoder. The decoder is the reference the firmware's
C decoder (tabla/, step 5) must match bit for bit. A recording is stored with its own starting step index (chosen by
the encoder for the least error: a sharp attack otherwise smears while the step grows), its predictor starting at 0.

    encode(int16 array, start_index) -> (bytes, start_index); decode(bytes, n, start_index) -> int16 array
Nibbles: low nibble first in each byte."""
import numpy as np

INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]
STEP_TABLE = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
              107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
              796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
              4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
              20350, 22385, 24623, 27086, 29794, 32767]

def _step(code, pred, idx):
    step = STEP_TABLE[idx]
    diff = step >> 3
    if code & 4: diff += step
    if code & 2: diff += step >> 1
    if code & 1: diff += step >> 2
    pred = pred - diff if code & 8 else pred + diff
    pred = -32768 if pred < -32768 else 32767 if pred > 32767 else pred
    idx += INDEX_TABLE[code]
    idx = 0 if idx < 0 else 88 if idx > 88 else idx
    return pred, idx

def _encode_with(x, idx):
    pred, codes = 0, np.empty(len(x), dtype=np.uint8)
    for i, s in enumerate(x):
        step = STEP_TABLE[idx]
        d = int(s) - pred
        code = 0
        if d < 0:
            code, d = 8, -d
        if d >= step: code |= 4; d -= step
        if d >= step >> 1: code |= 2; d -= step >> 1
        if d >= step >> 2: code |= 1
        codes[i] = code
        pred, idx = _step(code, pred, idx)
    return codes

def decode_codes(codes, idx):
    pred, out = 0, np.empty(len(codes), dtype=np.int16)
    for i, c in enumerate(codes):
        pred, idx = _step(int(c), pred, idx)
        out[i] = pred
    return out

def encode(x, start_index=None):
    """x: int16. start_index None: try a few and keep the one with the least error over the first 2000 samples"""
    x = np.asarray(x, dtype=np.int16)
    cands = [start_index] if start_index is not None else [0, 16, 24, 32, 40, 48, 56, 64]
    best = None
    for idx in cands:
        head = x[:2000]
        err = np.sum((decode_codes(_encode_with(head, idx), idx).astype(np.int64) - head) ** 2)
        if best is None or err < best[0]:
            best = (err, idx)
    idx = best[1]
    codes = _encode_with(x, idx)
    if len(codes) & 1:
        codes = np.append(codes, 0)
    packed = (codes[0::2] | (codes[1::2] << 4)).astype(np.uint8)
    return packed.tobytes(), idx

def decode(data, n, start_index):
    b = np.frombuffer(data, dtype=np.uint8)
    codes = np.empty(len(b) * 2, dtype=np.uint8)
    codes[0::2], codes[1::2] = b & 15, b >> 4
    return decode_codes(codes[:n], start_index)
