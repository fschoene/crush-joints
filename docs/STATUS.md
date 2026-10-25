# Implementation Status

Last updated: **2026-10-25**.

This is a factual snapshot of the repository, verified by building, running
the offline DSP tests and launching the Standalone app. It supersedes the old
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
```

## 2. Status by area

| Area | Status | Notes |
|------|--------|-------|
| CMake build | ✅ working | VST3, AU and Standalone on macOS; clean build warning-free |
| Offline DSP tests | ✅ passing | `CrushJointsTests`, 8 test groups |
| Plugin entry point | ✅ working | `createPluginFilter()` present |
| Processor/buses/state | ✅ working | stereo in/out; APVTS XML state save/load |
| Parameter set | ✅ working | 12 parameters, editor attachments bound |
| Distortion DSP | ✅ implemented | reverse-camel 101-entry Tube/Mech tables, Tube→Mech series, bypass |
| Filter DSP | ✅ implemented | ZDF/TPT SVF low-pass, Q 0.7071–4.2071, wide-open bypass |
| Compressor DSP | ✅ implemented | peak envelope, soft knee, auto makeup, Phat parallel path |
| Master DSP | ✅ implemented | dry/wet, volume, full bypass, ±2.0 limiter, NaN/Inf cleanup |
| Oversampling | ⚠️ not in path | deliberate: matches reverse-camel, keeps dry/wet latency-free |
| Preset bank | ✅ data, ⚠️ program API partial | 12 presets; Telephone flags corrected; `getCurrentProgram()` still returns 0 |
| GUI shell | ⚠️ partial | controls work; value readout, meter, MIDI Learn, right-click modes, styling and A/B UI are missing |
| A/B testing | ❌ not wired | DSP classes have state helpers; no UI or APVTS integration |
| Host validation | ❌ not started | no pluginval/auval/DAW runs |

## 3. What works end to end

- The plugin loads as a VST3/AU/Standalone bundle; the Standalone was launched
  for six seconds without crashing or logging errors.
- The editor opens, displays four module columns and the preset/randomize bar.
- Knobs and toggles update the APVTS parameters immediately.
- Selecting a preset writes all 12 parameter values.
- Plugin state is saved and restored as APVTS XML.
- Audio is now processed through the full chain:
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

Result:

```
$ ./build/CrushJointsTests_artefacts/Release/CrushJointsTests
All DSP tests passed.
```

## 5. Known gaps and deviations

Ordered roughly by impact.

1. **No host validation yet.** `pluginval`, `auval` and DAW testing have not
   been run. Offline tests and a Standalone launch are the only verification.
2. **No oversampling.** This matches reverse-camel and keeps the dry/wet path
   latency-free, but high Mech settings can alias like the original. The
   `Oversampler` wrapper exists for a future optional quality mode.
3. **GUI feature gaps.** No value readout, output level meter, MIDI Learn,
   Linear/Circular right-click menu, custom skin, resizable layout or A/B UI.
4. **Program API is incomplete.** `getCurrentProgram()` returns 0;
   `changeProgramName()` does nothing; the editor's preset box does not update
   when the host changes program.
5. **Randomize is naive.** It assigns uniform random normalised values to every
   parameter, including on/off switches, so it can produce silence.
6. **A/B helpers are not connected.** `copyAtoB`, `copyBtoA` and `toggleAB`
   exist on the DSP classes but are not driven by parameters or the editor.
7. **Master volume range differs from the reference.** The plugin uses
   -24..+12 dB (CrushClone's range, required by presets such as MAS Loudener at
   +2 dB) instead of the original -51..0 dB.
8. **Filter and compressor are reference-inspired, not measured clones.** The
   SVF topology, Q range, compressor curve and Phat shelf come from CrushClone
   rather than black-box measurement of the original.
9. **Format coverage is narrower than old docs claimed.** Only VST3, AU and
   Standalone are built; there is no CLAP, LV2 or AAX target.
10. **No CI and no LICENSE file.** The README claims GPLv3, but the repository
    does not yet contain a `LICENSE` file.

## 6. Validation checklist

| Check | Status |
|-------|--------|
| Clean CMake configure/build | ✅ passed on macOS arm64, no source warnings |
| Offline DSP/reference tests | ✅ passed |
| Standalone launch smoke test | ✅ passed (6 s, no crash) |
| `pluginval --strictness-level 5` on VST3 | ⬜ not run |
| `auval` on the AU component | ⬜ not run |
| State save/load round-trip in a host | ⬜ not run |
| Automation of all 12 parameters | ⬜ not run |
| Preset switching in a host | ⬜ not run |
| Stereo channel balance / bypass behaviour | ⬜ not run in a host |
| Null test against dry signal for all-off state | ⬜ not run (offline bypass tests pass) |
| Reference comparison with original CamelCrusher | ⬜ blocked on access to the original |
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
```

The repository no longer tracks `build/`, `.idea/` or `.DS_Store`; they are
covered by `.gitignore`.
