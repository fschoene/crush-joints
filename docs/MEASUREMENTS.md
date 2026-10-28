# Measurements of the original CamelCrusher

Ground truth from black-box measurements of the original plugin. Where this
document disagrees with [RESEARCH.md](RESEARCH.md), CAMELCRUSHER_REBUILT.md or
the code, **this document wins** (RESEARCH.md has been corrected accordingly;
the code deviations are tracked in [STATUS.md](STATUS.md)).

Started 2026-10-28. Everything here is reproducible with the scripts in
[`tools/analysis/`](../tools/analysis/README.md).

## 1. Method

- **Subject:** `CamelCrusher.vst` v1.01 from the original Mac installer
  (universal `ppc7400` / `i386` / `x86_64`; the plugin is not part of this
  repository).
- **Host:** `tools/analysis/vsthost.c`, a minimal offline VST2 host built as
  `x86_64` and run under Rosetta on Apple Silicon. It renders raw float32 files
  with optional timed parameter changes and can read the plugin's own parameter
  display strings.
- **The AU cannot be used.** It is a legacy Component Manager component
  (`.rsrc` `thng` resource, `aumf` / `CaCr` / `CamA`) that macOS 26 never
  registers.
- **Plugin identity seen through the host:** name `CamelCrusher`, vendor
  `Camel Audio`, unique ID `CaCr` (`0x43614372`), version 1, VST 2400, flags 57,
  20 programs, 17 parameters, 2 in / 2 out, initial delay 0.
- **Measurement hygiene:** the plugin fades in over about 10 ms after start, so
  all analyses use a silent lead-in or discard the first 4410 frames. Output is
  deterministic and independent of the block size (512 vs 64 identical after the
  fade-in). Parameters are normalised 0..1 and quantised internally; about 1e-3
  RMS error against a reference curve means exact agreement.
- **Wake-up quirk:** the Filter stays bypassed until the plugin has seen input
  above about 1e-5 (an impulse in digital silence passes unfiltered; once any
  signal has passed, the filter stays active even through later silence). The
  harness therefore primes the plugin with a short −60 dB noise burst before every
  render.
- **Not yet measured:** the compressor's static curve and detector (only
  preliminary data in section 6), Phat in the compressed region, oversampling or
  aliasing, and the filter/compressor module order.

## 2. Parameters and display mapping

The plugin reports each parameter in real units through its own display strings
(`effGetParamDisplay` / `effGetParamLabel`). The index order is the legacy
VST2 order.

| Index | Name | Display | Mapping |
|---:|---|---|---|
| 0 | `DistOn` | on/off | switch |
| 1 | `DistMech` | % | linear, 0–100 % |
| 2 | `DistTube` | % | linear, 0–100 % |
| 3 | `MmFilterOn` | on/off | switch |
| 4 | `MmFilterCutoff` | Hz | **80 Hz – 16 kHz, non-exponential** (table below) |
| 5 | `MmFilterRes` | % | linear, 0–100 % |
| 6 | `CompressOn` | on/off | switch |
| 7 | `CompressAmount` | dB | **0 dB → −40 dB, strongly non-linear** (table below) |
| 8 | `CompressMode` | on/off | Phat switch |
| 9 | `MasterOn` | on/off | switch, full bypass |
| 10 | `MasterMix` | % | linear, 0–100 % |
| 11 | `MasterVolume` | dB | **−∞ … 0 dB** (table below) |
| 12–16 | `Unused1`–`Unused5` | – | no function |

### Cutoff (index 4)

The curve is not exponential. It is close to exponential up to about 0.9, then
steepens sharply; the last step is a jump from 12 020 Hz at 0.999 to 16 000 Hz at
1.0.

