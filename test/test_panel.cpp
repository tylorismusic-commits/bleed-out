// Bleed Out — panel interaction tests (no X server needed).
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cairo.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
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
    check(std::fabs(shared.get(P_DEBLEED) - kProfiles[1].deBleed) < 1e-6,
          "Snare profile applies its DeBleed default");

    // Ghost icon toggles ghost mode.
    panel.pointerDown(shared, state, 886, 392, false);
    panel.pointerUp();
    check(shared.get(P_GHOST_MODE) == 1.0, "ghost icon enables ghost mode");

    // Only three primary knobs are exposed; the rest is automated.
    int primary = 0;
    for (const auto& r : panel.widgetRects()) {
      if (r.knob && (r.param == P_THRESHOLD || r.param == P_DEBLEED ||
                     r.param == P_GHOST)) {
        ++primary;
      }
    }
    check(primary == 3, "primary controls are exactly Threshold, DeBleed and Ghost");
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
    check(parseParamValue(shared, P_DEBLEED, "85 %", &v) &&
              std::fabs(v - 85.0) < 1e-9,
          "typed DeBleed accepts 85 %");
    check(parseParamValue(shared, P_DRUM, "Snare", &v) &&
              std::fabs(v - 1.0) < 1e-9,
          "typed drum accepts Snare");
  }


  {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "bleed-out-preset-test";
    fs::remove_all(root);
    fs::create_directories(root / "config");
    fs::create_directories(root / "home");
    const std::string oldXdg =
        std::getenv("XDG_CONFIG_HOME") ? std::getenv("XDG_CONFIG_HOME") : "";
    const std::string oldHome =
        std::getenv("HOME") ? std::getenv("HOME") : "";
    setenv("XDG_CONFIG_HOME", (root / "config").string().c_str(), 1);
    setenv("HOME", (root / "home").string().c_str(), 1);

    Shared userShared;
    PanelState userState;
    Panel userPanel;
    userShared.set(P_DEBLEED, 0.71);
    const double saved = userShared.get(P_DEBLEED);
    userPanel.handlePresetClick(userShared, userState, 72 + 165, 32);
    userPanel.handlePresetClick(userShared, userState, 100, 450);
    check(userState.savePreset, "SAVE opens the name field");
    for (char c : std::string("My Gate")) {
      userPanel.textKey(userShared, userState, 0, c);
    }
    userPanel.textKey(userShared, userState, Panel::kTextEnter, 0);
    check(userState.userPresets.size() == 1, "user preset is written");
    check(userState.presetIndex == kPresetCount, "saved preset becomes active");
    userShared.set(P_DEBLEED, 0.1);
    userPanel.applyPreset(userShared, userState, userState.presetIndex);
    check(std::fabs(userShared.get(P_DEBLEED) - saved) < 1e-9,
          "user preset recalls every parameter");

    userPanel.handlePresetClick(userShared, userState, 72 + 165, 32);
    userPanel.handlePresetClick(userShared, userState, 100, 480);
    for (char c : std::string("Renamed")) {
      userPanel.textKey(userShared, userState, 0, c);
    }
    userPanel.textKey(userShared, userState, Panel::kTextEnter, 0);
    check(userState.userPresets.size() == 1 &&
              userState.userPresets[0].name == "Renamed",
          "user preset renames");

    userPanel.handlePresetClick(userShared, userState, 72 + 165, 32);
    userPanel.handlePresetClick(userShared, userState, 180, 450);
    check(userState.userPresets.size() == 2, "DUPLICATE creates a preset");

    userPanel.handlePresetClick(userShared, userState, 72 + 165, 32);
    userPanel.handlePresetClick(userShared, userState, 250, 480);
    const fs::path exported = userPresetExportDir() /
                              (std::string("Renamed copy") +
                               kUserPresetExt);
    check(fs::exists(exported), "EXPORT writes a preset file");

    userPanel.handlePresetClick(userShared, userState, 72 + 165, 32);
    userPanel.handlePresetClick(userShared, userState, 180, 480);
    userPanel.handlePresetClick(userShared, userState, 180, 480);
    check(userState.userPresets.size() == 1, "DELETE removes the preset");
    check(userPanel.importDroppedFile(userShared, userState,
                                      exported.string()),
          "IMPORT accepts a dropped preset file");
    check(userState.userPresets.size() == 2, "import adds a preset");

    if (!oldXdg.empty()) setenv("XDG_CONFIG_HOME", oldXdg.c_str(), 1);
    else unsetenv("XDG_CONFIG_HOME");
    if (!oldHome.empty()) setenv("HOME", oldHome.c_str(), 1);
    else unsetenv("HOME");
    fs::remove_all(root);
  }

  cairo_destroy(cr);
  cairo_surface_destroy(surf);
  std::printf("%d failure(s)\n", gFailures);
  return gFailures == 0 ? 0 : 1;
}
