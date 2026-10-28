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
- **Not yet measured:** oversampling/aliasing, and the details listed in
  section 8.

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
−40 dB**. Measurement (section 6) shows that the compressor's **low-level gain
equals the same number with the sign flipped** (makeup of 0 … +40 dB), i.e. the
display is `−G`. The curve is slow at the start and steep at the
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
- `DistOn = 0` switches both stages off; the Filter and Compressor are separate
  (section 7). Not yet measured: aliasing/oversampling behaviour.

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
- Below the output limiter described next, the Filter does not clip or colour
  the signal.

**Output limiter (measured, not implemented).** The original's filter limits its own
output. With a DC or sine input the output settles at ±1.0 (a sine peaks at 1.027
to 1.029 above 1 kHz, 1.017 at 100 Hz and 1.005 at 30 Hz), with no harmonic
distortion: the signal is simply scaled by a slowly moving gain `g = 1/e`
applied after the filter (stereo-linked behaviour not yet checked).

- Below 1.0 nothing happens, which is why the earlier linearity checks passed.
- The envelope `e` attacks linearly at **113.33 per second** towards `|y|`
  (i.e. 8.82 ms per unit): a step from 0.5 to 2.0 takes 4.4 ms before the gain
  starts to fall, a step from silence 8.8 ms, and then `1/g` rises linearly.
  This fits DC steps exactly (error 0.00000).
- When `|y|` falls below `e` and `e < 1`, `e` falls linearly at about
  **20.8 per second** (0.8 → 0 loses 0.11 in 5 ms).
- When `e > 1` the gain recovers exponentially, `dg/dt = 0.13·(1 − g)` per
  second (time constant about 7.7 s), independent of the signal level below 1.2.
  For `e = 3` that is 0.78 per second.
- What is not understood: the steady sine limit of 1.027 (the model gives 1.000),
  and the behaviour with noise (the plugin lets peaks of 2.2 through where the
  model, with the same constants, limits to 1.4). A detector that smooths `|y|`
  by about 20 µs fixes the sine but not the noise.

With the limiter missing, `tools/analysis/compare_clone.py` shows a typical
agreement of −45 to −60 dB against the original over random settings and an
error up to −7 dB RMS for hot, resonant settings with Filter and Distortion on.

| Cutoff | Hz shown | Res | fitted fc (Hz) | fitted Q |
|---:|---:|---:|---:|---:|
| 0.30 | 696 | 0 / 0.5 / 1 | 696.8 | 0.999 / 3.990 / 6.968 |
| 0.50 | 1278 | 0 / 0.5 / 1 | 1278.2 | 1.000 / 3.998 / 6.995 |
| 0.70 | 2161 | 0 / 0.5 / 1 | 2161.1 | 1.000 / 3.999 / 6.999 |
| 0.90 | 4059 | 0 / 0.5 / 1 | 4060.0 | 1.000 / 4.000 / 6.999 |
| 0.95 | 5258 | 0 / 0.5 / 1 | 5258.1 | 1.000 / 4.000 / 7.000 |

## 6. Compressor

The compressor is fully identified. `tools/analysis/compressor_model.py` reproduces
the plugin to within −65 dB (Phat off) / −79 dB (Phat on) relative to the output
on sines, noise, bursts and stereo-unbalanced signals at six Amount settings; most
cases are below −85 dB (float precision). `check_compressor.py` repeats the comparison.

**Signal flow, per sample, stereo-linked:**

1. **Input clamp:** both channels are clamped to ±3.3. The detector input is
   `m = max(|L|, |R|)`, so a loud channel also controls the other one.
2. **Envelope `env`:**
   - **Phat off:** a *slew-limited follower* of `m`: it moves towards `m` at a
     constant rate, up by 1.0 per **48 ms** (attack) and down by 1.0 per **80 ms**
     (release), per full scale. Rates are time-based (identical at 44.1, 48 and
     96 kHz) and independent of Amount and step size. A sine therefore settles at
     about 0.83 of its peak.
   - **Phat on:** `env = m` (instant attack and release).
3. **Gain target `gt(env)`:** `G = 1 / (1 − 0.99·Amount)` (1 to 100, i.e. 0 to
   +40 dB) up to `env = e0`, and `P(env) / env` above, with the parabola
   `P(e) = 1 − c·(1 − e)²`. `e0` is the larger root of `G·e = P(e)`, and `c`
   depends on Amount (table below). `P(1) = 1`, so a full-scale signal always
   passes at unity, and the static curve is the continuous join of the line `G·x`
   and the parabola. Above 1.0 the parabola turns down, so overs are folded back
   (the output peaks at 1.0 for an input of 1.0 and then falls, going negative above
   roughly 1.6 to 2.2 depending on Amount). This is the original's behaviour and it is
   what the "soft-limiting" in the manual is.
4. **Gain ramp:** the gain falls to `gt` immediately, and rises towards `gt` at
   most `(G − g_anchor) / N` per sample, with `N = 0.25 ms` (11.025 samples at
   44.1 kHz). The slope is fixed when the ramp starts and recomputed whenever the
   gain equals its target again. With Phat off the slow envelope almost never lets
   the ramp matter; with Phat on it is the release.
