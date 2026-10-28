# CamelCrusher Research Findings

Background research used to design CrushJoints. This document describes the
**reference behaviour** we are aiming for. For the current implementation
state, see [STATUS.md](STATUS.md). Black-box measurements of the original
plugin are in [MEASUREMENTS.md](MEASUREMENTS.md); where they disagree with
secondary sources below, the measurements win.

## 1. Overview

**CamelCrusher** is a free multi-effect plugin by **Camel Audio** (concept,
design and programming credited to Ben Gillett in the manual). The manual is
"Version 1.0", © 2005; the original VST bundle is 1.01 (2006) and the Mac
installer is labelled 1.4 (2011). It was the "little brother" of the paid
**CamelPhat** distortion/compressor (the manual advertises CamelPhat 3). Camel
Audio was acquired by Apple in 2015 and the plugins were discontinued;
CamelPhat's technology was folded into Logic Pro's **Phat FX**.

CamelCrusher is a single-window, four-module colouring effect: intentionally
simple, with one knob per parameter. Its character comes from the interaction
between Tube distortion, Mech distortion, a low-pass filter and a one-knob
compressor with Phat mode. The manual counts **three effect modules**
(Distortion, Filter, Compressor) plus the Master module. The original shipped as
VST on Windows and VST/AU on macOS; the 2011 Mac installer also contains an RTAS
build. All Mac binaries are universal (ppc7400, i386, x86_64); no AAX build is in
the installer.

## 2. Signal chain

The module order is **not stated in the original manual**. It was measured on
the original (MEASUREMENTS.md §7); the Filter sits *between* the two distortion
stages:

```
Audio In
  → Mech distortion
  → Low-Pass Filter
  → Tube distortion
  → Compressor (+ Phat)
  → Master (Mix, Volume, hard clip at ±0.9999)
  → Audio Out
```

Each module has an individual blue On/Off toggle. Turning a module off bypasses
it. Turning **Master** off bypasses the whole plugin, leaving only the dry
input (manual, page 12). The manual's Distortion text says the two types "are
not exclusive", so Tube and Mech can be blended.

## 3. Distortion

### Parameters

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| TUBE | 0–100 | 0 | Tube-style saturation amount |
| MECH | 0–100 | 0 | Mechanical/harsh distortion amount |

Tube and Mech are not exclusive; they can be blended. Most of CamelCrusher's
character lives in that blend. The manual describes Tube as "a warm and
musical, analogue-style overdrive" and Mech as "a bit nastier (hint: it's great
for booming kick drums)". The manual states no ranges or defaults; the 0–100
range matches the `%` readout it mentions.

### Tube distortion (DistTube)

The reverse-camel project (soerenbnoergaard) extracted the waveshaper by
amplitude-sweep analysis. The reference uses a **101-entry lookup table**
indexed by the TUBE knob (0–100). Each entry stores a gain plus four
coefficients `{gain, a, b, c}`.

Per sample (Tube runs **after** Mech, so `x` is the Mech output):

1. `s = gain * x`, where gain maps 0–100 to approximately `1.0 … 5.0`.
2. Three-region piecewise function:
   - `s < -0.6`: `y = a·s² + b·s + c`
   - `s >  0.6`: `y = -a·s² + b·s - c` (asymmetric)
   - otherwise: `y = s`

Properties:

- Frequency-independent (verified from 20 Hz to 20 kHz).
- Asymmetric, which adds even-order harmonics (the "warm" tube character).
- At TUBE = 0 the gain is 1.0 and the curve is essentially linear.
- At TUBE = 100 the gain is about 5.0, pushing the signal into saturation.

Reference entry 0: `{1.0, -0.000207935, 0.999713, -9.81884e-05}`.
Reference entry 100: `{4.99996, 0.0206626, 0.206618, -0.483468}` (full saturation).

### Mech distortion (DistMech)

Applied **first**, before Tube (measured on the original; earlier notes and
our first implementation had the opposite order). Also uses a **101-entry
table** with three coefficients `{gain, b, c}`.

Per sample:

1. `s = gain * y`, where `y` is the input sample and gain maps 0–100 to
   approximately `1.0 … 11.0`.
2. Three-region piecewise function:
   - `s < -0.75`: `y = b·s + c`
   - `s >  0.75`: `y = b·s - c` (asymmetric)
   - otherwise: `y = s`

Properties:

