"""A/B the CrushJoints DSP chain against the original plugin on random settings.

Needs the original plugin (see README), data/display_cutoff.tsv and a build with
-DCRUSHJOINTS_BUILD_TOOLS=ON (build/CrushJointsRender_artefacts/Release/CrushJointsRender).
Original knob values are mapped to CrushJoints units (cutoff curve, volume law).
"""
import os
import subprocess
import sys
import tempfile

import numpy as np

from cc import render

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CJ = os.path.join(ROOT, "build", "CrushJointsRender_artefacts", "Release", "CrushJointsRender")
sr = 44100
rows = [l.split("\t") for l in open("data/display_cutoff.tsv")]
CV = np.array([float(r[0]) for r in rows])
CF = np.array([float(r[1]) for r in rows])


def to_clone_args(p):
    vol_db = 54.74 * np.log10(max(p["Vol"], 1e-6))
    args = dict(tube=p["Tube"], mech=p["Mech"], tube_on=p["DistOn"], cutoff=float(np.interp(p["Cutoff"], CV, CF)),
                res=p["Res"], lpf_on=p["FiltOn"], amount=p["Amount"], phat=p["Phat"], comp_on=p["CompOn"],
                volume=float(np.clip(vol_db, -24, 12)), mix=p["Mix"], master_on=p["MasterOn"])
    return [f"{k}={v}" for k, v in args.items()]


def run_clone(x, p):
    with tempfile.TemporaryDirectory() as d:
        i, o = os.path.join(d, "i.f32"), os.path.join(d, "o.f32")
        np.ascontiguousarray(x, np.float32).tofile(i)
        subprocess.run([CJ, i, o, str(sr), "512"] + to_clone_args(p), check=True)
        return np.fromfile(o, np.float32).reshape(-1, 2)


def signal_set(rng):
    t = np.arange(sr * 2) / sr
    notes = sum(0.12 * np.sin(2 * np.pi * f * t) for f in (110, 220, 330, 1760, 3300))
    env = np.where((t % 0.5) < 0.25, 1.0, 0.25)
    noise = 0.05 * rng.standard_normal(len(t))
    sig = notes * env + noise
    return {"tonal+noise": np.stack([sig, 0.8 * sig], 1), "drums-ish": np.stack(
        [np.exp(-8 * (t % 0.5)) * np.sin(2 * np.pi * 60 * (t % 0.5) * 8) * 0.6 + noise] * 2, 1)}


def main(n=12, seed=1):
    rng = np.random.default_rng(seed)
    worst = -200.0
    for k in range(n):
        p = dict(DistOn=int(rng.random() < 0.85), Mech=rng.random(), Tube=rng.random(), FiltOn=int(rng.random() < 0.7),
                 Cutoff=rng.uniform(0.2, 1.0), Res=rng.random(), CompOn=int(rng.random() < 0.85), Amount=rng.random(),
                 Phat=int(rng.random() < 0.5), MasterOn=1, Mix=rng.choice([1.0, 1.0, rng.random()]),
                 Vol=rng.uniform(0.45, 1.0))
        for name, x in signal_set(rng).items():
            x = x.astype(np.float32)
            orig = render(x, p)
            ours = run_clone(x, p)
            s = slice(int(sr * 0.2), None)
            err = float(np.sqrt(np.mean((orig[s] - ours[s]) ** 2)))
            ref = float(np.sqrt(np.mean(orig[s] ** 2)))
            rel = 20 * np.log10(err / ref + 1e-12)
            worst = max(worst, rel)
            print(f"{k:2d} {name:12s} Dist{p['DistOn']} Filt{p['FiltOn']} Comp{p['CompOn']} Phat{p['Phat']} "
                  f"tube {p['Tube']:.2f} mech {p['Mech']:.2f} cut {p['Cutoff']:.2f} res {p['Res']:.2f} "
                  f"amt {p['Amount']:.2f} mix {p['Mix']:.2f} vol {p['Vol']:.2f}: error {rel:6.1f} dB")
    print(f"worst error {worst:.1f} dB relative to the original output")


if __name__ == "__main__":
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 12)
