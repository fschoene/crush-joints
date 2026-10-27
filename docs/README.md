# CrushJoints Documentation

This folder is the single source of truth for the project's background,
architecture, current status and remaining work. The root `README.md` is kept
short and links here.

## Documents

| Document | Purpose |
|----------|---------|
| [RESEARCH.md](RESEARCH.md) | Background on CamelCrusher, the original DSP, UI and presets |
| [CAMELCRUSHER_REBUILT.md](CAMELCRUSHER_REBUILT.md) | External research: legacy identity, chunk format and original 20-preset bank |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Framework decision, project layout, parameters, build and GUI architecture |
| [STATUS.md](STATUS.md) | What actually works today, verified build results and known gaps |
| [ROADMAP.md](ROADMAP.md) | Version navigation, current focus and the unscheduled idea backlog |
| [VERSION_1.md](VERSION_1.md) | Finished clone scope plus the open v1 validation/release work |
| [VERSION_2.md](VERSION_2.md) | Planned GUI and workflow upgrades moved out of the v1 scope |

## Current snapshot

- **Build:** working. Clean build produces VST3, AU and Standalone on macOS
  with CMake + a local JUCE 9 checkout, including mono and stereo bus layouts.
- **DSP:** all four modules implemented (Tube/Mech tables, SVF low-pass,
  one-knob compressor + Phat, master limiter). Offline DSP and processor tests
  pass, including a comparison against the reverse-camel reference path.
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

- Documents describe the **actual** repository state, not the intended state.
  Planned or missing behaviour is marked explicitly.
- When the original CamelCrusher specification and the current code disagree,
  both values are recorded. The research target lives in
  [RESEARCH.md](RESEARCH.md); the implementation gap lives in
  [STATUS.md](STATUS.md).
- Update these files in the same commit as the behaviour they describe.
