// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    StepSequencer.h
// @brief   Real-time step sequencer with double-buffered state management.
// @details Handles sample-accurate timing, pitch glide, gate slew, accent
//          smoothing, and thread-safe UI/audio buffer swapping.
// ============================================================================

#pragma once
#include <juce_core/juce_core.h>
#include <atomic>

// --- Public API ---
class StepSequencer {
 public:
  StepSequencer();
  ~StepSequencer() = default;

  void prepare(double sampleRate, int stepsPerBeat = 4);
  void processSample(float slewTime);

  // Thread-safe: can be called from UI/message thread at any time.
  // Data is written into the pending buffer and swapped in at the
  // beginning of the next processSample() call on the audio thread.
  void setStep(int index, float pitch, float accent, bool slide, bool gate);

  void setPatternLength(int steps) { pendingPatternLength.store(juce::jlimit(1, 32, steps)); }

  void setRandomEnabled(bool on) {
    randomEnabled.store(on);
    if (on)
      randomizeNow.store(true); // audio thread will call applyRandomizationToNext() immediately
  }
  void setRandomAmount(float amt) { randomAmount.store(juce::jlimit(0.0f, 1.0f, amt)); }
  bool getRandomEnabled() const   { return randomEnabled.load(); }
  float getRandomAmount() const   { return randomAmount.load(); }

  // Call from UI thread whenever a step is edited or a preset is loaded,
  // so the base anchor stays in sync with user intent.
  void commitBasePattern() {
    commitBaseNow.store(true); // audio thread will copy active→basePattern on next sample
  }  int getPatternLength() const { return patternLength; }
  void setTempo(float bpm);
  void setRun(bool shouldRun) { running.store(shouldRun); }
  void resetPosition();

  float getCurrentPitch() const { return currentPitch; }
  float getCurrentAccent() const { return currentAccent; }
  float getCurrentAccentSmoothed() const { return currentAccentSmoothed; }
  float getCurrentEnvAmount() const { return currentEnvAmount; }
  bool getCurrentGate() const { return currentGate; }
  float getSmoothedGate() const { return smoothedGate; }
  int getCurrentStep() const { return currentStep; }
  float getGlidedPitch() const { return glidedPitch; }
  float getGlidedPitchRatio() const { return glidedPitchRatio; }

  // These read from the pending (write) buffer, safe to call from UI thread.
  float getAccentForStep(int i) const {
    return (i >= 0 && i < maxPatternLength) ? pending.accents[i] : defaultAccent;
  }
  bool getSlideForStep(int i) const {
    return (i >= 0 && i < maxPatternLength) ? pending.slides[i] : false;
  }

  // --- Private Implementation ---
 private:
  void advanceStep();
  void updateCurrentValues();
  void updateTiming();

  // Swaps pending data into the active buffer if a swap is flagged.
  // Called only from the audio thread at the top of processSample().
  void applyPendingStepData();

  static constexpr int maxPatternLength = 32;

  // --- Timing & Envelope Constants ---
  static constexpr float accentSlewMs   = 1.0f;    // Accent smoothing time in ms
  static constexpr float envChargeMs    = 0.5f;    // Envelope attack time in ms
  static constexpr float defaultAccent  = 0.7f;    // Default accent level

  // --- Double-Buffer ---
  struct StepData {
    float pitches[maxPatternLength];
    float accents[maxPatternLength];
    bool  slides[maxPatternLength];
    bool  gates[maxPatternLength];
  };

  StepData active;
  StepData pending;

  // Separate buffer owned exclusively by the audio thread.
  // applyRandomizationToNext() writes here; updateCurrentValues() reads
  // from it when randomization is on. The normal active/pending swap
  // is completely unaffected.
  StepData randomizedActive;

  // Set to true by the UI thread after writing to `pending`.
  // Consumed (set to false) by the audio thread in applyPendingStepData().
  std::atomic<bool> swapPending { false };

  // --- Randomization ---
  // basePattern is a snapshot of the "intended" pattern, set whenever the
  // user edits a step or loads a preset. Randomization always derives from
  // this anchor so variation never drifts cumulatively over loops.
  StepData basePattern;
  std::atomic<float> randomAmount { 0.0f };
  std::atomic<bool>  randomEnabled { false };
  std::atomic<bool>  randomizeNow  { false };
  std::atomic<bool>  commitBaseNow { false }; // audio thread commits active→basePattern

  // Simple LCG — audio-thread only, no locking needed.
  uint32_t rngState { 12345 };
  uint32_t nextRng() {
    rngState = rngState * 1664525u + 1013904223u;
    return rngState;
  }
  void applyRandomizationToNext();

  // patternLength changes are also written atomically so no struct swap needed.
  std::atomic<int>  pendingPatternLength { 16 };
  std::atomic<bool> running { true };

  int patternLength = 16;
  int currentStep = 0;

  // Integer-based sample counter to avoid floating-point drift on tempo changes.
  int samplesUntilNextStep = 0;

  float currentPitch = 0.0f;
  float currentAccent = 1.0f;
  float currentAccentSmoothed = 0.7f;
  float currentEnvAmount = 0.0f;
  bool  currentGate = false;
  float smoothedGate = 0.0f;

  float glidedPitch = 0.0f;
  float glidedPitchRatio = 1.0f; // cached: pow(2, glidedPitch/12) – updated per sample
  float glideTimeMs = 20.0f;

  double sampleRate = 44100.0;
  double samplesPerStep = 5512.5;

  float tempo = 120.0f;
  int stepsPerBeat = 4;
};
