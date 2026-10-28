"""Sanity checks for the harness itself: bypass, unity, determinism, latency, clip level."""
import numpy as np

from cc import WARMUP, render, stereo

sr = 44100
t = np.arange(sr * 2) / sr
sine = stereo(0.5 * np.sin(2 * np.pi * 1000 * t))

y = render(sine, {"MasterOn": 0})
print("master off: max|y-x| =", np.abs(y - sine).max())

y = render(sine, {})
print("all modules off after warm-up: max|y-x| =", np.abs(y - sine)[WARMUP:].max())

a = render(sine, {"DistOn": 1, "Tube": 0.7, "Mech": 0.3})
b = render(sine, {"DistOn": 1, "Tube": 0.7, "Mech": 0.3})
print("deterministic:", np.array_equal(a, b))

c = render(sine, {"DistOn": 1, "Tube": 0.7, "Mech": 0.3}, block=64)
print("block 512 vs 64 max diff after warm-up:", np.abs(a - c)[WARMUP:].max())

imp = np.zeros((sr, 2), np.float32)
imp[sr // 2] = 0.5
yi = render(imp, {})
print("impulse latency (frames):", int(np.argmax(np.abs(yi[:, 0]))) - sr // 2)

hot = stereo(4 * np.sin(2 * np.pi * 100 * t))
yh = render(hot, {})
print("hard clip level (modules off, amp 4): +%.4f / %.4f" % (yh[WARMUP:].max(), yh[WARMUP:].min()))
print("master off, amp 4 not clipped:", render(hot, {"MasterOn": 0}).max())
