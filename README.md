# CrushJoints

A JUCE-based clone of the classic CamelCrusher DSP plugin by Camel Audio.

## Overview

CrushJoints aims to be a faithful recreation of the CamelCrusher multi-effect
plugin, featuring:
- Tube and Mech distortion with the original polynomial coefficient tables
- 24dB/octave low-pass filter with resonance
- One-knob compressor with Phat mode
- Master volume and mix controls
- Full preset system with 12 built-in presets

> **Current status:** the plugin builds as VST3/AU/Standalone and the editor is
> interactive, but the DSP modules are still pass-through stubs, so audio is not
> processed yet. See [docs/STATUS.md](docs/STATUS.md) for details.

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

## Development

Project documentation lives in [docs/](docs/):

- [Research](docs/RESEARCH.md) — CamelCrusher background, DSP and presets
- [Architecture](docs/ARCHITECTURE.md) — build system, signal chain, GUI and parameters
- [Status](docs/STATUS.md) — what works today and known gaps
- [Roadmap](docs/ROADMAP.md) — remaining work, validation and ideas

This project follows a phased development approach described in the roadmap.

## License

This project is licensed under the GPLv3 license - see the LICENSE file for details.