| Value | Hz | Value | Hz | Value | Hz |
|---:|---:|---:|---:|---:|---:|
| 0.00 | 80 | 0.35 | 824 | 0.70 | 2161 |
| 0.05 | 168 | 0.40 | 962 | 0.75 | 2476 |
| 0.10 | 262 | 0.45 | 1113 | 0.80 | 2861 |
| 0.15 | 360 | 0.50 | 1278 | 0.85 | 3359 |
| 0.20 | 465 | 0.55 | 1460 | 0.90 | 4059 |
| 0.25 | 577 | 0.60 | 1663 | 0.95 | 5258 |
| 0.30 | 696 | 0.65 | 1894 | 1.00 | 16000 |

Further points: 0.97 → 6141 Hz, 0.98 → 6841, 0.99 → 8040, 0.995 → 9238,
0.999 → 12 020. The display is an integer-rounded readout of the internal
frequency, not necessarily the exact filter cutoff; this needs to be confirmed
by measuring the filter response (see section 5).

### Volume (index 11)

The measured linear gain is **`gain = value ^ 2.737`** (`value ^ log₀.₅(0.15)`,
which gives exactly 0.15 at 0.5) over the whole 0..1 range; it agrees with the
rounded dB display below to within 0.5 dB. Measured on 101 positions
(`tools/analysis/measure_mappings.py`).

| Value | dB | Value | dB | Value | dB |
|---:|---:|---:|---:|---:|---:|
| 0.00 | −∞ | 0.35 | −25 | 0.70 | −8 |
| 0.05 | −71 | 0.40 | −22 | 0.75 | −7 |
| 0.10 | −55 | 0.45 | −19 | 0.80 | −5 |
| 0.15 | −45 | 0.50 | −16 | 0.85 | −4 |
| 0.20 | −38 | 0.55 | −14 | 0.90 | −3 |
| 0.25 | −33 | 0.60 | −12 | 0.95 | −1 |
| 0.30 | −29 | 0.65 | −10 | 1.00 | 0 |

There is **no positive gain**: the loudest setting is 0 dB. A rendered value of
0.5 gives a peak of 0.15 for a unit input (about −16.5 dB), consistent with the
display. The reverse-camel table `-60, -55, -38, -29, -22, -16, -12, -8, -5, -3, 0`
at 0.1 steps matches this exactly except at 0.0, which is −∞ (not −60 dB).

### Amount (index 7)

The compressor Amount knob is displayed as a **level in dB that runs from 0 dB to
−40 dB**. Measurement (section 6) shows this is the compressor's **threshold, and
that the low-level gain of the compressor equals the same number with the sign
flipped** (makeup of 0 … +40 dB). The curve is slow at the start and steep at the
end:

| Value | dB | Value | dB | Value | dB |
|---:|---:|---:|---:|---:|---:|
| 0.00 | 0 | 0.35 | −4 | 0.70 | −10 |
| 0.05 | −0 | 0.40 | −4 | 0.75 | −12 |
| 0.10 | −1 | 0.45 | −5 | 0.80 | −14 |
| 0.15 | −1 | 0.50 | −6 | 0.85 | −16 |
| 0.20 | −2 | 0.55 | −7 | 0.90 | −19 |
| 0.25 | −2 | 0.60 | −8 | 0.95 | −25 |
| 0.30 | −3 | 0.65 | −9 | 1.00 | −40 |

Dense (1001-point) versions of these three tables are produced by
`tools/analysis/dump_param_displays.py`.

## 3. Master and signal level

- **Bypass:** `MasterOn = 0` returns the input unchanged and **does not clip**
  (a ±4 input stays at ±4).
- **Unity:** with every module off, Master on, Mix 100 % and Volume 1.0, the
  output equals the input to within 3e-8 after the fade-in, with zero latency.
- **Hard clip at ±0.9999.** With Master on, the output is clipped to
  ±0.9999 (no knee, no soft limiting) regardless of module settings, Mix or
  Volume. This was verified for modules off with a ±4 input, for Mix 0 % and 50 %,
  and for Volume 0.5 with a ±8 input. Clipping sits after volume and mix.
  (CrushJoints limited at ±2.0 until 2026-10-28.)
