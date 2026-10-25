# Camel Crusher — Research Findings

## 1. Overview

**CamelCrusher** is a free multi-effect plugin by **Camel Audio** (founded by Ben Gillett). Released as v1.0 (~2007), it was the "little brother" of Camel Audio's paid **CamelPhat** multi-band distortion/compression processor. Camel Audio was acquired by Apple in 2015; all plugins (including CamelCrusher and CamelPhat) were discontinued. Apple integrated CamelPhat's technology into **Logic Pro's Phat FX**.

CamelCrusher is a single-window, 4-module colouring multi-effect. It is intentionally simple — one knob per parameter, no hidden menus. Its character comes from the interaction between Tube distortion, Mech distortion, a low-pass filter, and a one-knob compressor with Phat mode. The original shipped as VST, AU, and AAX for 32-bit and 64-bit Windows and macOS.

---

## 2. Signal Chain

The canonical signal processing order (confirmed by the original manual, the CrushClone recreation, and black-box analysis):

```
Audio In → Distortion (Tube + Mech) → Low-Pass Filter → Compressor (+ Phat) → Master (Volume + Mix) → Audio Out
```

Each of the four modules (Distortion, Filter, Compressor, Master) has an individual **blue On/Off toggle switch**. When a module is off, it is bypassed entirely.

---

## 3. Module: Distortion

### Parameters
| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| TUBE | 0–100 (0–100%) | 0 | Tube-style saturation amount |
| MECH | 0–100 (0–100%) | 0 | Mechanical/harsh distortion amount |

The two distortion types are **not exclusive** — they can be blended simultaneously. Most of CamelCrusher's character lives in the blend between the two.

### Tube Distortion (DistTube) — Black-Box Analysis

The reverse-camel project (soerenbnoergaard) extracted the actual coefficient tables through amplitude sweep analysis. The Tube distortion uses a **101-entry lookup table** indexed by the TUBE knob (0–100). Each entry stores 4 polynomial coefficients `{a, b, c, d}` and a gain factor.

**Algorithm (per sample):**
1. Apply gain: `s = gain * x` (gain maps 0–100 → 1.0 to ~5.0)
2. Three-region piecewise function:
   - If `s < -0.6`: `y = a·s² + b·s + c` (lower saturation, quadratic polynomial)
   - If `s > 0.6`: `y = -a·s² + b·s - c` (upper saturation, **asymmetric** — different polynomial)
   - Else (`|s| ≤ 0.6`): `y = s` (linear passthrough)

Key properties:
- **Frequency independent** — the waveshaper function is the same at all frequencies (verified by the notebook analysis at 20 Hz through 20 kHz).
- **Asymmetric** — positive and negative excursions use different polynomial coefficients, adding even-order harmonics (warm, tube-like character).
- At TUBE = 0: the gain is 1.0 and the function is essentially linear (no distortion).
- At TUBE = 100: gain is ~5.0, pushing more signal into the saturation regions.

The first table entry (index 0, TUBE=0) is `{1, -0.000208, 0.9997, -0.000098}` with gain 1.0 — essentially a near-linear pass-through with negligible distortion.

The 101st entry (index 100, TUBE=100) is `{4.99996, 0.02066, -0.7302}` with gain ~5.0 — full saturation.

**Source:** `reverse-camel/src/reverse-camel/ReverseCamel.cpp` — the `disttube_coeffs[101][4]` table and the `run()` function implement this exactly. A Jupyter notebook accompanies the project performing the amplitude sweep and curve fitting.

### Mech Distortion (DistMech) — Black-Box Analysis

The Mech distortion is applied **after** Tube distortion and also uses a **101-entry lookup table** indexed by the MECH knob (0–100). Each entry stores 3 coefficients `{a, b, c}` and a gain factor.

**Algorithm (per sample):**
1. Apply gain: `s = gain * y` (where `y` is the Tube output; gain maps 0–100 → 1.0 to ~11.0)
2. Three-region piecewise function:
   - If `s < -0.75`: `y = b·s + c` (lower saturation, linear)
   - If `s > 0.75`: `y = b·s - c` (upper saturation, **asymmetric**)
   - Else (`|s| ≤ 0.75`): `y = s` (linear passthrough)

