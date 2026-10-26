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
   original (for example the -24..+12 dB master range), record it in the code
   and here.

## Milestone 1 — Real DSP (completed 2026-10-25)

All four DSP blocks are implemented and verified by offline tests.

- [x] Import the reference 101-entry Tube and Mech coefficient tables from
      reverse-camel; store them in `Common/CamelCrusherCoeffs.cpp`.
- [x] Implement Tube followed by Mech in series, with interpolation between
      table entries.
- [x] Implement the SVF low-pass filter with Q 0.7071–4.2071 and wide-open
      bypass.
- [x] Implement the one-knob soft-knee compressor, auto makeup and Phat
      parallel path.
- [x] Implement Master volume, dry/wet, full bypass, ±2.0 safety limiter and
      NaN/Inf sanitation.
- [x] Add `Tests/DSPTests.cpp`, including a literal transcription of
      reverse-camel's `run()` loop for end-to-end comparison.
- [x] Correct the Telephone preset flags (filter on, compressor off).
- [x] Verify a clean, warning-free build and passing tests.

Decision: **no oversampling in v1.** It matches reverse-camel, avoids latency
in the dry/wet path, and preserves the original's aliasing character. The
`Oversampler` wrapper remains available for a future optional quality mode.

Residual work from this milestone is tracked under Milestone 4 (host
validation, CPU testing, optional oversampling).

## Milestone 2 — State, presets and automation (completed 2026-10-25)

- [x] Return the real current program from `getCurrentProgram()`.
- [x] Store/restore the program index as part of the plugin state.
- [x] Implement `changeProgramName()` for user presets.
- [x] Synchronise the editor's preset box with host-driven program changes via
      a change broadcaster.
- [x] Persist A/B slots and the active slot in the plugin state.
- [x] Implement processor-level A/B comparison with edit capture, slot
      switching and copy-active-to-inactive.
- [x] Add user preset save/overwrite/delete backed by an XML bank in the user
      application data directory (rename is available through the host API).
- [x] Add editor controls for A/B, copy, save and delete.
- [x] Add processor tests for A/B, programs and state.
- [x] Add preset persistence tests using a temporary bank file.

Still to verify under a real host: automation of all 12 parameters, preset
recall and A/B switching while recording automation. This is tracked in
Milestone 4.

## Milestone 3 — GUI completion (in progress)

Completed:

- [x] Value readout showing the preset name, then the dragged parameter value.
- [x] CamelCrusher-inspired dark blue + yellow theme with high-contrast knobs,
      arcs and switches.
- [x] Visible labels for every parameter knob.
- [x] Double-click reset to each parameter's default value.
- [x] Per-parameter padlock buttons that exclude a value from Randomize.
- [x] Randomize now skips on/off and Phat switches and respects locks.
- [x] Lock flags persisted in plugin state.
- [x] Resizable/scalable layout from 80% to 150% with a fixed aspect ratio.
- [x] Module containers dim when bypassed; Master off dims all modules.
- [x] Visual snapshots at multiple sizes via `CRUSHJOINTS_RENDER_UI=1`.

Remaining:

- [ ] Output level meter / clipping indicator fed by the processor.
- [ ] Right-click knob menu with Linear/Circular drag and MIDI Learn.
- [ ] Keyboard navigation and accessibility labels.
- [ ] Pixel-exact CamelCrusher styling (the current theme is inspired, not a
      reproduction).
- [ ] Optional "intelligent" Randomize constraints to avoid extreme settings.

## Milestone 4 — Validation and release (priority 3)

- Extend the test harness (offline render comparison, fuzzing, CPU benchmarks).
- Run `pluginval` at strictness 5+ on the VST3.
- Run `auval` on the AU component.
- Test in at least Reaper (VST3) and Logic Pro (AU); add Ableton if available.
- Add CI (GitHub Actions) for macOS build + pluginval; consider Windows VST3
  and Linux VST3/LV2 later.
- Verify state round-trip, automation and preset recall in each host.
- Keep the clean build warning-free as new code lands.
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

- [x] All four DSP modules implemented and matched against references.
- [x] No pass-through stubs left in the signal chain.
- [x] A/B comparison and a persistent user preset bank implemented.
- [x] Preset, program, A/B and state behaviour covered by offline tests.
- [ ] Presets, state and host automation verified in at least two DAWs.
- [ ] `pluginval` strictness 5+ and `auval` pass.
- [x] VST3, AU and Standalone artefacts built from a clean checkout.
- [ ] README, docs and LICENSE consistent with the implementation.
