# CrushJoints Documentation

This folder is the single source of truth for the project's background,
architecture, current status and remaining work. The root `README.md` is kept
short and links here.

## Documents

| Document | Purpose |
|----------|---------|
| [RESEARCH.md](RESEARCH.md) | Background on CamelCrusher, the original DSP, UI and presets |
| [MEASUREMENTS.md](MEASUREMENTS.md) | Black-box measurements of the original plugin (ground truth), made with `tools/analysis/` |
| [CAMELCRUSHER_REBUILT.md](CAMELCRUSHER_REBUILT.md) | External research: legacy identity, chunk format and original 20-preset bank |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Framework decision, project layout, parameters, build and GUI architecture |
| [STATUS.md](STATUS.md) | What actually works today, verified build results and known gaps |
| [ROADMAP.md](ROADMAP.md) | Version navigation, current focus and the unscheduled idea backlog |
| [VERSION_1.md](VERSION_1.md) | Finished clone scope plus the open v1 validation/release work |
| [VERSION_2.md](VERSION_2.md) | Planned GUI and workflow upgrades moved out of the v1 scope |

## Current snapshot

- **Build:** working. Clean build produces VST3, AU and Standalone on macOS
  with CMake + a local JUCE 9 checkout, including mono and stereo bus layouts.
- **DSP:** all four modules implemented as measured models of the original
  (Mech → Filter → Tube → Compressor → Master): reverse-camel Tube/Mech tables,
  2-pole low-pass with the original cutoff curve, the original's one-knob
  compressor and Phat mode, hard clip at ±0.9999. Offline DSP and processor
  tests pass, including comparisons with values recorded from the original.
  Known gap: the original's filter output limiter.
- **Presets/state:** Default at the top outside the groups, plus 20 original
  Legacy/Original and 11 CrushClone presets grouped with a User category;
  disk-backed user presets (save,
  overwrite, delete in the editor), real program API, A/B snapshots and full
  state persistence.
- **GUI:** interactive and resizable, with labelled knobs, a dark blue/yellow
  theme, A/B, Save/Delete, preset sync, a value readout, double-click reset,
  per-parameter Randomize locks and bypass dimming (Master off dims all
  modules). Output meter, MIDI Learn and right-click knob modes are still
  missing.
- **Validation:** `pluginval`, `auval` and DAW testing have not been run yet.
  This is the remaining Version 1 work; see [VERSION_1.md](VERSION_1.md).
- **Version 2:** planned GUI/workflow upgrades. See
  [VERSION_2.md](VERSION_2.md).

See [STATUS.md](STATUS.md) for the detailed breakdown.

## Documentation conventions

- Measured behaviour of the original plugin ([MEASUREMENTS.md](MEASUREMENTS.md))
  outranks secondary sources; if another document disagrees, fix that document.
- Documents describe the **actual** repository state, not the intended state.
  Planned or missing behaviour is marked explicitly.
- When the original CamelCrusher specification and the current code disagree,
  both values are recorded. The research target lives in
  [RESEARCH.md](RESEARCH.md); the implementation gap lives in
  [STATUS.md](STATUS.md).
- Update these files in the same commit as the behaviour they describe.
