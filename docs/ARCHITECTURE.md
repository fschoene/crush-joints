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
│   ├── MEASUREMENTS.md
│   ├── ARCHITECTURE.md
│   ├── STATUS.md
│   ├── ROADMAP.md
│   ├── VERSION_1.md
│   └── VERSION_2.md
└── Source/
    ├── PluginProcessor.h/.cpp      # APVTS, presets, signal chain, plugin entry
    ├── PluginEditor.h/.cpp         # native JUCE editor
    ├── GUI/
    │   ├── CrushJointsLookAndFeel.h/.cpp  # dark blue / yellow theme
    │   └── LockButton.h/.cpp              # padlock button for randomize locks
    ├── DSP/
    │   ├── Distortion.h/.cpp       # Tube + Mech waveshapers and coefficient tables
    │   ├── LadderFilter.h/.cpp     # SVF low-pass wrapper
    │   ├── Compressor.h/.cpp       # one-knob compressor + Phat
    │   ├── Oversampler.h/.cpp      # 4× oversampling wrapper (not in the current path)
    │   └── Master.h/.cpp           # volume, mix, limiter
    └── Common/
        ├── CamelCrusherCoeffs.h/.cpp  # reverse-camel coefficient tables
        └── Presets.h/.cpp             # CrushClone + legacy + user preset bank
