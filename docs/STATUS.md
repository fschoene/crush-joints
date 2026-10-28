# Implementation Status

Last updated: **2026-10-28**.

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
| Processor tests | ✅ passing | `CrushJointsProcessorTests`, A/B, programs, state, locks and randomize |
| Plugin entry point | ✅ working | `createPluginFilter()` present |
| Processor/buses/state | ✅ working | stereo + mono/mono + mono/stereo I/O; APVTS XML plus `currentProgram`, A/B slots and lock flags |
| Parameter set | ✅ working | 12 parameters, editor attachments bound |
| Distortion DSP | ✅ implemented | reverse-camel 101-entry Tube/Mech tables, run as Mech → Filter → Tube (measured order), bypass |
| Filter DSP | ✅ implemented | ZDF/TPT SVF low-pass (2-pole), Q = 1 + 6·Res, original cutoff curve (80 Hz–16 kHz), 10 ms parameter ramps; the original's output limiter is missing |
| Compressor DSP | ✅ implemented | measured model of the original: linked slew-limited follower, parabola gain computer, 0.25 ms gain ramp; Phat = instant detector; old Phat design kept as hidden "Boom" |
| Master DSP | ✅ implemented | dry/wet, volume, full bypass, ±0.9999 hard clip (measured), NaN/Inf cleanup |
| Oversampling | ⚠️ not in path | deliberate: matches reverse-camel, keeps dry/wet latency-free |
| Factory presets | ✅ working | Default at the top outside the groups; 20 Legacy/Original + 11 CrushClone presets in groups |
| User presets | ✅ working | User category, empty until saved; save, overwrite, delete in the editor; rename via host API; XML in user app data |
| Program API | ✅ working | real current program, host program names and renames |
| A/B comparison | ✅ working | processor-level snapshots; A/B and copy in the editor |
| GUI shell | ⚠️ partial | labels, dark blue/yellow theme, value readout, double-click reset, locks, resizing and bypass dimming are in; meter, MIDI Learn, right-click modes and accessibility are missing |
| Host validation | ❌ not started | no pluginval/auval/DAW runs |

## 3. What works end to end

- The plugin loads as a VST3/AU/Standalone bundle; the Standalone was launched
  for six seconds without crashing or logging errors.
- The editor opens with four module columns, A/B controls, the preset bar,
  Save/Delete, Randomize and a value readout. The window scales from 80% to
  150% with a fixed aspect ratio. Modules dim when switched off; turning Master
  off dims all four module containers to show full bypass.
- Every knob has a visible uppercase label, a padlock button and a high-contrast
  dark blue/yellow style. Double-click resets a knob to its parameter default.
- Knobs and toggles update the APVTS parameters immediately.
- Mono tracks are supported natively: mono in → mono out and mono in → stereo
  out (centred), alongside the existing stereo path. Mono VST3 and AU were
  verified in Reaper.
- Randomize only changes continuous parameters; it skips every on/off switch
  and leaves locked parameters untouched. The readout shows the current preset
  name, or the dragged knob's value while adjusting.
- The preset selector starts with **Default** outside the groups, then
  **Legacy / Original** (20), **CrushClone** (11) and **User** (empty until a
  preset is saved).
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
- Plugin state is saved and restored as APVTS XML plus `currentProgram`, the
  two A/B snapshots and the seven lock flags.
- Audio is processed through the chain measured on the original,
  **Mech → Filter → Tube → Compressor → Master**:
  - **Distortion:** the reverse-camel Mech and Tube table stages with linear
    interpolation between table entries. At amount 0 both stages are
    near-transparent; `tube_on` bypasses both stages (the Filter is independent).
  - **Filter:** 2-pole (12 dB/oct) zero-delay-feedback low-pass with
    `Q = 1 + 6·Res` (1 to 7), the original's cutoff curve (80 Hz to 16 kHz, a
    lookup table, see `Source/Common/CutoffCurve.*`), linear 10 ms ramps for
    cutoff and resonance and per-sample coefficients while they move. Switching
    it on starts from a cleared state. The original's output limiter is not
    implemented.
  - **Compressor:** the measured model. Inputs are clamped to ±3.3; a stereo-linked
    follower tracks `max(|L|,|R|)` at 1.0 per 48 ms up / 1.0 per 80 ms down (Phat
    mode: instantaneous); the gain target is `G = 1/(1−0.99·Amount)` up to a knee
    and `(1−c(1−e)²)/e` above it; the gain falls at once and rises along a 0.25 ms
    ramp. Amount ramps over 10 ms; switching the module on starts from unity gain.
    Phat has no bass shelf. The earlier Phat design is kept as a hidden "Boom"
    stage (DSP-level switch, no parameter, no UI).
  - **Master:** optional dry/wet blend against a pre-effects copy, output
    volume, full bypass when `master_on` is false, hard clipping at ±0.9999 (as
    measured on the original) and NaN/Inf sanitation.

## 4. Test results

`Tests/DSPTests.cpp` verifies:

1. Coefficient table anchor values (Tube gain 1→4.99996, Mech gain 1→11.0001).
2. Distortion output against an independent reference implementation for
   several input levels.
3. The complete Distortion → Master path against a reference chain built from
   the reverse-camel tables (Mech → Tube, ±0.9999 clip) at several
   Tube/Mech/mix/volume settings.
3a. Distortion output against values recorded from the original plugin for
   three Tube/Mech combinations (order-sensitive).
4. Distortion transparency at amount 0 and bypass at `tube_on = false`.
5a. Cutoff curve points and inverse (values read from the original).
5b. Filter against the analytic bilinear biquad (Q = 1 + 6·Res) at several
   frequencies, gain Q at the cutoff, linearity.
