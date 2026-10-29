# CrushJoints

A JUCE-based clone of the classic CamelCrusher DSP plugin by Camel Audio.

## Overview

CrushJoints aims to be a faithful recreation of the CamelCrusher multi-effect
plugin, featuring:
- Tube and Mech distortion with the original polynomial coefficient tables
- 12 dB/octave low-pass filter with resonance (Q 1–7)
- One-knob compressor with Phat mode (a measured model of the original)
- Master volume and mix controls
- Default preset at the top, plus 20 original and 11 CrushClone presets grouped into Legacy/Original, CrushClone and User categories
- Saveable user presets in a separate User group
- A/B parameter comparison with copy-between-slots
- Per-parameter randomize locks with double-click reset
- Native mono/stereo bus support for mono tracks
- Resizable dark blue/yellow interface with labelled controls

> **Current status:** Version 1 is feature-complete and validated with
> `pluginval`, `auval` and a manual Reaper pass (automation, presets, project
> reload, A/B, bypass). The remaining release work is a second DAW (Logic is not
> installed here) and optional CI. Version 2 covers GUI and workflow upgrades.
> See [docs/STATUS.md](docs/STATUS.md) and [docs/ROADMAP.md](docs/ROADMAP.md).

## Building

### Prerequisites
- Xcode Command Line Tools
- CMake 3.22 or newer (install via Homebrew: `brew install cmake`)
- A JUCE 8/9 source checkout (tested with JUCE 9.0.4)

### Build Steps
1. Clone the repository
2. Point CMake at your JUCE checkout (defaults to `/Volumes/WDB/user/flo/JUCE`):
   ```sh
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
   ```
3. Build:
   ```sh
   cmake --build build --config Release -j8
   ```

On macOS the built artefacts are in `build/CrushJoints_artefacts/Release/`:

- `VST3/CrushJoints.vst3`
- `AU/CrushJoints.component`
- `Standalone/CrushJoints.app`

Copy the VST3/AU bundles to `~/Library/Audio/Plug-Ins/VST3/` and
`~/Library/Audio/Plug-Ins/Components/` respectively, or run the standalone app
directly.

Run the offline tests with:

```sh
./build/CrushJointsTests_artefacts/Release/CrushJointsTests
./build/CrushJointsProcessorTests_artefacts/Release/CrushJointsProcessorTests
```

User presets are saved to
`~/Library/Application Support/CrushJoints/Presets.xml` on macOS.

## Development

Project documentation lives in [docs/](docs/):

- [Background](docs/RESEARCH.md) — CamelCrusher history, manual, original UI and sources
- [Measurements](docs/MEASUREMENTS.md) — measured behaviour of the original (ground truth)
- [Architecture](docs/ARCHITECTURE.md) — build, signal chain, parameters, presets, state and editor
- [Status](docs/STATUS.md) — tests, validation results, known gaps and deviations
- [Roadmap](docs/ROADMAP.md) — version scope, milestones and idea backlog

This project follows a phased development approach described in the roadmap.

## License

This project is licensed under the GPLv3 license - see the LICENSE file for details.