"""Python wrapper around the offline VST2 host (vsthost) for the original CamelCrusher.

Parameter indices follow the original VST2 parameter list (see docs/MEASUREMENTS.md).
Values are the plugin's normalised 0..1 values.
"""
import os
import subprocess
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
HOST = os.path.join(HERE, "build", "vsthost")

P = dict(DistOn=0, Mech=1, Tube=2, FiltOn=3, Cutoff=4, Res=5, CompOn=6, Amount=7,
         Phat=8, MasterOn=9, Mix=10, Vol=11)

# Everything off, master on, unity volume, fully wet, cutoff open.
BASE = {P["DistOn"]: 0, P["Mech"]: 0, P["Tube"]: 0, P["FiltOn"]: 0, P["Cutoff"]: 1,
        P["Res"]: 0, P["CompOn"]: 0, P["Amount"]: 0, P["Phat"]: 0, P["MasterOn"]: 1,
        P["Mix"]: 1, P["Vol"]: 1}

# The plugin fades in for ~10 ms after start; discard at least this many frames.
WARMUP = 4410

# The Filter stays bypassed until the plugin has seen input above roughly 1e-5
# (an impulse in otherwise digital silence passes unfiltered). render() therefore
# primes the plugin with a short low-level noise burst and discards it again.
PRIME_FRAMES = 4410
PRIME_LEVEL = 1e-3


def _run_host(args):
    env = dict(os.environ)
    return subprocess.run(["arch", "-x86_64", HOST] + args, check=True, cwd=HERE,
                          env=env, capture_output=True, text=True)


def render(x, params=None, sr=44100, block=512, events=(), prime=True):
    """Render x (n, 2) float32 through the plugin; returns n frames.

    params: dict name -> normalised value, applied on top of BASE before rendering.
    events: iterable of (frame, name, value) applied at block boundaries.
    prime: prepend a quiet noise burst (see PRIME_FRAMES) and drop it from the output,
           so the plugin is awake and past its fade-in when x starts.
    """
    p = dict(BASE)
    for k, v in (params or {}).items():
        p[P[k]] = v
    x = np.ascontiguousarray(x, dtype=np.float32)
    offset = 0
    if prime:
        rng = np.random.default_rng(12345)
        burst = (PRIME_LEVEL * rng.standard_normal((PRIME_FRAMES, 2))).astype(np.float32)
        x = np.concatenate([burst, x])
        offset = PRIME_FRAMES
        events = [(f + offset, k, v) for f, k, v in events]
    with tempfile.TemporaryDirectory() as d:
        i = os.path.join(d, "i.f32")
        o = os.path.join(d, "o.f32")
        x.tofile(i)
        cmd = ["render", i, o, str(sr), str(block)]
        for k, v in p.items():
            cmd += ["--set", f"{k}={v}"]
        for f, k, v in events:
            cmd += ["--setat", f"{f}:{P[k]}={v}"]
        _run_host(cmd)
        return np.fromfile(o, dtype=np.float32).reshape(-1, 2)[offset:]


def sweep_display(index, steps=100):
    """Return [(value, display, label)] from the plugin's own parameter display strings."""
    out = _run_host(["sweep", str(index), str(steps)]).stdout
    rows = []
    for line in out.splitlines():
        v, disp, label = (line.split("\t") + ["", ""])[:3]
        rows.append((float(v), disp.strip(), label.strip()))
    return rows


def stereo(mono):
    m = np.asarray(mono, dtype=np.float32)
    return np.stack([m, m], 1)
