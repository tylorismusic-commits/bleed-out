# Changelog

## 0.1.0-alpha.3 — 2026-11-18

- MIDI ghost note/CC no longer writes the Ghost amount parameter, so the knob
  stays where the user set it.
- MIDI velocity now scales the ghost window separately; the Ghost knob remains
  the amount control.
- Added a regression test for velocity-scaled ghost windows.

## 0.1.0-alpha.1 — public alpha — 2026-11-17

- First working Bleed Out prototype: CLAP + LV2, 1040×640 Comp Suite-style
  panel, widened meters and a compact three-knob layout.
- Drum selector: Kick, Snare, Tom, Hat, Ride, Room, OH, Bus.
- Three-knob workflow: Threshold, DeBleed and Ghost. Reduction, decay, attack,
  hold, hysteresis, EQ and saturation are automated by the drum profile.
- De-bleed sidechain gates the high band from the drum body with its own
  one-shot hold/release, so it closes during the hit as well as between hits.
  Measured 34 dB bleed cut between hits and 13 dB during the hit after the
  attack, with body and attack unchanged.
- Ghost-note mode with a Pac-Man-style ghost icon, automatable Ghost amount,
  MIDI CC control and MIDI note-triggered ghost windows. Ghost mode opens the
  de-bleed high band so ghost notes keep their top end.
- Scope, gate envelope, GR and ghost indicators.
- Factory presets, including Ghost Notes.
- DSP, panel, layout, geometry and TTL tests, including a de-bleed efficacy
  regression test.