- Linear/`hard-clip`-style knees, unlike Tube's quadratic soft clipping.
- Asymmetric offsets create even harmonics on top of the odd harmonics.
- At MECH = 0 it is near-linear (`{1, 1.0, 0.000054}`).
- At MECH = 100 it is heavy clipping: `{11.0001, 0.0243904, -0.731707}`.

### Sound character

| Distortion | Character | Typical use |
|------------|-----------|-------------|
| Tube | Warm, smooth, analog-style saturation; subtle even harmonics | Synths, vocals, master bus at 2–30% |
| Mech | Fizzy, mechanical, aggressive; destroys the signal at high settings | Kicks, aggressive synths, risers |
| Blend | Gritty, faintly analog character that is hard to get from stock waveshapers | Most production use |

High distortion settings raise the level considerably; the Master Volume is
needed to compensate.

### Oversampling

The original does **not** oversample, so Mech can alias at high settings.
Recreations such as CrushClone add 4× oversampling as a deliberate
improvement. CrushJoints currently runs the waveshapers at the host sample
rate, matching reverse-camel and keeping the dry/wet path latency-free. The
`Oversampler` wrapper is available for a future optional quality mode.

## 4. Filter

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| CUTOFF | 80–16,000 Hz, custom curve (measured) | open (16,000) | Low-pass cutoff, shown in Hz |
| RES | 0–100 (≈0–5 effective Q, assumed) | 0 | Resonance around the cutoff |

Characteristics:

- Described by the manual as a "warm, smooth analogue-style low-pass filter,
  with buckets of resonance" and an "analogue-modelled resonant low-pass
  filter". The manual does **not** state the slope. Measurement shows a
  **linear 2-pole (12 dB/octave) low-pass biquad with `Q = 1 + 6·Res`**
  (MEASUREMENTS.md §5); the 24 dB/octave (4-pole) figure from reviews and
  CrushClone is wrong.
- The manual only says Cutoff "sets the filter Cutoff point, in Hz" (example
  readout `71.7 Hz`) and gives **no range**. The plugin's own display strings
  show **80 Hz at the bottom and 16 kHz at the top**, with a non-exponential curve
  (about 1.3 kHz at the midpoint, 5.3 kHz at 0.95; see MEASUREMENTS.md §2). This
  confirms the YouTube analysis and replaces our earlier 20 Hz–20 kHz
  assumption. The displayed value equals the actual filter cutoff.
- The original binary contains several filter classes (biquad, `MoogAFilter`,
  `MS20AFilter`, `OTAAFilter`, `SVFAFilter`, comb, ring-mod and others), which
  looks like a library shared with CamelPhat. The measured response is a plain
  biquad (`CAFilterBiquad`-like), not a Moog/MS20/OTA model.
- Resonance peaks the cutoff by up to +16.9 dB (Q = 7) without any passband
  loss and without saturation.
- The filter's job is cleanup: distortion creates new high harmonics and the
  filter tames them.

The original is a resonant 2-pole low-pass; CrushClone models it with JUCE's
`StateVariableTPTFilter` (ZDF/TPT SVF) in low-pass mode, which has the same
magnitude response as the bilinear biquad that was measured. Reverse-camel does
not implement the filter.

## 5. Compressor

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| AMOUNT | 0–100 | 0 | Gain/limiting intensity (low-level gain 0 to +40 dB) |
| PHAT MODE | On/Off | Off | Instant detector with a 0.25 ms gain release |

The manual describes a "sophisticated and responsive 'soft-limiting'
compressor algorithm, modelled after the classic analogue compressor designs
renowned in studio folklore", with two parameters: Amount ("the amount of
compression applied to the sound, and thus the apparent 'loudness'") and the
**P** (Phat) button, which it recommends turning off for clean signals to avoid
"occasional crackles at high levels".

**The behaviour is fully measured** (MEASUREMENTS.md §6) and replaces everything
that earlier versions of this document took from CrushClone:

- A stereo-linked follower of `max(|L|, |R|)` slews at 1.0 per 48 ms (up) and
  1.0 per 80 ms (down); in Phat mode it is the instantaneous magnitude.
- The gain is `G = 1 / (1 − 0.99·Amount)` (0 to +40 dB) up to a knee, and
  `P(env)/env` above it with `P(e) = 1 − c(Amount)(1 − e)²`, so full scale
  always passes at unity and overs fold back.
