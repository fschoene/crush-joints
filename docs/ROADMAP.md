# Roadmap

Prioritised work remaining for CrushJoints, plus the longer-term idea backlog.
The current state is documented in [STATUS.md](STATUS.md); the intended
behaviour is in [RESEARCH.md](RESEARCH.md).

## Guiding principles

1. **Sound first.** A faithful DSP chain is the point of the project. GUI polish
   and format coverage come after the audio is right.
2. **Verify against references.** Use the reverse-camel tables and CrushClone's
   presets as the ground truth, and keep a documented comparison harness.
3. **Keep the plugin shippable at every step.** Do not break the working
   VST3/AU/Standalone build while adding features.
4. **Document deviations.** If the clone intentionally differs from the
   original (for example 4× oversampling), record it in the code and here.

## Milestone 1 — Real DSP (priority 0)

This is the critical path. Everything else is secondary until this is done.

### 1.1 Fix the coefficient tables

- Re-derive or import the reference 101-entry Tube and Mech tables.
- Decide on one storage location: either `Common/CamelCrusherCoeffs.cpp` or the
  static members in `Distortion`, not both.
- Add a unit test or a small offline checker that verifies:
  - row count,
  - monotonic gain mapping (Tube 1.0→≈5.0, Mech 1.0→≈11.0),
  - index 0 is near-linear,
  - index 100 reaches the documented saturation.
- Document the table format (`{gain, a, b, c}` for Tube, `{gain, b, c}` for
  Mech) next to the data.

### 1.2 Distortion

- Implement `Distortion::processBlock` using `tubeDistort` and `mechDistort`.
- Decide whether Tube and Mech run in series (research says Mech follows Tube)
  and how the two amounts blend. Implement the chosen topology explicitly.
- Respect `tube_on` bypass.
- Use linear interpolation between table entries to avoid zipper noise.
- Run the distortion inside the 4× oversampler (or remove the oversampler
  wrapper if the original character is preferred without it).
- Handle NaN/Inf and denormals at the module boundary.

### 1.3 Filter

- Implement `LadderFilter::processBlock` with
  `StateVariableTPTFilter<float>` in low-pass mode.
- Map resonance 0–1 to Q ≈ 0.707–4.2 and clamp below self-oscillation.
- Bypass when `lpf_on` is false; optionally auto-bypass at 20 kHz / zero
  resonance if that matches the original.
- Recompute coefficients only when cutoff/Q change.

### 1.4 Compressor

- Implement the one-knob mapping: Amount → threshold (0…-30 dB), ratio
  (1:1…8:1), attack (1–10 ms), release (50–150 ms), makeup (0…+12 dB).
- Implement soft-knee gain computation and a sample-rate-independent envelope.
- Implement Phat: parallel compressed path plus 120 Hz low shelf (+8 dB), with
  the blend increasing with Amount.
- Add gain-reduction metering data (even if it is not shown in the GUI yet).
- Respect `comp_on` and `phat` flags.

### 1.5 Master

- Implement volume gain, dry/wet mix, and full bypass (`master_on`).
- Add a hard safety limiter at ±2.0 and NaN/Inf/denormal flushing.
- Resolve the volume range discrepancy (-24…+12 dB in code vs -51…0 dB in the
  research) and update [RESEARCH.md](RESEARCH.md) or the parameter accordingly.
- Ensure a true null when Master is off or Mix is 0%.

### 1.6 DSP acceptance criteria

- Sine sweep through every module produces no NaN/Inf, clicks or denormals.
- All-off state is bit-transparent (or within the documented latency).
- Distortion output matches the reference curves at several knob positions.
- Filter magnitude response matches a 24 dB/oct low-pass with the expected Q.
- Compressor gain reduction follows the documented Amount mapping.
- Phat increases low-frequency energy without exceeding the limiter.

## Milestone 2 — State, presets and automation (priority 1)

- Return the real current program from `getCurrentProgram()`.
- Store/restore the program index as part of the APVTS state.
- Make `changeProgramName()` either work or clearly document it as unsupported.
- Synchronise the editor's preset box with host-driven program changes
  (`AudioProcessorListener`/timer or parameter listeners).
- Verify preset switching under automation and state restore.
- Decide whether A/B testing belongs in the release; if yes, connect the
  existing DSP A/B helpers to real parameter state and add UI.

## Milestone 3 — GUI completion (priority 2)

Reference features still missing:

- Value readout that shows the preset name, then the adjusted parameter value.
- Output level meter / clipping indicator fed by the processor.
- Right-click knob menu with Linear/Circular drag, MIDI Learn and value
  display.
- Blue illuminated module switches and a CamelCrusher-like colour scheme.
- Optional resizable/scalable layout.
- Randomize that only touches active modules and avoids silent settings.
- A/B controls, if A/B is kept.
- Keyboard navigation and accessibility labels.

## Milestone 4 — Validation and release (priority 3)

- Add a test harness (unit tests for DSP helpers, offline render comparison).
- Run `pluginval` at strictness 5+ on the VST3.
- Run `auval` on the AU component.
- Test in at least Reaper (VST3) and Logic Pro (AU); add Ableton if available.
- Add CI (GitHub Actions) for macOS build + pluginval; consider Windows VST3
  and Linux VST3/LV2 later.
- Verify state round-trip, automation and preset recall in each host.
- Fix the compiler warnings listed in [STATUS.md](STATUS.md).
- Add a LICENSE file: the README claims GPLv3 but the repository currently has
  no `LICENSE` file.

## Idea backlog

Grouped from the earlier `IDEAS.md`; none of these are commitments.

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

### Workflow

- Preset categories, search/filter and comparison.
- Session management and auto-save.
- Favourite presets.
- Preset import/export (`.FXP`/`.FXB` or a simple JSON format).

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

## Definition of done for v1.0

- [ ] All four DSP modules implemented and matched against references.
- [ ] No pass-through stubs left in the signal chain.
- [ ] Presets, state and host automation verified in at least two DAWs.
- [ ] `pluginval` strictness 5+ and `auval` pass.
- [ ] VST3, AU and Standalone artefacts built from a clean checkout.
- [ ] README, docs and LICENSE consistent with the implementation.
