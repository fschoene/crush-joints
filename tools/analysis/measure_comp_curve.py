"""Measure the compressor's parabola coefficient c(Amount) at 101 amounts.

DC input 1.2 (above full scale) hits the parabola branch for every amount:
    out = 1 - c (1 - 1.2)^2   ->   c = (1 - out) / 0.04
Also verifies the low-level gain against G = 1 / (1 - 0.99 Amount).
"""
import os

import numpy as np

from cc import render, stereo

sr = 44100
VOL = 0.14999665  # Vol = 0.5 leaves headroom below the output clip
os.makedirs("data", exist_ok=True)
rows = []
for k in range(101):
    a = k / 100
    y = render(stereo(np.full(int(sr * 0.15), 1.2, np.float32)), dict(CompOn=1, Amount=a, Vol=0.5))[-1, 0] / VOL
    lo = render(stereo(np.full(4410, 0.0005, np.float32)), dict(CompOn=1, Amount=a, Vol=0.5))[-1, 0] / VOL / 0.0005
    rows.append((a, (1 - y) / 0.04, lo))
rows = np.array(rows)
np.savetxt("data/comp_c_of_amount.tsv", rows, fmt="%.6f", delimiter="\t")
print("amount  c       low-level gain  predicted G")
for r in rows[::10]:
    print(f"{r[0]:.2f}  {r[1]:.4f}  {r[2]:10.4f}  {1 / (1 - 0.99 * r[0]):10.4f}")