- The gain falls instantly and rises along a 0.25 ms ramp.
- Phat does not change the static curve or the frequency response; there is **no
  low shelf and no parallel path** in the original. CrushClone's Phat design
  (120 Hz +8 dB shelf into a parallel compressor) is not the original's.
- The plugin displays Amount as `−G` in dB (0 to −40 dB).

CrushJoints implements this model. The earlier CrushClone-derived Phat design is
kept as an optional "Boom" stage that is not exposed yet (see
[VERSION_2.md](VERSION_2.md)).

## 6. Master

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| ON/OFF | Toggle | On | Bypasses the entire plugin (dry only) |
| VOLUME | −∞ to 0 dB (measured, no positive gain) | 0 (unity) | Output level, shown in dB |
| MIX | 0–100% | 100% (wet) | Dry/wet blend |

> **Implementation note:** the plugin's Volume parameter currently uses **-24 to
> +12 dB**, matching CrushClone's preset bank (some presets use +2 dB) rather
> than the original's −∞ to 0 dB. This was an intentional deviation, but the
> measurements show the original has no gain above 0 dB; see
> [STATUS.md](STATUS.md).

Notes:

- Master On/Off is a full bypass, unlike the per-module switches.
- Volume can go from silence to unity and is used for gain staging.
- Mix is arguably the most important control; 30–60% often sounds better than
  fully wet and enables parallel processing from one plugin.
- The original **hard-clips the output at ±0.9999** when Master is on (no knee,
  after volume and mix; measured). With Master off the signal is passed through
  unclipped.
- The original applies hard output limiting, which can distort hot signals
  (manual FAQ). Turning down Volume or disabling Phat resolves it. The manual
  also mentions an **Output Level meter** in the main display that shows
  clipping.
- CrushClone uses a safety limiter at ±2.0 (8× headroom). The original clips at
  ±0.9999; CrushJoints follows the original (since 2026-10-28), plus
  NaN/Inf/denormal sanitation.

## 7. UI reference

```
┌────────────────────────────────────────────────────────┐
│  PATCH SELECTOR   [Preset Name]        RANDOMIZE       │
│                                                          │
│  [ON]  DISTORTION                                        │
│          TUBE  [knob]    MECH  [knob]                    │
│                                                          │
│  [ON]  FILTER                                            │
│          CUTOFF  [knob]  RES  [knob]                     │
│                                                          │
│  [ON]  COMPRESSOR                                        │
│          AMOUNT  [knob]  PHAT MODE  [button]             │
│                                                          │
│  [ON]  MASTER                                            │
│          VOLUME  [knob]  MIX  [knob]                     │
└────────────────────────────────────────────────────────┘
```

Reference features:

- **Patch selector:** preset dropdown with up/down navigation and the current
  preset name.
- **Randomize:** assigns new values to every parameter in every *active*
  module, and avoids settings that produce no sound.
- **On/Off switches:** small blue toggles at the top-left of each module;
  illuminated when active.
- **Value readout:** shows the preset name; while a knob is adjusted it shows
  the value (for example `75 %`, `1.3 Hz`) and the preset name returns after
  about three seconds. Clicking it opens a menu: Load/Save Preset, Load/Save
  Bank, Load Bank A, Load Bank B and Clear Preset (resets all parameters to
  their defaults). Up/down arrows step through patches.
- **Output meter:** indicates clipping.
- **Knob modes:** right-click (Ctrl-click on Mac) menu with four items:
  Display Value, MIDI Learn, Linear and Circular. Linear (vertical drag) is the
  default and the choice applies to all knobs. Shift-drag gives fine control.
- **Presets:** the factory bank installed with the Mac installer has 20
  programs (`CamelCrusherPresetsA.fxb`). The manual says there are two default
  banks (A and B) and that a saved bank holds 64 sounds; Bank B is not in the
  installer we have. `.FXP`/`.FXB` import/export is supported.
- **Skin geometry:** the original panel is 345×373 px; control positions are in
  `Skins/default/SkinParameters.txt` (Tube left of Mech, Cutoff left of Res,
  Volume left of Mix).

## 8. Presets

### Original factory bank

The original 20-program factory bank (Annihilate … Turn it to 11) is documented
in [CAMELCRUSHER_REBUILT.md](CAMELCRUSHER_REBUILT.md) section 4 and was checked
against the original `CamelCrusherPresetsA.fxb`. The 20 Legacy/Original presets
in CrushJoints match it value for value (see section 12).

### CrushClone patches

