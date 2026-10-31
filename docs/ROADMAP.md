# Roadmap

Navigation for CrushJoints development. Measured facts about the original live
in [MEASUREMENTS.md](MEASUREMENTS.md), the code design in
[ARCHITECTURE.md](ARCHITECTURE.md), and the current state in
[STATUS.md](STATUS.md).

## Versions

| Version | Status | Summary |
|---------|--------|---------|
| **Version 1** | Core complete; release validation finishing | Faithful CamelCrusher clone plus the workflow needed to use it: full DSP chain, original 20-program bank plus CrushClone presets, user presets, mono/stereo support, A/B and a resizable editor |
| **Version 2** | Planned, not started | GUI and workflow upgrades: output meter, knob modes/MIDI Learn, accessibility, final styling, smarter Randomize, exposed Boom mode |

## Version 1 — faithful clone + modern workflow

### Scope at a glance

- Tube + Mech distortion from the measured coefficient tables.
- 2-pole low-pass filter with the original cutoff curve, between Mech and Tube.
- The original's one-knob compressor and Phat mode (measured model).
- Master mix, volume, full bypass and the original ±0.9999 clip.
- Default preset plus the original 20-program factory bank and 11 CrushClone
  presets, grouped as Legacy/Original, CrushClone and User.
- Disk-backed user presets, real program API, A/B snapshots and state
  persistence.
- Native mono/mono, mono/stereo and stereo/stereo bus layouts.
- Resizable dark blue/yellow editor with labels, locks, readout and
  double-click reset.

### Milestone 1 — Real DSP (completed 2026-10-25)

- [x] Import the 101-entry Tube and Mech coefficient tables and store them in
      `Source/Common/CamelCrusherCoeffs.cpp`.
- [x] Implement the Tube and Mech stages with interpolation between table
      entries (run as Mech → Filter → Tube since Milestone 4c).
- [x] Implement the 2-pole low-pass filter (reworked in Milestone 4c: Q 1–7,
      original cutoff curve).
- [x] Implement the one-knob compressor with Phat (replaced in Milestone 4c by
      the measured model of the original).
- [x] Implement Master volume, dry/wet, full bypass, the ±0.9999 clip and
      NaN/Inf sanitation.
- [x] Add `Tests/DSPTests.cpp`.
- [x] Correct the Telephone preset flags (filter on, compressor off).
- [x] Keep the build warning-free.

Decision: **no oversampling in v1.** It matches the original's aliasing
character and keeps the dry/wet path latency-free. The `Oversampler` wrapper
remains available for a future optional quality mode.

### Milestone 2 — State, presets and automation (completed 2026-10-25)

- [x] Return the real current program from `getCurrentProgram()`.
- [x] Store/restore the program index in the plugin state.
- [x] Implement `changeProgramName()` for user presets.
- [x] Sync the editor's preset box with host-driven program changes.
- [x] Persist A/B slots and the active slot.
- [x] Processor-level A/B comparison with edit capture and slot copying.
- [x] User preset save/overwrite/delete backed by an XML bank in the user
      application data directory.
- [x] Editor controls for A/B, copy, save and delete.
- [x] Processor tests for A/B, programs and state, including a temporary preset
      bank file.

### Milestone 3 — GUI completion (completed)

- [x] Value readout showing the preset name, then the dragged parameter value.
- [x] CamelCrusher-inspired dark blue + yellow theme.
- [x] Visible labels for every parameter knob.
- [x] Double-click reset to each parameter's default.
- [x] Per-parameter padlock buttons that exclude a value from Randomize.
- [x] Randomize skips on/off and Phat switches and respects locks.
- [x] Lock flags persisted in plugin state.
- [x] Resizable/scalable layout from 80 % to 150 % with a fixed aspect ratio.
- [x] Module containers dim when bypassed; Master off dims all modules.
- [x] Visual snapshots at multiple sizes via `CRUSHJOINTS_RENDER_UI=1`.

