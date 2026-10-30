# CrushJoints

A JUCE-based distortion and compressor plugin, inspired by the classic CamelCrusher by Camel Audio. As I struggled
to get the original to work on my new system, I had to compromise and tinker alot with other options when opening 
older projects. Even then, sometimes the results still left something to be desired, so as an avid enjoyer of the
original, this project was incepted. It has to be stated, that this is no attempt of a complete one-to-one clone
of the plugin, but rather a focused attempt to approximate the original's sounds as close as possible. This allows to 
add some quality-of-life features, while still offering the same sound.  
This project is heavily influenced/inspired by other peoples projects, specifically:
 - **[reverse-camel](https://github.com/soerenbnoergaard/reverse-camel)**: for the black box analysis of the tube and mech distortion modules
 - **[CrushClone](https://github.com/doxiedevops/CrushClone)**: for the general idea of building this (and I snatched their presets)

## Overview

CrushJoints fundamental aim is to be a faithful recreation of the CamelCrusher effect chain. In this regard it is featuring:
- Tube and Mech distortion dials
- 12 dB/octave low-pass filter with resonance (Q 1–7)
- One-knob compressor with Phat mode
- A "Random"-izer button
- Master volume and mix controls
- The 20 presets of the original

Beyond that, CrushJoints also has:
- A/B parameter comparison with copy-between-slots
- A resizable interface
- Per-parameter randomize locks
- 11 additional CrushClone presets
- Native mono/stereo bus support for mono tracks

## Building (MacOS)

### Prerequisites
- Xcode Command Line Tools
- CMake 3.22 or newer
- A JUCE 8/9 source checkout (tested with JUCE 9.0.4)

### Build Steps
1. Clone the repo
2. Point CMake at your JUCE checkout:
   ```sh
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_ROOT=/path/to/JUCE
   ```
3. Build:
   ```sh
   cmake --build build --config Release -j8
   ```

The built artefacts are in `build/CrushJoints_artefacts/Release/`.
Copy the VST3/AU bundles to `~/Library/Audio/Plug-Ins/VST3/` and
`~/Library/Audio/Plug-Ins/Components/` respectively.

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
