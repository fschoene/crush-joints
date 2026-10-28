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
- **Not yet measured:** filter response, compressor and Phat behaviour, oversampling
  or aliasing, smoothing times, and the order of volume and mix inside Master.

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
−40 dB**. The semantics are not measured yet; it looks like a threshold, which
would fit the one-knob design. The curve is slow at the start and steep at the end:

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
  (CrushJoints currently limits at ±2.0.)
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
  averaging the stages is far worse. CrushJoints currently applies Tube then
  Mech, which audibly changes the character at any setting where both are above 0.
- The stages are memoryless (the same input value produces the same output on the
  rising and falling edge of a slow triangle).
- Not yet measured: behaviour with signals fast enough to reveal oversampling or
  aliasing, any level dependence beyond the static curve, and how `DistOn = 0`
  interacts with the other stages.

## 5. Open measurements (next)

In this order:

1. **Cutoff and resonance:** filter response for several cutoff and resonance
   settings; whether the displayed Hz equals the −3 dB point; slope and
   resonance behaviour; level dependence (saturation); self-oscillation.
2. **Master internals:** order of volume and mix, and exact clip value.
3. **Compressor and Phat:** static curve, attack/release, knee, makeup gain,
   Amount semantics, Phat shelf and parallel structure, and the module order.
4. **Aliasing / oversampling** of the distortion stages.
5. **Factory programs:** load `CamelCrusherPresetsA.fxb` into the plugin through
   `effSetChunk` to confirm that the parsed values reproduce the expected audio.

## 6. Effect on CrushJoints

| Finding | Current CrushJoints behaviour | Status |
|---------|--------------------------------|--------|
| Mech runs before Tube | Tube then Mech | open (fix follows) |
| Output hard clip at ±0.9999 | limiter at ±2.0 | open (fix follows) |
| Cutoff is 80 Hz – 16 kHz with a custom curve | 20 Hz – 20 kHz range; legacy presets use a 20 Hz – 20 kHz exponential map | open |
| Volume is −∞ … 0 dB | −24 … +12 dB range, table starts at −60 dB | open |
| Amount reads 0 … −40 dB | assumed threshold 0 … −30 dB | open, pending compressor measurement |
| Phat/compressor/filter internals | CrushClone-derived guesses | open, pending measurement |
