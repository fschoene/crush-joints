# Architecture

How CrushJoints is built today. Measured behaviour of the original is in
[MEASUREMENTS.md](MEASUREMENTS.md); current state, gaps and validation in
[STATUS.md](STATUS.md); planned work in [ROADMAP.md](ROADMAP.md).

## 1. Framework choice

JUCE (C++17) was chosen over NIH-plug, iPlug2 and DPF because:

- it provides the DSP building blocks the project needs (`IIR`,
  `StateVariableTPTFilter`, `Oversampling`, `SmoothedValue`, `AudioBuffer`);
- it covers VST3, AU, AAX and Standalone from one codebase with mature CMake
  support (`juce_add_plugin`);
- the GUI is native, with no web runtime;
- `pluginval` and the Steinberg validator work with it out of the box.

Trade-off: JUCE is GPLv3 or commercially licensed; this project is GPLv3. The
distortion coefficient tables come from reverse-camel, and the 11 additional
presets come from CrushClone; both are credited in
[RESEARCH.md](RESEARCH.md).

## 2. Repository layout

```
crush-joints/
├── CMakeLists.txt                  # JUCE CMake build
├── LICENSE                         # GPLv3
├── README.md                       # quick start
├── docs/
│   ├── RESEARCH.md                 # background, manual and sources
│   ├── MEASUREMENTS.md             # measured behaviour of the original (ground truth)
│   ├── ARCHITECTURE.md             # this file
│   ├── STATUS.md                   # current state, tests, validation, gaps
│   └── ROADMAP.md                  # versions, milestones and backlog
├── tools/analysis/                 # black-box harness for the original plugin
├── Tests/
│   ├── DSPTests.cpp                # offline DSP tests
│   └── ProcessorTests.cpp          # processor, presets and state tests
└── Source/
    ├── PluginProcessor.h/.cpp      # APVTS, presets, signal chain, plugin entry
    ├── PluginEditor.h/.cpp         # native JUCE editor
    ├── GUI/
    │   ├── CrushJointsLookAndFeel.h/.cpp  # dark blue / yellow theme
    │   └── LockButton.h/.cpp              # padlock button for randomize locks
    ├── DSP/
    │   ├── Distortion.h/.cpp       # Tube + Mech waveshapers
    │   ├── LadderFilter.h/.cpp     # 2-pole resonant low-pass (historical name)
    │   ├── Compressor.h/.cpp       # measured one-knob compressor + Phat (+ hidden Boom)
    │   ├── Oversampler.h/.cpp      # 4x wrapper, not in the current path
    │   └── Master.h/.cpp           # mix, volume, hard clip
    └── Common/
        ├── CamelCrusherCoeffs.h/.cpp  # 101-entry distortion tables (reverse-camel)
        ├── CutoffCurve.h/.cpp         # original cutoff knob curve
        └── Presets.h/.cpp             # factory, CrushClone and user presets
```

The harness in `tools/analysis/` is not part of the plugin build. The original
plugin is not in the repository.

## 3. Build system

- `cmake_minimum_required(VERSION 3.22)`; C++17.
- JUCE is not fetched automatically. `JUCE_ROOT` must point at a JUCE checkout
  (tested with JUCE 9.0.4).
- `juce_add_plugin(CrushJoints …)` produces **VST3**, **AU** and **Standalone**
  on macOS. There is no CLAP, LV2 or AAX target.
- Plugin metadata: code `CJTS`, manufacturer `Crsh`, VST3 category `Fx` /
  subcategory `Filter`.
- Compile definitions: `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`,
  `JUCE_STRICT_MODE=1`, `JUCE_VST3_CAN_REPLACE_VST2=0`.
- Recommended JUCE config, LTO and warning flags are enabled.

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

The test targets are disabled with `-DCRUSHJOINTS_BUILD_TESTS=OFF`. The offline
renderer used by `tools/analysis/compare_clone.py` and
`tools/analysis/benchmark_cpu.py` is built only with
`-DCRUSHJOINTS_BUILD_TOOLS=ON`.

