// Bleed Out — multiband de-bleed gate
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <hdsp/adaa.hpp>
#include <hdsp/crossover.hpp>
#include <hdsp/db.hpp>
#include <hdsp/smoother.hpp>
#include <hdsp/util.hpp>

#include "controls.hpp"

namespace bleed {

class BleedOut {
public:
  void prepare(float sampleRate) noexcept {
    sr_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    crossDetL_.prepare(sr_);
    crossDetR_.prepare(sr_);
    crossAudL_.prepare(sr_);
    crossAudR_.prepare(sr_);
    setProfile(0);

    delaySize_ = std::max(8, static_cast<int>(sr_ * 0.010f) + 4);
    delayL_.assign(static_cast<size_t>(delaySize_), 0.0f);
    delayR_.assign(static_cast<size_t>(delaySize_), 0.0f);
    delayWrite_ = 0;

    input_.setTime(15.0f, sr_);
    output_.setTime(15.0f, sr_);
    input_.reset(1.0f);
    output_.reset(1.0f);
    fastRelease_ = std::exp(-1.0f / (0.020f * sr_));
    slowRelease_ = std::exp(-1.0f / (0.150f * sr_));
    detRelease_ = std::exp(-1.0f / (0.080f * sr_));
    reset();
  }

  void setControls(const Controls& c) noexcept {
    controls_ = c;
    drum_ = std::clamp(c.drum, 0, kDrumCount - 1);
    if (drum_ != profileDrum_) {
      setProfile(drum_);
    }
    threshold_ = hdsp::dbToGain(c.thresholdDb);
    closeThreshold_ = threshold_ * hdsp::dbToGain(-c.hysteresisDb);
    minGain_ = hdsp::dbToGain(-c.reductionDb);
    deBleed_ = hdsp::clampf(c.deBleed * 0.01f, 0.0f, 1.0f);
    attackCoef_ = 1.0f - hdsp::timeCoeff(std::max(0.1f, c.attackMs), sr_);
    releaseCoef_ = 1.0f - hdsp::timeCoeff(std::max(1.0f, c.lengthMs), sr_);
    holdSamples_ = static_cast<int>(c.holdMs * 0.001f * sr_);
    lookaheadSamples_ = static_cast<int>(hdsp::clampf(c.lookaheadMs, 0.0f, 5.0f) *
                                         0.001f * sr_);
    lookaheadSamples_ = std::min(lookaheadSamples_, delaySize_ - 2);
    lowGain_ = hdsp::dbToGain(c.lowGainDb);
    midGain_ = hdsp::dbToGain(c.midGainDb);
    highGain_ = hdsp::dbToGain(c.highGainDb);
    lowDrive_ = 1.0f + hdsp::clampf(c.lowDrive * 0.01f, 0.0f, 1.0f) * 8.0f;
    midDrive_ = 1.0f + hdsp::clampf(c.midDrive * 0.01f, 0.0f, 1.0f) * 8.0f;
    highDrive_ = 1.0f + hdsp::clampf(c.highDrive * 0.01f, 0.0f, 1.0f) * 8.0f;
    satMix_ = hdsp::clampf(c.satMix, 0.0f, 1.0f);
    ghostAmount_ = hdsp::clampf(c.ghost * 0.01f, 0.0f, 1.0f);
    ghostRange_ = hdsp::dbToGain(-24.0f * ghostAmount_);
    ghostAttack_ = 1.0f - hdsp::timeCoeff(0.5f, sr_);
    ghostRelease_ = 1.0f - hdsp::timeCoeff(50.0f, sr_);
    ghostHoldSamples_ = static_cast<int>(0.012f * sr_);
    if (c.ghostTrigger > 0) {
      midiGhostHold_ = std::max(midiGhostHold_, static_cast<int>(0.060f * sr_));
    }
    input_.setTarget(hdsp::dbToGain(c.inputDb));
    output_.setTarget(hdsp::dbToGain(c.outputDb));
  }

  void reset() noexcept {
    crossDetL_.reset();
    crossDetR_.reset();
    crossAudL_.reset();
    crossAudR_.reset();
    std::fill(delayL_.begin(), delayL_.end(), 0.0f);
    std::fill(delayR_.begin(), delayR_.end(), 0.0f);
    delayWrite_ = 0;
    gateEnv_ = 0.0f;
    ghostEnv_ = 0.0f;
    hold_ = 0;
    ghostHold_ = 0;
    midiGhostHold_ = 0;
    gateOpen_ = false;
    envDet_ = 0.0f;
    for (int i = 0; i < 3; ++i) {
      bandFast_[i] = 0.0f;
      bandSlow_[i] = 0.0f;
    }
    lowSatL_.reset(); lowSatR_.reset();
    midSatL_.reset(); midSatR_.reset();
    highSatL_.reset(); highSatR_.reset();
    input_.snap();
    output_.snap();
  }

