# Architecture

How CrushJoints is put together **today**. Intended-but-missing behaviour is
called out explicitly; see [STATUS.md](STATUS.md) and
[ROADMAP.md](ROADMAP.md).

## 1. Framework choice

JUCE (C++17) was chosen over NIH-plug, iPlug2 and DPF because:

- JUCE provides the DSP building blocks the project needs
  (`StateVariableTPTFilter`, `IIR`, `Oversampling`, `Compressor`, `Gain`,
  `ProcessorChain`).
- It covers VST3, AU, AAX and Standalone from one codebase, with mature CMake
  support (`juce_add_plugin`).
- Native GUI, no web runtime.
- `pluginval` and the Steinberg validator work with JUCE out of the box.
- The main reference projects (CrushClone, reverse-camel) are close enough to
  follow.

Trade-off: JUCE is GPLv3 or commercially licensed; this project is GPLv3. The
reference tables and behaviour come from reverse-camel, while the preset bank
and parameter ranges come from CrushClone.

## 2. Repository layout

```
crush-joints/
├── CMakeLists.txt                  # JUCE CMake build
├── README.md                       # quick start
├── docs/                           # this documentation set
│   ├── README.md
│   ├── RESEARCH.md
│   ├── ARCHITECTURE.md
│   ├── STATUS.md
│   └── ROADMAP.md
└── Source/
    ├── PluginProcessor.h/.cpp      # APVTS, presets, signal chain, plugin entry
    ├── PluginEditor.h/.cpp         # native JUCE editor
    ├── DSP/
    │   ├── Distortion.h/.cpp       # Tube + Mech waveshapers and coefficient tables
    │   ├── LadderFilter.h/.cpp     # SVF low-pass wrapper
    │   ├── Compressor.h/.cpp       # one-knob compressor + Phat
    │   ├── Oversampler.h/.cpp      # 4× oversampling wrapper
    │   └── Master.h/.cpp           # volume, mix, limiter
    └── Common/
        ├── CamelCrusherCoeffs.h/.cpp  # shared coefficient declarations (placeholder)
        └── Presets.h/.cpp             # 12 built-in presets
```

There is currently no CI workflow, no test target and no plugin host project in
the repository.

## 3. Build system

- `cmake_minimum_required(VERSION 3.22)`; the project is C++17.
- JUCE is **not** fetched automatically. `JUCE_ROOT` is a cache variable that
  must point at a JUCE checkout (default:
  `/Volumes/WDB/user/flo/JUCE`, tested with JUCE 9.0.4).
- `juce_add_plugin(CrushJoints …)` produces **VST3**, **AU** and **Standalone**
  on macOS. There is no CLAP, LV2 or AAX target.
- Plugin metadata: code `CJTS`, manufacturer `Crsh`, VST3 category `Fx` /
  subcategory `Filter`.
- Compile definitions:
  - `JUCE_WEB_BROWSER=0`
  - `JUCE_USE_CURL=0`
  - `JUCE_STRICT_MODE=1`
  - `JUCE_VST3_CAN_REPLACE_VST2=0` (required for the VST3 build because this
    plugin has no released VST2 version whose parameter IDs must be preserved)
- Linked modules: `juce_audio_basics`, `juce_audio_devices`,
  `juce_audio_formats`, `juce_audio_processors`, `juce_audio_utils`,
  `juce_core`, `juce_dsp`, `juce_gui_basics`, `juce_gui_extra`.
- Recommended JUCE config, LTO and warning flags are enabled.

Build commands:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
cmake --build build --config Release -j8
```

Artefacts:

```
build/CrushJoints_artefacts/Release/
├── AU/CrushJoints.component
├── VST3/CrushJoints.vst3
└── Standalone/CrushJoints.app
```

## 4. Signal chain

The processor advertises one stereo input and one stereo output and rejects
other layouts.

```
Audio In
  → Distortion (Tube + Mech)
  → LadderFilter (SVF low-pass)
  → Compressor (+ Phat)
  → Master (volume + mix + limiter)
  → Audio Out
