# Bleed Out

**Version 0.1.0-alpha.4** · GPL-3.0-or-later

> **Public alpha.** The de-bleed gate is usable and measured; factory presets
> and final tuning are planned after feedback. Expect changes.
>
> [Download v0.1.0-alpha.2](https://github.com/tylorismusic-commits/bleed-out/releases/tag/v0.1.0-alpha.4)

A multiband de-bleed gate for drums, built in the same visual language as Comp
Suite and the rest of the Suite series. It removes hi-hat and cymbal bleed while
keeping the attack, body and tone of the drum.

This is the public-alpha prototype.

![Bleed Out — Kick](docs/kick.png)

## Signal flow

```text
Input
  → 3-band detector (body + cymbal bands)
  → main gate
  → high-band de-bleed sidechain (driven by the drum body)
  → multiband ADAA saturation
  → multiband EQ
  → output
```

## Three controls, one job

The drum profile does the rest. There are only three things to set:

1. **Threshold** — raise it until the gate opens on every real hit and stays
   closed on the bleed.
2. **DeBleed** — raise it until the hi-hat/cymbal wash disappears between hits.
3. **Ghost** — turn the ghost icon on and set the amount so quiet ghost notes
   survive.

Reduction, decay, attack, hold, hysteresis, crossover points, multiband tone and
saturation are automated by the selected drum profile. That is deliberate: a
drum gate should not need a second page of knobs to sound good.

### How DeBleed works

The high band (cymbals, hi-hat, snare wires) is gated **from the target hit**,
not from the cymbal wash itself. This is the same idea as the Nolly/FabFilter
Saturn trick and the Black Salt Audio Silencer de-bleeding circuit: the same
detector that opens the main gate opens the high band for the attack, then the
high band closes again **even while the body is still ringing**. DeBleed sets
how far down that high band goes.

Anything that does not open the main gate cannot open the high band either.
That matters on a full-kit sample: a snare body a few dB below the kick
threshold used to open the high band on its own, so its crack came through as a
short blip while the visible gate stayed closed. Now closed means closed.

The de-bleed sidechain is one-shot, with its own short hold/release (4/16 ms on
Snare), so it does not ride the main gate's long tail. At DeBleed 100 the high
band is 48 dB down once the attack window closes; at DeBleed 0 it is untouched.
The Snare crossover sits at 5 kHz, so the 2–5 kHz presence stays in the mid band
while the hat/cymbal top gets removed.

Measured on a synthetic snare (200 Hz body, 4 kHz presence, continuous 8 kHz
bleed), DeBleed 0 → 100:

| Signal | DeBleed 0 | DeBleed 100 |
|---|---:|---:|
| Bleed between hits | −31.3 dB | −65.5 dB |
| Bleed during the hit, after the attack | −30.4 dB | −43.3 dB |
| Snare body at the hit | −11.0 dB | −11.0 dB |
| High-frequency attack | −30.6 dB | −31.3 dB |

That is a 34 dB cut in bleed between hits, 13 dB during the hit, and the body
and attack are unchanged. The test is part of `bleed_dsp`.

## Drum profiles

Kick, Snare, Tom, Hat, Ride, Room, OH, Bus. Selecting a profile changes the
detector weights, crossovers and the automated gate/tone/saturation settings.
It is not a full preset: your Threshold and DeBleed stay where you put them
until you pick a new profile, which loads its sensible starting values.

## Ghost note mode

![Bleed Out — Snare ghost mode](docs/snare-ghost.png)

Ghost notes are quiet snare hits that a normal gate would remove. Bleed Out
adds a second, lower-threshold transient detector and a dedicated ghost
envelope. It opens both the main gate and the de-bleed high band, so a ghost
note keeps its body and its top end instead of sounding dull.

- The **ghost icon** toggles ghost note mode. It is a Pac-Man-style ghost, as
  required by metal law.
- **Ghost amount** is a normal automatable parameter and the knob next to the
  icon.
- **MIDI CC (default 74)** sets the ghost velocity scale without moving the
  Ghost knob.
- **MIDI note-on (default 38)** scales the ghost window from velocity and opens a
  short ghost window, so a MIDI performance can add ghost notes.
- The scope shows a small ghost when the ghost window is open.

Host automation and MIDI both work; the parameter is saved with the session.

## Build

Requires CMake 3.16+, a C++17 compiler, Cairo, FreeType, X11 and the LV2
development headers.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Install for the current user:

```sh
cmake -S . -B build \
  -DCLAP_DIR="$HOME/.clap" \
  -DLV2_DIR="$HOME/.lv2"
cmake --build build -j
cmake --install build
```

## Status

**0.1.0-alpha.2:** the de-bleed high band now triggers from the main gate edge,
so sub-threshold snare/hat bleed can no longer open it while the gate is closed.

0.1.0 is a working prototype: three-knob de-bleed gate, drum profiles, ghost-note
mode, MIDI note/CC control, CLAP and LV2, plus a factory and user preset bank.

Known prototype limits:

- The LV2 UI scope is flat (the CLAP UI has the live scope). The LV2 meters and
  controls work.
- Lookahead is fixed at 2 ms so switching drum profiles never changes plugin
  delay.
- Learn/Listen and the full Drum Suite Sample/EQ/Comp/Drive/Clip/Limit pages
  are not part of this standalone prototype.

## Licence

GPL-3.0-or-later. See `THIRD_PARTY_NOTICES.md`.
