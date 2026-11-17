// Bleed Out — panel interaction tests (no X server needed).
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cairo.h>

#include <cmath>
#include <cstdio>
#include <string>

#include "params.hpp"
#include "presets.hpp"
#include "ui/panel.hpp"
#include "value_text.hpp"

using namespace bleed;

namespace {
int gFailures = 0;
void check(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) ++gFailures;
}
} // namespace

int main() {
  cairo_surface_t* surf =
      cairo_image_surface_create(CAIRO_FORMAT_RGB24, kPanelWidth, kPanelHeight);
  cairo_t* cr = cairo_create(surf);

  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    check(shared.get(P_DRUM) == 0.0, "starts on Kick");

    // Snare drum tab.
    panel.pointerDown(shared, state, 211, 88, false);
    panel.pointerUp();
    check(shared.get(P_DRUM) == 1.0, "Snare tab selects Snare");
    check(std::fabs(shared.get(P_THRESHOLD) - kProfiles[1].thresholdDb) < 1e-6,
          "Snare profile applies its threshold");

    // Ghost icon toggles ghost mode.
    panel.pointerDown(shared, state, 882, 380, false);
    panel.pointerUp();
    check(shared.get(P_GHOST_MODE) == 1.0, "ghost icon enables ghost mode");

    // Gate pill.
    panel.pointerDown(shared, state, 700, 312, false);
    panel.pointerUp();
    check(shared.get(P_GATE_ON) == 0.0, "gate pill bypasses the gate");
  }

  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    panel.pointerDown(shared, state, 72 + 330 - 18, 32, false);
    panel.pointerUp();
    check(state.presetIndex == 0, "preset next arrow applies a preset");
  }

  {
    Shared shared;
    Panel panel;
    PanelState state;
    panel.draw(cr, shared, state);
    int ghostPreset = -1;
    for (int i = 0; i < kPresetCount; ++i) {
      if (std::string(kPresets[i].name) == "Ghost Notes") ghostPreset = i;
    }
    check(ghostPreset >= 0, "Ghost Notes preset exists");
    if (ghostPreset >= 0) {
      panel.applyPreset(shared, state, ghostPreset);
      check(shared.get(P_GHOST_MODE) == 1.0, "Ghost Notes enables ghost mode");
      check(shared.get(P_DRUM) == 1.0, "Ghost Notes selects Snare");
    }
  }

  {
    Shared shared;
    double v = -1.0;
    check(parseParamValue(shared, P_THRESHOLD, "-40 dB", &v) &&
              std::fabs(v + 40.0) < 1e-9,
          "typed threshold accepts -40 dB");
    check(parseParamValue(shared, P_LENGTH, "250 ms", &v) &&
              std::fabs(v - 250.0) < 1e-9,
          "typed length accepts 250 ms");
    check(parseParamValue(shared, P_DRUM, "Snare", &v) &&
              std::fabs(v - 1.0) < 1e-9,
          "typed drum accepts Snare");
  }

  cairo_destroy(cr);
  cairo_surface_destroy(surf);
  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