Key properties:
- **Hard clipping** with linear saturation knees (unlike Tube's quadratic soft-clipping).
- **Asymmetric** — the `+c` vs `-c` offset creates even harmonics alongside the odd harmonics from hard clipping.
- At MECH = 0: gain is 1.0, coefficients `{1, 1.0, 0.000054}` — near-linear.
- At MECH = 100: gain is ~11.0, coefficients `{10.04, 0.025, -0.731}` — heavy clipping.
- The gain ranges much higher than Tube (up to ~11× vs ~5×), making Mech the aggressive channel.

### Sound Character

| Distortion | Sound Description | Best Use Cases |
|------------|-------------------|----------------|
| **Tube** | Warm, smooth, analog-style saturation. Adds subtle even-order harmonics. Like a "cherry on top" of sparkle. | Subtle warming of synths, vocals, master bus. Low settings (2–30%) add sparkle without harshness. |
| **Mech** | Fizzy, mechanical, aggressive. Destroys signal at high settings. Adds hiss and white noise. | Booming kick drums, aggressive synth destruction, creative riser effects. Use sparingly (high settings are very destructive). |
| **Tube + Mech blend** | The combination produces a gritty, faintly analog character that's hard to replicate with stock waveshapers. | Most production use — blending gives the widest tonal palette. |

**Important note on levels:** Using high distortion amplifies the signal. The Master Volume knob is needed to keep levels under control.

### No Oversampling in Original

The original CamelCrusher does **not** use oversampling (confirmed by review sources). This means it can produce aliasing, especially with the Mech distortion at high settings. Recreation projects like CrushClone add 4× oversampling to prevent this — a deliberate improvement over the original.

---

## 4. Module: Filter

### Parameters
| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| CUTOFF | 20–20,000 Hz | 20,000 (open) | Low-pass filter cutoff frequency |
| RES | 0–100 (0–~5 effective) | 0 | Resonance / emphasis around cutoff |

### Characteristics
- **Type:** 24dB/octave low-pass filter (4-pole), described as "warm, smooth analogue-style" with "buckets of resonance."
- **Cutoff range:** 80 Hz to 16 kHz (per YouTube analysis). The full manual range appears to be 20 Hz to 20,000 Hz.
- **Resonance:** Boosts frequencies around the cutoff point. Can be "screamed" at high settings. The resonance is described as having a "slightly analog characteristic."
- **Purpose:** Cleanup. Distortion spawns new high harmonics; the filter tames them so you rarely need an EQ after the plugin.

### Filter Type Analysis

The original Camel Audio manual describes it as a "resonant low-pass filter." The CrushClone recreation uses JUCE's `StateVariableTPTFilter` (Zero-Delay Feedback / TPT SVF) in lowpass mode, which is a common model for analog-style low-pass filters. The reverse-camel project doesn't implement the filter at all.

The "warm, smooth, analogue-style" description and "buckets of resonance" suggest either a Moog-style ladder filter or a state-variable filter with nonlinear resonance. The frequency response is 24dB/octave (4-pole), typical of Moog ladder designs.

---

## 5. Module: Compressor

### Parameters
| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| AMOUNT | 0–100 (0–100%) | 0 | Compression intensity / perceived loudness |
| PHAT MODE | On/Off (toggle button) | Off | Overdrive compression harder, adds bass thickness |

### Characteristics
- **Design:** A "sophisticated and responsive 'soft-limiting' compressor algorithm, modelled after the classic analogue compressor designs renowned in studio folklore." (Official manual)
- **Mapping:** The single Amount knob simultaneously controls threshold, ratio, attack, and release, plus makes up gain.
  - Amount 0 → threshold 0 dB, ratio 1:1 (no compression)
  - Amount 100 → threshold ~-30 dB, ratio ~8:1 (heavy squash)
- **Auto-gain:** Built-in makeup gain compensation increases with Amount.
- **Character:** Not a transparent utility compressor. It's a "density and slight pump" tool designed to work on distorted signals.

### Phat Mode
- **Parallel compression + low-shelf boost** for bass thickness.
- When engaged, the dry (uncompressed) signal is blended with a heavily compressed, low-shelf-boosted copy.
- Low-shelf: ~120 Hz, +8 dB (per CrushClone analysis).
- The blend/fatness increases as Amount is raised.
- **Caution:** Can cause "crackles" at high levels on clean signals (e.g., clean guitar). The manual recommends turning Phat Mode off for smoother results on clean signals.
- **Best use:** Almost always left on for 808s and basses.

### Sound Character

The compressor works as an OTT-style (over-easy transient shaping) processor — it's not meant to be subtle. It enhances density, brings out reverb tails, and adds slight pumping. Users often start fresh, set Master to max, then dial in the compressor for glue and density.

---

## 6. Module: Master

### Parameters
| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| ON/OFF | Toggle | On | Bypasses the entire plugin (dry signal only when off) |
| VOLUME | -51 to 0 dB | 0 (unity) | Output level control |
| MIX | 0–100 (0–100%) | 100 (wet) | Dry/wet blend |

### Characteristics
- **Master On/Off:** Unlike other modules, turning Master off completely bypasses CamelCrusher — only the dry, unprocessed signal is heard.
- **Volume:** Can go from -inf (mute) to 0 dB. Useful for gain staging. If the output clip indicator shows clipping, reduce Volume.
- **Mix:** The single most important control. Works better at 30–60% than fully wet for most applications. Enables parallel processing from a single plugin.
- **Output hard limiter:** The original applies hard limiting on the output. This can cause unwanted distortion if the signal is too hot. Turn down Master Volume or disable Phat Mode to resolve.

CrushClone adds a safety hard limiter at ±2.0 (8x headroom above 0 dBFS) and denormal/NaN/Inf killing, which is a good practice for a faithful clone.

---

## 7. UI / Interface

### Layout
```
┌────────────────────────────────────────────────────────┐
│  PATCH SELECTOR   [Preset Name]        RANDOMIZE       │
│                                                          │
│  [ON]  DISTORTION                                        │
│          TUBE  [knob]    MECH  [knob]                      │
│                                                          │
│  [ON]  FILTER                                            │
│          CUTOFF  [knob]  RES  [knob]                       │
│                                                          │
│  [ON]  COMPRESSOR                                        │
│          AMOUNT  [knob]  PHAT MODE  [button]            │
│                                                          │
│  [ON]  MASTER                                            │
│          VOLUME  [knob]  MIX  [knob]                     │
└────────────────────────────────────────────────────────┘
```

### Features
- **Patch Selector:** Dropdown for presets, with up/down arrows. Shows current preset name.
- **Randomize Button:** Big button at top. Assigns new values to every parameter in every **active** module (modules that are off are not affected). "Intelligent" — won't produce settings that make no sound.
- **On/Off switches:** Small blue toggle buttons (upper-left of each module). Illuminated = active, dimmed = bypassed.
- **Value Readout:** Central display showing current preset name. When a knob is adjusted, its value appears temporarily (e.g., "75%" or "1.3 kHz"). Clicking it opens a menu for preset load/save.
- **Output Level Meter:** Visual meter showing clipping.
- **Knob modes:** Right-click (or Ctrl-click on Mac) opens a menu:
  - **Linear** (default): drag vertically to adjust
  - **Circular**: drag like a hardware knob
  - **MIDI Learn**: assign a MIDI controller
  - **Display Value**: show current value in the value readout
- **MIDI Learn:** All knobs support MIDI learn via right-click menu.
- **Presets:** 20 built-in presets. Original presets include: "BigBadBeat" (DRM), "BAS FatSub" (BAS), and others. Custom presets can be saved/loaded as .FXP/.FXB files.

### Presets (known)
From CrushClone's recreation:
- Default, DRM BigBadBeat, BAS FatSub, GTR Crunch, SYN WarmPad, VOX Gritty, MAS Loudener, DRM LoFiHit, SYN Screamer, Gentle Warmth, Telephone, Destroy Everything

Other sources mention presets: "Dream", "Annihilate", "YouWereWarned"

---

## 8. Preset List

From the reverse-camel and CrushClone projects, the built-in presets map to:

| Preset | Tube | Mech | Cutoff | Res | Comp | Phat | Dist On | Filter On | Comp On | Master (dB) | Mix |
|--------|------|------|--------|-----|------|------|---------|-----------|---------|-------------|-----|
| Default | 0 | 0 | 20000 | 0 | 0 | off | yes | no | yes | 0 | 100 |
| DRM BigBadBeat | 50 | 30 | 4000 | 20 | 60 | on | yes | yes | yes | 0 | 100 |
| BAS FatSub | 70 | 0 | 800 | 30 | 50 | on | yes | yes | yes | 0 | 100 |
| GTR Crunch | 40 | 50 | 6000 | 10 | 30 | off | yes | yes | yes | 0 | 100 |
| SYN WarmPad | 30 | 0 | 3000 | 40 | 20 | off | yes | yes | yes | 0 | 100 |
| VOX Gritty | 60 | 20 | 5000 | 15 | 40 | off | yes | yes | yes | 0 | 100 |
| MAS Loudener | 20 | 10 | 12000 | 0 | 70 | on | yes | yes | yes | +2 | 100 |
| DRM LoFiHit | 80 | 60 | 2000 | 50 | 40 | off | yes | yes | yes | -2 | 100 |
| SYN Screamer | 30 | 80 | 7000 | 60 | 50 | on | yes | yes | yes | -1 | 100 |
| Gentle Warmth | 20 | 0 | 15000 | 0 | 10 | off | yes | yes | yes | 0 | 50 |
| Telephone | 10 | 0 | 3500 | 30 | 0 | off | yes | no | yes | 0 | 100 |
| Destroy Everything | 100 | 100 | 1000 | 70 | 80 | on | yes | yes | yes | -4 | 100 |

These are from CrushClone's `initPresets()` in PluginProcessor.cpp — they represent faithful recreations of the original CamelCrusher presets based on memory and analysis.

---

## 9. Known References

### Existing Clones / Recreations

1. **CrushClone** (doxiedevops) — JUCE/C++ project
   - Full faithful recreation: Distortion (Tube/Mech), Filter, Compressor (+Phat), Master
   - 4× oversampling on distortion
   - Uses JUCE's `dsp::WaveShaper` with tanh for Tube, hard clipper for Mech
   - ZDF SVF (StateVariableTPTFilter) for filter
   - One-knob compressor with soft-knee, adaptive attack/release
   - Phat mode: parallel compression + 120 Hz low shelf
   - WebUI GUI (HTML/JS/CSS)
   - VST3 + AU on macOS, VST3 on Windows
   - CMake build, fetches JUCE automatically
   - Project structure:
     ```
     CrushClone/
     ├── CMakeLists.txt
     ├── Source/
     │   ├── PluginProcessor.cpp  (APVTS params, presets, processBlock)
     │   ├── PluginEditor.cpp     (WebUI GUI)
     │   └── DSP/
     │       ├── Distortion.cpp   (Tube + Mech waveshapers)
     │       ├── LadderFilter.cpp (ZDF SVF low-pass)
     │       ├── Compressor.cpp   (One-knob + Phat)
     │       └── Oversampler.cpp  (4× oversampling)
     ```

2. **Reverse Camel** (soerenbnoergaard) — DPF/C++ project
   - Black-box reverse engineering using amplitude sweeps
   - Uses extracted polynomial coefficient lookup tables (101 entries each for Tube and Mech)
   - **Only implements Distortion + Master** (no Filter or Compressor)
   - Includes a Jupyter notebook with the analysis methodology
   - Cross-platform (Linux, macOS, Windows cross-compile)
   - Build: simple Makefile + DPF (Distrho Plugin Framework)
   - The coefficient tables are the most faithful representation of the original DSP

3. **Logic Pro Phat FX** — Apple's integration of CamelPhat (the paid bigger brother)
   - Not identical to CamelCrusher but shares the distortion/compression/filter architecture
   - Useful reference for understanding the "Camel Audio sound"

---

## 10. DSP Summary for Implementation

### Tube Distortion (faithful to original)
- **Method:** Piecewise polynomial waveshaper with 101-entry lookup table
- **Regions:** Linear for |x| ≤ 0.6; quadratic polynomial for |x| > 0.6
- **Asymmetry:** Upper and lower saturation use different coefficients
- **Gain mapping:** 0–100 → 1.0× to ~5.0×
- **Frequency response:** Flat (frequency independent)

### Mech Distortion (faithful to original)
- **Method:** Piecewise linear waveshaper with 101-entry lookup table
- **Regions:** Linear for |x| ≤ 0.75; linear saturation for |x| > 0.75
- **Asymmetry:** Positive uses `+c`, negative uses `-c`
- **Gain mapping:** 0–100 → 1.0× to ~11.0×
- **Pre-emphasis:** CrushClone adds low/high shelf EQ before clipping (200 Hz boost, 4 kHz boost)

### Filter
- **Type:** 24dB/octave low-pass (4-pole)
- **Cutoff:** 20 Hz – 20,000 Hz
- **Resonance:** 0 – ~5 (nonlinear at high settings)
- **Model:** ZDF/TPT SVF (state-variable filter) recommended

### Compressor
- **Type:** One-knob soft-knee compressor with auto-gain
- **Threshold:** 0 dB (Amount=0) to -30 dB (Amount=100)
- **Ratio:** 1:1 (Amount=0) to ~8:1 (Amount=100)
- **Knee:** Soft, ~6 dB width
- **Attack:** 1–10 ms (faster at higher Amount)
- **Release:** 50–150 ms (faster at higher Amount)
- **Makeup gain:** Up to +12 dB at max Amount
- **Phat Mode:** Parallel compression + 120 Hz low shelf (+8 dB)

### Master
- **Output limiting:** Hard limiter at ±2.0 (safety)
- **Denormal killing:** Flush to zero
- **Volume:** -51 to 0 dB (discrete 11 steps in original)
- **Mix:** Dry/wet blend, default 100% wet

---

## 11. Similar Effects (for reference)

| Plugin | Similar To | Notes |
|--------|-----------|-------|
| **CamelPhat** (Camel Audio) | CamelCrusher's big brother | 4 distortion types, 3 filters, 2 LFOs, envelope follower, Magic EQ. Discontinued with Camel Audio. |
| **Logic Pro Phat FX** | CamelPhat | Apple's integrated version of CamelPhat. Different UI but similar architecture. |
| **OTT** (Ableton) | CamelCrusher Compressor | Aggressive upward compression. |
| **Decapitator** (Soundtoys) | CamelCrusher Tube | Harmonic saturation with character modes. |
| **Saturn** (FabFilter) | CamelCrusher Filter+Distortion | Multi-band saturation with various coloration modes. |
| **Trash 2** (iZotope) | CamelCrusher Mech | Aggressive, destructive distortion. |
