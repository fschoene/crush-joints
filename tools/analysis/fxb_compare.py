"""Parse an original CamelCrusher .fxb and compare it with kLegacyPresets in Presets.cpp.

usage: python fxb_compare.py path/to/CamelCrusherPresetsA.fxb
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def parse_fxb(path):
    d = open(path, "rb").read()
    magic, _size, chunk_magic, _ver, fx_id, fx_ver, n_prog = struct.unpack(">4sI4sI4sII", d[:28])
    assert magic == b"CcnK" and chunk_magic == b"FBCh" and fx_id == b"CaCr", "not a CamelCrusher bank"
    off = 28 + 128
    chunk_size = struct.unpack(">I", d[off:off + 4])[0]
    chunk = d[off + 4:off + 4 + chunk_size]
    pos, out = 0, []
    while pos < len(chunk):
        marker = struct.unpack("<f", chunk[pos:pos + 4])[0]
        pos += 4
        end = chunk.index(b"\0", pos)
        name = chunk[pos:end].decode("latin1")
        pos = end + 1
        n = struct.unpack("<I", chunk[pos:pos + 4])[0]
        pos += 4
        vals = struct.unpack("<%df" % n, chunk[pos:pos + 4 * n])
        pos += 4 * n
        out.append({"name": name, "marker": marker, "vals": list(vals)})
    assert len(out) == n_prog
    return out


def code_presets():
    src = open(os.path.join(ROOT, "Source", "Common", "Presets.cpp")).read()
    num = r"([\d.]+)f?"
    pat = r'\{\s*"([^"]+)",\s*' + r",\s*".join([num] * 11) + r"\s*\}"
    return re.findall(pat, src)


def main(path):
    fxb = parse_fxb(path)
    rows = code_presets()
    print("programs in fxb:", len(fxb), " rows in Presets.cpp:", len(rows))
    names = ["distOn", "mech", "tube", "filterOn", "cutoff", "res", "compOn", "amount", "phat", "mix", "vol"]
    fidx = [0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11]  # fxb index 9 is MasterOn
    bad = 0
    for r, p in zip(rows, fxb):
        if r[0] != p["name"]:
            print("NAME MISMATCH", r[0], p["name"])
            bad += 1
        for n, c, fi in zip(names, [float(x) for x in r[1:]], fidx):
            if abs(c - p["vals"][fi]) > 1e-3:
                print(f"DIFF {p['name']:16s} {n:9s} code={c} fxb={p['vals'][fi]:.4f}")
                bad += 1
        if p["vals"][9] != 1.0 or any(p["vals"][12:]):
            print("unexpected MasterOn/unused slot in", p["name"])
    print("total differences:", bad)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
