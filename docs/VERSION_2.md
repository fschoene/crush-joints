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

## Notes

- This is the committed initial V2 scope. Additional features will only be
  added by promoting items from the [roadmap backlog](ROADMAP.md#idea-backlog).
- V2 work starts after the Milestone 4 validation/release pass, unless
  priorities change.
- The output meter needs a small processor-side level report; MIDI Learn needs
  a persistent mapping model; accessibility needs labels and keyboard focus
  order. These should be designed before implementation.
