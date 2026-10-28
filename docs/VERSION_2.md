# Version 2 — GUI and workflow upgrades

> **Status:** planned, not started. Back to the [roadmap](ROADMAP.md).

Version 2 starts from the GUI work that was still open when the v1 clone was
declared feature-complete. These items are intentionally outside the v1 scope.

## Initial scope

- [ ] Output level meter / clipping indicator fed by the processor.
- [ ] Right-click knob menu with Linear/Circular drag and MIDI Learn.
- [ ] Keyboard navigation and accessibility labels.
- [ ] Pixel-exact CamelCrusher styling (the current theme is inspired, not a
      reproduction).
- [ ] Optional "intelligent" Randomize constraints to avoid extreme settings.
- [ ] Expose the **Boom** compressor mode (see below): parameter, editor control,
      state and preset support, and a decision on its final name.

## Boom mode (hidden compressor option)

CrushJoints' original Phat design (a 120 Hz +8 dB low shelf feeding a heavy
parallel compressor that is blended in more strongly as Amount rises) does not
exist in the original CamelCrusher: measurements show that the real Phat mode only
changes the detector and release timing (see [MEASUREMENTS.md](MEASUREMENTS.md)
section 6). The design is worth keeping as an extra, so it lives on as a hidden
stage of `Compressor` called **Boom** (working name; "Cloud" was the other
candidate):

- `Compressor::setBoomMode(bool)` switches it on at DSP level; there is no
  APVTS parameter, no editor control and no state entry yet.
- It runs *behind* the main compressor: the compressed signal is low-shelf
  boosted (120 Hz, +8 dB), squashed by a fixed heavy compressor (-18 dB, 6:1,
  5 ms / 100 ms) and crossfaded back in (30 % to 50 % with Amount, 20 ms fade).
  Feeding it from the compressor output rather than the input was a deliberate
  change: the measured compressor can add up to +40 dB, so a leg fed from the
  input would sit far below the main path and never be heard. At Amount 0 the
  main path is unity, so Boom then simply adds the shelved bass.
- `Tests/DSPTests.cpp` (`testCompressorBoom`) checks that it is off by default,
  stays finite and adds low-end level when on.
- Open design questions for the exposure work: an extra toggle versus a third
  Phat state, its threshold and ratio against typical program material, preset
  storage, and whether it should follow Amount or have its own control.

## Notes

- This is the committed initial V2 scope. Additional features will only be
  added by promoting items from the [roadmap backlog](ROADMAP.md#idea-backlog).
- V2 work starts after the Milestone 4 validation/release pass, unless
  priorities change.
- The output meter needs a small processor-side level report; MIDI Learn needs
  a persistent mapping model; accessibility needs labels and keyboard focus
  order. These should be designed before implementation.
