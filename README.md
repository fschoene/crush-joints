# CrushJoints

A JUCE-based clone of the classic CamelCrusher DSP plugin by Camel Audio.

## Overview

CrushJoints aims to be a faithful recreation of the CamelCrusher multi-effect
plugin, featuring:
- Tube and Mech distortion with the original polynomial coefficient tables
- 24dB/octave low-pass filter with resonance
- One-knob compressor with Phat mode
- Master volume and mix controls
- 12 factory presets plus saveable user presets
- A/B parameter comparison with copy-between-slots
- Per-parameter randomize locks with double-click reset
- Resizable dark blue/yellow interface with labelled controls

> **Current status:** Version 1 is feature-complete: full signal chain,
> A/B comparison, user presets and the modern editor are implemented and
> covered by offline tests. The plugin builds as VST3/AU/Standalone. Host
> validation (`pluginval`/`auval`/DAW testing) remains as release work; further
> GUI/workflow items are planned for Version 2. See
> [docs/STATUS.md](docs/STATUS.md), [docs/VERSION_1.md](docs/VERSION_1.md) and
> [docs/VERSION_2.md](docs/VERSION_2.md).

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

- [Research](docs/RESEARCH.md) — CamelCrusher background, DSP and presets
- [Architecture](docs/ARCHITECTURE.md) — build system, signal chain, GUI and parameters
- [Status](docs/STATUS.md) — what works today and known gaps
- [Roadmap](docs/ROADMAP.md) — version navigation and idea backlog
- [Version 1](docs/VERSION_1.md) — finished clone scope and release work
- [Version 2](docs/VERSION_2.md) — planned GUI and workflow upgrades

This project follows a phased development approach described in the roadmap.

## License

This project is licensed under the GPLv3 license - see the LICENSE file for details.