// Bleed Out — DSP sanity tests
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "dsp/engine.hpp"
#include "params.hpp"

using namespace bleed;

namespace {
int gFailures = 0;
void check(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) ++gFailures;
}

constexpr float kSr = 48000.0f;

struct Render {
  std::vector<float> l, r;
  Meters meters;
};

Render run(const Controls& c, const std::vector<float>& in) {
  Engine engine;
  engine.prepare(kSr);
  engine.setControls(c);
  engine.reset();
  if (c.ghostTrigger > 0) engine.setControls(c);
  Render out;
  out.l = in;
  out.r = in;
  const int n = static_cast<int>(in.size());
  for (int start = 0; start < n; start += 128) {
    const int len = std::min(128, n - start);
    engine.process(out.l.data() + start, out.r.data() + start, len, out.meters);
  }
  return out;
}

Controls base() {
  Controls c;
  c.drum = 1;  // Snare
  c.gateOn = true;
  c.thresholdDb = -30.0f;
  c.reductionDb = 60.0f;
  c.deBleed = 80.0f;
  c.lengthMs = 100.0f;
  c.attackMs = 0.5f;
  c.holdMs = 5.0f;
  c.hysteresisDb = 6.0f;
  c.lookaheadMs = 0.0f;
  c.satMix = 0.0f;
  c.ghostMode = false;
  c.ghost = 100.0f;
  c.power = true;
  c.bypass = false;
  return c;
}

double rms(const std::vector<float>& x, size_t a, size_t b) {
  double sum = 0.0;
  for (size_t i = a; i < b && i < x.size(); ++i) sum += x[i] * x[i];
  return std::sqrt(sum / std::max<size_t>(1, b - a));
}

bool finite(const std::vector<float>& x) {
  for (float v : x) if (!std::isfinite(v)) return false;
  return true;
}
} // namespace

int main() {
  {
    // Bypass with zero lookahead is the exact input.
    Controls c = base();
    c.bypass = true;
    std::vector<float> in(1024);
    for (size_t i = 0; i < in.size(); ++i)
      in[i] = 0.25f * std::sin(2.0f * 3.14159265f * 220.0f * i / kSr);
    Render r = run(c, in);
    float err = 0.0f;
    for (size_t i = 0; i < in.size(); ++i)
      err = std::max(err, std::fabs(r.l[i] - in[i]));
    check(err < 1e-6f, "bypass returns the exact input");
  }

  {
    // A quiet ghost hit is removed without ghost mode and kept with it.
    const int n = static_cast<int>(kSr * 0.30f);
    std::vector<float> in(n, 0.0f);
    const int hit = static_cast<int>(kSr * 0.10f);
    for (int i = 0; i < static_cast<int>(kSr * 0.003f); ++i) {
      const float env = 1.0f - static_cast<float>(i) / (kSr * 0.003f);
      in[hit + i] = 0.020f * env *
                    std::sin(2.0f * 3.14159265f * 1000.0f * i / kSr);
    }
    Controls off = base();
    off.ghostMode = false;
    Render quiet = run(off, in);
    Controls on = base();
    on.ghostMode = true;
    on.ghost = 100.0f;
    Render ghost = run(on, in);
    const double a = rms(quiet.l, hit, hit + static_cast<int>(kSr * 0.02f));
    const double b = rms(ghost.l, hit, hit + static_cast<int>(kSr * 0.02f));
    std::printf("  ghost rms off=%.6f on=%.6f\n", a, b);
    check(b > a * 1.5, "ghost note mode lets a quiet snare hit through");
  }

  {
    // A MIDI-triggered ghost window reports activity and stays finite.
    Controls c = base();
    c.ghostMode = true;
    c.ghost = 80.0f;
    c.ghostTrigger = 1;
    std::vector<float> in(256, 0.0f);
    Render r = run(c, in);
    check(r.meters.ghostActive, "MIDI ghost trigger opens the ghost window");
    check(finite(r.l), "MIDI ghost output is finite");
  }

  {
    // A loud hit opens the gate.
    const int n = static_cast<int>(kSr * 0.20f);
    std::vector<float> in(n, 0.0f);
    const int hit = static_cast<int>(kSr * 0.05f);
    for (int i = 0; i < static_cast<int>(kSr * 0.005f); ++i) {
      in[hit + i] = 0.5f *
                    std::sin(2.0f * 3.14159265f * 200.0f * i / kSr);
    }
    Render r = run(base(), in);
    const double a = rms(r.l, hit, hit + static_cast<int>(kSr * 0.02f));
    check(a > 0.05, "a loud hit opens the gate");
    check(finite(r.l), "gate output is finite");
  }

  {
    // Extreme settings stay finite and bounded.
    Controls c = base();
    c.reductionDb = 80.0f;
    c.deBleed = 100.0f;
    c.ghostMode = true;
    c.ghost = 100.0f;
    c.lowDrive = c.midDrive = c.highDrive = 100.0f;
    c.satMix = 100.0f;
    std::vector<float> in(48000);
    for (size_t i = 0; i < in.size(); ++i)
      in[i] = 0.5f * std::sin(2.0f * 3.14159265f * 100.0f * i / kSr);
    Render r = run(c, in);
    float peak = 0.0f;
    for (float v : r.l) peak = std::max(peak, std::fabs(v));
    check(finite(r.l) && finite(r.r), "extreme settings stay finite");
    check(peak < 8.0f, "extreme settings do not run away");
  }

  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
