"""Model of the original compressor, fitted from black-box measurements (docs/MEASUREMENTS.md §6).

Per sample, stereo-linked:
    m   = max(|L|, |R|) after clamping the inputs to +-3.3
    env = follower of m
            Phat off: slews linearly, +1 per 48 ms (attack) and -1 per 80 ms (release)
            Phat on : env = m
    gt  = G                        for env <= e0
          P(env) / env             above, with P(e) = 1 - c(Amount) (1 - e)^2
          G = 1 / (1 - 0.99 Amount); e0 = larger root of G e = P(e)
    g   = gain ramp: falls instantly to gt; rises at most (G - anchor) / N per sample
          (N = 0.25 ms; anchor = the gain at the last sample where g == gt)
    out = clamped input * g
c(Amount) is measured (data/comp_c_of_amount.tsv, produced by measure_comp_curve.py).
"""
import os

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
_PATH = os.path.join(HERE, "data", "comp_c_of_amount.tsv")
_C = np.loadtxt(_PATH, delimiter="\t") if os.path.exists(_PATH) else None

ATTACK_S = 0.048
RELEASE_S = 0.080
RAMP_S = 0.00025
CLAMP = 3.3


def c_of_amount(a):
    if _C is None:
        raise RuntimeError("run measure_comp_curve.py first (data/comp_c_of_amount.tsv missing)")
    return float(np.interp(a, _C[:, 0], _C[:, 1]))


def process(x, amount, sr=44100, phat=False, attack_s=ATTACK_S, release_s=RELEASE_S, ramp_s=RAMP_S):
    x = np.clip(np.asarray(x, dtype=np.float64), -CLAMP, CLAMP)
    G = 1.0 / (1.0 - 0.99 * amount)
    c = c_of_amount(amount)
    disc = (G - 2 * c) ** 2 - 4 * c * (c - 1)
    e0 = ((2 * c - G) + np.sqrt(max(disc, 0.0))) / (2 * c)
    up = 1.0 / (attack_s * sr)
    down = 1.0 / (release_s * sr)
    N = ramp_s * sr
    m = np.max(np.abs(x), axis=1)
    out = np.empty_like(x)
    env, g, anchor = 0.0, G, G
    for n in range(len(m)):
        if phat:
            env = m[n]
        elif m[n] > env:
            env = min(m[n], env + up)
        else:
            env = max(m[n], env - down)
        gt = G if env <= e0 else (1.0 - c * (1.0 - env) ** 2) / env
        step = (G - anchor) / N
        if gt <= g + step:
            g = gt
            anchor = g
        else:
            g += step
        out[n] = x[n] * g
    return out
