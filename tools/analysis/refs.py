"""Reference implementations of the reverse-camel distortion stages.

Coefficients are parsed from Source/Common/CamelCrusherCoeffs.cpp, so these
functions reproduce what CrushJoints uses (before interpolation).
"""
import os
import re

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
_SRC = open(os.path.join(ROOT, "Source", "Common", "CamelCrusherCoeffs.cpp")).read()


def _table(name, k):
    body = _SRC.split(f"{name}[101][{k}] = ")[1].split("};")[0]
    rows = re.findall(r"\{([^{}]*)\}", body)
    return np.array([[float(v.rstrip("f")) for v in re.findall(r"-?[\d.]+(?:e-?\d+)?f?", r)]
                     for r in rows])


TUBE = _table("disttube_coeffs", 4)
MECH = _table("distmech_coeffs", 3)


def tube(x, amount):
    g, a, b, c = TUBE[int(round(amount * 100))]
    s = g * np.asarray(x)
    return np.where(s < -0.6, a * s * s + b * s + c,
                    np.where(s > 0.6, -a * s * s + b * s - c, s))


def mech(x, amount):
    g, b, c = MECH[int(round(amount * 100))]
    s = g * np.asarray(x)
    return np.where(s < -0.75, b * s + c, np.where(s > 0.75, b * s - c, s))