```

Black-box analysis of the original plugin lives in `tools/analysis/` (offline
x86_64 VST2 host and Python scripts; see its README). It is not part of the
plugin build.

Tests live in `Tests/`:

```
Tests/
├── DSPTests.cpp                     # offline DSP/reference tests
└── ProcessorTests.cpp               # A/B, programs and state tests
```

There is currently no CI workflow and no plugin-host project in the
repository.

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

build/CrushJointsTests_artefacts/Release/CrushJointsTests
build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

## 4. Signal chain

The processor supports these main bus layouts:

| Input | Output | Behaviour |
|-------|--------|-----------|
| Stereo | Stereo | Primary path, unchanged |
| Mono | Mono | Single-channel processing |
| Mono | Stereo | Mono input duplicated before the dry capture and DSP chain; centred stereo output |

Stereo input with mono output is rejected.

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
3. If the Master is off or the mix is below 100%, the pre-effects signal is
   copied into a pre-allocated dry buffer.
4. `distortion.processBlock`, `filter.processBlock`,
   `compressor.processBlock` and `master.processBlock` are called in order.
   Master receives the dry buffer so it can perform the dry/wet blend or a full
   bypass.

Distortion always runs the Mech stage followed by the Tube stage (the order
measured on the original; see [MEASUREMENTS.md](MEASUREMENTS.md)).
The filter bypasses itself when it is off, wide open and non-resonant. The
compressor bypasses when it is off, or when Amount is near zero and Phat is
off. Master applies the
mix, volume, a hard clip at ±0.9999 (measured on the original) and NaN/Inf
sanitation.

There is currently **no oversampling** in the signal path, matching the
reverse-camel reference and keeping the dry/wet path free of latency. The
`Oversampler` wrapper remains available for an optional future quality mode.

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

- The original Master volume is −∞…0 dB
  ([MEASUREMENTS.md](MEASUREMENTS.md)); the implementation uses -24–+12 dB to match CrushClone's preset bank (some
  presets use positive gain). This is an intentional deviation, documented in
  [STATUS.md](STATUS.md).
- The cutoff parameter is not skewed in the APVTS; the editor applies a
  `setSkewFactorFromMidPoint(1000.0)` to the knob so the low end is usable.
- A/B comparison is implemented at the processor level as two snapshots of
  all 12 normalised parameter values. Switching slots captures the current
  parameters, then applies the other snapshot through
  `setValueNotifyingHost`, so host automation sees the change. The DSP classes
  do not keep their own A/B state.

## 6. Presets, programs and state

### Factory and user presets

- `Common/Presets.cpp` defines 32 factory patches: the Default preset, 11
  CrushClone presets and 20 original Legacy/Original presets.
- User presets are stored as XML in
  `~/Library/Application Support/CrushJoints/Presets.xml` on macOS
  (`userApplicationDataDirectory` is used so the same code works on other
  platforms).
- Program indices are: Default `0`, CrushClone `1–11`, Legacy/Original
  `12–31`, then user presets from `32` in insertion order. `getNumPrograms()`
  returns the combined count. The editor places Default at the top outside the
  groups, then displays Legacy/Original, CrushClone and User, mapping selected
  combo IDs back to these program indices.
- `saveUserPreset(name)` captures the current 12 parameter values and appends
  or overwrites a user preset with the same name (case-insensitive). It
  returns the combined program index or -1. The editor exposes save and
  delete; renaming is available through the host program API.
- `deleteUserPreset(index)` only accepts user presets. Deleting the active
  program falls back to the previous program and loads it.
- `renameUserPreset` is used by the host-facing `changeProgramName`; factory
  presets cannot be renamed or deleted.
- The editor places **Default** at the top, then uses grouped preset-selector
  headings: **Legacy / Original**, **CrushClone** and **User**. The User group
  shows a "none saved yet" heading until a user preset exists. Saving and
  deleting live in the preset bar.

### Program API

- `getCurrentProgram()` returns the real current index.
- `setCurrentProgram(index)` applies the preset, updates the active A/B slot,
  calls `updateHostDisplay()` and broadcasts a change so the editor follows
  host-driven program changes.
- `getProgramName(index)` returns the raw preset name.
- `changeProgramName(index, newName)` renames user presets; factory presets are
  ignored.

### State

`getStateInformation`/`setStateInformation` serialise the APVTS parameter
tree plus:

- `currentProgram`,
- an `AB` child tree containing both 12-value snapshots and the active slot,
- a `LOCKS` child tree containing the seven randomize lock flags.

On restore, the stored parameter values are authoritative; the program index
is used for display and clamped if the user preset bank changed. Legacy state
without an `AB` child initialises both slots from the restored parameters;
missing lock flags default to unlocked.

## 7. Editor

The editor is a native JUCE component with an 860×440 design resolution:

- Header: "CRUSHJOINTS" title, A/B buttons, copy button, patch label, preset
  combo, Save/Delete and Randomize.
- Four equal module columns: Distortion, Filter, Compressor, Master.
- Every knob has a visible uppercase label and a small padlock button.
- Module ON switches (and PHAT) sit at the bottom of each module.
- Switching a module off dims its panel, labels, knobs and locks; the switch
  itself stays bright so it remains easy to find. Turning Master off dims every
  module container to show that the whole plugin is bypassed.
- Knobs use `RotaryHorizontalVerticalDrag` with a text box below.
- Double-clicking a knob resets it to the parameter's default (Tube/Mech/Amount
  = 0, Cutoff = 20 kHz, Res = 0, Volume = 0 dB, Mix = 100%).
- On/off and Phat controls are `ToggleButton`s.
- The area below the modules contains a readout showing the current preset;
  while a knob is dragged it shows the parameter name and value.
- A custom `CrushJointsLookAndFeel` uses CamelCrusher-inspired dark blue and
  yellow, with high-contrast arcs and switch indicators.
- All controls use `AudioProcessorValueTreeState::SliderAttachment` and
  `ButtonAttachment`, so parameter changes are reflected immediately and are
  undo/host-automation safe.
- `Randomize` only changes continuous parameters. It skips module switches and
  respects the per-parameter locks.
- The A/B buttons switch snapshots; the copy button copies the active slot
  into the other one.
- `Save...` opens a name dialog and writes a user preset. `Delete` is enabled
  only for user presets and asks for confirmation.
- The editor listens to processor change broadcasts, so host-driven program
  changes and newly saved/deleted presets refresh the combo automatically.
- The window is resizable from 80% to 150% with a fixed 860:440 aspect ratio.
  All controls live in a design-sized content component that is transformed as
  a unit, so fonts, knobs and padding scale together.

Implemented versus reference GUI:

| Feature | Status |
|---------|--------|
| Four module groups | implemented |
| Knobs, labels and on/off toggles | implemented |
| Preset dropdown | implemented with Legacy/Original, CrushClone and User groups and host sync |
| Save/Delete user presets | implemented |
| A/B comparison + copy | implemented |
| Per-parameter randomize locks | implemented and persisted |
| Double-click reset to default | implemented |
| Value readout | implemented (preset name / dragged knob value) |
| Dark blue + yellow theme | implemented |
| Resizable/scalable layout | implemented (80–150%, fixed aspect) |
| Dimmed modules when bypassed | implemented (Master off dims all modules) |
| Output level meter | missing |
| MIDI Learn / right-click knob mode menu | missing |
| Keyboard navigation / accessibility labels | missing |
## 8. DSP module structure

Each DSP class follows the same pattern:

- `prepareToPlay(sampleRate, samplesPerBlock, numChannels)` configures JUCE DSP
  objects and resets state.
- `processBlock(AudioBuffer<float>&)` is the audio hook.
- Setters clamp/store parameters.

`Distortion` looks up the reference coefficient tables with linear
interpolation between neighbouring entries, so automated changes are smooth
while integer knob positions match the reference exactly. `Common/CamelCrusherCoeffs.h`
declares the shared `CrushJoints::disttube_coeffs`/`distmech_coeffs` arrays and
`Common/CamelCrusherCoeffs.cpp` defines them from the reverse-camel data.

### Tests

`Tests/DSPTests.cpp` is an offline JUCE console app (`CrushJointsTests`) that
verifies:

- coefficient table dimensions and anchor values,
- the Tube/Mech waveshapers against an independent transcription of the
  reverse-camel formulas,
- the complete Distortion → Master path against a reference chain built from
  the reverse-camel tables (Mech → Tube, ±0.9999 clip),
- the Distortion stage against values recorded from the original plugin,
- distortion transparency at amount 0 and bypass at `tube_on = false`,
- low-pass attenuation and filter bypass,
- compressor gain reduction, finite output and bypass,
- compressor level behaviour (transparent at Amount 0, makeup follows the
  curve, progressive squash) and Phat (bass tilt, no thinning, click-free
  automation),
- master volume, limiter, bypass and dry/wet mix,
- the preset bank, including the Telephone flags,
- the 32-preset factory bank, Default placement and category assignment,
- original-preset conversion, including the factory-FXB mix values,
- custom preset save/overwrite/rename/delete and XML round-trip through a
  temporary file.

`Tests/ProcessorTests.cpp` (`CrushJointsProcessorTests`) verifies:

- A/B switching, edit capture, slot copying and restoration,
- mono/mono, mono/stereo and stereo/stereo bus layouts,
- program changes and the real `getCurrentProgram()` value, including legacy
  presets and category counts,
- plugin state round-trip including `currentProgram`, both A/B slots and the
  lock flags,
- user preset save/delete through the processor using a temporary bank file,
- locked parameters staying unchanged during Randomize and module switches
  never being randomised,
- filter resonance boosting a tone at the cutoff and compressor Amount/Phat
  changing the rendered output.

`CRUSHJOINTS_RENDER_UI=1` also renders 80%, 100% and 150% editor snapshots to
`/tmp/crush-editor-*.png` for visual inspection.

Run both after building:

```sh
./build/CrushJointsTests_artefacts/Release/CrushJointsTests
./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

The test targets can be disabled with `-DCRUSHJOINTS_BUILD_TESTS=OFF`.

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