## 4. Signal chain

Supported main bus layouts:

| Input | Output | Behaviour |
|-------|--------|-----------|
| Stereo | Stereo | Primary path |
| Mono | Mono | Single-channel processing |
| Mono | Stereo | Mono input duplicated before the dry capture and DSP chain; centred output |

Stereo input with mono output is rejected.

```
Audio In
  → Distortion: Mech stage
  → LadderFilter (2-pole low-pass)
  → Distortion: Tube stage
  → Compressor (+ Phat)
  → Master (mix + volume + hard clip)
  → Audio Out
```

`CrushJointsAudioProcessor::processBlock`:

1. Activates `ScopedNoDenormals`.
2. Reads the APVTS parameters and pushes them into the DSP objects.
3. If Master is off or Mix is below 100 %, copies the pre-effects signal into a
   pre-allocated dry buffer.
4. Calls `distortion.processMech`, `filter.processBlock`,
   `distortion.processTube`, `compressor.processBlock` and
   `master.processBlock`, in that order. Master receives the dry buffer for the
   blend or a full bypass.

The order and the module behaviour are measured; see
[MEASUREMENTS.md](MEASUREMENTS.md) §7. `tube_on` bypasses both distortion
stages; the filter and compressor have their own switches. There is no
oversampling in the path (matching the original); the `Oversampler` wrapper is
unused.

## 5. Parameters

Twelve APVTS parameters, created in the processor constructor and bound by ID in
the editor.

| Parameter | ID | Type | Range | Default |
|-----------|----|------|-------|---------|
| Tube | `tube` | float | 0.0–1.0 | 0.0 |
| Mech | `mech` | float | 0.0–1.0 | 0.0 |
| Distortion on | `tube_on` | bool | — | true |
| Cutoff | `cutoff` | float Hz, original knob curve | 80–16000 | 16000 |
| Resonance | `res` | float | 0.0–1.0 | 0.0 |
| Filter on | `lpf_on` | bool | — | false |
| Amount | `amount` | float | 0.0–1.0 | 0.0 |
| Phat | `phat` | bool | — | false |
| Compressor on | `comp_on` | bool | — | true |
| Volume | `volume` | float dB | −24–+12 | 0.0 |
| Mix | `mix` | float | 0.0–1.0 | 1.0 |
| Master on | `master_on` | bool | — | true |

Notes:

- The original volume is −∞…0 dB with `gain = knob^2.737`. The −24…+12 dB range
  is a deliberate deviation, kept for the CrushClone presets that use positive
  gain; the gain law and its 10 ms knob-domain ramp are the measured ones. See
  [STATUS.md](STATUS.md).
- `cutoff` stores Hz, but its `NormalisableRange` uses the measured knob curve
  (`CutoffCurve`), so the knob, host automation and preset conversion share one
  law.
- A/B comparison is a processor-level pair of snapshots of all 12 normalised
  values. Switching captures the current parameters and applies the other
  snapshot through `setValueNotifyingHost`, so the host sees the change. DSP
  classes hold no A/B state.

## 6. Presets, programs and state

- `Common/Presets.cpp` defines 32 factory patches: Default, 11 CrushClone and
  20 Legacy/Original.
- User presets are XML in
  `~/Library/Application Support/CrushJoints/Presets.xml` on macOS
  (`userApplicationDataDirectory`, so the same code works elsewhere).
- Program indices: Default `0`, CrushClone `1–11`, Legacy/Original `12–31`,
  user presets from `32` in insertion order. `getNumPrograms()` returns the
  combined count. The editor puts Default at the top outside the groups, then
  displays Legacy/Original, CrushClone and User, mapping combo IDs back to
  program indices.
- `saveUserPreset(name)` captures the 12 values and appends or overwrites a user
  preset with the same name (case-insensitive); it returns the combined index or
  −1. Renaming is available through the host API.
- `deleteUserPreset(index)` only accepts user presets; deleting the active
  program falls back to the previous one.
- Factory presets cannot be renamed or deleted.

### Program API

