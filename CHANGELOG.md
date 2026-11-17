# Changelog

## 0.1.0 — 2026-11-17

- First working Bleed Out prototype: CLAP + LV2, 1040×720 Comp Suite-style
  panel.
- Drum selector: Kick, Snare, Tom, Hat, Ride, Room, OH, Bus.
- Three-knob workflow: Threshold, DeBleed and Ghost. Reduction, decay, attack,
  hold, hysteresis, EQ and saturation are automated by the drum profile.
- De-bleed sidechain gates the high band from the drum body with its own fast
  hold/release. Measured 36 dB bleed cut between hits with body and attack
  unchanged.
- Ghost-note mode with a Pac-Man-style ghost icon, automatable Ghost amount,
  MIDI CC control and MIDI note-triggered ghost windows. Ghost mode opens the
  de-bleed high band so ghost notes keep their top end.
- Scope, gate envelope, GR and ghost indicators.
- Factory presets, including Ghost Notes.
- DSP, panel, layout, geometry and TTL tests, including a de-bleed efficacy
  regression test.