5. **Output:** `out = clamped input × gain`, then Master (hard clip at ±0.9999).

Static DC curves match `out = min(G·x, P(x))` exactly for Amount ≥ 0.5 (error
< 1e-4), and the parabola side for all amounts. With the compressor on and Amount
at 0 the gain is exactly 1 below 0.998, so only the fold-back above 1.0 and the
±3.3 clamp remain; at Amount 0 it is still bit-transparent for normal levels.

**`c` against Amount** (measured on 101 points by
`tools/analysis/measure_comp_curve.py`, interpolated linearly):

| Amount | c | Amount | c | Amount | c |
|---:|---:|---:|---:|---:|---:|
| 0.0 | 1.4493 | 0.4 | 1.0446 | 0.8 | 0.7884 |
| 0.1 | 1.3285 | 0.5 | 0.9699 | 0.9 | 0.7391 |
| 0.2 | 1.2222 | 0.6 | 0.9030 | 0.95 | 0.7162 |
| 0.3 | 1.1282 | 0.7 | 0.8428 | 1.0 | 0.6943 |

**Other compressor facts**

- **Phat does not change the static curve** (DC curves are identical to
  4 decimals) and does not shape the frequency response (flat ±0.03 dB). It only
  changes the dynamics: instant detector plus the 0.25 ms release ramp instead of
  the 48 ms / 80 ms follower. It sounds punchier and, above threshold, gives less
  gain reduction on sustained signals. The manual's note about "crackles" fits an
  instant, ripple-following gain.
- **Switching the module on** starts the follower from zero and fades the Amount
  in; **switching Phat off** restarts the follower from zero and the gain rises
  through the 0.25 ms ramp. Switching the module off is an instant bypass.
- **Amount** changes are smoothed (see section 7).

## 7. Signal chain, switches and smoothing

**Chain (measured, `check_chain.py`):**

```
in → Mech → Filter → Tube → Compressor → Master (mix, volume, clip ±0.9999) → out
```

The filter sits **between Mech and Tube**. Of the 24 orderings of the four
stages, this one has an RMS error of 0.0019 and the next best 0.060 (30× worse); the
remaining 0.002 is the distortion table precision. The **Dist** switch
turns Mech *and* Tube on or off together; the Filter and Compressor switches only
affect their own stage, so every combination of the three switches was verified.

**Parameter smoothing** (steps applied mid-stream with DC or noise input):

| Parameter | Behaviour |
|---|---|
| Volume, Mix | linear ramp in the **knob value**, about 10 ms (441 samples); Volume's gain curve `v^2.737` is applied to the ramping value |
| Amount | ramps like Volume/Mix over about 10 ms |
| Cutoff | linear ramp in the knob value over about 10 ms with the coefficients recomputed every sample (error vs the per-sample model 3 % of the signal; per-block updates are clearly worse) |
| Tube, Mech | no smoothing; changes apply on the next sample |
| Resonance | not measured separately; assumed like Cutoff |
| Module switches | instant (Compressor-on is treated as Amount ramping up from 0) |

## 8. Open measurements (next)

1. **The filter output limiter** (section 5): detector shape, release law and
   stereo linking.
2. **Cutoff/Resonance smoothing** and the Filter's behaviour when it is switched
   on (state reset) in more detail.
3. **Aliasing / oversampling** of the distortion stages.
4. **Factory programs:** load `CamelCrusherPresetsA.fxb` into the plugin through
   `effSetChunk` to confirm that the parsed values reproduce the expected audio
   end to end.
5. **Resonance/Q ranges at the extremes** (cutoff near 16 kHz at high sample
   rates) and the exact behaviour of denormals/silence in the compressor.

## 9. Effect on CrushJoints

| Finding | Behaviour in CrushJoints before this work | Status |
|---------|-------------------------------------------|--------|
| Chain is Mech → Filter → Tube → Compressor → Master | Tube → Mech, then Filter, then Compressor | fixed (see STATUS.md) |
| Output hard clip at ±0.9999 | limiter at ±2.0 | fixed 2026-10-28 |
| Cutoff is 80 Hz – 16 kHz with a custom curve | 20 Hz – 20 kHz exponential-skew knob | fixed |
| Volume is −∞ … 0 dB, gain = value^2.737 | −24 … +12 dB range; stepped legacy table | range kept deliberately; legacy conversion uses the measured law |
| Filter is a linear 2-pole biquad, Q = 1 + 6·Res | TPT SVF, Q 0.7071 … 4.2071 | fixed |
| Compressor: measured model above | CrushClone-derived design | replaced |
| Filter output limiter | not present | open (section 5) |
| Phat: instant detector, no shelf | 120 Hz shelf into a parallel compressor | replaced; the old design is kept as the hidden "Boom" mode |
| Master is clip(vol · (mix·wet + (1−mix)·dry)) | same | confirmed |
