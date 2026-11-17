// Bleed Out — params.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

#include "dsp/controls.hpp"

namespace bleed {

enum ParamId : uint32_t {
  P_POWER = 0,
  P_BYPASS,
  P_INPUT,
  P_OUTPUT,

  P_DRUM,

  P_GATE_ON,
  P_THRESHOLD,
  P_REDUCTION,
  P_DEBLEED,
  P_LENGTH,
  P_ATTACK,
  P_HOLD,
  P_HYSTERESIS,
  P_LOOKAHEAD,

  P_LOW_GAIN,
  P_MID_GAIN,
  P_HIGH_GAIN,
  P_LOW_DRIVE,
  P_MID_DRIVE,
  P_HIGH_DRIVE,
  P_SAT_MIX,

  P_GHOST_MODE,
  P_GHOST,
  P_GHOST_NOTE,
  P_GHOST_CC,
  P_LISTEN,

  P_COUNT
};

struct Labels {
  const char* const* names = nullptr;
  int count = 0;
};

template <size_t N>
constexpr Labels labels(const char* const (&arr)[N]) noexcept {
  return Labels{arr, static_cast<int>(N)};
}

struct ParamSpec {
  const char* symbol;
  const char* name;
  const char* module;
  double min;
  double max;
  double def;
  bool stepped;
  bool hidden;
  bool toggled;
  const char* unit;
  Labels labels;
};

inline constexpr const char* kDrumLabels[] = {"Kick", "Snare", "Tom", "Hat",
                                              "Ride", "Room", "OH", "Bus"};

inline constexpr ParamSpec kParamSpecs[P_COUNT] = {
    {"power", "Power", "Global", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"bypass", "Bypass", "Global", 0.0, 1.0, 0.0, true, false, true, nullptr, {}},
    {"input", "Input", "Global", -18.0, 18.0, 0.0, false, false, false, "dB", {}},
    {"output", "Output", "Global", -18.0, 18.0, 0.0, false, false, false, "dB", {}},

    {"drum", "Drum", "Profile", 0.0, 7.0, 0.0, true, false, false, nullptr,
     labels(kDrumLabels)},

    {"gate_on", "Gate On", "Gate", 0.0, 1.0, 1.0, true, false, true, nullptr, {}},
    {"threshold", "Threshold", "Gate", -60.0, 0.0, -30.0, false, false, false,
     "dB", {}},
    {"reduction", "Reduction", "Gate", 0.0, 80.0, 60.0, false, false, false, "dB",
     {}},
    {"debleed", "DeBleed", "Gate", 0.0, 100.0, 70.0, false, false, false, "%",
     {}},
    {"length", "Length", "Gate", 10.0, 1000.0, 80.0, false, false, false, "ms",
     {}},
    {"attack", "Attack", "Gate", 0.1, 50.0, 0.5, false, false, false, "ms", {}},
    {"hold", "Hold", "Gate", 0.0, 200.0, 5.0, false, false, false, "ms", {}},
    {"hysteresis", "Hysteresis", "Gate", 0.0, 20.0, 6.0, false, false, false,
     "dB", {}},
    {"lookahead", "Lookahead", "Gate", 0.0, 5.0, 2.0, false, false, false, "ms",
     {}},

    {"low_gain", "Low Gain", "Multiband", -12.0, 12.0, 0.0, false, false, false,
     "dB", {}},
    {"mid_gain", "Mid Gain", "Multiband", -12.0, 12.0, 0.0, false, false, false,
     "dB", {}},
    {"high_gain", "High Gain", "Multiband", -12.0, 12.0, 0.0, false, false,
     false, "dB", {}},
    {"low_drive", "Low Drive", "Multiband", 0.0, 100.0, 0.0, false, false,
     false, "%", {}},
    {"mid_drive", "Mid Drive", "Multiband", 0.0, 100.0, 0.0, false, false,
     false, "%", {}},
    {"high_drive", "High Drive", "Multiband", 0.0, 100.0, 0.0, false, false,
     false, "%", {}},
    {"sat_mix", "Sat Mix", "Multiband", 0.0, 100.0, 30.0, false, false, false,
     "%", {}},

    {"ghost_mode", "Ghost Mode", "Ghost", 0.0, 1.0, 0.0, true, false, true,
     nullptr, {}},
    {"ghost", "Ghost", "Ghost", 0.0, 100.0, 50.0, false, false, false, "%", {}},
    {"ghost_note", "Ghost Note", "MIDI", 0.0, 127.0, 38.0, true, false, false,
     nullptr, {}},
    {"ghost_cc", "Ghost CC", "MIDI", 0.0, 127.0, 74.0, true, false, false,
     nullptr, {}},
    {"listen", "Listen", "Gate", 0.0, 1.0, 0.0, true, true, true, nullptr, {}},
};

static_assert(sizeof(kParamSpecs) / sizeof(kParamSpecs[0]) == P_COUNT,
              "kParamSpecs must cover every parameter id");

class Shared {
public:
  Shared() {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      values_[p].store(kParamSpecs[p].def, std::memory_order_relaxed);
      dirty_[p].store(false, std::memory_order_relaxed);
    }
    inPeakDb.store(-120.0f);
    outPeakDb.store(-120.0f);
    inPeakLDb.store(-120.0f);
    inPeakRDb.store(-120.0f);
    outPeakLDb.store(-120.0f);
    outPeakRDb.store(-120.0f);
    grDb.store(0.0f);
    detectorDb.store(-120.0f);
    gate.store(1.0f);
    ghostActive.store(false);
    scopeHead.store(0);
    for (int i = 0; i < kScopeSize; ++i) {
      scopeIn[i].store(-120.0f);
      scopeGate[i].store(0.0f);
    }
  }