  void process(float* left, float* right, int frames, Meters& meters) noexcept {
    float inPeakL = 0.0f, inPeakR = 0.0f;
    float outPeakL = 0.0f, outPeakR = 0.0f;
    float grMax = 0.0f;
    float detMax = 0.0f;
    bool ghostSeen = false;

    for (int i = 0; i < frames; ++i) {
      const float rawL = std::isfinite(left[i]) ? left[i] : 0.0f;
      const float rawR = std::isfinite(right[i]) ? right[i] : 0.0f;
      inPeakL = std::max(inPeakL, std::fabs(rawL));
      inPeakR = std::max(inPeakR, std::fabs(rawR));

      // Delay the raw signal so the detector can open ahead of the transient.
      delayL_[delayWrite_] = rawL;
      delayR_[delayWrite_] = rawR;
      int read = delayWrite_ - lookaheadSamples_;
      if (read < 0) read += delaySize_;
      const float dryL = delayL_[read];
      const float dryR = delayR_[read];
      delayWrite_ = (delayWrite_ + 1) % delaySize_;

      const float gainIn = input_.next();
      const float detL = rawL * gainIn;
      const float detR = rawR * gainIn;

      // Detector split.
      float dl, dm, dh, drl, drm, drh;
      crossDetL_.process(detL, dl, dm, dh);
      crossDetR_.process(detR, drl, drm, drh);
      const float bandPeak[3] = {
          std::max(std::fabs(dl), std::fabs(drl)),
          std::max(std::fabs(dm), std::fabs(drm)),
          std::max(std::fabs(dh), std::fabs(drh))};
      for (int b = 0; b < 3; ++b) {
        bandFast_[b] = std::max(bandPeak[b], bandFast_[b] * fastRelease_);
        bandSlow_[b] = std::max(bandPeak[b], bandSlow_[b] * slowRelease_);
      }
      const float det = weights_[0] * bandFast_[0] +
                        weights_[1] * bandFast_[1] +
                        weights_[2] * bandFast_[2];
      envDet_ += (det - envDet_) * (1.0f - detRelease_);
      const float transient = std::max(0.0f, det - envDet_);

      // Main gate.
      if (det > threshold_) {
        gateOpen_ = true;
        hold_ = holdSamples_;
      } else if (det < closeThreshold_) {
        gateOpen_ = false;
      }
      if (hold_ > 0) --hold_;
      const bool target = controls_.gateOn && (gateOpen_ || hold_ > 0);
      gateEnv_ += (target ? (1.0f - gateEnv_) : (0.0f - gateEnv_)) *
                  (target ? attackCoef_ : releaseCoef_);
      float gateGain = minGain_ + (1.0f - minGain_) * gateEnv_;

      // Ghost note mode: a lower threshold plus a transient requirement.
      bool ghostOpen = false;
      if (controls_.ghostMode && ghostAmount_ > 0.0f) {
        const float ghostThreshold = threshold_ * ghostRange_;
        ghostOpen = det > ghostThreshold &&
                    transient > ghostThreshold * 0.20f;
        if (ghostOpen) {
          ghostHold_ = ghostHoldSamples_;
          ghostSeen = true;
        }
      }
      if (ghostHold_ > 0) --ghostHold_;
      if (midiGhostHold_ > 0) {
        --midiGhostHold_;
        ghostOpen = true;
        ghostSeen = true;
      }
      const bool ghostTarget = controls_.ghostMode && ghostAmount_ > 0.0f &&
                               (ghostOpen || ghostHold_ > 0 ||
                                midiGhostHold_ > 0);
      ghostEnv_ += (ghostTarget ? (1.0f - ghostEnv_) : (0.0f - ghostEnv_)) *
                   (ghostTarget ? ghostAttack_ : ghostRelease_);
      const float ghostGain =
          minGain_ + (1.0f - minGain_) * ghostEnv_ * ghostAmount_;
      gateGain = std::max(gateGain, ghostGain);

      const float highGate =
          gateGain * (1.0f - deBleed_ * (1.0f - gateEnv_));

      // Audio path split.
      float al, am, ah, arl, arm, arh;
      const float audL = dryL * gainIn;
      const float audR = dryR * gainIn;
      crossAudL_.process(audL, al, am, ah);
      crossAudR_.process(audR, arl, arm, arh);

      al = saturate(lowSatL_, al, lowDrive_);
      am = saturate(midSatL_, am, midDrive_);
      ah = saturate(highSatL_, ah, highDrive_);
      arl = saturate(lowSatR_, arl, lowDrive_);
      arm = saturate(midSatR_, arm, midDrive_);
      arh = saturate(highSatR_, arh, highDrive_);

      float outL = al * lowGain_ * gateGain + am * midGain_ * gateGain +
                   ah * highGain_ * highGate;
      float outR = arl * lowGain_ * gateGain + arm * midGain_ * gateGain +
                   arh * highGain_ * highGate;
      const float gainOut = output_.next();
      outL *= gainOut;
      outR *= gainOut;

      // Power/bypass crossfade to the delayed raw input, so timing does not
      // move when the gate is switched out.
      const float active = controls_.power ? (controls_.bypass ? 0.0f : 1.0f) : 0.0f;
      outL = dryL + active * (outL - dryL);
      outR = dryR + active * (outR - dryR);

      left[i] = std::isfinite(outL) ? outL : 0.0f;
      right[i] = std::isfinite(outR) ? outR : 0.0f;
      outPeakL = std::max(outPeakL, std::fabs(outL));
      outPeakR = std::max(outPeakR, std::fabs(outR));
      grMax = std::max(grMax, -hdsp::gainToDb(gateGain));
      detMax = std::max(detMax, hdsp::gainToDb(det));
    }

    meters.inPeakLDb = hdsp::gainToDb(inPeakL);
    meters.inPeakRDb = hdsp::gainToDb(inPeakR);
    meters.inPeakDb = std::max(meters.inPeakLDb, meters.inPeakRDb);
    meters.outPeakLDb = hdsp::gainToDb(outPeakL);
    meters.outPeakRDb = hdsp::gainToDb(outPeakR);
    meters.outPeakDb = std::max(meters.outPeakLDb, meters.outPeakRDb);
    meters.grDb = std::max(0.0f, grMax);
    meters.detectorDb = detMax;
    meters.gate = gateEnv_;
    meters.ghostActive = ghostSeen;
  }

