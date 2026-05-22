// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PluginProcessor.h
// @brief   Core audio processor for PCF ReBirth.
// @details Manages parameter state, audio routing, filter algorithms (Moog/SVF),
//          sequencer integration, and preset persistence.
// ============================================================================

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <vector>
#include <array>
#include "StepSequencer.h"
#include "PresetManager.h"

class PCFEditor;

class PCFProcessor : public juce::AudioProcessor,
                     public juce::AudioProcessorValueTreeState::Listener,
                     public juce::ChangeBroadcaster
{
public:
  PCFProcessor();
  ~PCFProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources();
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "PCF ReBirth"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& state) override;
  void setStateInformation(const void* stateInformation, int sizeInBytes) override;

  void loadFactoryPreset(int index);
  void loadUserPreset(int index);
  void loadPresetByIndex(int index);

  int saveCurrentAsUserPreset(const juce::String& presetName);
  bool overwriteCurrentUserPreset(int userIndex, const juce::String& presetName);
  bool renameUserPreset(int index, const juce::String& newName) { return presetManager.renameUserPreset(index, newName); }

  void deleteUserPreset(int index);

  const std::vector<SequencerPreset>& getFactoryPresets() const { return presetManager.getFactoryPresets(); }
  PresetManager& getPresetManager() { return presetManager; }

  juce::AudioProcessorValueTreeState apvts;
  StepSequencer& getStepSequencer() { return stepSequencer; }
  std::atomic<float> currentTempo{120.0f};

  bool isPresetDirty() const { return isDirty.load(); }
  void clearDirtyFlag() { isDirty.store(false); }

  int getCurrentPresetIndex() const { return activePresetIndex; }
  void setActivePresetIndex(int index);

  void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
  // --- Audio Engine Members ---
  double V[2][4];                    // Moog filter state variables
  float dcOffset[2] = {0.0f, 0.0f};  // DC offset correction
  double svfLow[2] = {};             // SVF low-pass state
  double svfBand[2] = {};            // SVF band-pass state
  double currentSampleRate = 44100.0;

  juce::LinearSmoothedValue<float> smoothedFreq;
  juce::LinearSmoothedValue<float> smoothedQAmt;
  juce::LinearSmoothedValue<float> smoothedEnvMod;

  bool wasPlaying = false;
  StepSequencer stepSequencer;
  PresetManager presetManager;

  std::atomic<bool> isDirty { false };
  std::atomic<bool> isInternalLoading { false };
  std::atomic<bool> isSaving { false };

  // Optimization: Flag to track parameter changes on audio thread
  std::atomic<bool> paramsDirty { true };

  int activePresetIndex = 0;

  // Optimization: Cached pointers to avoid map lookups in processBlock
  std::array<std::atomic<float>*, 16> stepPitchPointers;
  std::array<std::atomic<float>*, 16> stepGatePointers;
  std::atomic<float>* patternLengthPointer = nullptr;

  // --- Audio Engine Constants ---
  static constexpr double smoothingTimeSecs  = 0.02;   // Parameter smoothing ramp length
  static constexpr double moogGMaxLimit      = 1.98;   // Moog filter g saturation ceiling
  static constexpr double moogResLimit       = 0.99;   // Moog resonance hard limit
  static constexpr double svfBandLimit       = 0.99;   // SVF f coefficient hard limit
  static constexpr float  outputGain         = 2.0f;   // Post-filter output gain

  void resetFilter();
  void updateSequencerTiming();
  void applyPresetToApvts(const SequencerPreset& preset);
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
  void syncActivePresetIndexToState();
  int loadActivePresetIndexFromState();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PCFProcessor)
};
