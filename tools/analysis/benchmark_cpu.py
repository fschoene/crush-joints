"""Measure the realtime headroom of the CrushJoints DSP chain.

Renders a fixed 20 s stereo signal through cjrender (built with
-DCRUSHJOINTS_BUILD_TOOLS=ON) for several sample rates and block sizes and
reports the processing time relative to the audio duration. "CPU" is the
fraction of one core; values well below 100 % mean the plugin can run in real
time on that core.

Usage: python3 benchmark_cpu.py [seconds]
"""
import os
import subprocess
import sys
import tempfile
import time

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CJ = os.path.join(ROOT, "build", "CrushJointsRender_artefacts", "Release", "CrushJointsRender")

# Everything on, with a hot but stable setting: the filter moves its
# coefficients every sample, Phat uses the instant detector and the compressor
# is doing heavy gain reduction. This is close to the worst case.
SETTINGS = [
    "tube=0.8", "mech=0.8", "tube_on=1", "cutoff=5000", "res=0.7", "lpf_on=1",
    "amount=0.8", "phat=1", "comp_on=1", "volume=0", "mix=1", "master_on=1",
]


def make_signal(seconds, sr, path):
    rng = np.random.default_rng(1)
    t = np.arange(int(seconds * sr)) / sr
    left = 0.25 * np.sin(2 * np.pi * 110 * t) + 0.1 * rng.standard_normal(len(t))
    right = 0.25 * np.sin(2 * np.pi * 220 * t) + 0.1 * rng.standard_normal(len(t))
    np.stack([left, right], 1).astype(np.float32).tofile(path)


def main(seconds=20.0):
    if not os.path.exists(CJ):
        sys.exit(f"missing {CJ}; configure with -DCRUSHJOINTS_BUILD_TOOLS=ON and build")

    print(f"CrushJoints DSP benchmark, {seconds:.0f} s stereo, all modules on")
    print("sample rate | block | wall time | CPU (one core) | realtime factor")

    with tempfile.TemporaryDirectory() as directory:
        source = os.path.join(directory, "in.f32")
        target = os.path.join(directory, "out.f32")

        for sr in (44100, 48000, 96000):
            make_signal(seconds, sr, source)
            for block in (64, 128, 256, 512, 1024):
                best = None

                for _ in range(2):
                    started = time.perf_counter()
                    subprocess.run([CJ, source, target, str(sr), str(block)] + SETTINGS,
                                   check=True, stdout=subprocess.DEVNULL)
                    elapsed = time.perf_counter() - started
                    best = elapsed if best is None else min(best, elapsed)

                cpu = 100.0 * best / seconds
                print(f"{sr:11d} | {block:5d} | {best * 1000:7.1f} ms | "
                      f"{cpu:8.1f} %     | {seconds / best:6.0f}x")


if __name__ == "__main__":
    main(float(sys.argv[1]) if len(sys.argv) > 1 else 20.0)
