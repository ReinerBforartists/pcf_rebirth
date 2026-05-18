// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    StepSequencer.cpp
// @brief   Implementation of the real-time step sequencer.
// @details Handles double-buffer synchronization, sample-accurate timing,
//          pitch glide calculation, gate slew, and accent smoothing.
// ============================================================================

#include "StepSequencer.h"
#include <cmath>
#include <algorithm>

// --- Constructor & Initialization ---
StepSequencer::StepSequencer() {
  const float defaultPitches[] = {
      0.00f, 0.00f, 0.07f, 0.00f, 0.04f, 0.07f, 0.00f, 0.00f,
      0.00f, 0.07f, 0.00f, 0.04f, 0.07f, 0.00f, 0.00f, 0.00f};
  const float defaultAccents[] = {
      0.7f, 0.7f, 1.0f, 0.7f, 0.8f, 1.0f, 0.7f, 0.7f,
      0.7f, 1.0f, 0.7f, 0.8f, 1.0f, 0.7f, 0.7f, 0.7f};
  const bool defaultGates[] = {
      true, true, true, true, true, true, true, true,
      true, true, true, true, true, true, true, true};
  const bool defaultSlides[] = {
      false, false, true, false, false, true, false, false,
      false, true, false, false, true, false, false, false};

  for (int i = 0; i < maxPatternLength; ++i) {
    if (i < 16) {
      active.pitches[i]  = defaultPitches[i];
      active.accents[i]  = defaultAccents[i];
      active.gates[i]    = defaultGates[i];
      active.slides[i]   = defaultSlides[i];
    } else {
      active.pitches[i]  = 0.0f;
      active.accents[i]  = defaultAccent;
      active.gates[i]    = false;
      active.slides[i]   = false;
    }
    // Keep pending in sync initially
    pending.pitches[i] = active.pitches[i];
    pending.accents[i] = active.accents[i];
    pending.gates[i]   = active.gates[i];
    pending.slides[i]  = active.slides[i];
  }

  glidedPitch           = active.pitches[0];
  currentAccentSmoothed = defaultAccent;
  currentEnvAmount      = 0.0f;
  smoothedGate          = 0.0f;

  updateCurrentValues();
  updateTiming();
  samplesUntilNextStep = static_cast<int>(samplesPerStep);
}

// --- UI Thread Methods ---
void StepSequencer::setStep(int index, float pitch, float accent, bool slide, bool gate) {
  if (index >= 0 && index < maxPatternLength) {
    pending.pitches[index] = pitch;
    pending.accents[index] = accent;
    pending.slides[index]  = slide;
    pending.gates[index]   = gate;
    // Signal the audio thread to swap on next processSample()
    swapPending.store(true, std::memory_order_release);
  }
}

void StepSequencer::setTempo(float bpm) {
  tempo = juce::jlimit(30.0f, 300.0f, bpm);
  updateTiming();
  // Don't reset samplesUntilNextStep here – let it count down naturally
  // so an in-progress step finishes at the correct length. The new tempo
  // takes effect from the next step boundary.
}

void StepSequencer::prepare(double sr, int spb) {
  sampleRate   = sr;
  stepsPerBeat = spb;
  updateTiming();
  samplesUntilNextStep = static_cast<int>(samplesPerStep);
  running.store(true);
}

// --- Audio Thread Methods ---
void StepSequencer::applyPendingStepData() {
  // Check with acquire so we see all writes done before the store(release) in setStep().
  if (swapPending.load(std::memory_order_acquire)) {
    // Copy the pending buffer into the active buffer.
    // Both are plain structs (no dynamic allocation), so memcpy is safe and fast.
    active = pending;
    swapPending.store(false, std::memory_order_relaxed);
  }

  // Apply pattern length change atomically (no struct needed, just an int).
  patternLength = pendingPatternLength.load(std::memory_order_relaxed);
}

// --- Per-Sample Processing & Smoothing ---
void StepSequencer::processSample(float slewTimeMs) {
  // Always apply pending data first, even when not running, so UI edits
  // are reflected immediately on the next playback start.
  applyPendingStepData();

  if (!running.load(std::memory_order_relaxed))
    return;

  // Integer counter: immune to floating-point drift across tempo changes.
  --samplesUntilNextStep;
  if (samplesUntilNextStep <= 0) {
    samplesUntilNextStep = static_cast<int>(samplesPerStep);
    advanceStep();
  }

  const float srF = static_cast<float>(sampleRate);

  // 1. Pitch Glide
  float targetPitch = currentGate ? currentPitch : 0.0f;
  float glideTau    = glideTimeMs * 0.001f;
  float glideCoeff  = 1.0f - std::expf(-1.0f / (glideTau * srF));
  glidedPitch      += (targetPitch - glidedPitch) * glideCoeff;
  glidedPitchRatio  = std::pow(2.0f, glidedPitch / 12.0f);

  // 2. Gate Slew
  float tau        = slewTimeMs * 0.001f;
  float gateCoeff  = 1.0f - std::expf(-1.0f / (tau * srF));
  float targetGate = currentGate ? 1.0f : 0.0f;
  smoothedGate    += (targetGate - smoothedGate) * gateCoeff;

  // 3. Accent Smoothing
  float accentSlewCoeff  = 1.0f - std::expf(-1.0f / (srF * accentSlewMs * 0.001f));
  currentAccentSmoothed += (currentAccent - currentAccentSmoothed) * accentSlewCoeff;

  // 4. Envelope Amount
  float targetEnv = smoothedGate;
  float envCharge = 1.0f - std::expf(-1.0f / (srF * envChargeMs * 0.001f));
  float envDecay  = 1.0f - std::expf(-1.0f / (tau * srF));
  float envCoeff  = targetEnv > currentEnvAmount ? envCharge : envDecay;
  currentEnvAmount += (targetEnv - currentEnvAmount) * envCoeff;
}

// --- Step Advancement & Timing ---
void StepSequencer::advanceStep() {
  ++currentStep;
  if (currentStep >= patternLength)
    currentStep = 0;

  updateCurrentValues();
}

void StepSequencer::updateCurrentValues() {
  if (currentStep < maxPatternLength) {
    currentPitch  = active.pitches[currentStep];
    currentAccent = active.accents[currentStep];
    currentGate   = active.gates[currentStep];
  }
}

void StepSequencer::resetPosition() {
  currentStep          = 0;
  samplesUntilNextStep = static_cast<int>(samplesPerStep);
  glidedPitch          = active.pitches[0];
  currentAccentSmoothed = defaultAccent;
  currentEnvAmount     = 0.0f;
  smoothedGate         = 0.0f;
}

void StepSequencer::updateTiming() {
  if (sampleRate > 0.0 && tempo > 0.0 && stepsPerBeat > 0) {
    double samplesPerBeat = sampleRate / (tempo / 60.0);
    samplesPerStep = samplesPerBeat / static_cast<double>(stepsPerBeat);
    if (samplesPerStep < 1.0)
      samplesPerStep = 1.0;
  }
}