- **Volume of 1.0 means 0 dB, with no headroom boost**, so a hot signal can only
  be turned down, never up.

## 4. Distortion

- **Each stage alone matches the reverse-camel tables.** With only Tube or only
  Mech engaged and a slow triangle input, the measured transfer curve agrees with
  the table-based formulas to RMS error ≤ 0.0008 for 0.25 / 0.5 / 0.75 / 1.0 and is
  exact at 0.0 and 1.0. The reverse-camel coefficients themselves are good.
- **The stage order is Mech first, then Tube.** With both knobs engaged, the
  chain `tube(mech(x))` matches the plugin to RMS error 0.0001–0.0008 in five
  tested combinations; `mech(tube(x))` is off by 0.037–0.070, and summing or
  averaging the stages is far worse. CrushJoints applied Tube then
  Mech until 2026-10-28, which audibly changed the character at any setting where
  both are above 0. `Tests/DSPTests.cpp` now holds recorded original outputs for
  three Tube/Mech combinations.
- The stages are memoryless (the same input value produces the same output on the
  rising and falling edge of a slow triangle).
- Not yet measured: behaviour with signals fast enough to reveal oversampling or
  aliasing, any level dependence beyond the static curve, and how `DistOn = 0`
  interacts with the other stages.

## 5. Filter

Measured with broadband noise (cross-spectrum against the input) and a
least-squares fit of a bilinear-transform 2-pole low-pass biquad (RBJ /
TPT-SVF; the two have the same magnitude response).
`tools/analysis/check_filter.py` reproduces the table.

- **Topology:** a **linear 2-pole (12 dB/octave) resonant low-pass**. The fit error is
  below 0.005 dB from 700 Hz upwards (≤ 0.08 dB at 262 Hz, limited by the
  spectral resolution). There is no saturation: gain and distortion are
  identical at every level up to the output clip (THD 0.000 % with Q = 7).
  The 24 dB/oct "4-pole" figure in earlier notes is wrong.
- **Cutoff:** the fitted cutoff equals the displayed Hz (ratio 1.000 above
  700 Hz), so section 2's cutoff table is the real `fc`, 80 Hz … 16 kHz.
- **Resonance:** **`Q = 1 + 6 · Res`** (Q = 1.0 at 0 %, 2.5 at 25 %, 4.0 at 50 %,
  5.5 at 75 %, 7.0 at 100 %). Fitted Q is within 0.002 of this at every tested
  setting. There is **no passband loss** at high resonance (DC gain 0.00 dB), so
  the peak is +16.9 dB at Res = 100 %.
- **Cutoff changes** are smoothed: after a step the response has settled in
  about 10–20 ms. The exact smoothing law is not yet measured.
- The Filter does not clip or colour the signal itself; only the master clip
  applies.

| Cutoff | Hz shown | Res | fitted fc (Hz) | fitted Q |
|---:|---:|---:|---:|---:|
| 0.30 | 696 | 0 / 0.5 / 1 | 696.8 | 0.999 / 3.990 / 6.968 |
| 0.50 | 1278 | 0 / 0.5 / 1 | 1278.2 | 1.000 / 3.998 / 6.995 |
| 0.70 | 2161 | 0 / 0.5 / 1 | 2161.1 | 1.000 / 3.999 / 6.999 |
| 0.90 | 4059 | 0 / 0.5 / 1 | 4060.0 | 1.000 / 4.000 / 6.999 |
| 0.95 | 5258 | 0 / 0.5 / 1 | 5258.1 | 1.000 / 4.000 / 7.000 |

## 6. Compressor (preliminary)

Only a first pass; the static curve and detector are **not** modelled yet.
Sine tests at 1 kHz, Phat off unless stated (`measure_mappings.py` and ad-hoc
scripts; to be turned into `check_compressor.py`).