  int latencySamples() const noexcept { return lookaheadSamples_; }

private:
  void setProfile(int drum) noexcept {
    profileDrum_ = drum;
    const Profile& p = kProfiles[drum];
    crossDetL_.setCrossovers(p.lowXover, p.highXover);
    crossDetR_.setCrossovers(p.lowXover, p.highXover);
    crossAudL_.setCrossovers(p.lowXover, p.highXover);
    crossAudR_.setCrossovers(p.lowXover, p.highXover);
    weights_[0] = p.lowWeight;
    weights_[1] = p.midWeight;
    weights_[2] = p.highWeight;
  }

  float saturate(hdsp::AsymShaper& shaper, float x, float drive) noexcept {
    if (satMix_ <= 0.0f) return x;
    const float wet = shaper.process(x, drive, 0.0f);
    return x + satMix_ * (wet - x);
  }

  float sr_ = 48000.0f;
  int drum_ = 0;
  int profileDrum_ = -1;
  Controls controls_;

  hdsp::ThreeBandCrossover crossDetL_, crossDetR_;
  hdsp::ThreeBandCrossover crossAudL_, crossAudR_;
  hdsp::AsymShaper lowSatL_, lowSatR_, midSatL_, midSatR_, highSatL_, highSatR_;
  hdsp::Smoothed input_, output_;

  std::vector<float> delayL_, delayR_;
  int delaySize_ = 1;
  int delayWrite_ = 0;
  int lookaheadSamples_ = 0;

  float threshold_ = 1.0f;
  float closeThreshold_ = 1.0f;
  float minGain_ = 0.001f;
  float deBleed_ = 0.7f;
  float attackCoef_ = 0.1f;
  float releaseCoef_ = 0.001f;
  int holdSamples_ = 0;
  int hold_ = 0;
  bool gateOpen_ = false;
  float gateEnv_ = 0.0f;

  float lowGain_ = 1.0f, midGain_ = 1.0f, highGain_ = 1.0f;
  float lowDrive_ = 1.0f, midDrive_ = 1.0f, highDrive_ = 1.0f;
  float satMix_ = 0.0f;

  float bandFast_[3] = {};
  float bandSlow_[3] = {};
  float weights_[3] = {0.8f, 0.15f, 0.05f};
  float envDet_ = 0.0f;
  float fastRelease_ = 0.99f;
  float slowRelease_ = 0.999f;
  float detRelease_ = 0.9995f;

  float ghostAmount_ = 0.5f;
  float ghostRange_ = 0.1f;
  float ghostEnv_ = 0.0f;
  float ghostAttack_ = 0.1f;
  float ghostRelease_ = 0.001f;
  int ghostHold_ = 0;
  int ghostHoldSamples_ = 0;
  int midiGhostHold_ = 0;
};

} // namespace bleed
