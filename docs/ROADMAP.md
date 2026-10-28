# Roadmap

This file is the navigation layer for CrushJoints development. Detailed
version plans live in [VERSION_1.md](VERSION_1.md) and
[VERSION_2.md](VERSION_2.md); current implementation status is in
[STATUS.md](STATUS.md) and the reference behaviour is in
[RESEARCH.md](RESEARCH.md).

## Versions

| Version | Status | Summary | Details |
|---------|--------|---------|---------|
| **Version 1** | Core feature-complete; release validation pending | Faithful CamelCrusher clone plus the workflow features needed to use it: full DSP chain, the original 20-program preset bank plus CrushClone presets, user presets, mono/stereo host support, A/B and a modern resizable editor | [VERSION_1.md](VERSION_1.md) |
| **Version 2** | Planned, not started | Remaining GUI/workflow work: output meter, knob modes/MIDI Learn, accessibility, styling and smarter Randomize | [VERSION_2.md](VERSION_2.md) |

### Version 1 at a glance

- Tube + Mech distortion using the reverse-camel coefficient tables.
- 24 dB/oct SVF low-pass filter with resonance.
- One-knob compressor with Phat parallel path.
- Master volume, mix, safety limiter and full bypass.
- Default preset at the top outside the groups, plus the original 20-program
  factory bank and 11 CrushClone presets organized into Legacy/Original,
  CrushClone and User groups.
- Saveable user presets; the User group starts empty.
- Native mono/stereo bus support for mono tracks.
- A/B comparison with copy-between-slots.
- Resizable dark blue/yellow editor with labels, locks, readout and
  double-click reset.
- **Open:** Milestone 4 host validation and release work.

### Version 2 at a glance

- Output level meter / clipping indicator.
- Right-click knob menu with Linear/Circular drag and MIDI Learn.
- Keyboard navigation and accessibility labels.
- Pixel-exact or otherwise finalised visual styling.
- Optional "intelligent" Randomize constraints.

## Current focus

1. Finish Version 1: complete Milestone 4 validation and release work.
2. Then start Version 2 from its initial scope.
3. Promote backlog items into a version plan only when they are deliberately
   scheduled.

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

## Idea backlog

Unscheduled ideas. None of these are commitments. To schedule one, move it
into [VERSION_1.md](VERSION_1.md) or [VERSION_2.md](VERSION_2.md) and add the
appropriate acceptance criteria.

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
    the output level tracks the input level;
  - show the applied gain (and ideally the input/output levels) in the readout.
- Start with the simple, well-defined version: peak or RMS detection over a
  short window with smooth attack/release to avoid pumping.
- Optional second mode: perceived-loudness matching using a K-weighted
  short-term loudness estimate, exposed as a separate "Gain Match Style"
  option. This is more work but closer to what users actually hear.
- Decisions to settle before implementation:
  - measurement points: input before the effect chain vs after Mix, and
    whether the safety limiter is included in the measurement;
  - interaction with Mix, Master bypass, compressor/Phat makeup and the ±2.0
    limiter;
  - mono/stereo linking, latency, smoothing time and maximum correction range;
  - whether Gain Match is an automatable parameter and whether Volume
    automation is ignored while it is on;
  - UI: greyed volume knob, applied-gain readout, optional small level meter;
  - tests: deterministic static gain, silence/noise, stereo, limiter, bypass,
    automation and state restore.
- Priority: unscheduled. Revisit after Version 1 release work and the
  initial Version 2 GUI scope. Not a v1.0 blocker; the original CamelCrusher
  has no such feature.

### Legacy compatibility and reference data

- Original parameter order/IDs and legacy chunk import/export (v2 or separate
  compatibility work).
- Same-identity VST2 session recall (`CaCr`, 17 parameters, 20 programs,
  chunk state) — larger separate compatibility project. The original AU is
  `aumf` / `CaCr` / `CamA`.
- Original UI geometry/assets for pixel-exact styling (v2; artwork and
  trademark review required).

### Workflow

- Preset search/filter and comparison.
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

