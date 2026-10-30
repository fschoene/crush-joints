# Implementation status

Last updated: **2026-10-29**.

A factual snapshot of the repository: build, tests, validation, known gaps and
deliberate deviations. How the code is organised is in
[ARCHITECTURE.md](ARCHITECTURE.md); measured behaviour of the original is in
[MEASUREMENTS.md](MEASUREMENTS.md); planned work is in
[ROADMAP.md](ROADMAP.md).

## 1. Verified environment

| Item | Value |
|------|-------|
| OS | macOS 26 (arm64) |
| Compiler | AppleClang 21.0.0 |
| CMake | 4.4.4 |
| JUCE | 9.0.4, |
| Build type | Release |

A clean build succeeds with no warnings from project sources and produces:

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
| CMake build | ✅ working | VST3, AU and Standalone on macOS; warning-free |
| Offline DSP tests | ✅ passing | `CrushJointsTests` |
| Processor tests | ✅ passing | `CrushJointsProcessorTests` |
| Plugin entry point | ✅ working | `createPluginFilter()` present |
| Processor/buses/state | ✅ working | stereo, mono/mono and mono/stereo; APVTS XML plus program, A/B and locks |
| Parameter set | ✅ working | 12 parameters, editor attachments bound |
| Distortion DSP | ✅ implemented | measured 101-entry Tube/Mech tables, Mech → Filter → Tube |
| Filter DSP | ✅ implemented | 2-pole low-pass, Q = 1 + 6·Res, original cutoff curve, 10 ms ramps; the original's output limiter is missing |
| Compressor DSP | ✅ implemented | measured model; Phat = instant detector; old design kept as hidden "Boom" |
| Master DSP | ✅ implemented | mix, volume, full bypass, ±0.9999 clip, 10 ms ramps, NaN/Inf cleanup |
| Oversampling | ⚠️ not in path | deliberate: matches the original, keeps the dry/wet path latency-free |
| Factory presets | ✅ working | Default + 20 Legacy/Original + 11 CrushClone, grouped |
| User presets | ✅ working | save, overwrite, delete; XML in user app data |
| Program API | ✅ working | real current program, host names and renames |
| A/B comparison | ✅ working | processor snapshots; A/B and copy in the editor |
| GUI shell | ⚠️ partial | labels, theme, readout, reset, locks, resizing, bypass dimming; meter, MIDI Learn, knob modes and accessibility are missing |
| Host validation | ✅ Reaper + validators | pluginval and auval pass; Reaper manual pass covers automation, presets, reload, A/B and bypass. Logic not installed |

## 3. Tests and validation

Both suites pass:

```
$ ./build/CrushJointsTests_artefacts/Release/CrushJointsTests
All DSP tests passed.
$ ./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
All processor tests passed.
```

`CrushJointsTests` covers the distortion tables and recorded original output,
the cutoff curve, the filter against the analytic biquad, the compressor against
recorded original behaviour (static curve, dynamics, stereo link, bypass,
switch-on, Boom), master clipping/bypass/mix/smoothing and the preset bank
including the legacy conversion. `CrushJointsProcessorTests` covers A/B
switching and copying, bus layouts, programs, state round-trip, user presets,
randomize locks and the signal-chain order.

### Validation results

| Check | Result |
|-------|--------|
| Clean CMake configure/build | ✅ macOS arm64, no source warnings |
| Standalone launch | ✅ 6 s, no crash |
| `auval -v aufx CJTS Crsh` | ✅ `AU VALIDATION SUCCEEDED` |
| pluginval 1.0.4, VST3, strictness 5 | ✅ in-process and out-of-process |
| pluginval 1.0.4, VST3, strictness 10 | ✅ fuzzing, thread safety, state, automation, bus layouts, Steinberg validator |
| pluginval 1.0.4, AU, strictness 5 and 10 | ✅ (see warning note) |
| Reaper manual pass | ✅ automation, factory/user presets, project reload, A/B slots, master bypass |
| Logic Pro, AU | ⬜ not installed on the development machine |
| CPU / latency / buffer sizes | ✅ see below |
| Cross-platform build | ⬜ not configured |

CPU benchmark (`tools/analysis/benchmark_cpu.py`, Apple M4, 20 s stereo, all
modules on, hot setting): 0.1 % of one core at 44.1 and 48 kHz for block sizes
64–1024 (about 1200× realtime), and 0.1 % at 96 kHz (about 700×). pluginval
reports zero latency and zero tail. There is no oversampling, so the load does
not scale with a quality factor.

