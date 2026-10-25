# CrushJoints Documentation

This folder is the single source of truth for the project's background,
architecture, current status and remaining work. The root `README.md` is kept
short and links here.

## Documents

| Document | Purpose |
|----------|---------|
| [RESEARCH.md](RESEARCH.md) | Background on CamelCrusher, the original DSP, UI and presets |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Framework decision, project layout, parameters, build and GUI architecture |
| [STATUS.md](STATUS.md) | What actually works today, verified build results and known gaps |
| [ROADMAP.md](ROADMAP.md) | Prioritised remaining work, validation plan and future ideas |

## Current snapshot

- **Build:** working. Clean build produces VST3, AU and Standalone on macOS
  with CMake + a local JUCE 9 checkout.
- **DSP:** all four modules implemented (Tube/Mech tables, SVF low-pass,
  one-knob compressor + Phat, master limiter). Offline tests pass, including a
  comparison against the reverse-camel reference path.
- **GUI:** interactive, but value readout, level meter, MIDI Learn, right-click
  knob modes and A/B controls are still missing.
- **Validation:** `pluginval`, `auval` and DAW testing have not been run yet.

See [STATUS.md](STATUS.md) for the detailed breakdown.

## Documentation conventions

- Documents describe the **actual** repository state, not the intended state.
  Planned or missing behaviour is marked explicitly.
- When the original CamelCrusher specification and the current code disagree,
  both values are recorded. The research target lives in
  [RESEARCH.md](RESEARCH.md); the implementation gap lives in
  [STATUS.md](STATUS.md).
- Update these files in the same commit as the behaviour they describe.