- `getCurrentProgram()` returns the real index.
- `setCurrentProgram(index)` applies the preset, updates the active A/B slot,
  calls `updateHostDisplay()` and broadcasts a change so the editor follows
  host-driven changes.
- `getProgramName(index)` returns the raw preset name.
- `changeProgramName(index, newName)` renames user presets only.

### State

`getStateInformation`/`setStateInformation` serialise the APVTS tree plus:

- `currentProgram`,
- an `AB` child tree with both 12-value snapshots and the active slot,
- a `LOCKS` child tree with the seven randomize lock flags.

On restore the stored parameter values are authoritative; the program index is
for display and is clamped if the user bank changed. Legacy state without an
`AB` child initialises both slots from the restored parameters; missing lock
flags default to unlocked.

## 7. Editor

Native JUCE component with an 860×440 design resolution:

- Header: title, A/B buttons, copy, patch label, preset combo, Save/Delete and
  Randomize.
- Four equal module columns: Distortion, Filter, Compressor, Master.
- Every knob has an uppercase label and a padlock button; module ON switches
  (and PHAT) sit at the bottom of each module.
- Switching a module off dims its panel, labels, knobs and locks; the switch
  stays bright. Turning Master off dims all four modules.
- Knobs use `RotaryHorizontalVerticalDrag` with a text box. Double-click resets
  to the parameter default (Tube/Mech/Amount 0, Cutoff 16 kHz, Res 0, Volume
  0 dB, Mix 100 %).
- On/off and Phat are `ToggleButton`s.
- The readout shows the preset name, or the parameter name and value while a
  knob is dragged.
- `CrushJointsLookAndFeel` uses a CamelCrusher-inspired dark blue/yellow theme
  with high-contrast arcs and switch indicators.
- All controls use `SliderAttachment`/`ButtonAttachment`, so changes are
  reflected immediately and are host-automation safe.
- Randomize changes only continuous parameters, skips switches and respects
  locks.
- The editor listens to processor broadcasts, so host program changes and
  saved/deleted presets refresh the combo.
- The window scales from 80 % to 150 % with a fixed 860:440 aspect ratio; a
  design-sized content component is transformed as a unit.

Missing GUI features and planned upgrades are tracked in
[STATUS.md](STATUS.md) and [ROADMAP.md](ROADMAP.md).

## 8. DSP modules

Each DSP class follows the same pattern:

- `prepareToPlay(sampleRate, samplesPerBlock, numChannels)` configures state;
- `processBlock(AudioBuffer<float>&)` is the audio hook;
- setters clamp and store parameters.

`Distortion` looks up the 101-entry tables with linear interpolation, so
automation is smooth while integer knob positions match the measured curves.
`Compressor` contains the measured model plus the hidden "Boom" stage (see
[ROADMAP.md](ROADMAP.md)). `Master` applies the dry/wet blend, the measured
volume law with 10 ms knob-domain ramps, the ±0.9999 clip and NaN/Inf
sanitation.

### Tests

`Tests/DSPTests.cpp` (`CrushJointsTests`) covers the coefficient anchors, the
waveshapers against an independent reference and against recorded original
output, the cutoff curve, the filter against the analytic biquad, the compressor
against recorded original behaviour (static curve, dynamics, stereo link,
bypass, switch-on, Boom), master clipping/bypass/mix/smoothing and the preset
bank including the legacy conversion.

`Tests/ProcessorTests.cpp` (`CrushJointsProcessorTests`) covers A/B switching
and copying, bus layouts, programs, state round-trip, user presets, randomize
locks and the signal-chain order.

```sh
./build/CrushJointsTests_artefacts/Release/CrushJointsTests
./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

`CRUSHJOINTS_RENDER_UI=1` renders 80 %, 100 % and 150 % editor snapshots to
`/tmp/crush-editor-*.png`. Detailed results and known gaps are in
[STATUS.md](STATUS.md).

## 9. Plugin entry point

`Source/PluginProcessor.cpp` defines:

```cpp
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrushJointsAudioProcessor();
}
```
