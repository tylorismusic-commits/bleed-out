// Bleed Out — panel.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cairo.h>

#include <string>
#include <vector>

#include "params.hpp"
#include "user_presets.hpp"

namespace bleed {

constexpr int kPanelWidth = 1040;
constexpr int kPanelHeight = 640;

struct PanelState {
  int presetIndex = -1;
  bool presetDirty = false;
  int presetOpen = 0;
  int presetScroll = 0;
  bool savePreset = false;
  bool renamePreset = false;
  bool deletePresetConfirm = false;
  bool presetOverwriteConfirm = false;
  std::string presetText;
  bool presetTextFresh = false;
  std::string presetError;
  std::string presetNotice;
  double presetNoticeUntil = 0.0;
  std::vector<UserPreset> userPresets;
  float inLevelL = -120.0f;
  float inLevelR = -120.0f;
  float outLevelL = -120.0f;
  float outLevelR = -120.0f;
  float grDb = 0.0f;
  float detectorDb = -120.0f;
  float gate = 1.0f;
  bool ghostActive = false;
  float scopeIn[Shared::kScopeSize] = {};
  float scopeGate[Shared::kScopeSize] = {};
};

class Panel {
public:
  Panel();
  ~Panel();
  Panel(const Panel&) = delete;
  Panel& operator=(const Panel&) = delete;

  void loadFont(const std::string& dir);
  void tick(const Shared& shared, PanelState& state) const;
  void draw(cairo_t* cr, Shared& shared, const PanelState& state);

  bool pointerDown(Shared& shared, PanelState& state, double x, double y,
                   bool fine = false);
  bool pointerMove(Shared& shared, PanelState& state, double x, double y,
                   bool fine = false);
  void pointerUp();
  bool handlePresetClick(Shared& shared, PanelState& state, double x, double y);
  void applyPreset(Shared& shared, PanelState& state, int index) const;
  void drawPresetBar(cairo_t* cr, const Shared& shared,
                     const PanelState& state) const;
  void drawPresetList(cairo_t* cr, const Shared& shared,
                      const PanelState& state) const;
  int presetCount(const PanelState& state) const;
  const char* presetName(const PanelState& state, int index) const;
  const char* presetCategory(const PanelState& state, int index) const;
  std::vector<int> presetOrder(const PanelState& state) const;
  void reloadUserPresets(PanelState& state);
  void rescanUserPresets(PanelState& state);
  void setPresetNotice(PanelState& state, const std::string& text) const;
  void duplicatePreset(Shared& shared, PanelState& state);
  void exportPreset(const Shared& shared, PanelState& state) const;
  bool importPresetFile(Shared& shared, PanelState& state,
                        const std::string& path);
  bool importDroppedFile(Shared& shared, PanelState& state,
                         const std::string& path);
  bool commitPresetName(Shared& shared, PanelState& state);
  bool wheel(Shared& shared, PanelState& state, double x, double y, double dy);
  void beginTextEntry(Shared& shared, PanelState& state, double x, double y);
  static constexpr int kTextEnter = 1;
  static constexpr int kTextEscape = 2;
  static constexpr int kTextBackspace = 3;
  bool textKey(Shared& shared, PanelState& state, int key, char ch);

  bool dragging() const { return dragParam_ >= 0; }

  struct WidgetRect {
    uint32_t param = 0;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    double r = 0.0;
    const char* label = "";
    bool knob = false;
    bool toggle = false;
  };
  std::vector<WidgetRect> widgetRects() const;

private:
  enum class Kind {
    Knob,
    Power,
    DrumTab,
    Pill,
    Button,
    Toggle,
    Ghost,
  };

  struct Widget {
    Kind kind = Kind::Knob;
    uint32_t param = 0;
    double value = 0.0;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    double r = 0.0;
    double scaleRadius = 1.44;
    int steps = 0;
    const char* label = "";
    std::vector<const char*> scale;
    bool labelBelow = false;
    std::vector<const char*> buttonText;
  };

  void layout(const Shared& shared, const PanelState& state);
  void drawHeader(cairo_t* cr, const Shared& shared,
                  const PanelState& state) const;
  void drawBody(cairo_t* cr, int drum) const;
  void drawScope(cairo_t* cr, const Shared& shared,
                 const PanelState& state) const;
  void drawLevelMeter(cairo_t* cr, double x, double y, double w, double h,
                      double dbL, double dbR, double minDb, double maxDb,
                      bool labelsRight) const;
  void drawWidget(cairo_t* cr, const Widget& w, const Shared& shared,
                  const PanelState& state, int drum) const;
  void drawKnob(cairo_t* cr, const Widget& w, double value, int drum) const;
  void drawButton(cairo_t* cr, const Widget& w, bool lit, int drum) const;
  void drawToggle(cairo_t* cr, const Widget& w, bool on, int drum) const;
  void drawPill(cairo_t* cr, const Widget& w, bool on, int drum) const;
  void drawGhost(cairo_t* cr, const Widget& w, bool on, int drum) const;
  void drawTextEntry(cairo_t* cr, const Shared& shared) const;
  void text(cairo_t* cr, const char* s, double x, double y, double size,
            int align, double r, double g, double b) const;
  bool hitWidget(const Widget& w, double x, double y) const;
  bool commitTextEntry(Shared& shared, PanelState& state);
  bool hitTextEntry(double x, double y) const;
  double presetResetValue(const PanelState& state, uint32_t param) const;

  std::vector<Widget> widgets_;
  int dragParam_ = -1;
  int dragSteps_ = 0;
  double dragStartY_ = 0.0;
  double dragStartValue_ = 0.0;
  int lastClickParam_ = -1;
  double lastClickTime_ = 0.0;
  static constexpr double kDoubleClickMs = 400.0;

  bool editing_ = false;
  uint32_t editParam_ = 0;
  std::string editText_;
  double editBoxX_ = 0.0;
  double editBoxY_ = 0.0;
  static constexpr double kEditW = 150.0;
  static constexpr double kEditH = 32.0;

  cairo_surface_t* grain_ = nullptr;
  void* ft_ = nullptr;
  void* ftFace_ = nullptr;
  cairo_font_face_t* font_ = nullptr;
  cairo_font_face_t* fallbackFont_ = nullptr;
};

} // namespace bleed
