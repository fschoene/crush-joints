# Background and reference

Background on CamelCrusher and the sources this project draws on. It is not a
specification: measured behaviour of the original lives in
[MEASUREMENTS.md](MEASUREMENTS.md) and always wins, and the current
implementation state is in [STATUS.md](STATUS.md).

## 1. Overview

**CamelCrusher** is a free multi-effect plugin by **Camel Audio** (concept,
design and programming credited to Ben Gillett in the manual). The manual is
"Version 1.0", © 2005; the original VST bundle is 1.01 (2006) and the Mac
installer is labelled 1.4 (2011). It was the "little brother" of the paid
**CamelPhat** distortion/compressor. Camel Audio was acquired by Apple in 2015
and the plugins were discontinued; CamelPhat's technology was folded into Logic
Pro's **Phat FX**.

CamelCrusher is a single-window, four-module colouring effect: intentionally
simple, with one knob per parameter. Its character comes from the interaction
between Tube distortion, Mech distortion, a low-pass filter and a one-knob
compressor with Phat mode. The manual counts three effect modules (Distortion,
Filter, Compressor) plus the Master module.

## 2. Original identity and formats

Facts below come from the original installer and binary, not from a remake.

| Format | Identity |
|--------|----------|
| VST2 | Name `CamelCrusher`, vendor `Camel Audio`, unique ID `CaCr`, version 1, VST 2400, flags 57 (editor, CanMono, CanReplacing, ProgramChunks), 2 in / 2 out, **17 parameters** (12 used, 5 reserved), **20 programs** |
| AU | VST-to-AU wrapper (`CVST2AUPluginEntry`), type `aumf` (music effect), subtype `CaCr`, manufacturer `CamA`, bundle version 1.01 |
| RTAS | Included in the 2011 Mac installer |
| VST3 / AAX | Not shipped |

The Mac binaries are universal (`ppc7400`, `i386`, `x86_64`); there is no arm64
slice. The 2011 installer places the VST at
`/Library/Audio/Plug-Ins/VST/CamelCrusher.vst`, the AU at
`/Library/Audio/Plug-Ins/Components/CamelCrusher.component` and the data
(manual, `CamelCrusherPresetsA.fxb`, skins) under
`/Library/Application Support/Camel Audio/CamelCrusherData/`.

CrushJoints uses its own identity (`CrushJoints`, code `CJTS`, manufacturer
`Crsh`) and 12 public parameters. Reopening old sessions by identity is a
separate compatibility project, listed in the [roadmap backlog](ROADMAP.md).

## 3. Signal chain

The manual does not state the module order. Measurement shows **Mech → Filter →
Tube → Compressor → Master**, so the filter sits between the two distortion
stages. Each module has its own on/off toggle; **Master** off bypasses the whole
plugin and passes the dry input through, and the Distortion switch covers both
Tube and Mech. The ordering evidence is in
[MEASUREMENTS.md](MEASUREMENTS.md) §7; the implementation is in
[ARCHITECTURE.md](ARCHITECTURE.md).

## 4. The four modules

### Distortion

| Control | Range | Default | Manual description |
|---------|-------|---------|--------------------|
| TUBE | 0–100 | 0 | "a warm and musical, analogue-style overdrive" |
| MECH | 0–100 | 0 | "a bit nastier (hint: it's great for booming kick drums)" |

Tube and Mech are not exclusive and can be blended; most of CamelCrusher's
character lives in that blend. The manual gives no ranges or defaults; 0–100
matches the `%` readout.

The waveshapers are modelled from the **reverse-camel** coefficient tables:
101 entries per stage, each a gain plus polynomial coefficients, with a linear
region and asymmetric quadratic (Tube) or linear (Mech) saturation outside it.
Our own measurements of the original agree with those tables to ≤ 0.0008 RMS
error ([MEASUREMENTS.md](MEASUREMENTS.md) §4), so the tables are a good model of
the real transfer curves. High settings raise the level considerably and need
Master Volume compensation. The original does not oversample; see
[STATUS.md](STATUS.md).

### Filter

| Control | Range | Default | Manual description |
|---------|-------|---------|--------------------|
| CUTOFF | 80 Hz–16 kHz, custom curve (measured) | open (16 kHz) | "sets the filter Cutoff point, in Hz" |
| RES | 0–100 % | 0 | resonance around the cutoff |

