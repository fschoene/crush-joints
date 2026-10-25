# CrushJoints — Development Plan

> **Project name:** CrushJoints (CamelCrusher clone)
> **Working directory:** `/Volumes/WDB/user/flo/Code/crush-joints/`

---

## 1. Framework Decision

### Decision Matrix

| Criterion | JUCE (C++) | NIH-plug (Rust) | iPlug2 (C++) | DPF (C++) |
|-----------|-----------|-----------------|--------------|-----------|
| License | GPL/commercial | ISC (free) | MIT-like | GPL/mixed |
| Formats | VST3, AU, AAX, LV2 | VST3, CLAP (no AU) | VST3, AU, AAX, Web | VST3, CLAP, LV2, JACK |
| GUI toolkit | JUCE GUI | iced/egui/Vizia | WDL-UTFX/IPlugNG | DGL/VSTGUI |
| DSP library | `juce::dsp` (extensive) | `nih_plug::util` (basic) | Basic DSP | Minimal |
| Build system | CMake (v8+) | Cargo | CMake | Makefile |
| Community | Very large | Small/growing | Medium | Small |
| Documentation | Excellent | Good (docs.rs) | Good | Fair |
| Tooling on macOS | Xcode CLI tools | Rustup + Cargo | Xcode CLI tools | Makefile + pkg-config |
| Learning curve | Moderate | Steep (Rust) | Moderate | Moderate |

### Recommendation: **JUCE (C++)**

**Rationale:**

1. **Existing reference project:** The CrushClone project (the most complete CamelCrusher recreation) is built on JUCE with a clean CMake-based structure. Its source code, presets, and architecture provide an immediately followable blueprint.

2. **DSP completeness:** JUCE's `juce::dsp` module provides exactly what we need — `WaveShaper`, `StateVariableTPTFilter` (ZDF SVF), `IIR::Coefficients`/`IIR::Filter`, `Oversampler`, `Gain`, `ProcessorChain`. These are production-tested and numerically stable.

3. **Format coverage:** JUCE targets VST3, AU, AAX, and (via `clap-juce-extensions`) CLAP from a single C++ codebase. The user specified "VST3 or CLAP with no preference" — having both is strictly better.

4. **GUI:** JUCE's native GUI toolkit requires no external web runtime. The plugin window renders directly in the host.

5. **Build system maturity:** JUCE 8+ uses CMake natively (`juce_add_plugin`). No Projucer required. `FetchContent` can pull JUCE from Git automatically (as demonstrated by CrushClone).

6. **Host compatibility testing:** `pluginval` and the Steinberg VST3 validator work out of the box with JUCE builds.

### Why not NIH-plug (Rust)?

