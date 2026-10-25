# Implementation Status

Last updated: **2026-10-25**.

This is a factual snapshot of the repository, verified by building, running
the offline tests and launching the Standalone app. It supersedes the old
`PHASE_SUMMARY.md`, which described several phases as complete when their DSP
bodies were still empty.

## 1. Verified environment

| Item | Value |
|------|-------|
| OS | macOS 26.3 (arm64) |
| Compiler | AppleClang 21.0.0 |
| CMake | 4.4.4 |
| JUCE | 9.0.4, local checkout at `/Volumes/WDB/user/flo/JUCE` |
| Build type | Release |

A clean `--clean-first` build succeeds with no warnings from project sources.
It produces:

```
build/CrushJoints_artefacts/Release/
├── AU/CrushJoints.component
├── VST3/CrushJoints.vst3
└── Standalone/CrushJoints.app

build/CrushJointsTests_artefacts/Release/CrushJointsTests
build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

## 2. Status by area

| Area | Status | Notes |
|------|--------|-------|
| CMake build | ✅ working | VST3, AU and Standalone on macOS; clean build warning-free |
| Offline DSP tests | ✅ passing | `CrushJointsTests`, 8 test groups |
| Processor tests | ✅ passing | `CrushJointsProcessorTests`, A/B, programs and state |
| Plugin entry point | ✅ working | `createPluginFilter()` present |
| Processor/buses/state | ✅ working | stereo in/out; APVTS XML plus `currentProgram` and A/B slots |
| Parameter set | ✅ working | 12 parameters, editor attachments bound |
| Distortion DSP | ✅ implemented | reverse-camel 101-entry Tube/Mech tables, Tube→Mech series, bypass |
| Filter DSP | ✅ implemented | ZDF/TPT SVF low-pass, Q 0.7071–4.2071, wide-open bypass |
| Compressor DSP | ✅ implemented | peak envelope, soft knee, auto makeup, Phat parallel path |
| Master DSP | ✅ implemented | dry/wet, volume, full bypass, ±2.0 limiter, NaN/Inf cleanup |
| Oversampling | ⚠️ not in path | deliberate: matches reverse-camel, keeps dry/wet latency-free |
| Factory presets | ✅ working | 12 patches |
| User presets | ✅ working | save, overwrite, delete in the editor; rename via host API; XML in user app data |
| Program API | ✅ working | real current program, host program names and renames |
| A/B comparison | ✅ working | processor-level snapshots; A/B and copy in the editor |
| GUI shell | ⚠️ partial | A/B and preset saving are in; value readout, meter, MIDI Learn, right-click modes, styling and resizing are missing |
| Host validation | ❌ not started | no pluginval/auval/DAW runs |

## 3. What works end to end

- The plugin loads as a VST3/AU/Standalone bundle; the Standalone was launched
  for six seconds without crashing or logging errors.
- The editor opens with four module columns, A/B controls, the preset bar,
  Save/Delete and Randomize.
- Knobs and toggles update the APVTS parameters immediately.
- Selecting a factory or user preset writes all 12 parameter values.
- `Save...` captures the current settings as a user preset in
  `~/Library/Application Support/CrushJoints/Presets.xml` on macOS. Saving a
  name that already exists overwrites that user preset. `Delete` removes the
  selected user preset after confirmation. Factory presets cannot be deleted.
- The preset combo follows host-driven program changes because the editor
  listens to processor change broadcasts.
- A/B: two full parameter snapshots. Switching slots preserves edits made to
  the slot being left, then applies the other snapshot through
  `setValueNotifyingHost`, so host automation sees the change. `A -> B` /
  `B -> A` copies the active snapshot into the inactive one.
- Plugin state is saved and restored as APVTS XML plus `currentProgram` and the
  two A/B snapshots.
- Audio is processed through the full chain:
  - **Distortion:** Tube table stage followed by Mech table stage, with the
    reference gain/polynomial formulas and linear interpolation between table
    entries. At amount 0 both stages are near-transparent; `tube_on` bypasses
    the whole stage.
  - **Filter:** 24 dB/oct state-variable low-pass. Resonance 0..1 maps to
    Q 0.7071..4.2071. The block is skipped when the filter is off, or when
    cutoff ≥ 18 kHz with resonance < 0.05.
  - **Compressor:** peak detector across channels, one-pole attack/release
    (10→1 ms and 150→50 ms with Amount), soft-knee gain computer (0→-30 dB
    threshold, 1:1→8:1 ratio), auto makeup up to +12 dB. Phat blends the dry
    signal with a compressed, 120 Hz low-shelved copy with a blend increasing
    from 0.2 to 0.65.
  - **Master:** optional dry/wet blend against a pre-effects copy, output
    volume, full bypass when `master_on` is false, hard limiting at ±2.0 and
    NaN/Inf sanitation.

## 4. Test results

`Tests/DSPTests.cpp` verifies:

1. Coefficient table anchor values (Tube gain 1→4.99996, Mech gain 1→11.0001).
2. Distortion output against an independent reference implementation for
   several input levels.
3. The complete Distortion → Master path against a literal transcription of
   reverse-camel's `run()` loop at several Tube/Mech/mix/volume settings.
4. Distortion transparency at amount 0 and bypass at `tube_on = false`.
5. 10 kHz attenuation through a 1 kHz low-pass, plus filter bypass.
6. Compressor gain reduction, finite output and bypass.
7. Master volume boost, ±2.0 limiting, full bypass and dry/wet mix.
8. The 12-preset bank, including the corrected Telephone flags.
9. Custom preset save/overwrite/rename/delete and XML round-trip through a
   temporary file.

`Tests/ProcessorTests.cpp` verifies:

1. A/B switching captures edits before leaving a slot.
2. Copying the active slot into the inactive slot.
3. `setCurrentProgram`/`getCurrentProgram` and `getProgramName`.
4. Plugin state round-trip including `currentProgram`, the active A/B slot and
   both snapshots.
5. Custom preset save/delete through the processor and factory/user program metadata.

Result:

```
$ ./build/CrushJointsTests_artefacts/Release/CrushJointsTests
All DSP tests passed.
$ ./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
All processor tests passed.
```

## 5. Known gaps and deviations

Ordered roughly by impact.

1. **No host validation yet.** `pluginval`, `auval` and DAW testing have not
   been run. Offline tests and a Standalone launch are the only verification.
2. **No oversampling.** This matches reverse-camel and keeps the dry/wet path
   latency-free, but high Mech settings can alias like the original. The
   `Oversampler` wrapper exists for a future optional quality mode.
3. **GUI feature gaps.** No value readout, output level meter, MIDI Learn,
   Linear/Circular right-click menu, custom skin or resizable layout.
4. **Randomize is naive.** It assigns uniform random normalised values to every
   parameter, including on/off switches, so it can produce silence.
5. **Master volume range differs from the reference.** The plugin uses
   -24..+12 dB (CrushClone's range, required by presets such as MAS Loudener at
   +2 dB) instead of the original -51..0 dB.
6. **Filter and compressor are reference-inspired, not measured clones.** The
   SVF topology, Q range, compressor curve and Phat shelf come from CrushClone
   rather than black-box measurement of the original.
7. **Format coverage is narrower than old docs claimed.** Only VST3, AU and
   Standalone are built; there is no CLAP, LV2 or AAX target.
8. **No CI and no LICENSE file.** The README claims GPLv3, but the repository
   does not yet contain a `LICENSE` file.
9. **User presets are a single XML bank.** There is no import/export, category
   or sorting UI. Corrupt or unreadable files are ignored gracefully, leaving
   the factory bank available.

## 6. Validation checklist

| Check | Status |
|-------|--------|
| Clean CMake configure/build | ✅ passed on macOS arm64, no source warnings |
| Offline DSP/reference tests | ✅ passed |
| Processor A/B/program/state tests | ✅ passed |
| Standalone launch smoke test | ✅ passed (6 s, no crash) |
| `pluginval --strictness-level 5` on VST3 | ⬜ not run |
| `auval` on the AU component | ⬜ not run |
| State save/load round-trip in a host | ✅ offline, ⬜ not in a host |
| Automation of all 12 parameters | ⬜ not run |
| Preset switching in a host (factory + user) | ⬜ not run |
| A/B switching under automation | ⬜ not run in a host |
| Stereo channel balance / bypass behaviour | ⬜ not run in a host |
| Cross-platform build (Windows/Linux) | ⬜ not configured |
| CPU/latency/buffer-size testing | ⬜ not run |

Suggested commands once validation starts (macOS):

```sh
# Install the AU and VST3 for host/validator visibility
cp -R build/CrushJoints_artefacts/Release/AU/CrushJoints.component \
      ~/Library/Audio/Plug-Ins/Components/
cp -R build/CrushJoints_artefacts/Release/VST3/CrushJoints.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/

# Apple AU validation
auval -v aufx CJTS Crsh

# Steinberg/JUCE plugin validation (if pluginval is installed)
pluginval --strictness-level 5 --validate-in-process \
          ~/Library/Audio/Plug-Ins/VST3/CrushJoints.vst3
```

## 7. Reproducing the build and tests

```sh
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
cmake --build build --config Release -j8
./build/CrushJointsTests_artefacts/Release/CrushJointsTests
./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

The repository no longer tracks `build/`, `.idea/` or `.DS_Store`; they are
covered by `.gitignore`.