6. Compressor against values recorded from the original: static curves for four
   Amounts (Phat off and on), Phat release ramp, slow attack/release samples,
   stereo link, bypass and switch-on fade, plus the hidden Boom stage.
7. Master volume boost, ±0.9999 clipping, full bypass and dry/wet mix.
8. The 32-preset factory bank (Default + 20 legacy + 11 grouped CrushClone),
   including the corrected Telephone flags, category assignment, legacy value
   conversion; values re-verified against the original factory `.fxb`
   (2026-10-28), including American Crunch and British Clean at 100% wet.
9. Custom preset save/overwrite/rename/delete and XML round-trip through a
   temporary file, including User-category assignment.

`Tests/ProcessorTests.cpp` verifies:

1. A/B switching captures edits before leaving a slot.
2. Copying the active slot into the inactive slot.
3. `setCurrentProgram`/`getCurrentProgram` and `getProgramName`.
4. Plugin state round-trip including `currentProgram`, the active A/B slot,
   both snapshots and the lock flags.
5. Custom preset save/delete through the processor and factory/user program metadata.
6. Legacy preset application and category counts (20 legacy, 11 grouped CrushClone; Default separate).
7. Locked parameters staying fixed during Randomize and module switches never
   being randomised.
8. Filter resonance boosting a tone at the cutoff and compressor amount/Phat
   changing the rendered output.
9. Mono/mono, mono/stereo and stereo/stereo bus layouts, including centred
   mono-to-stereo output and stereo independence.
10. Optional editor snapshots at 80%, 100% and 150% (`CRUSHJOINTS_RENDER_UI=1`).

Result:

```
$ ./build/CrushJointsTests_artefacts/Release/CrushJointsTests
All DSP tests passed.
$ ./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
All processor tests passed.
```

## 5. Known gaps and deviations

Ordered roughly by impact. Items marked "measured" come from black-box
measurements of the original ([MEASUREMENTS.md](MEASUREMENTS.md)).

0. **Filter output limiter is missing (measured, not implemented).** The
   original's filter limits its own output at about ±1.0: gain falls as
   `1/envelope` once an envelope that attacks at 113.3 per second passes 1.0, and
   recovers very slowly. It is exact for steps but only partly identified for
   sines and noise (the steady limit is 1.027 for a sine at every frequency
   above 1 kHz and the release depends on the envelope). It matters for hot
   settings with resonance, where it makes the clone's output differ by up to
   about −7 dB RMS error in random A/B tests; most settings agree to better than
   −45 dB. See [MEASUREMENTS.md](MEASUREMENTS.md) §5 and
   `tools/analysis/compare_clone.py`.

1. **No host validation yet.** `pluginval`, `auval` and DAW testing have not
   been run. Offline tests and a Standalone launch are the only verification.
2. **No oversampling.** This matches reverse-camel and keeps the dry/wet path
   latency-free, but high Mech settings can alias like the original. The
   `Oversampler` wrapper exists for a future optional quality mode.
3. **GUI feature gaps.** No output level meter, MIDI Learn, Linear/Circular
   right-click menu, keyboard navigation or accessibility labels. The theme is
   CamelCrusher-inspired rather than pixel-exact.
4. **Randomize can still produce extreme settings.** It leaves module switches
   alone and respects locks, but it can set several parameters to their
   extremes at once. There is no "intelligent" constraint yet.
5. **Master volume range is wider than the original (deliberate).** The plugin
   keeps -24..+12 dB (the original is −∞..0 dB with gain `knob^2.737`; the 20
   factory presets are converted with that law and stay inside our range).
   Like the A/B comparison and the randomize locks, the positive range is an
   intended deviation. Master Mix and Volume are not smoothed here; the
   original ramps them over 10 ms.
6. **Compressor and filter are measured models.** They match the original to
   better than −65 dB on test signals; the differences are the missing filter
   limiter (item 0), the exact behaviour of the filter when it is switched on,
   and the Amount-fade shape when the compressor is switched on (within a few
   percent). The hidden "Boom" stage (the former Phat design) is a CrushJoints
   invention and has no counterpart in the original.
7. **Format coverage is narrower than old docs claimed.** Only VST3, AU and
   Standalone are built; there is no CLAP, LV2 or AAX target.
8. **No CI and no LICENSE file.** The README claims GPLv3, but the repository
   does not yet contain a `LICENSE` file.
9. **User presets are a single XML bank.** There is no import/export, search,
   filter or sorting UI. Corrupt or unreadable files are ignored gracefully,
   leaving the factory bank available.
10. **Original preset conversion uses measured laws.** The 20 raw preset values
    match the original `CamelCrusherPresetsA.fxb`; cutoff uses the original knob
    curve and volume the measured `knob^2.737` law. Not yet checked: loading the
    `.fxb` into the original and comparing audio with our presets end to end.
11. **Program indices changed with Milestone 4a.** Default is 0, CrushClone
    1–11, Legacy/Original 12–31 and User starts at 32. States saved before this
    change keep their parameter values but may display a different current
    program name.
12. **Mono/stereo host verification is done for Reaper.** Mono/mono,
    mono/stereo and stereo/stereo layouts pass offline tests and mono VST3/AU
    work in Reaper; Logic mono-track verification remains optional for the
    broader Milestone 4 host matrix.
13. **Cutoff parameter range changed.** `cutoff` is now 80 Hz–16 kHz with the
    original's knob curve (it was 20 Hz–20 kHz with a skew). Saved states and user
    presets keep their Hz value, clamped to the new range, so the knob position
    changes accordingly.

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
