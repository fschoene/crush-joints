# CamelCrusher Rebuilt — External Research

Source: [lostlessvisuals/camelcrusher-rebuilt](https://github.com/lostlessvisuals/camelcrusher-rebuilt)
· Apache-2.0 · checked 2026-10-27.

This is the most useful external reference we have found for **legacy
compatibility and the original factory bank**. It is a native Apple Silicon
remake whose primary goal is reopening old CamelCrusher sessions, so it
documents the original host identity and saved-state format rather than trying
to be a clean-room DSP reference.

The repository's own `docs/OPEN_SOURCE_NOTES.md` flags unresolved questions
about bundled artwork and trademark-sensitive naming. Treat facts and metadata
as research input; do not copy assets or the `Camel Audio` identity without a
licence/trademark review.

## 1. Original host identity

| Format | Documented legacy identity |
|--------|----------------------------|
| VST2 | Name `CamelCrusher`; vendor `Camel Audio`; unique ID `CaCr` (`1130447730`); 2 in / 2 out; 17 parameters; 20 programs; version 1; VST version 2400; `AEffect.flags = 57` (editor, CanMono, CanReplacing, ProgramChunks); plug category unknown |
| AU | Type `aufx`; subtype `CcrR`; manufacturer `CmAu`; display name `Camel Audio: CamelCrusher`; component version 8 |
| VST3 | Subcategories `Fx / Distortion / Dynamics / Filter`; the repo uses `CamelCrusher` / `Camel Audio`. The VST3 FUIDs in the repo are not proven to be the original ones. |

The original plugin was found in Live sets at paths such as
`/Library/Audio/Plug-Ins/VST2/CamelCrusher.vst`, but one host test restored
from `~/Library/Audio/Plug-Ins/VST/CamelCrusher.vst`. Their installer keeps
both aliases because hosts and old sessions differ.

**Our position:** CrushJoints is `CrushJoints` with plugin code `CJTS` and
manufacturer code `Crsh`, and 12 public parameters. We cannot get old-session
recall by changing metadata alone; that is a separate same-identity
compatibility target or importer.

## 2. Original parameter surface

The VST2 surface has **17 parameters**: 12 exposed controls and 5 reserved
slots. The order is part of the legacy contract.

| Legacy index | Legacy name | Meaning | CrushJoints equivalent |
|---:|---|---|---|
| 0 | `DistOn` | Distortion module on/off | `tube_on` |
| 1 | `DistMech` | Mech amount | `mech` |
| 2 | `DistTube` | Tube amount | `tube` |
| 3 | `MmFilterOn` | Filter on/off | `lpf_on` |
| 4 | `MmFilterCutoff` | Filter cutoff, normalized | `cutoff` |
| 5 | `MmFilterRes` | Filter resonance, normalized | `res` |
| 6 | `CompressOn` | Compressor on/off | `comp_on` |
| 7 | `CompressAmount` | Compressor amount | `amount` |
| 8 | `CompressMode` | Phat mode (switch-like) | `phat` |
| 9 | `MasterOn` | Global plugin bypass | `master_on` |
| 10 | `MasterMix` | Dry/wet mix | `mix` |
| 11 | `MasterVolume` | Output volume, normalized | `volume` |
| 12–16 | `Unused1`–`Unused5` | Reserved compatibility slots | none |

This confirms two semantics we already implement:

- `CompressMode` is the **Phat** switch, not a continuous compressor mode.
- `MasterOn` is a **whole-plugin bypass**, not just a master-section disable.

Our 12 public parameters map one-to-one semantically, but our IDs and order
differ. The original order above matters if we ever import automation or
legacy state.

## 3. Legacy chunk format

Their decoded Ableton/VST2 state chunks contain **20 program records**:

- `float32` record marker (observed as `1.0`)
- NUL-terminated preset name
- `uint32` parameter count (`17`)
- 17 little-endian `float32` normalized parameter values

The full factory chunk is 1767 bytes. Ableton stores 128 `PluginFloatParameter`
nodes but only the 17 named legacy parameters matter; the rest are blank
placeholders. The saved `ProgramNumber` selects a chunk record, and the
explicit 17-value parameter list normally matches it.

This format is directly useful for a preset importer/exporter or for
fixture-based compatibility tests. It is not required for normal JUCE
VST3/AU state, which we already persist through the APVTS.

## 4. Original 20-program factory bank

These are the original preset names and the raw **normalized VST2 values**
decoded from the factory chunk. `MasterOn` is on in all 20 and the five unused
slots are zero, so neither is shown.

| Preset | Dist | Tube | Mech | Filter | Cutoff | Res | Comp | Amount | Phat | Mix | Vol |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Annihilate | on | 0.340 | 0.000 | off | 0.350 | 0.050 | on | 0.905 | on | 1.000 | 0.780 |
| American Crunch | on | 1.000 | 0.280 | on | 1.000 | 0.475 | on | 0.815 | on | 0.000 | 0.655 |
| American HiGain | on | 1.000 | 1.000 | on | 0.445 | 0.725 | on | 0.815 | on | 1.000 | 0.605 |
| British Clean | on | 0.075 | 0.050 | on | 1.000 | 0.000 | on | 0.710 | off | 0.615 | 0.840 |
| British Crunch | on | 1.000 | 0.280 | on | 1.000 | 0.050 | on | 0.815 | on | 1.000 | 0.645 |
| British HiGain | on | 1.000 | 1.000 | on | 1.000 | 1.000 | on | 1.000 | on | 1.000 | 0.640 |
| Bass Maker | on | 0.625 | 0.020 | on | 0.170 | 0.125 | on | 0.725 | on | 1.000 | 0.775 |
| Clean Rainbow | off | 0.000 | 0.020 | on | 0.825 | 1.000 | on | 0.910 | off | 1.000 | 0.700 |
| Tube Warmth | on | 0.370 | 0.000 | on | 1.000 | 0.190 | off | 0.000 | off | 1.000 | 1.000 |
| Dark Crunch | on | 0.000 | 0.895 | on | 0.945 | 0.170 | on | 0.910 | off | 1.000 | 0.615 |
| Destroyer | on | 0.940 | 0.800 | on | 0.580 | 1.000 | on | 0.910 | on | 1.000 | 0.600 |
| Ultra Phat | on | 0.985 | 0.180 | off | 1.000 | 0.175 | on | 0.670 | off | 1.000 | 0.665 |
| Distort | on | 1.000 | 1.000 | on | 0.690 | 1.000 | on | 0.910 | on | 1.000 | 0.595 |
| Dream | off | 0.000 | 0.020 | on | 0.765 | 0.935 | on | 0.910 | off | 1.000 | 0.650 |
| More Beef | on | 0.200 | 0.000 | off | 0.500 | 0.000 | on | 0.780 | off | 1.000 | 0.760 |
| Santana Row | on | 0.580 | 0.440 | on | 0.615 | 0.935 | off | 0.910 | off | 1.000 | 0.700 |
| Fuzz Filter | on | 0.760 | 0.205 | on | 0.430 | 0.380 | on | 0.433 | on | 0.270 | 1.000 |
| Subtle Master | on | 0.105 | 0.000 | on | 1.000 | 0.095 | on | 0.100 | off | 1.000 | 1.000 |
| YouWereWarned | on | 0.965 | 0.940 | on | 0.710 | 0.345 | on | 0.390 | on | 0.685 | 0.645 |
| Turn it to 11 | on | 1.000 | 1.000 | on | 0.695 | 1.000 | on | 1.000 | on | 1.000 | 0.555 |

Notes and caveats:

- `MasterVolume` is normalized. The reverse-camel research suggests a stepped
  mapping from `-inf` to `0 dB` (roughly `0.0=-inf, 0.1=-55, 0.2=-38, ...,
  1.0=0 dB`). Our volume range is `-24..+12 dB`, so low values would clamp.
- `MmFilterCutoff` is normalized. A plausible mapping is 20 Hz–20 kHz
  exponential; their modern prototype uses 40 Hz–18 kHz. The exact original
  curve is not measured in this repo.
- `American Crunch` has `Mix = 0` in the original chunk, which would be fully
  dry. That looks like an original preset anomaly or an unresolved parameter
  inversion; verify against the original before shipping it as-is.
- Preset names/values are creative data from an Apache-2.0 repository but
  ultimately originate with Camel Audio. Attribution and legal review are
  advisable before shipping the names/artwork.

## 5. What is useful for Version 1

1. **Original 20 factory presets** — adopted in v1 (Milestone 4a). The 20
   original presets now sit in the Legacy/Original category alongside the
   Default preset and 11 grouped CrushClone presets, with provisional
   cutoff/volume mappings. The raw values are above.
2. **Mono input support.** The original VST2 is `CanMono` and supports
   mono-in/stereo-out. Our `isBusesLayoutSupported` currently requires stereo
   in and stereo out. This is scheduled as Milestone 4b in
   [VERSION_1.md](VERSION_1.md).
3. **Host/editor validation lessons.** Their tests cover AU/VST3 editor view
   creation, bundle loading, mono/stereo routing, and `auvaltool` success.
   Useful additions for Milestone 4:
   - `auvaltool -v aufx CJTS Crsh`
   - verify the VST3 editor view is not null in a host
   - test mono/stereo bus negotiation, not only stereo
   - test state/automation/session recall in Reaper and Logic.
4. **Parameter semantics confirmation.** `CompressMode = Phat` and
   `MasterOn = global bypass` match our implementation and are worth keeping
   as regression tests.
5. **Fixture/test ideas.** A small chunk reader plus expected preset values
   would let us compare imported original presets and catch mapping mistakes.

## 6. What is not useful / carries risk

- **Their DSP is not a fidelity reference.** The shared processor is explicitly
  a non-parity approximation (tanh/fold distortion, custom filter/compressor).
  Our reverse-camel tables and CrushClone-derived topology are better grounded.
- **Same-identity VST2 session recall is a separate project.** To reopen old
  Live sets you need the `CaCr` UID, `CamelCrusher` name, 17 parameters, 20
  programs, chunk save/load and specific install paths. That conflicts with our
  current identity and is much larger than a preset import. It belongs in v2 or
  a dedicated compatibility tool, not v1.
- **Original UI assets/geometry.** Their notes on the 345×373 panel, center
  anchors and sprite frame order are useful for v2 pixel-exact styling, but the
  artwork/trademark status is explicitly unresolved in their own docs.
- **Vendored VST3 SDK and VST2 SDK code** has its own licensing and is not
  something to copy into this GPLv3 project.
- **Their factory bank data** should be treated as reference material until
  provenance/licensing is reviewed.

## 7. Suggested next step

The original 20-preset bank is implemented in v1 (Milestone 4a). Mono/stereo
bus support is scheduled as Milestone 4b. Legacy VST2 session compatibility
remains a separate v2/separate-tool idea.

## Sources

- [README](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/README.md)
- [Engineering guide](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/docs/ENGINEERING_GUIDE.md)
- [Learnings](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/docs/LEARNINGS.md)
- [Open-source notes](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/docs/OPEN_SOURCE_NOTES.md)
- [Legacy schema](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/include/camelcrusher_recalled/LegacySchema.h)
- [Legacy parameter descriptors](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/src/core/LegacyParameters.cpp)
- [Decoded factory bank](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/src/core/DefaultLegacyFactoryBank.cpp)
- [Legacy chunk codec](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/src/core/LegacyChunk.cpp)
- [Fixture example](https://github.com/lostlessvisuals/camelcrusher-rebuilt/blob/main/tests/fixtures/catalyst_patron_camelcrusher.json)