The 12 patches used by CrushClone and CrushJoints. **These are not the original
factory presets**; only the name `DRM BigBadBeat` appears in the original
material (the manual's screenshot shows it as the active patch, so it comes from
a bank we do not have, presumably Bank B or an earlier version). The screenshot
shows knob positions that look quite different from CrushClone's values (Tube
and Mix far clockwise, Mech and Amount roughly two o'clock, Volume near the
top); that is a visual reading, not a measurement, but it suggests CrushClone's
values for this patch are invented.

| Preset | Tube | Mech | Cutoff | Res | Amount | Phat | Filter on | Volume (dB) | Mix |
|--------|------|------|--------|-----|--------|------|-----------|-------------|-----|
| Default | 0 | 0 | 20000 | 0 | 0 | off | no | 0 | 100 |
| DRM BigBadBeat | 50 | 30 | 4000 | 20 | 60 | on | yes | 0 | 100 |
| BAS FatSub | 70 | 0 | 800 | 30 | 50 | on | yes | 0 | 100 |
| GTR Crunch | 40 | 50 | 6000 | 10 | 30 | off | yes | 0 | 100 |
| SYN WarmPad | 30 | 0 | 3000 | 40 | 20 | off | yes | 0 | 100 |
| VOX Gritty | 60 | 20 | 5000 | 15 | 40 | off | yes | 0 | 100 |
| MAS Loudener | 20 | 10 | 12000 | 0 | 70 | on | yes | +2 | 100 |
| DRM LoFiHit | 80 | 60 | 2000 | 50 | 40 | off | yes | -2 | 100 |
| SYN Screamer | 30 | 80 | 7000 | 60 | 50 | on | yes | -1 | 100 |
| Gentle Warmth | 20 | 0 | 15000 | 0 | 10 | off | yes | 0 | 50 |
| Telephone | 10 | 0 | 3500 | 30 | 0 | off | yes | 0 | 100 |
| Destroy Everything | 100 | 100 | 1000 | 70 | 80 | on | yes | -4 | 100 |

The patches "Dream", "Annihilate" and "YouWereWarned" are part of the original
factory bank (confirmed in `CamelCrusherPresetsA.fxb`), not of this 12-preset
bank.

## 9. References

1. **CrushClone** (doxiedevops) — JUCE/C++ recreation.
   Full four-module implementation, 4× oversampling, ZDF SVF filter,
   one-knob compressor, Phat mode, WebUI editor, VST3/AU on macOS and VST3 on
   Windows. The preset table and parameter mappings used here originate from
   this project.
2. **Reverse Camel** (soerenbnoergaard) — DPF/C++ black-box reverse
   engineering. Most faithful source for the distortion coefficient tables;
   implements only Distortion and Master. Includes a Jupyter notebook with the
   amplitude-sweep and curve-fitting methodology.
3. **CamelCrusher Rebuilt** (lostlessvisuals) — native Apple Silicon remake
   focused on legacy session recall. Provides the original host identity,
   legacy VST2 state/chunk format and the decoded 20-program factory bank.
   See [CAMELCRUSHER_REBUILT.md](CAMELCRUSHER_REBUILT.md).
4. **Logic Pro Phat FX** — Apple's integration of CamelPhat, the paid sibling.
   Not identical to CamelCrusher but architecturally related.
5. **Black-box measurements** of the original plugin —
   [MEASUREMENTS.md](MEASUREMENTS.md), produced with
   [`tools/analysis/`](../tools/analysis/README.md). Highest-priority source for
   behaviour.

## 10. DSP implementation targets

### Tube

- Piecewise polynomial waveshaper with a 101-entry table, applied **after**
  Mech.
- Linear for `|x| ≤ 0.6`, quadratic saturation outside.
- Asymmetric upper/lower coefficients.
- Gain 1.0×–5.0× across the knob range.

### Mech

- Piecewise linear waveshaper with a 101-entry table, applied **before** Tube.
- Linear for `|x| ≤ 0.75`, linear saturation outside.
- Asymmetric `+c`/`-c` offsets.
- Gain 1.0×–11.0×.
- CrushClone adds pre-emphasis (about +200 Hz and +4 kHz shelves) before
  clipping; evaluate whether that belongs in a faithful clone.

### Filter

- Sits between Mech and Tube.
- Linear 2-pole (12 dB/octave) low-pass, 80 Hz–16 kHz, `Q = 1 + 6·Res`
  (measured; a TPT SVF reproduces it). Cutoff and resonance ramp over 10 ms.