```

`CrushJointsAudioProcessor::processBlock` performs these steps on every block:

1. `ScopedNoDenormals` is active for the block.
2. Parameter values are read with `AudioParameterFloat/Bool::get()` and pushed
   into the corresponding DSP object via its setters.
3. `distortion.processBlock`, `filter.processBlock`,
   `compressor.processBlock` and `master.processBlock` are called in order.

**Important:** all four `processBlock` implementations are currently empty or
commented-out stubs, so the audio is passed through unmodified. The
`Distortion::tubeDistort`/`mechDistort` helpers exist but are not called.
`Oversampler` is prepared but never invoked. See [STATUS.md](STATUS.md).

## 5. Parameters

Twelve APVTS parameters are created in the processor constructor. The editor
binds to them by ID.

| Parameter | ID | Type | Range | Default |
|-----------|----|------|-------|---------|
| Tube | `tube` | float | 0.0–1.0 | 0.0 |
| Mech | `mech` | float | 0.0–1.0 | 0.0 |
| Distortion on | `tube_on` | bool | — | true |
| Cutoff | `cutoff` | float Hz | 20–20000 | 20000 |
| Resonance | `res` | float | 0.0–1.0 | 0.0 |
| Filter on | `lpf_on` | bool | — | false |
| Amount | `amount` | float | 0.0–1.0 | 0.0 |
| Phat | `phat` | bool | — | false |
| Compressor on | `comp_on` | bool | — | true |
| Volume | `volume` | float dB | -24–+12 | 0.0 |
| Mix | `mix` | float | 0.0–1.0 | 1.0 |
| Master on | `master_on` | bool | — | true |

Notes:

- The research reference describes the original Master volume as -51–0 dB;
  the implementation uses -24–+12 dB. This is a deliberate/unresolved
  discrepancy and is listed in [STATUS.md](STATUS.md).
- The cutoff parameter is not skewed in the APVTS; the editor applies a
  `setSkewFactorFromMidPoint(1000.0)` to the knob so the low end is usable.
- A/B state storage exists inside the DSP classes (`copyAtoB`, `copyBtoA`,
  `toggleAB`) but is not connected to the APVTS, the editor or the processor.

## 6. Presets and program handling

- `Common/Presets.cpp` defines 12 patches. `Presets::getPreset(index)` returns
  a copy; `getNumPresets()` is 12.
- `setCurrentProgram(index)` writes each value through the
  `AudioParameterFloat/Bool` assignment operators, which handles
  normalisation.
- `getCurrentProgram()` always returns `0`; `changeProgramName()` is a no-op.
  The editor's preset combo is initialised once and does not follow
  host-driven program changes.

## 7. Editor

The editor is a native JUCE component, currently 720×400:

- Header: "CrushJoints" title, patch label, preset combo, randomize button.
- Four equal module columns: Distortion, Filter, Compressor, Master.
- Knobs use `RotaryHorizontalVerticalDrag` with a text box below.
- On/off and Phat controls are `ToggleButton`s.
- All controls use `AudioProcessorValueTreeState::SliderAttachment` and
  `ButtonAttachment`, so parameter changes are reflected immediately and are
  undo/host-automation safe.
- `Randomize` sets every parameter to a uniform random normalised value.

Implemented versus reference GUI:

| Feature | Status |
|---------|--------|
| Four module groups | implemented |
| Knobs and on/off toggles | implemented |
| Preset dropdown | implemented (one-way, no host sync) |
| Randomize | implemented, but not "intelligent" and can turn modules off |
| Value readout | missing |
| Output level meter | missing |
| MIDI Learn / right-click knob menu | missing |
| Custom CamelCrusher styling | missing (default JUCE look) |
| A/B testing UI | missing |
| Resizable/scalable layout | missing (fixed size) |

## 8. DSP module structure

Each DSP class follows the same pattern:

- `prepareToPlay(sampleRate)` (plus block size for `Oversampler`) configures
  JUCE DSP objects.
- `processBlock(AudioBuffer<float>&)` is the audio hook.
- Setters clamp/store parameters.
- A/B helpers copy/toggle the stored parameter values.

`Distortion` additionally owns the coefficient tables (see
[RESEARCH.md](RESEARCH.md) and the discrepancy note in
[STATUS.md](STATUS.md)). `Common/CamelCrusherCoeffs.h` declares shared
`CrushJoints::disttube_coeffs`/`distmech_coeffs` arrays, but
`CamelCrusherCoeffs.cpp` is a placeholder with no definitions; the actual data
lives in `Distortion.cpp` as static class members. Nothing links against the
unused shared declarations today.

## 9. Plugin entry point

`Source/PluginProcessor.cpp` defines:

```cpp
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrushJointsAudioProcessor();
}
```

This is required by every JUCE wrapper and was missing in the original
scaffold, which caused the AU and Standalone links to fail.