- **Linear region:** at low levels the compressor is a pure gain of
  **`G = −20·log10(1 − 0.99·Amount)` dB**, from 0 dB at Amount 0 to +40 dB at
  Amount 1 (0.25 → +2.47, 0.5 → +5.93, 0.75 → +11.78, 0.9 → +19.25,
  0.95 → +24.51 dB). It equals minus the displayed Amount value. Equivalently:
  the threshold is the linear level `T = 1 − 0.99·Amount` and the makeup gain is
  `1/T`.
- **It acts as a limiter-like compressor** that starts well below the
  displayed threshold and tends toward a ceiling near −2 dB RMS for high Amount:

| Input (dB RMS) | −60 | −50 | −40 | −30 | −24 | −18 | −12 | −8 | −4 | −2 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| gain, Amount 0.25 | 2.47 | 2.47 | 2.47 | 2.47 | 2.47 | 2.47 | 2.47 | 2.47 | 1.57 | −0.13 |
| gain, Amount 0.50 | 5.93 | 5.93 | 5.93 | 5.93 | 5.93 | 5.93 | 4.89 | 3.81 | 1.63 | −0.13 |
| gain, Amount 0.75 | 11.78 | 11.78 | 11.78 | 11.78 | 11.78 | 8.82 | 6.09 | 4.32 | 1.68 | −0.13 |
| gain, Amount 1.00 | 40.00 | 38.00 | 28.85 | 19.70 | 14.76 | 10.50 | 6.92 | 4.69 | 1.71 | −0.13 |

  The last column is the master clip at ±0.9999. The knee is wide; with
  Amount 1.0 compression is already audible 10 dB below the displayed threshold.
- **Time behaviour:** envelope settling after a level step takes roughly
  10–20 ms in both directions (attack and release are both fast); no slow
  release was seen. Needs a proper step-response measurement with a better
  detector window.
- **Phat:** at low level the frequency response is **flat** (±0.03 dB from
  40 Hz to 15 kHz at Amount 0.5 and 1.0). **There is no bass shelf in the linear region.**
  In the compressed region Phat gives 0.3–0.9 dB *less* gain reduction at high
  levels (e.g. Amount 1.0, −4 dB: 1.71 → 2.49 dB gain; −2 dB: −0.13 → +0.90).
- **Amount 0 / compressor on** is bit-transparent.

## 7. Open measurements (next)

In this order:

1. **Compressor:** a clean static curve (level sweep with RMS and peak
   detectors, several waveforms), knee shape, attack and release laws, program
   dependence, stereo link, Phat behaviour above threshold, and the
   filter/compressor order.
2. **Cutoff smoothing** and **Volume/Mix smoothing** laws.
3. **Aliasing / oversampling** of the distortion stages.
4. **Factory programs:** load `CamelCrusherPresetsA.fxb` into the plugin through
   `effSetChunk` to confirm that the parsed values reproduce the expected audio.

## 8. Effect on CrushJoints

| Finding | Current CrushJoints behaviour | Status |
|---------|--------------------------------|--------|
| Mech runs before Tube | Mech then Tube | fixed 2026-10-28 |
| Output hard clip at ±0.9999 | hard clip at ±0.9999 | fixed 2026-10-28 |
| Cutoff is 80 Hz – 16 kHz with a custom curve | 20 Hz – 20 kHz range; legacy presets use a 20 Hz – 20 kHz exponential map | open |
| Volume is −∞ … 0 dB, gain = value^2.737 | −24 … +12 dB range, table starts at −60 dB | open |
| Filter is a linear 2-pole biquad LPF, Q = 1 + 6·Res | TPT SVF (same 2-pole shape), Q = 0.7071 … 4.2071 | open: only the Q range differs |
| Master is clip(vol · (mix·wet + (1−mix)·dry)) | same | confirmed, no change |
| Compressor: low-level gain 0 … +40 dB = −20·log10(1−0.99·Amount) | threshold 0 … −30 dB, makeup derived from a static curve | open, redesign after the static-curve measurement |
| Phat has no bass shelf in the linear region | 120 Hz +8 dB low shelf into a parallel compressor | open, redesign |
