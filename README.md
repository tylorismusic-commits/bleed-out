# Bleed Out

**Version 0.1.0** · GPL-3.0-or-later

A multiband de-bleed gate for drums, built in the same visual language as Comp
Suite and the rest of the Suite series. It removes hi-hat and cymbal bleed while
keeping the attack, body and tone of the drum.

This is the working prototype for local testing.

![Bleed Out — Kick](docs/kick.png)

## Signal flow

```text
Input
  → 3-band crossover
  → weighted detector + gate/expander
  → per-band gain (high band gets DeBleed)
  → multiband ADAA saturation
  → multiband EQ
  → output
```

## Drum profiles

Kick, Snare, Tom, Hat, Ride, Room, OH, Bus. The selector changes the detector
weights, crossovers and sensible gate defaults. It is not a full preset.

## Controls

- **Threshold** — gate opening level.
- **Reduction** — gate range, 0–80 dB.
- **DeBleed** — extra high-band attenuation when the gate is closed.
- **Length** — release/tail length.
- **Ghost** — ghost-note amount.
- **Attack, Hold, Hysteresis, Lookahead** — advanced gate timing.
- **MIDI Note / MIDI CC** — ghost-note MIDI control.
- **Low/Mid/High Gain and Drive, Sat Mix** — multiband tone and saturation.
- **Input / Output / Bypass**.

## Ghost note mode

Ghost notes are quiet snare hits that a normal gate would remove. Bleed Out
adds a second, lower-threshold transient detector and a dedicated ghost
envelope.

- The **ghost icon** toggles ghost note mode. It is a Pac-Man-style ghost, as
  required by metal law.
- **Ghost** is a normal automatable parameter.
- **MIDI CC (default 74)** sets the ghost amount.
- **MIDI note-on (default 38)** sets the ghost amount from velocity and opens a
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

0.1.0 is the first working prototype: gate, DeBleed, multiband saturation/EQ,
drum profiles, ghost-note mode, MIDI note/CC control, CLAP and LV2.

Known prototype limits:

- The LV2 UI scope is flat (the CLAP UI has the live scope). The LV2 meters and
  controls work.
- The lookahead control is functional but changing it during playback may shift
  timing until the host restarts.
- Learn/Listen and the full Drum Suite Sample/EQ/Comp/Drive/Clip/Limit pages
  are not part of this standalone prototype.

## Licence

GPL-3.0-or-later. See `THIRD_PARTY_NOTICES.md`.
