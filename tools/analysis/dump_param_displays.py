"""Dump the plugin's own display strings (Hz, dB, %) for the parameters that map nonlinearly."""
import os
import sys

from cc import P, sweep_display

steps = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
os.makedirs("data", exist_ok=True)
for name in ("Cutoff", "Vol", "Amount"):
    rows = sweep_display(P[name], steps)
    with open(f"data/display_{name.lower()}.tsv", "w") as f:
        for v, disp, label in rows:
            f.write(f"{v:.5f}\t{disp}\t{label}\n")
    print(name, "->", f"data/display_{name.lower()}.tsv", f"({len(rows)} rows)")
