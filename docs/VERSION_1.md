# Version 1 — Faithful CamelCrusher clone + modern workflow

> **Status:** feature-complete for the clone scope; the Milestone 4 release
> work is still open. Back to the [roadmap](ROADMAP.md).

Version 1 is the CamelCrusher clone itself, plus the workflow features that
made it practical to use and test:

- the full signal chain (Tube/Mech distortion, SVF filter, custom one-knob
  compressor with Phat, master section),
- 12 factory presets and disk-backed user presets,
- A/B comparison with copy-between-slots,
- a modern resizable editor with visible labels, dark blue/yellow theme,
  value readout, double-click reset and randomize locks.

## Scope boundary

Anything that is not required to call this a faithful, usable CamelCrusher
clone is deferred. The remaining Milestone 3 GUI work now lives in
[Version 2](VERSION_2.md); unscheduled ideas stay in the
[roadmap backlog](ROADMAP.md#idea-backlog).

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

## Milestone 3 — GUI completion (v1 scope, completed)

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

The remaining GUI work was moved to [Version 2](VERSION_2.md).

## Milestone 4 — Validation and release (pending)

**Status:** not started.

- Extend the test harness (offline render comparison, fuzzing, CPU benchmarks).
- Run `pluginval` at strictness 5+ on the VST3.
- Run `auval` on the AU component.
- Test in at least Reaper (VST3) and Logic Pro (AU); add Ableton if available.
- Consider CI (GitHub Actions) for macOS build + pluginval; consider Windows VST3
  and Linux VST3/LV2 later.
- Verify state round-trip, automation and preset recall in each host.
- Keep the clean build warning-free as new code lands.
- Add a LICENSE file: the README claims GPLv3 but the repository currently has
  no `LICENSE` file.

## Definition of done for v1.0

- [x] All four DSP modules implemented with documented provenance: reference
      tables for distortion and a documented custom compressor design.
- [x] No pass-through stubs left in the signal chain.
- [x] A/B comparison and a persistent user preset bank implemented.
- [x] Preset, program, A/B and state behaviour covered by offline tests.
- [ ] Presets, state and host automation verified in at least two DAWs.
- [ ] `pluginval` strictness 5+ and `auval` pass.
- [x] VST3, AU and Standalone artefacts built from a clean checkout.
- [ ] README, docs and LICENSE consistent with the implementation.

