"""Master section: where volume and mix act, and the mix law.

Wet is made different from dry with Tube 1.0 (a known static curve), then
y = a*wet + b*dry is solved from two DC input levels for each mix/volume setting.
"""
import numpy as np

from cc import render, stereo

sr = 44100


def dc(v, params):
    return float(render(stereo(np.full(8820, v, np.float32)), params)[-1, 0])


x1, x2 = 0.05, 0.4
tube = dict(DistOn=1, Tube=1.0)
w1, w2 = dc(x1, tube), dc(x2, tube)  # wet-only signal (Mix 1, Volume 0 dB)
print(f"wet({x1})={w1:.5f}  wet({x2})={w2:.5f}")
print("mix  vol  -> a (wet weight)  b (dry weight)")
for mix in (0.0, 0.25, 0.5, 0.75, 1.0):
    for vol in (1.0, 0.5):
        y1 = dc(x1, dict(tube, Mix=mix, Vol=vol))
        y2 = dc(x2, dict(tube, Mix=mix, Vol=vol))
        a, b = np.linalg.solve([[w1, x1], [w2, x2]], [y1, y2])
        print(f"{mix:4.2f} {vol:4.2f}   a={a:7.4f}  b={b:7.4f}")
