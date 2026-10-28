"""Find the order of Mech, Filter, Tube and Compressor, then check the full chain.

Candidate chains are built from refs.py (Tube/Mech), a bilinear biquad (Filter) and
compressor_model.py, and compared with the plugin.  Needs data/comp_c_of_amount.tsv and
data/display_cutoff.tsv (dump_param_displays.py).
"""
import itertools

import numpy as np
from scipy import signal

import compressor_model as cm
import refs
from cc import render, stereo

sr = 44100
VOL = 0.14999665
CLIP = 0.9999 / VOL
rows = [l.split("\t") for l in open("data/display_cutoff.tsv")]
CV = np.array([float(r[0]) for r in rows])
CF = np.array([float(r[1]) for r in rows])

TUBE, MECH, CUT, RES, AMT = 0.6, 0.4, 0.45, 0.6, 0.7


def filt(x):
    fc = float(np.interp(CUT, CV, CF))
    q = 1 + 6 * RES
    w0 = 2 * np.pi * fc / sr
    a = np.sin(w0) / (2 * q)
    c = np.cos(w0)
    return signal.lfilter([(1 - c) / 2, 1 - c, (1 - c) / 2], [1 + a, -2 * c, 1 - a], x, axis=0)


stages = {
    "M": lambda s: refs.mech(s, MECH),
    "F": filt,
    "T": lambda s: refs.tube(s, TUBE),
    "C": lambda s: cm.process(s, AMT, sr),
}

t = np.arange(sr * 3) / sr
rng = np.random.default_rng(9)
x = stereo(0.2 * np.sin(2 * np.pi * 200 * t) + 0.2 * np.sin(2 * np.pi * 3000 * t)
           + 0.05 * rng.standard_normal(sr * 3)).astype(np.float32)

params = dict(DistOn=1, Tube=TUBE, Mech=MECH, FiltOn=1, Cutoff=CUT, Res=RES, CompOn=1, Amount=AMT, Vol=0.5)
y = np.clip(render(x, params) / VOL, -CLIP, CLIP)
print("rms error of every stage order (lower is better):")
results = []
for order in itertools.permutations("MFTC"):
    s = x.astype(np.float64)
    for k in order:
        s = stages[k](s)
    s = np.clip(s, -CLIP, CLIP)
    results.append((float(np.sqrt(np.mean((y - s)[sr // 2:] ** 2))), "".join(order)))
for err, order in sorted(results)[:6]:
    print(f"  {order}: {err:.6f}")

print("module switches (order M F T C):")
for d, f, c in itertools.product((0, 1), repeat=3):
    ps = dict(params, DistOn=d, FiltOn=f, CompOn=c)
    y2 = np.clip(render(x, ps) / VOL, -CLIP, CLIP)
    s = x.astype(np.float64)
    for k in "MFTC":
        if (k in "MT" and d) or (k == "F" and f) or (k == "C" and c):
            s = stages[k](s)
    s = np.clip(s, -CLIP, CLIP)
    print(f"  dist {d} filter {f} comp {c}: {np.sqrt(np.mean((y2 - s)[sr // 2:] ** 2)):.6f}")