- The original also has an output limiter that is **not implemented yet**
  (MEASUREMENTS.md §5).
- Resonance: `Q = 1 + 6·Res` (1 to 7), linear at all levels.
- ZDF/TPT SVF is the recommended model.

### Compressor

- Measured model, see section 5 and MEASUREMENTS.md §6: linked slew-limited
  follower (48 ms / 80 ms per full scale; instant in Phat mode), gain `G` up to
  a knee and `P(env)/env` above, instant attack and a 0.25 ms release ramp,
  inputs clamped to ±3.3, Amount ramps over 10 ms.
- The former CrushClone-derived design survives only as the hidden "Boom"
  stage.

### Master

- Hard clip at ±0.9999 when Master is on (measured); Master off is unclipped.
- Denormal and NaN/Inf flushing.
- Reference volume range −∞ to 0 dB (current code uses -24 to +12 dB; see
  [STATUS.md](STATUS.md)).
- Dry/wet mix, default 100% wet.

## 11. Similar effects

| Plugin | Similar to | Notes |
|--------|------------|-------|
| CamelPhat | CamelCrusher's bigger brother | Four distortions, three filters, LFOs, envelope follower, Magic EQ |
| Logic Pro Phat FX | CamelPhat | Apple's integrated version |
| OTT | CamelCrusher compressor | Aggressive upward compression |
| Decapitator | CamelCrusher Tube | Harmonic saturation with character modes |
| Saturn | CamelCrusher filter + distortion | Multi-band saturation |
| Trash 2 | CamelCrusher Mech | Aggressive destructive distortion |

## 12. Verification status

The research was re-checked against the reference sources before implementing
Milestone 1:

- **Distortion tables and formulas** were verified against
  `soerenbnoergaard/reverse-camel` (`src/reverse-camel/ReverseCamel.cpp`). The
  101-entry tables in `Source/Common/CamelCrusherCoeffs.cpp` are copied from
  that file. `Tests/DSPTests.cpp` compares `Distortion + Master` against a
  reference chain built from those tables (Mech → Tube and ±0.9999 clip, as
  measured on the original) and against values recorded from the original.
- **Filter** behaviour followed CrushClone's MIT-licensed implementation (SVF
  low-pass with Q 0.7071–4.2071); measurement showed the topology is right but
  the Q range should be 1–7 (MEASUREMENTS.md §5).
- **Compressor** curve numbers follow CrushClone, but they are unverified
  against the original (see section 5). Makeup gain and the Phat topology were
  re-designed in CrushJoints after measuring CrushClone's behaviour.
- **Presets** were checked against CrushClone's `initPresets()`. The
  `Telephone` flags were corrected (filter on, compressor off).
- **Legacy/Original presets** were re-checked against the original
  `CamelCrusherPresetsA.fxb` (2026-10-28): 20 names and all 11 compared
  parameters per preset match exactly after correcting `British Clean` Mix
  (0.615 → 1.0). `American Crunch`
  Mix is 1.0 in the original, so the earlier "Mix = 0 anomaly" was a Rebuilt
  data error.
- **Compressor and chain (2026-10-28):** the compressor, Phat mode and the
  Mech → Filter → Tube → Compressor order were identified by black-box
  measurement and are implemented in CrushJoints (MEASUREMENTS.md §6–7).
- **Black-box measurements (2026-10-28)** corrected the Tube/Mech order
  (Mech first), the output limiter (hard clip at ±0.9999), the cutoff range
  (80 Hz–16 kHz) and the volume range (−∞ to 0 dB); the reverse-camel tables
  themselves were confirmed to ~1e-3. See [MEASUREMENTS.md](MEASUREMENTS.md).
- **Manual cross-check (2026-10-28):** corrected the release year (2005, not
  2007), the format list (no AAX), the claim that the manual confirms the signal
  order, the filter slope, the cutoff range and the Phat description (none are in
  the manual), and added the readout/menu/knob-mode details.
- **Oversampling:** the claim that the original does not oversample was not
  independently verified, but reverse-camel does not oversample and neither
  does the current CrushJoints path.
- **CrushClone's distortion is not table-based**; it uses `tanh` for Tube and
  a hard clipper with a cubic knee for Mech. CrushJoints deliberately uses the
  reverse-camel polynomial tables instead, because they are the closest
  available model of the original.
