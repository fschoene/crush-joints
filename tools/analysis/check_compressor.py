"""Compare compressor_model.py with the original on several signals, Phat off and on.

Needs data/comp_c_of_amount.tsv (run measure_comp_curve.py first). The plugin's output clip
sits at +-0.9999 after Vol 0.5 (gain 0.15), so both sides are clipped at the same level.
"""
import numpy as np

import compressor_model as cm
from cc import render, stereo

sr = 44100
VOL = 0.14999665
CLIP = 0.9999 / VOL
rng = np.random.default_rng(11)
t = np.arange(sr * 2) / sr

signals = {
    "sine 1 kHz -6 dB": stereo(0.5 * np.sin(2 * np.pi * 1000 * t)),
    "sine 60 Hz -12 dB": stereo(0.25 * np.sin(2 * np.pi * 60 * t)),
    "sine 1 kHz -30 dB": stereo(0.03 * np.sin(2 * np.pi * 1000 * t)),
    "noise rms -12 dB": stereo(0.25 * rng.standard_normal(sr * 2)),
    "bursts 0.7/0.02 @440": stereo(np.where((t % 0.5) < 0.2, 0.7, 0.02) * np.sin(2 * np.pi * 440 * t)),
    "stereo loud L / quiet R": np.stack([0.6 * np.sin(2 * np.pi * 500 * t),
                                         0.02 * np.sin(2 * np.pi * 730 * t)], 1),
    "hot sine 1 kHz +6 dB": stereo(2.0 * np.sin(2 * np.pi * 1000 * t)),
}

worst = {}
for phat in (0, 1):
    for amount in (0.0, 0.1, 0.3, 0.6, 0.85, 1.0):
        for name, x in signals.items():
            xin = np.ascontiguousarray(x, np.float32)
            y = np.clip(render(xin, dict(CompOn=1, Amount=amount, Phat=phat, Vol=0.5)) / VOL, -CLIP, CLIP)
            m = np.clip(cm.process(xin, amount, sr, phat=bool(phat)), -CLIP, CLIP)
            s = slice(int(sr * 0.1), None)
            err = float(np.sqrt(np.mean((y[s] - m[s]) ** 2)))
            ref = float(np.sqrt(np.mean(y[s] ** 2)))
            worst[(phat, amount, name)] = 20 * np.log10(err / ref + 1e-12)
    print(f"Phat {phat}: worst model error {max(v for k, v in worst.items() if k[0] == phat):6.1f} dB "
          f"relative to the output (all {len(signals) * 6} signal/amount pairs)")
