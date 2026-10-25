# CrushJoints

A JUCE-based clone of the classic CamelCrusher DSP plugin by Camel Audio.

## Overview

CrushJoints is a faithful recreation of the CamelCrusher multi-effect plugin, featuring:
- Tube and Mech distortion with original polynomial coefficient tables
- 24dB/octave low-pass filter with resonance
- One-knob compressor with Phat mode
- Master volume and mix controls
- Full preset system with 12 built-in presets

## Building

### Prerequisites
- Xcode Command Line Tools
- CMake (install via Homebrew: `brew install cmake`)

### Build Steps
1. Clone the repository
2. Run `cmake -B build`
3. Run `cmake --build build`

The built plugin will be in `build/bin/` or `build/CrushJoints.artifact/` depending on your platform.

## Development

This project follows a phased development approach detailed in PLAN.md.

## License

This project is licensed under the GPLv3 license - see the LICENSE file for details.