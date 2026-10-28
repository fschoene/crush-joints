"""Compare the original's Tube and Mech stages with the reverse-camel tables.

Static curves: a slow triangle through each stage alone.
Chain order: both knobs up, compare candidate orderings.
"""
import numpy as np

import refs
from cc import WARMUP, render, stereo

sr = 44100
lead = sr // 2
t = np.arange(sr * 2) / sr
tri = (2 / np.pi * np.arcsin(np.sin(2 * np.pi * 1.0 * t))).astype(np.float32)
x = stereo(np.concatenate([np.zeros(lead, np.float32), tri]))


def measure(**params):
    return render(x, dict(DistOn=1, **params))[lead:, 0]


def rms(a, b):
    return float(np.sqrt(np.mean((a - b) ** 2)))


print("== single stage, RMS error vs reverse-camel (parameters are quantised, ~1e-3 is exact)")
for a in (0.0, 0.25, 0.5, 0.75, 1.0):
    print(f"tube={a:4}: {rms(measure(Tube=a), refs.tube(tri, a)):.4f}")
for a in (0.25, 0.5, 0.75, 1.0):
    print(f"mech={a:4}: {rms(measure(Mech=a), refs.mech(tri, a)):.4f}")

print("== stage order, RMS error of candidate chains")
for tb, mc in [(0.5, 0.5), (0.25, 0.75), (0.75, 0.25), (1.0, 0.5), (0.5, 1.0)]:
    y = measure(Tube=tb, Mech=mc)
    cands = {
        "tube->mech": refs.mech(refs.tube(tri, tb), mc),
        "mech->tube": refs.tube(refs.mech(tri, mc), tb),
        "sum clip": np.clip(refs.tube(tri, tb) + refs.mech(tri, mc), -1, 1),
    }
    print(f"tube={tb} mech={mc}:", {k: round(rms(y, v), 4) for k, v in cands.items()})
