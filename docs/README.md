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

- **Build:** working. VST3, AU and Standalone are produced on macOS with
  CMake + a local JUCE 9 checkout.
- **Runtime:** the Standalone app launches and the editor is interactive.
- **DSP:** the signal chain is wired but every DSP module is still a
  pass-through stub. The plugin currently does not colour the audio.
- **Validation:** no pluginval/AU validation or DAW testing has been run yet.

See [STATUS.md](STATUS.md) for the detailed breakdown.

## Documentation conventions

- Documents describe the **actual** repository state, not the intended state.
  Planned or missing behaviour is marked explicitly.
- When the original CamelCrusher specification and the current code disagree,
  both values are recorded. The research target lives in
  [RESEARCH.md](RESEARCH.md); the implementation gap lives in
  [STATUS.md](STATUS.md).
- Update these files in the same commit as the behaviour they describe.