  double get(uint32_t p) const noexcept {
    return values_[p].load(std::memory_order_relaxed);
  }

  void set(uint32_t p, double v) noexcept {
    if (!std::isfinite(v)) return;
    const ParamSpec& s = kParamSpecs[p];
    v = std::min(std::max(v, s.min), s.max);
    if (s.stepped) v = static_cast<double>(static_cast<int>(v + (v >= 0 ? 0.5 : -0.5)));
    values_[p].store(v, std::memory_order_relaxed);
    dirty_[p].store(true, std::memory_order_release);
  }

  void setFromHost(uint32_t p, double v) noexcept {
    if (!std::isfinite(v)) return;
    const ParamSpec& s = kParamSpecs[p];
    v = std::min(std::max(v, s.min), s.max);
    if (s.stepped) v = static_cast<double>(static_cast<int>(v + (v >= 0 ? 0.5 : -0.5)));
    values_[p].store(v, std::memory_order_relaxed);
  }

  bool takeDirty(uint32_t p) noexcept {
    return dirty_[p].exchange(false, std::memory_order_acquire);
  }

  bool isDirty(uint32_t p) const noexcept {
    return dirty_[p].load(std::memory_order_acquire);
  }

  bool anyDirty() const noexcept {
    for (uint32_t p = 0; p < P_COUNT; ++p) {
      if (dirty_[p].load(std::memory_order_acquire)) return true;
    }
    return false;
  }

  uint32_t takeGhostTrigger() noexcept {
    return midiGhostTrigger.exchange(0, std::memory_order_acq_rel);
  }

  static constexpr int kScopeSize = 128;

  std::atomic<float> inPeakDb, outPeakDb;
  std::atomic<float> inPeakLDb, inPeakRDb;
  std::atomic<float> outPeakLDb, outPeakRDb;
  std::atomic<float> grDb, detectorDb, gate;
  std::atomic<bool> ghostActive;
  std::atomic<uint32_t> midiGhostTrigger{0};
  std::atomic<float> scopeIn[kScopeSize];
  std::atomic<float> scopeGate[kScopeSize];
  std::atomic<int> scopeHead;

private:
  std::atomic<double> values_[P_COUNT];
  std::atomic<bool> dirty_[P_COUNT];
};

inline void applyDrumDefaults(Shared& s, int drum) noexcept {
  drum = std::clamp(drum, 0, kDrumCount - 1);
  const Profile& p = kProfiles[drum];
  s.setFromHost(P_THRESHOLD, p.thresholdDb);
  s.setFromHost(P_ATTACK, p.attackMs);
  s.setFromHost(P_HOLD, p.holdMs);
  s.setFromHost(P_LENGTH, p.lengthMs);
  s.setFromHost(P_DEBLEED, p.deBleed);
}

inline Controls controlsFrom(Shared& s) noexcept {
  Controls c;
  c.drum = static_cast<int>(s.get(P_DRUM) + 0.5);
  c.power = s.get(P_POWER) > 0.5;
  c.bypass = s.get(P_BYPASS) > 0.5;
  c.inputDb = static_cast<float>(s.get(P_INPUT));
  c.outputDb = static_cast<float>(s.get(P_OUTPUT));
  c.gateOn = s.get(P_GATE_ON) > 0.5;
  c.thresholdDb = static_cast<float>(s.get(P_THRESHOLD));
  c.reductionDb = static_cast<float>(s.get(P_REDUCTION));
  c.deBleed = static_cast<float>(s.get(P_DEBLEED));
  c.lengthMs = static_cast<float>(s.get(P_LENGTH));
  c.attackMs = static_cast<float>(s.get(P_ATTACK));
  c.holdMs = static_cast<float>(s.get(P_HOLD));
  c.hysteresisDb = static_cast<float>(s.get(P_HYSTERESIS));
  c.lookaheadMs = static_cast<float>(s.get(P_LOOKAHEAD));
  c.lowGainDb = static_cast<float>(s.get(P_LOW_GAIN));
  c.midGainDb = static_cast<float>(s.get(P_MID_GAIN));
  c.highGainDb = static_cast<float>(s.get(P_HIGH_GAIN));
  c.lowDrive = static_cast<float>(s.get(P_LOW_DRIVE));
  c.midDrive = static_cast<float>(s.get(P_MID_DRIVE));
  c.highDrive = static_cast<float>(s.get(P_HIGH_DRIVE));
  c.satMix = static_cast<float>(s.get(P_SAT_MIX) / 100.0);
  c.ghostMode = s.get(P_GHOST_MODE) > 0.5;
  c.ghost = static_cast<float>(s.get(P_GHOST));
  c.ghostNote = static_cast<int>(s.get(P_GHOST_NOTE) + 0.5);
  c.ghostCc = static_cast<int>(s.get(P_GHOST_CC) + 0.5);
  c.ghostTrigger = static_cast<int>(s.takeGhostTrigger());
  c.listen = s.get(P_LISTEN) > 0.5;
  return c;
}

} // namespace bleed
