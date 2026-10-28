# Black-box analysis harness

Tools for measuring the original CamelCrusher so the JUCE clone can be checked
against real behaviour. Results live in [docs/MEASUREMENTS.md](../../docs/MEASUREMENTS.md).

The original plugin is **not** in this repository (Camel Audio's licence forbids
copying it). Point the host at your own copy of `CamelCrusher.vst`, extracted
from the original installer, by putting it at `tools/analysis/vst/CamelCrusher.vst`
or setting `CC_VST=/path/to/CamelCrusher.vst`.

## Requirements

- Apple Silicon or Intel Mac. The plugin only has `x86_64` (plus `i386`/`ppc`)
  slices, so on Apple Silicon it runs under Rosetta.
- Xcode Command Line Tools (`clang`).
- Python 3 with `numpy` (`scipy` is useful for later filter/compressor work).

## Why a VST2 host and not the AU

The original AU is a legacy Component Manager component (`.rsrc` `thng`
resource) that current macOS never registers, so `AudioComponentFindNext` does
not find it. The VST2 bundle loads directly through `CFBundle`.

## Files

| File | Purpose |
|------|---------|
| `vsthost.c` | Minimal offline VST2 host: `info`, `sweep <param> <steps>`, `render` (own struct declarations, no SDK) |
| `build.sh` | Builds `build/vsthost` as x86_64 |
| `cc.py` | Python wrapper: `render()`, `sweep_display()`, parameter map |
| `refs.py` | reverse-camel Tube/Mech reference, parsed from `Source/Common/CamelCrusherCoeffs.cpp` |
| `check_host.py` | Harness sanity: bypass, unity, determinism, latency, clip level |
| `check_distortion.py` | Tube/Mech static curves and chain order |
| `dump_param_displays.py` | Cutoff/Volume/Amount display strings to `data/` |
| `fxb_compare.py` | Parse the original `CamelCrusherPresetsA.fxb` and diff it against `Presets.cpp` |

## Usage

```sh
tools/analysis/build.sh
cd tools/analysis
python3 check_host.py
python3 check_distortion.py
python3 dump_param_displays.py
python3 fxb_compare.py /path/to/CamelCrusherPresetsA.fxb
arch -x86_64 build/vsthost info
```

## Measurement notes

- The plugin fades in over about 10 ms after start; discard the first
  `WARMUP` (4410) frames. `check_*` scripts use a silent lead-in.
- Output is hard-clipped at ±0.9999, so use amplitudes and gains that stay
  below that when measuring linear behaviour.
- Parameters are normalised 0..1 and quantised internally (about 1e-3 error
  against the reference tables is exact agreement).