The manual calls it a "warm, smooth analogue-style low-pass filter, with buckets
of resonance" but gives no slope or range. Measurement shows a **linear 2-pole
(12 dB/octave) low-pass with `Q = 1 + 6·Res`** (Q 1–7, +16.9 dB peak at maximum
resonance, no passband loss), with the displayed Hz as the real cutoff. The
24 dB/octave figure from reviews is wrong. The filter's job in the effect is to
tame the harmonics created by the distortion. See
[MEASUREMENTS.md](MEASUREMENTS.md) §5, including the output limiter that
CrushJoints does not implement yet.

### Compressor

| Control | Range | Default | Manual description |
|---------|-------|---------|--------------------|
| AMOUNT | 0–100 | 0 | "the amount of compression applied to the sound, and thus the apparent 'loudness'" |
| PHAT MODE | On/Off | Off | punchier mode; the manual advises turning it off for clean signals to avoid "occasional crackles at high levels" |

The manual describes a "sophisticated and responsive 'soft-limiting' compressor
algorithm, modelled after the classic analogue compressor designs renowned in
studio folklore". The measured behaviour is a stereo-linked slew-limited
follower feeding a parabolic gain computer with a fast release ramp, with
0…+40 dB of makeup; Phat replaces the follower with the instantaneous magnitude.
Phat has no bass shelf and no parallel path in the original. The full model is
in [MEASUREMENTS.md](MEASUREMENTS.md) §6.

### Master

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| ON/OFF | Toggle | On | bypasses the entire plugin |
| VOLUME | −∞…0 dB (measured) | 0 (unity) | output level, shown in dB |
| MIX | 0–100 % | 100 % (wet) | dry/wet blend |

The original hard-clips at ±0.9999 after volume and mix when Master is on;
Master off is unclipped. Volume has no positive gain (gain = knob^2.737). Mix is
arguably the most important control; 30–60 % often sounds better than fully wet.
Volume and Mix ramp linearly over 10 ms in the knob value. CrushJoints keeps a
wider −24…+12 dB volume range as a deliberate deviation; see
[STATUS.md](STATUS.md).

## 5. Original UI and preset banks

The original panel is **345×373 px**, with Tube left of Mech, Cutoff left of
Res, and Volume left of Mix. Features:

- **Patch selector** with up/down navigation and the current preset name.
- **Randomize** across the active modules.
- **On/off switches** at the top-left of each module, illuminated when active.
- **Value readout** showing the preset name, or the value while a knob moves;
  clicking it opens Load/Save Preset, Load/Save Bank, Load Bank A/B and Clear
  Preset.
- **Output level meter** that indicates clipping.
- **Knob modes:** right-click menu with Display Value, MIDI Learn, Linear and
  Circular; Shift-drag for fine control.
- **Presets:** the Mac installer ships **Bank A** with 20 programs
  (`CamelCrusherPresetsA.fxb`). The manual describes two default banks and saved
  banks of 64 sounds; Bank B is not in the installer. `.FXP`/`.FXB` import and
  export are supported.

## 6. Presets in CrushJoints

- **Legacy / Original (20):** the original Bank A programs, verified value for
  value against `CamelCrusherPresetsA.fxb`. Cutoff and volume are converted with
  the measured knob laws. The values live in `Source/Common/Presets.cpp`.
- **CrushClone (11):** additional presets taken from the CrushClone project as a
  starting point and kept as a separate group. They are not original factory
  presets. The values live in the same file.
- **Default:** CrushJoints' own neutral preset, outside the groups at the top.
- **User:** saved by the user, stored as XML in the user application data
  directory.

## 7. References

1. **Reverse Camel** (soerenbnoergaard) — DPF/C++ black-box analysis of the
   original. Its amplitude-sweep and curve-fitting approach was the inspiration
   for `tools/analysis/`, and its distortion coefficient tables are the model
   used here. Our independent measurements of the original confirmed those
   tables ([MEASUREMENTS.md](MEASUREMENTS.md) §4). It implements only
   Distortion and Master.
2. **CrushClone** (doxiedevops) — JUCE recreation and an inspiration for the
   project. It is the source of the 11 additional presets. Its DSP (tanh/cubic
   waveshapers, 4× oversampling, SVF filter, parallel-compression Phat) is not a
   fidelity reference; the measurements above are.
3. **Logic Pro Phat FX** — Apple's integration of CamelPhat, the paid sibling.
   Architecturally related but not identical.
4. **The original plugin, manual and installer** — used for the product facts
   above and as the subject of every measurement in
   [MEASUREMENTS.md](MEASUREMENTS.md).
