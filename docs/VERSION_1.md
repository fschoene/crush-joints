# Version 1 — Faithful CamelCrusher clone + modern workflow

> **Status:** the v1 core is feature-complete; only the Milestone 4 release
> validation work remains open. Back to the [roadmap](ROADMAP.md).

Version 1 is the CamelCrusher clone itself, plus the workflow features that
made it practical to use and test:

- the full signal chain (Tube/Mech distortion, SVF filter, custom one-knob
  compressor with Phat, master section),
- the Default preset at the top outside the groups, plus the original
  20-program CamelCrusher factory bank and 11 CrushClone presets organized into
  **Legacy / Original**, **CrushClone** and **User** groups (Milestone 4a,
  completed),
- disk-backed user presets (the User group starts empty),
- native mono/stereo bus support for mono tracks (Milestone 4b, implemented),
- A/B comparison with copy-between-slots,
- a modern resizable editor with visible labels, dark blue/yellow theme,
  value readout, double-click reset and randomize locks.

## Scope boundary

Anything that is not required to call this a faithful, usable CamelCrusher
clone is deferred. The remaining Milestone 3 GUI work now lives in
[Version 2](VERSION_2.md); unscheduled ideas stay in the
[roadmap backlog](ROADMAP.md#idea-backlog). The original factory bank and
preset categories (Milestone 4a) and mono/stereo bus compatibility (Milestone
4b) are implemented. Only the Milestone 4 release validation remains.

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

## Milestone 4a — Original preset bank and categories (v1 scope, completed)

**Status:** completed.

- [x] Add the original 20-program CamelCrusher factory bank decoded in
      [CAMELCRUSHER_REBUILT.md](CAMELCRUSHER_REBUILT.md) (re-verified against
      the original `.fxb` on 2026-10-28).
- [x] Map legacy normalized cutoff and master-volume values to our parameter
      ranges; keep tube/mech/res/amount/mix and switch values direct.
- [x] Keep the current 11 CrushClone presets as a separate group, with Default
      outside the groups at the top of the selector.
- [x] Group presets in the selector as:
      **Legacy / Original** (provisional label; UI copy can be renamed),
      **CrushClone** (the current 11 grouped presets),
      **User** (empty until the user saves a preset).
- [x] Ensure saving a user preset adds it to the User group; deleting removes
      it.
- [x] Keep group headings display-only; program indices remain stable and
      existing program/state behaviour is preserved.
- [x] **American Crunch / British Clean mix:** both are 100% wet in the
      original `CamelCrusherPresetsA.fxb`. The earlier "Mix = 0" override was a
      data error in the external research and was removed (2026-10-28).
- [x] Tests:
      all 20 original presets load with expected converted values,
      Default sits outside the groups and the other groups have the expected
      contents, User starts empty, and user save/delete and host program names
      still work.
- [x] Document the chosen cutoff and volume mappings as provisional estimates.

## Milestone 4b — Mono/stereo bus compatibility (v1 scope, completed)

**Status:** implementation, offline tests and host verification complete.

- [x] Accept and test these layouts:
      mono in → mono out,
      stereo in → stereo out (unchanged),
      mono in → stereo out (centred stereo output, matching the original
      `CanMono` behaviour).
- [x] Update `isBusesLayoutSupported` to allow those combinations.
- [x] Make `processBlock` channel-count aware: mono→mono processes one channel,
      mono→stereo duplicates the mono input to both channels before the dry
      capture and DSP chain, stereo→stereo keeps the existing behaviour.
- [x] Make `prepareToPlay`, the dry buffer, filter, compressor and master use
      the negotiated channel count instead of assuming two.
- [x] Add offline tests for every supported layout, including level
      preservation, centred mono→stereo output and stereo independence.
- [x] Verify mono VST3 and AU in a host — passed in Reaper.
- Acceptance: mono works without host duplication and stereo results are
  unchanged in the offline tests.

## Milestone 4c — Measurement-driven fidelity pass (v1 scope, in progress)

**Status:** started 2026-10-28. Source of truth:
[MEASUREMENTS.md](MEASUREMENTS.md), produced with
[`tools/analysis/`](../tools/analysis/README.md).

- [x] Black-box harness: offline x86_64 VST2 host plus Python scripts.
- [x] Distortion order: Mech → Tube (measured), with a test against recorded
      original outputs.
- [x] Output clip: hard clip at ±0.9999 (measured).
- [ ] Cutoff: 80 Hz–16 kHz with the measured curve; fix the legacy preset
      conversion.
- [ ] Volume: −∞…0 dB with the measured table; decide how to treat the
      CrushClone presets that use positive gain.
- [ ] Measure and match the filter (response, resonance, level dependence).
- [ ] Measure and match the compressor and Phat.
- [ ] Add regression tests that compare against recorded measurements.

## Milestone 4 — Validation and release (pending)

**Status:** not started.

- Extend the test harness (offline render comparison, fuzzing, CPU benchmarks).
- Run `pluginval` at strictness 5+ on the VST3.
- Run `auval` on the AU component.
- Test in at least Reaper (VST3) and Logic Pro (AU); add Ableton if available.
- Consider CI (GitHub Actions) for macOS build + pluginval; consider Windows VST3
  and Linux VST3/LV2 later.
- Verify state round-trip, automation, original/CrushClone/User preset recall,
  category behaviour and mono/stereo bus negotiation in each host.
- Keep the clean build warning-free as new code lands.
- Add a LICENSE file: the README claims GPLv3 but the repository currently has
  no `LICENSE` file.

## Definition of done for v1.0

- [x] All four DSP modules implemented with documented provenance: reference
      tables for distortion and a documented custom compressor design.
- [x] No pass-through stubs left in the signal chain.
- [x] A/B comparison and a persistent user preset bank implemented.
- [x] Preset, program, A/B and state behaviour covered by offline tests.
- [x] Original 20-program factory bank integrated and verified.
- [x] Preset groups Legacy/Original, CrushClone and User implemented.
- [x] Mono/mono, stereo/stereo and mono/stereo bus layouts supported, covered
      by offline tests and verified in Reaper for VST3 and AU.
- [ ] Presets, state and host automation verified in at least two DAWs.
- [ ] `pluginval` strictness 5+ and `auval` pass.
- [x] VST3, AU and Standalone artefacts built from a clean checkout.
- [ ] README, docs and LICENSE consistent with the implementation.

