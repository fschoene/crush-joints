"""Measure the real Volume gain and Cutoff frequency curves (not just the display strings).

Volume: DC gain at 101 knob positions (modules off).
Cutoff: bilinear-biquad fit of the noise response at 101 positions (Res 0).
Writes data/volume_gain.tsv and data/cutoff_fit.tsv.
"""
import os

import numpy as np
from scipy import optimize, signal

from cc import render, stereo

sr = 44100
os.makedirs("data", exist_ok=True)

print("volume")
with open("data/volume_gain.tsv", "w") as f:
    x = stereo(np.full(4410, 0.1, np.float32))
    for k in range(101):
        v = k / 100
        g = float(render(x, dict(Vol=v))[-1, 0]) / 0.1
        f.write(f"{v:.2f}\t{g:.8f}\t{20 * np.log10(g) if g > 0 else -np.inf:.3f}\n")

print("cutoff")
rng = np.random.default_rng(3)
noise = (0.05 * rng.standard_normal(sr * 6)).astype(np.float32)
X = stereo(noise)
shown = {round(float(a), 2): float(b) for a, b, _ in
         (l.split("\t") for l in open("data/display_cutoff.tsv")) if round(float(a) * 100) % 1 == 0 and abs(float(a) * 100 - round(float(a) * 100)) < 1e-6}


def biquad_mag(fr, fc, q):
    w0 = 2 * np.pi * fc / sr
    alpha = np.sin(w0) / (2 * q)
    c = np.cos(w0)
    b = np.array([(1 - c) / 2, 1 - c, (1 - c) / 2])
    a = np.array([1 + alpha, -2 * c, 1 - alpha])
    return np.abs(signal.freqz(b, a, worN=fr, fs=sr)[1])


with open("data/cutoff_fit.tsv", "w") as f:
    for k in range(101):
        v = k / 100
        y = render(X, dict(FiltOn=1, Cutoff=v, Res=0.0))[:, 0]
        fr, pxy = signal.csd(noise, y, fs=sr, nperseg=16384)
        _, pxx = signal.welch(noise, fs=sr, nperseg=16384)
        m = np.abs(pxy / pxx)
        sel = (fr > 30) & (fr < 20000)
        guess = shown.get(round(v, 2), 1000.0)
        r = optimize.least_squares(
            lambda p: 20 * np.log10(biquad_mag(fr[sel], p[0], p[1]) + 1e-9)
            - 20 * np.log10(m[sel] + 1e-9),
            [guess, 1.0], bounds=([20, 0.3], [21000, 5]))
        f.write(f"{v:.2f}\t{r.x[0]:.2f}\t{r.x[1]:.4f}\t{guess}\n")
        if k % 10 == 0:
            print(f"{v:.2f}: fit {r.x[0]:9.2f} Hz  Q {r.x[1]:.3f}  (display {guess})")
