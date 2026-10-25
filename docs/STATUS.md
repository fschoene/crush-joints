# Implementation Status

Last updated: **2026-10-25**.

This is a factual snapshot of the repository, verified by building and running
the artefacts. It supersedes the old `PHASE_SUMMARY.md`, which described
several phases as complete when their DSP bodies were still empty.

## 1. Verified environment

| Item | Value |
|------|-------|
| OS | macOS 26.3 (arm64) |
| Compiler | AppleClang 21.0.0 |
| CMake | 4.4.4 |
| JUCE | 9.0.4, local checkout at `/Volumes/WDB/user/flo/JUCE` |
| Build type | Release |

`cmake -B build -DCMAKE_BUILD_TYPE=Release` configures successfully and
`cmake --build build --config Release -j8` produces:

```
build/CrushJoints_artefacts/Release/
├── AU/CrushJoints.component
├── VST3/CrushJoints.vst3
└── Standalone/CrushJoints.app
```

The Standalone binary was launched for six seconds: it stayed running, wrote no
errors to stdout/stderr and did not crash. No host, `pluginval` or `auval`
session has been run.

## 2. Status by area

| Area | Status | Notes |
|------|--------|-------|
| CMake build | ✅ working | VST3, AU and Standalone on macOS |
| Plugin entry point | ✅ working | `createPluginFilter()` added |
| Processor/buses/state | ✅ working | stereo in/out; APVTS XML state save/load |
| Parameter set | ✅ working | 12 parameters, editor attachments bound |
| Preset bank | ✅ data, ⚠️ program API partial | 12 presets load through `setCurrentProgram`; `getCurrentProgram()` always returns 0 and `changeProgramName()` is a no-op |
| GUI shell | ⚠️ partial | controls work; value readout, meter, MIDI Learn, right-click modes, styling and A/B UI are missing |
| Distortion DSP | ❌ stub | `processBlock` is empty; `tubeDistort`/`mechDistort` exist but are unused |
| Filter DSP | ❌ stub | SVF prepared, `processBlock` empty |
| Compressor DSP | ❌ stub | defaults configured, `processBlock` empty, Phat chain unused |
| Master DSP | ❌ stub | volume/mix/limiter code is commented out |
| Oversampling | ⚠️ wrapper only | 4× `Oversampling` prepares and can round-trip, but the processor never calls it |
| A/B testing | ❌ not wired | DSP classes have state helpers; no UI or parameter integration |
| Validation | ❌ not started | no pluginval/auval/DAW runs |

## 3. What works end to end

- The plugin loads as a VST3/AU/Standalone bundle (standalone smoke-tested).
- The editor opens, displays four module columns and the preset/randomize bar.
- Moving a knob or toggling a switch updates the corresponding APVTS parameter
  immediately (attachments).
- Selecting a preset calls `setCurrentProgram(index)` and writes all 12
  parameter values.
- Plugin state is serialised as APVTS XML and restored through
  `getStateInformation`/`setStateInformation`.
- Audio passes through the processor without crashing or denormals.

**The plugin does not currently change the sound.** All four DSP blocks are
pass-throughs, so the output is effectively the input.

## 4. Known discrepancies and bugs

Ordered roughly by impact.

1. **DSP is not implemented.** This is the dominant gap. The README/old phase
   summary overstated completion.
2. **Coefficient tables do not match the research target.**
   - `Distortion.h`/`Distortion.cpp` define `disttube_coeffs[201][4]` and
     `distmech_coeffs[402][3]`.
   - The reference describes 101-entry tables.
   - The lookup still uses `round(normalized * 100)`, so it addresses only the
     first 101 / 402 rows.
   - The first 101 Mech entries span a gain of only about 1.0–1.05, not the
     reference 1.0–11.0. The data needs to be re-derived or replaced with the
     reverse-camel tables before the waveshaper is wired up.
3. **`Common/CamelCrusherCoeffs.cpp` is a placeholder.** It defines nothing;
   the shared `extern` arrays in `CamelCrusherCoeffs.h` are unused. The real
   data is duplicated as static members in `Distortion.cpp`.
4. **Oversampling is dead code.** `Oversampler::prepareToPlay` creates a 4×
   oversampler, but `processBlock` in the processor never calls it and the
   distortion does not run inside it.
5. **Master volume range differs from the reference.** Code uses -24 to +12 dB;
   research describes -51 to 0 dB.
6. **Program API is incomplete.** `getCurrentProgram()` returns 0;
   `changeProgramName()` does nothing; the editor's preset box does not update
   when the host changes program.
7. **Randomize is naive.** It assigns uniform random normalised values to every
   parameter, including on/off switches, so it can produce silence. The
   reference only randomises active modules and avoids silent results.
8. **GUI feature gaps.** No value readout, no output level meter, no MIDI Learn,
   no Linear/Circular right-click menu, no custom skin, no A/B UI, fixed
   720×400 size.
9. **The shared coefficient header is misleading.** It declares tables that no
   translation unit defines; linking would fail if anything referenced them.
10. **Format coverage is narrower than old docs claimed.** Only VST3, AU and
    Standalone are built. There is no CLAP, LV2 or AAX target, and no CI.

## 5. Build warnings

The build succeeds with warnings only. The notable ones:

- `Presets.cpp`: signed/unsigned conversion when indexing the preset vector.
- `Distortion.cpp`: implicit float→int conversion in `round(...)`; unused
  `sampleRate`/`buffer` parameters in the stubs.
- `Compressor.cpp`, `LadderFilter.cpp`, `Master.cpp`: unused `buffer`
  parameters in the stubs.
- `PluginProcessor.cpp`: unused `index`/`newName` in `changeProgramName`.
- `PluginEditor.h`: the `processor` member shadows
  `AudioProcessorEditor::processor`.

These are safe to fix while implementing the relevant code; they are not
blocking.

## 6. Validation checklist

Adapted from the old validation plan. None of the runtime validation items have
been executed yet.

| Check | Status |
|-------|--------|
| Clean CMake configure/build | ✅ passed on macOS arm64 |
| Standalone launch smoke test | ✅ passed (6 s, no crash) |
| `pluginval --strictness-level 5` on VST3 | ⬜ not run |
| `auval` on the AU component | ⬜ not run |
| State save/load round-trip in a host | ⬜ not run |
| Automation of all 12 parameters | ⬜ not run |
| Preset switching in a host | ⬜ not run |
| Stereo channel balance / bypass behaviour | ⬜ blocked on DSP implementation |
| Null test against dry signal for all-off state | ⬜ blocked on DSP implementation |
| Reference comparison with CrushClone / reverse-camel | ⬜ blocked on DSP implementation |
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

## 7. Reproducing the build

```sh
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
cmake --build build --config Release -j8
```

The repository no longer tracks `build/`, `.idea/` or `.DS_Store`; they are
covered by `.gitignore`.