The remaining GUI work moved to [Version 2](#version-2--gui-and-workflow-upgrades).

### Milestone 4a — Original preset bank and categories (completed)

- [x] Add the original 20-program factory bank, verified value for value against
      the original `CamelCrusherPresetsA.fxb` (2026-10-28).
- [x] Map the legacy normalized cutoff and volume values with the measured knob
      laws; keep tube/mech/res/amount/mix and switches direct.
- [x] Keep the 11 CrushClone presets as a separate group, with Default outside
      the groups at the top.
- [x] Group presets as Legacy/Original, CrushClone and User.
- [x] Saving a user preset adds it to the User group; deleting removes it.
- [x] Keep group headings display-only and program indices stable.
- [x] Fix American Crunch / British Clean: both are 100 % wet in the original.
- [x] Tests for all 20 presets, group contents, user save/delete and host
      program names.

### Milestone 4b — Mono/stereo bus compatibility (completed)

- [x] Accept mono → mono, stereo → stereo and mono → stereo (centred output).
- [x] Update `isBusesLayoutSupported` and make `processBlock` channel-count
      aware.
- [x] Make `prepareToPlay`, the dry buffer, filter, compressor and master use
      the negotiated channel count.
- [x] Offline tests for every supported layout, including level preservation and
      stereo independence.
- [x] Verify mono VST3 and AU in Reaper.

### Milestone 4c — Measurement-driven fidelity pass (completed except the filter limiter)

Source of truth: [MEASUREMENTS.md](MEASUREMENTS.md), produced with
[`tools/analysis/`](../tools/analysis/README.md).

- [x] Black-box harness: offline x86_64 VST2 host plus Python scripts.
- [x] Distortion order Mech → Tube, with tests against recorded original output.
- [x] Output clip: hard clip at ±0.9999.
- [x] Cutoff: 80 Hz–16 kHz with the measured curve (`CutoffCurve`); legacy
      preset conversion fixed.
- [x] Volume: the positive range (−24…+12 dB) is kept as a deliberate
      deviation; the 20 factory presets use the measured `knob^2.737` law.
- [x] Filter: linear 2-pole biquad with `Q = 1 + 6·Res`, 10 ms parameter ramps.
- [x] Compressor and Phat: the measured model replaces the earlier design; the
      old Phat design is kept as a hidden "Boom" stage.
- [x] Signal chain order Mech → Filter → Tube → Compressor → Master, with a
      processor test.
- [x] Mix/Volume 10 ms smoothing, linear in the knob value.
- [x] Regression tests against recorded measurements.
- [ ] Filter output limiter of the original (measured, partly identified; see
      [MEASUREMENTS.md](MEASUREMENTS.md) §5).

### Milestone 4 — Validation and release (in progress)

- [x] Extend the test harness: offline render comparison, fuzzing through
      pluginval strictness 10, and `tools/analysis/benchmark_cpu.py`.
- [x] `pluginval` on the VST3 at strictness 5 and 10.
- [x] `pluginval` on the AU at strictness 5 and 10; `auval` passes.
- [x] Manual Reaper pass: parameter automation, factory/user preset switching,
      project reload, A/B slots and master bypass all work.
- [ ] Logic Pro (AU); not installed on the development machine.
- [ ] Verify presets, state and automation in a second DAW.
- [x] Add GitHub Actions CI: macOS on every PR/push (build, tests, `auval`,
      pluginval 5); Windows and Linux VST3 builds on `main` pushes and manual
      runs; release workflow for tags (macOS universal VST3+AU, Windows/Linux
      VST3, pluginval 5/10, packaged downloads).
- [x] Add the GPLv3 `LICENSE`.
- [x] Keep the build warning-free.

### Definition of done for v1.0

- [x] All four DSP modules implemented with documented provenance.
- [x] No pass-through stubs left in the signal chain.
- [x] A/B comparison and a persistent user preset bank.
- [x] Preset, program, A/B and state behaviour covered by offline tests.
- [x] Original 20-program factory bank integrated and verified.
- [x] Preset groups Legacy/Original, CrushClone and User.
- [x] Mono/mono, stereo/stereo and mono/stereo layouts supported, tested and
      verified in Reaper.
- [ ] Presets, state and host automation verified in at least two DAWs.
- [x] `pluginval` strictness 5+ and `auval` pass.
- [x] VST3, AU and Standalone artefacts built from a clean checkout.
- [x] README, docs and LICENSE consistent with the implementation.

## Version 2 — GUI and workflow upgrades

Starts after the v1 release work. Initial scope:

- [ ] Output level meter / clipping indicator fed by the processor.
- [ ] Right-click knob menu with Linear/Circular drag and MIDI Learn.
- [ ] Keyboard navigation and accessibility labels.
- [ ] Pixel-exact CamelCrusher styling (the current theme is inspired, not a
      reproduction).
- [ ] Optional "intelligent" Randomize constraints to avoid extreme settings.
- [ ] Expose the **Boom** compressor mode (see below): parameter, editor
      control, state and preset support, and a decision on its final name.

### Boom mode (hidden compressor option)

The earlier CrushJoints Phat design (itself inspired by CrushClone's parallel
compression) is a 120 Hz +8 dB low shelf feeding a heavy parallel compressor
that is blended in more strongly as Amount rises. It is not part of the original
CamelCrusher: measurements show that the real Phat mode only changes the
detector and release timing ([MEASUREMENTS.md](MEASUREMENTS.md) §6). The design
is kept as a hidden stage of `Compressor` called **Boom** (working name; "Cloud"
was the other candidate):

- `Compressor::setBoomMode(bool)` switches it on at DSP level; there is no
  APVTS parameter, no editor control and no state entry yet.
- It runs behind the main compressor: the compressed signal is low-shelf
  boosted (120 Hz, +8 dB), squashed by a fixed heavy compressor (−18 dB, 6:1,
  5 ms / 100 ms) and crossfaded back in (30 % to 50 % with Amount, 20 ms fade).
  Feeding it from the compressor output is deliberate: the measured compressor
  can add up to +40 dB, so a leg fed from the input would sit far below the main
  path and never be heard.
- `Tests/DSPTests.cpp` (`testCompressorBoom`) checks that it is off by default,
  stays finite and adds low-end level when on.
- Open questions for exposing it: an extra toggle versus a third Phat state, its
  threshold and ratio against typical material, preset storage, and whether it
  follows Amount or gets its own control.

### V2 notes

- Additional features only enter V2 by promoting an item from the backlog.
- The output meter needs a processor-side level report; MIDI Learn needs a
  persistent mapping model; accessibility needs labels and a keyboard focus
  order. Design these before implementation.

## Guiding principles

1. **Sound first.** A faithful DSP chain is the point of the project. GUI polish
   and format coverage come after the audio is right.
2. **Verify against the original.** Measure it with `tools/analysis/` and record
   the results in [MEASUREMENTS.md](MEASUREMENTS.md). Reverse-camel's tables and
   CrushClone's presets are secondary sources; measurements win.
3. **Keep the plugin shippable at every step.** Do not break the working
   VST3/AU/Standalone build while adding features.
4. **Document deviations.** If the clone intentionally differs from the original
   (for example the −24…+12 dB master range), record it in
   [STATUS.md](STATUS.md) and in the code.

## Idea backlog

Unscheduled ideas; none are commitments. To schedule one, move it into the
Version 1 or Version 2 scope above and add acceptance criteria.

### GUI

- Real-time parameter visualisation.
- Compact/expanded modular views.
- Colour coding for active modules.
- Parameter linking (for example Tube/Mech blend).
- Improved MIDI mapping.

### DSP

- Extra distortion flavours.
- EQ module (graphic or parametric).
- Delay/reverb.
- LFO modulation.
- Multi-band processing.
- Monophonic mode.
- Stereo widening.

### Master / gain matching (proposed, not implemented)

- Add a **Gain Match** toggle to the Master module.
- When enabled:
  - disable and visually grey out the Master Volume knob;
  - measure input and output level and apply an automatic compensation gain so
    the output tracks the input;
  - show the applied gain (and ideally the levels) in the readout.
- Start with peak or RMS detection over a short window with smooth
  attack/release. Optional second mode: K-weighted short-term loudness.
- Decisions to settle first: measurement points (before the chain vs after
  Mix), whether the ±0.9999 clip is included, interaction with Mix, bypass,
  compressor/Phat makeup, mono/stereo linking, latency, smoothing, maximum
  correction, automation behaviour and state.
- Priority: unscheduled. The original has no such feature.

### Legacy compatibility

- Original parameter order/IDs and legacy chunk import/export.
- Same-identity VST2 session recall: the original is `CamelCrusher` with unique
  ID `CaCr`, 17 parameters (12 used, 5 reserved), 20 programs and program
  chunks; the AU is `aumf` / `CaCr` / `CamA`. This is a larger, separate
  compatibility project.
- Original UI geometry/assets for pixel-exact styling (v2; artwork and
  trademark review required).

### Workflow

- Preset search/filter and comparison.
- Session management and auto-save.
- Favourite presets.
- Preset import/export (`.FXP`/`.FXB` or JSON).

### Technical

- CLAP support (for example via `clap-juce-extensions`).
- Deeper VST3 validation.
- Audio-buffer and SIMD optimisation.
- User themes/skins.
- OSC or web-based control surface.

### Accessibility and integration

- Keyboard shortcuts.
- Touch/tablet support.
- Localisation.
- Global settings panel.
- DAW-specific compatibility work.

### Performance

- Variable/quality-selectable oversampling.
- CPU usage indicator.
- SIMD paths for the waveshapers.
- Memory-efficient preset handling.