pluginval's AU validation logs `Current program is -1` as a warning. The value
comes from the host-side AU wrapper reading `kAudioUnitProperty_PresentPreset`;
JUCE's AU client only sets that property after a program change, so the warning
appears for JUCE AUs in general. The test passes.

Commands used (macOS):

```sh
# Install the AU for auval / host visibility
cp -R build/CrushJoints_artefacts/Release/AU/CrushJoints.component \
      ~/Library/Audio/Plug-Ins/Components/

auval -v aufx CJTS Crsh

pluginval --strictness-level 5 --validate-in-process \
          build/CrushJoints_artefacts/Release/VST3/CrushJoints.vst3
pluginval --strictness-level 10 --validate-in-process \
          build/CrushJoints_artefacts/Release/VST3/CrushJoints.vst3
pluginval --strictness-level 10 --validate-in-process \
          build/CrushJoints_artefacts/Release/AU/CrushJoints.component
```

## 4. Known gaps and deviations

Ordered roughly by impact. "Measured" items come from
[MEASUREMENTS.md](MEASUREMENTS.md).

0. **Filter output limiter is missing.** The original limits the filter output
   at about ±1.0: gain falls as `1/envelope` once an envelope attacking at
   113.3 per second passes 1.0, then recovers very slowly. It fits DC steps
   exactly but is only partly identified for sines and noise (a sine settles at
   1.027 where the model gives 1.000; noise peaks of 2.2 pass where the model
   limits to 1.4). It matters for hot, resonant settings, where random A/B tests
   differ by up to about −7 dB RMS; most settings agree to better than −45 dB.
   See [MEASUREMENTS.md](MEASUREMENTS.md) §5.
1. **Host validation is not complete.** Reaper (VST3/AU) and the validators
   pass. Logic Pro is not installed here, so the AU has not been checked in
   Logic and the "two DAWs" release criterion is still open.
2. **No oversampling.** This matches the original and keeps the dry/wet path
   latency-free, but high Mech settings can alias like the original. The
   `Oversampler` wrapper exists for a future optional mode.
3. **GUI feature gaps.** No output level meter, MIDI Learn, Linear/Circular
   knob menu, keyboard navigation or accessibility labels. The theme is
   CamelCrusher-inspired, not pixel-exact. Tracked for Version 2.
4. **Randomize can produce extreme settings.** It respects locks and leaves
   switches alone, but can set several parameters to their extremes at once.
5. **Master volume range is wider than the original (deliberate).** The plugin
   keeps −24…+12 dB; the original is −∞…0 dB with `gain = knob^2.737`. The 20
   factory presets are converted with that law and stay inside the range. Mix
   and Volume use the original's 10 ms knob-domain ramps.
6. **Compressor and filter are measured models.** They match the original to
   better than −65 dB on test signals. The remaining differences are the filter
   limiter (item 0), the exact filter state when switched on, and the Amount-fade
   shape when the compressor is switched on (within a few percent). The hidden
   "Boom" stage is a CrushJoints invention with no counterpart in the original.
7. **Format coverage.** Only VST3, AU and Standalone are built; there is no
   CLAP, LV2 or AAX target.
8. **No CI.** Build and validators are run manually. The GPLv3 `LICENSE` file is
   present.
9. **User presets are a single XML bank.** No import/export, search, filter or
   sorting UI. Unreadable files are ignored, leaving the factory bank available.
10. **Preset conversion uses measured laws.** The 20 raw values match the
    original `.fxb`; cutoff uses the original knob curve and volume the measured
    `knob^2.737` law. Factory and user presets load correctly in Reaper.
11. **Program indices changed with Milestone 4a.** Default is 0, CrushClone
    1–11, Legacy/Original 12–31 and User starts at 32. States saved before this
    change keep their parameter values but may show a different program name.
12. **Mono/stereo verification.** Mono/mono, mono/stereo and stereo/stereo pass
    offline tests and were verified in Reaper; Logic remains optional for the
    broader host matrix.
13. **Cutoff parameter range changed.** `cutoff` is now 80 Hz–16 kHz with the
    original knob curve (it was 20 Hz–20 kHz with a skew). Saved states keep
    their Hz value, clamped to the new range, so the knob position changes.

## 5. Reproducing the build and tests

```sh
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
cmake --build build --config Release -j8
./build/CrushJointsTests_artefacts/Release/CrushJointsTests
./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

`build/`, `.idea/` and `.DS_Store` are gitignored. The black-box harness is in
[`tools/analysis/`](../tools/analysis/README.md).
