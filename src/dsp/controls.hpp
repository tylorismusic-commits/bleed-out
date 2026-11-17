// Bleed Out — controls.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace bleed {

// Drum profiles.  The profile changes the detector weights, crossovers and
// sensible gate defaults.  It is not a full preset: the rest of the chain
// stays where the user put it.
struct Profile {
  const char* name;
  float lowXover;
  float highXover;
  float lowWeight;
  float midWeight;
  float highWeight;
  float thresholdDb;
  float attackMs;
  float holdMs;
  float lengthMs;
  float deBleed;
};

inline constexpr Profile kProfiles[8] = {
    {"Kick", 180.0f, 2000.0f, 0.80f, 0.15f, 0.05f, -30.0f, 0.5f, 5.0f, 80.0f, 70.0f},
    {"Snare", 300.0f, 4000.0f, 0.25f, 0.60f, 0.15f, -32.0f, 1.0f, 15.0f, 120.0f, 80.0f},
    {"Tom", 350.0f, 3000.0f, 0.60f, 0.30f, 0.10f, -32.0f, 1.0f, 10.0f, 140.0f, 60.0f},
    {"Hat", 1000.0f, 5000.0f, 0.05f, 0.25f, 0.70f, -34.0f, 0.5f, 5.0f, 50.0f, 30.0f},
    {"Ride", 1000.0f, 5000.0f, 0.05f, 0.30f, 0.65f, -34.0f, 0.5f, 8.0f, 80.0f, 20.0f},
    {"Room", 300.0f, 6000.0f, 0.20f, 0.50f, 0.30f, -36.0f, 2.0f, 30.0f, 300.0f, 20.0f},
    {"OH", 300.0f, 8000.0f, 0.10f, 0.45f, 0.45f, -36.0f, 2.0f, 30.0f, 300.0f, 20.0f},
    {"Bus", 250.0f, 6000.0f, 0.30f, 0.40f, 0.30f, -40.0f, 5.0f, 50.0f, 400.0f, 0.0f},
};

inline constexpr int kDrumCount = 8;

struct Controls {
  int drum = 0;  // Kick

  bool power = true;
  bool bypass = false;
  float inputDb = 0.0f;
  float outputDb = 0.0f;

  // Detector / gate
  bool gateOn = true;
  float thresholdDb = -30.0f;
  float reductionDb = 60.0f;
  float deBleed = 70.0f;
  float lengthMs = 80.0f;
  float attackMs = 0.5f;
  float holdMs = 5.0f;
  float hysteresisDb = 6.0f;
  float lookaheadMs = 2.0f;

  // Multiband tone / saturation
  float lowGainDb = 0.0f;
  float midGainDb = 0.0f;
  float highGainDb = 0.0f;
  float lowDrive = 0.0f;
  float midDrive = 0.0f;
  float highDrive = 0.0f;
  float satMix = 0.30f;

  // Ghost note mode
  bool ghostMode = false;
  float ghost = 50.0f;      // 0..100
  int ghostNote = 38;       // MIDI note that opens a ghost window
  int ghostCc = 74;         // MIDI CC that sets the ghost amount
  int ghostTrigger = 0;     // consumed by the engine after a MIDI note-on
  bool listen = false;
};

struct Meters {
  float inPeakDb = -120.0f;
  float outPeakDb = -120.0f;
  float inPeakLDb = -120.0f;
  float inPeakRDb = -120.0f;
  float outPeakLDb = -120.0f;
  float outPeakRDb = -120.0f;
  float grDb = 0.0f;
  float detectorDb = -120.0f;
  float gate = 1.0f;
  bool ghostActive = false;
};

} // namespace bleed