NIH-plug is a strong framework, but for this specific task:
- It does **not** support AU (Apple's format), which is important for macOS users. A separate `clap-wrapper` bridge would be needed, adding complexity.
- The Rust audio DSP ecosystem lacks the mature, out-of-the-box building blocks that JUCE provides (e.g., a production-ready ZDF SVF, 4× oversampler). These would need to be implemented from scratch or pulled from crates.
- The user's environment does not currently have Rust installed. While `rustup` is easy to install, the learning curve for Rust + nih-plug's macro-based API adds friction for what is essentially a well-understood DSP problem.
- No existing CamelCrusher clone exists in Rust for reference.

### Why not iPlug2 or DPF?

- **iPlug2:** Lighter than JUCE but lacks the same depth of DSP utilities. No built-in waveshaper or SVF that match JUCE's quality.
- **DPF:** The reverse-camel project uses DPF, but DPF's DSP library is minimal. You'd write most DSP from scratch. The reverse-camel project only implements 2 of 4 modules because of this.

### Build prerequisites to install (via Homebrew):
```bash
brew install cmake
```
Xcode Command Line Tools provide `clang`, `git`, etc.

---

## 2. Architecture

The plugin will follow the CrushClone architecture but use the **exact polynomial coefficient tables** from reverse-camel for the Tube and Mech waveshapers, ensuring maximum fidelity to the original.

### Project structure:

```
crush-joints/
├── CMakeLists.txt                 # JUCE CMake build, FetchContent for JUCE
├── RESEARCH.md                    # This research (done)
├── PLAN.md                        # This plan
├── Source/
│   ├── PluginProcessor.h          # APVTS parameters, signal chain
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h             # Native JUCE GUI
│   ├── PluginEditor.cpp
│   ├── DSP/
│   │   ├── Oversampler.h          # 4× oversampler wrapper
│   │   ├── Oversampler.cpp
│   │   ├── Distortion.h/cpp       # Tube + Mech waveshapers (polynomial tables)
│   │   ├── LadderFilter.h/cpp     # ZDF SVF low-pass
│   │   └── Compressor.h/cpp       # One-knob compressor + Phat mode
│   └── Common/
│       ├── CamelCrusherCoeffs.h   # The 101-entry polynomial coefficient tables
│       └── Presets.h/cpp          # Preset definitions
└── .github/workflows/
    └── build-and-test.yml         # CI: build + pluginval validation
```

### Signal chain (matches original):

```
Audio In
  → Distortion (Tube + Mech)  [4× oversampled, polynomial tables]
  → Low-Pass Filter (ZDF SVF)
  → Compressor (soft-knee, Phat mode)
  → Master (Volume + Mix + safety limiter)
  → Audio Out
```

### Parameter mapping:

| Parameter | ID | Type | Range | Default | Notes |
|-----------|----|------|-------|---------|-------|
| Tube | `tube` | float 0–1 | 0.0–1.0 | 0.0 | Indexes 0–100 in coefficient table |
| Mech | `mech` | float 0–1 | 0.0–1.0 | 0.0 | Indexes 0–100 in coefficient table |
| Distortion On | `tube_on` | bool | — | true | Module bypass |
| Cutoff | `cutoff` | float Hz | 20–20000 | 20000 | Log scale |
| Resonance | `res` | float 0–1 | 0.0–1.0 | 0.0 | Maps to Q |
| Filter On | `lpf_on` | bool | — | false | Module bypass (default off per CrushClone) |
| Amount | `amount` | float 0–1 | 0.0–1.0 | 0.0 | Compression intensity |
| Phat | `phat` | bool | — | false | Parallel comp + low shelf |
| Compressor On | `comp_on` | bool | — | true | Module bypass |
| Master Volume | `volume` | float dB | -24 to +12 | 0.0 | Output gain |
| Mix | `mix` | float 0–1 | 0.0–1.0 | 1.0 | Dry/wet |
| Master On | `master_on` | bool | — | true | Full bypass |

---

## 3. Development Phases

The user's workflow preference is **incremental**: implement one task, commit, wait for manual evaluation before proceeding to the next.

### Phase 0: Environment Setup
**Goal:** Get the build system working.
- Install Xcode CLI tools (if not present)
- Install CMake via Homebrew
- Create project skeleton with CMakeLists.txt that fetches JUCE
- Verify a stock JUCE audio plugin template builds (VST3 + CLAP)
- Commit: `chore: scaffold CrushJoints project with JUCE CMake build`

✅ IMPLEMENTED

**Estimated effort:** 1–2 hours (mostly build time for JUCE download)

### Phase 1: Parameter Layout & Presets
**Goal:** Establish the full APVTS parameter set and preset bank. No DSP yet — parameters exist but process block is a passthrough.
- Define 11 parameters (tube, mech, tube_on, cutoff, res, lpf_on, amount, phat, comp_on, volume, mix, master_on)
- Implement 12 presets (from research, based on CrushClone's initPresets)
- Build → verify preset switching works in EditorHost or a DAW
- Commit: `feat: add APVTS parameter layout with 12 presets`

✅ IMPLEMENTED

### Phase 1 Extension: A/B Testing
**Goal:** Add A/B testing capability with parameter state comparison
- Implement dual parameter states (A and B)
- Add "Copy A to B" and "Copy B to A" buttons
- Add toggle to switch between A/B states during real-time processing
- Commit: `feat: add A/B testing capability`

### Phase 2: Distortion Module (Tube + Mech)
**Goal:** Implement the core distortion using the extracted polynomial coefficient tables.
- Add `CamelCrusherCoeffs.h` — the 101-entry `disttube_coeffs` and `distmech_coeffs` tables (exact copy from reverse-camel)
- Implement Tube waveshaper: 3-region piecewise polynomial (`a·s² + b·s + c` above/below threshold, asymmetric)
- Implement Mech waveshaper: 3-region piecewise linear (`b·s ± c`, asymmetric)
- Add 4× oversampling to prevent aliasing
- Blend Tube + Mech per the original algorithm
- Build → test with sine sweeps to verify against reference recordings
- Commit: `feat: implement Tube + Mech distortion with extracted polynomial tables`

⏳ IN PROGRESS

### Phase 3: Low-Pass Filter
**Goal:** Implement the resonant low-pass filter.
- Use JUCE's `StateVariableTPTFilter` (ZDF SVF) in lowpass mode
- Cutoff: 20 Hz – 20 kHz, log-scaled parameter
- Resonance: 0.707 (flat) to ~4.2 (screaming), capped below self-oscillation
- Bypass when cutoff ≥ 18 kHz and resonance near zero (matches original behavior)
- Build → test with filtered white noise and synth sweeps
- Commit: `feat: add ZDF SVF low-pass filter with resonance`

### Phase 4: Compressor Module
**Goal:** Implement the one-knob compressor with Phat mode.
- Soft-knee compression curve
- Amount knob maps to threshold (0 → -30 dB) and ratio (1:1 → 8:1) simultaneously
- Adaptive attack (1–10 ms) and release (50–150 ms) based on Amount
- Auto makeup gain (up to +12 dB)
- Phat mode: parallel compression + 120 Hz low shelf (+8 dB), blend increases with Amount
- Gain reduction metering
- Build → test on drums (should add density and slight pump)
- Commit: `feat: implement one-knob compressor with Phat mode`

### Phase 5: Master Section + Safety Limiter
**Goal:** Complete the signal chain with output processing.
- Master volume: -24 to +12 dB
- Dry/wet mix
- Per-module and global on/off switches
- Hard safety limiter at ±2.0 (8x headroom) + denormal/NaN/Inf killing
- Build → verify gain staging and bypass behavior
- Commit: `feat: implement master section with safety limiter`

### Phase 6: Native GUI
**Goal:** Build a faithful native JUCE GUI matching the original layout.
- Four module sections with blue On/Off toggle switches
- Knobs with Linear/Circular modes (right-click menu)
- Patch selector dropdown + Randomize button
- Value readout display (preset name → knob value on adjustment)
- Output level meter
- MIDI Learn support
- Build → test in Reaper, Logic, FL Studio
- Commit: `feat: implement native JUCE GUI with original layout`

### Phase 7: Validation & Polish
**Goal:** Validate the plugin against standards and refine the sound.
- Run `pluginval` at strictness level 5+ (VST3)
- Run CLAP validator (if CLAP target added)
- Cross-platform build test (if Windows CI is set up)
- Compare against CrushClone and reference recordings for fidelity
- Document any deviations from the original in code comments
- Commit: `chore: add validation and polish`

---

## 4. DSP Implementation Notes

### Tube Distortion (faithful)

Use the **exact coefficient tables** from reverse-camel's `ReverseCamel.cpp`. The original uses a 101-entry table indexed by integer (0–100). In the plugin, the Tube knob (0–1 normalized) maps to table index `round(tube * 100)`, and the interpolation between entries can be linear for smooth parameter changes.

```cpp
float tubeDistort(float x, float tubeNormalized) {
    int idx = round(tubeNormalized * 100);
    idx = jlimit(0, 100, idx);
    float gain = disttube_coeffs[idx][0];  // 1.0 to ~5.0
    float a = disttube_coeffs[idx][1];
    float b = disttube_coeffs[idx][2];
    float c = disttube_coeffs[idx][3];
    float s = x * gain;
    if (s > 0.6f)      return -a*s*s + b*s - c;
    else if (s < -0.6f) return  a*s*s + b*s + c;
    else                return s;
}
```

### Mech Distortion (faithful)

Applied after Tube. Same 101-entry table lookup approach.

```cpp
float mechDistort(float y, float mechNormalized) {
    int idx = round(mechNormalized * 100);
    idx = jlimit(0, 100, idx);
    float gain = distmech_coeffs[idx][0];  // 1.0 to ~11.0
    float b = distmech_coeffs[idx][1];
    float c = distmech_coeffs[idx][2];
    float s = y * gain;
    if (s > 0.75f)      return b*s - c;
    else if (s < -0.75f) return b*s + c;
    else                return s;
}
```

### Filter

Use JUCE's `StateVariableTPTFilter<float>` in `lowpass` mode. This is a zero-delay feedback (ZDF) filter — the modern standard for analog-style low-pass. The resonance maps from 0 (0.707 Q = flat) to 1 (~4.2 Q = screaming). Cap resonance below self-oscillation (per CrushClone, cap at ~0.95 equivalent).

### Compressor

Implement a soft-knee gain computer with adaptive envelope following:

- **Envelope:** RMS-follower with attack/release times derived from Amount
- **Gain computer:** Soft-knee curve mapping Amount → threshold (-30 to 0 dB) and ratio (1:1 to 8:1)
- **Makeup gain:** `amount * 12 dB`
- **Phat mode:** Save dry copy → parallel blend with compressed signal → apply 120 Hz low shelf boost (+8 dB)

### Oversampling

Use JUCE's `dsp::Oversampler` with 4× factor, 3rd-order filter, and `autoUpDensity = true`. This prevents aliasing from the hard-clipping in Mech distortion.

---

## 5. Validation Strategy

1. **pluginval** (VST3): `--strictness-level 5` — validates parameter setup, state, audio processing, GUI
2. **CLAP Validator** (CLAP): `clap-validator validate` — same checks for CLAP format
3. **A/B comparison** with reference recordings (available from the reverse-camel analysis notebook)
4. **DAW testing:** Reaper (Windows/macOS), Logic Pro (macOS AU), Ableton Live (macOS VST3)
5. **Regression:** Sine sweep through each module to verify no crashes, denormals, or NaN

## Appendix: Additional Ideas
See IDEAS.md for potential future enhancements and features that could be added to CrushJoints.
