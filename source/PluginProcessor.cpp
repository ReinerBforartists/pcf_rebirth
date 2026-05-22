// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PluginProcessor.cpp
// @brief   Implementation of PCFProcessor.
// @details Handles parameter layout creation, audio thread optimization,
//          filter math (Moog/SVF), sequencer timing updates, and preset I/O.
// ============================================================================

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

// --- Parameter Layout Definition ---
juce::AudioProcessorValueTreeState::ParameterLayout PCFProcessor::createParameters() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterFloat>("freq", "Freq",
      juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 800.f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>("qAmt", "Q Amt",
      juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.85f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>("envMod", "Env Mod",
      juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>("slewTime", "Slew Time",
      juce::NormalisableRange<float>(5.f, 500.f, 0.1f, 0.3f), 40.f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>("tempo", "Tempo",
      juce::NormalisableRange<float>(40.f, 200.f, 1.f), 120.f));
  params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
  params.push_back(std::make_unique<juce::AudioParameterBool>("sequencerRun", "Sequencer Run", true));
  params.push_back(std::make_unique<juce::AudioParameterBool>("syncToHost", "Sync To Host", true));
  params.push_back(std::make_unique<juce::AudioParameterChoice>("filterMode", "Filter Mode",
      juce::StringArray { "SVF-LP", "SVF-BP", "SVF-HP", "MOOG-LP", "MOOG-BP", "MOOG-HP" }, 1));
  params.push_back(std::make_unique<juce::AudioParameterFloat>("patternLength", "Pattern Length",
      juce::NormalisableRange<float>(1.f, 16.f, 1.f), 16.f));

  const bool defaultGates[] = { true, false, true, false, true, false, true, false,
                                 true, false, true, false, true, false, true, false };

  for (int i = 0; i < 16; ++i) {
    juce::String idx = juce::String(i);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stepPitch" + idx, "Step " + idx + " Pitch",
        juce::NormalisableRange<float>(-24.f, 24.f, 1.f), 0.f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "stepGate" + idx, "Step " + idx + " Gate", defaultGates[i]));
  }

  return juce::AudioProcessorValueTreeState::ParameterLayout(params.begin(), params.end());
}

// --- Constructor: Initialization & Pointer Caching ---
PCFProcessor::PCFProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameters()) {

  // Cache parameter pointers for high-performance audio thread access
  patternLengthPointer = apvts.getRawParameterValue("patternLength");
  for (int i = 0; i < 16; ++i) {
    juce::String idx = juce::String(i);
    stepPitchPointers[i] = apvts.getRawParameterValue("stepPitch" + idx);
    stepGatePointers[i] = apvts.getRawParameterValue("stepGate" + idx);
  }

  const juce::StringArray mainParamIds = {
    "freq", "qAmt", "envMod", "slewTime", "tempo",
    "bypass", "sequencerRun", "syncToHost", "filterMode", "patternLength"
  };

  for (auto& id : mainParamIds) {
      apvts.addParameterListener(id, this);
  }

  for (int i = 0; i < 16; ++i) {
    juce::String idx = juce::String(i);
    apvts.addParameterListener("stepPitch" + idx, this);
    apvts.addParameterListener("stepGate" + idx, this);
  }

  activePresetIndex = loadActivePresetIndexFromState();
  loadPresetByIndex(activePresetIndex);
  resetFilter();

  // Explicitly initialize after full construction to avoid premature notifications
  presetManager.initialize();
}

// --- Destructor: Listener Cleanup ---
PCFProcessor::~PCFProcessor() {
    const juce::StringArray mainParamIds = {
      "freq", "qAmt", "envMod", "slewTime", "tempo",
      "bypass", "sequencerRun", "syncToHost", "filterMode", "patternLength"
    };

    for (auto& id : mainParamIds) {
        apvts.removeParameterListener(id, this);
    }

    for (int i = 0; i < 16; ++i) {
      juce::String idx = juce::String(i);
      apvts.removeParameterListener("stepPitch" + idx, this);
      apvts.removeParameterListener("stepGate" + idx, this);
    }
}

// --- Parameter Change Handler ---
void PCFProcessor::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    if (!isInternalLoading.load() && !isSaving.load()) {
        paramsDirty.store(true);
        isDirty.store(true);

        // When the user edits a step directly, update the randomization anchor
        // so mutations in subsequent loops are based on the new intended pattern.
        if (parameterID.startsWith("stepPitch") || parameterID.startsWith("stepGate"))
            stepSequencer.commitBasePattern();
    }
}

// --- Audio Preparation ---
void PCFProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
  currentSampleRate = sampleRate;

  smoothedFreq.reset(sampleRate, smoothingTimeSecs);
  smoothedQAmt.reset(sampleRate, smoothingTimeSecs);
  smoothedEnvMod.reset(sampleRate, smoothingTimeSecs);

  smoothedFreq.setCurrentAndTargetValue(apvts.getRawParameterValue("freq")->load());
  smoothedQAmt.setCurrentAndTargetValue(apvts.getRawParameterValue("qAmt")->load());
  smoothedEnvMod.setCurrentAndTargetValue(apvts.getRawParameterValue("envMod")->load());

  stepSequencer.prepare(sampleRate, 4);
  stepSequencer.resetPosition();
  stepSequencer.setRun(true);
  stepSequencer.setTempo(apvts.getRawParameterValue("tempo")->load());

  resetFilter();
}

void PCFProcessor::releaseResources() {}

// --- Filter State Reset ---
void PCFProcessor::resetFilter() {
  memset(V, 0, sizeof(V));
  memset(svfLow, 0, sizeof(svfLow));
  memset(svfBand, 0, sizeof(svfBand));
  dcOffset[0] = 0.0f;
  dcOffset[1] = 0.0f;
}

// --- Sequencer Timing & Host Sync ---
void PCFProcessor::updateSequencerTiming() {
  bool run = apvts.getRawParameterValue("sequencerRun")->load() > 0.5f;
  bool syncHost = apvts.getRawParameterValue("syncToHost")->load() > 0.5f;

  stepSequencer.setRun(run);

  float tempo = apvts.getRawParameterValue("tempo")->load();

  if (syncHost) {
    if (auto* ph = getPlayHead()) {
      if (auto pos = ph->getPosition()) {
        if (pos->getBpm().hasValue())
          tempo = static_cast<float>(*pos->getBpm());

        bool isPlaying = pos->getIsPlaying();

        if (wasPlaying && !isPlaying) {
          stepSequencer.resetPosition();
          resetFilter();
        }
        wasPlaying = isPlaying;
        stepSequencer.setRun(run && isPlaying);
      }
    }
  }

  stepSequencer.setTempo(tempo);
  currentTempo.store(tempo);

  int patLen = static_cast<int>(patternLengthPointer->load());
  stepSequencer.setPatternLength(patLen);

  // Always push current APVTS values into the sequencer's normal active/pending path.
  // Randomization reads from randomizedActive which is separate — no conflict.
  for (int i = 0; i < 16; ++i) {
    const float pitch = stepPitchPointers[i]->load();
    const bool  gate  = stepGatePointers[i]->load() > 0.5f;
    stepSequencer.setStep(i, pitch, stepSequencer.getAccentForStep(i),
                            stepSequencer.getSlideForStep(i), gate);
  }
}

// --- Audio Processing Loop & Filter Math ---
void PCFProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) {
    if (apvts.getRawParameterValue("bypass")->load() > 0.5f) {
        return;
    }

    bool syncHost = apvts.getRawParameterValue("syncToHost")->load() > 0.5f;
    if (paramsDirty.exchange(false) || syncHost) {
        updateSequencerTiming();
    }

    smoothedFreq.setTargetValue(apvts.getRawParameterValue("freq")->load());
    smoothedQAmt.setTargetValue(apvts.getRawParameterValue("qAmt")->load());
    smoothedEnvMod.setTargetValue(apvts.getRawParameterValue("envMod")->load());

    const int filterMode = static_cast<int>(apvts.getRawParameterValue("filterMode")->load());
    const double sr      = static_cast<double>(currentSampleRate);
    const double invSr   = 1.0 / sr;
    const float  srLimit = static_cast<float>(sr) * 0.49f;
    const int    numChannels = buffer.getNumChannels(); // cache outside loop

    float slewTimeMs = apvts.getRawParameterValue("slewTime")->load();

    // Cached filter coefficients – only recomputed when safeCutoff changes.
    float cachedSafeCutoff = -1.0f;
    double cachedG = 0.0;  // Moog
    double cachedF = 0.0;  // SVF

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        stepSequencer.processSample(slewTimeMs);

        const float slewGate = stepSequencer.getSmoothedGate();
        const float accent   = stepSequencer.getCurrentAccentSmoothed();

        const float freqBase = smoothedFreq.getNextValue();
        const float qAmt     = smoothedQAmt.getNextValue();
        const float envMod   = smoothedEnvMod.getNextValue();

        const float pitchRatio = stepSequencer.getGlidedPitchRatio();
        const float baseFreq   = freqBase * pitchRatio;

        const float minCutoff  = std::max(30.0f, baseFreq * 0.05f);
        const float sweepRange = 2.0f + envMod * 8.0f;
        const float maxCutoff  = baseFreq * sweepRange;

        // Replace std::pow with std::exp(x * log(y)) – same result, same cost,
        // but makes the intent clearer and avoids the pow() call overhead.
        const float logRatio  = std::log(maxCutoff / minCutoff);
        const float cutoff    = minCutoff * std::exp(slewGate * logRatio);
        const float safeCutoff = juce::jlimit<float>(20.0f, srLimit, cutoff);

        // Recompute tan/sin only when the cutoff frequency has actually changed.
        if (safeCutoff != cachedSafeCutoff) {
            cachedSafeCutoff = safeCutoff;
            if (filterMode >= 3)
                cachedG = std::tan(juce::MathConstants<double>::pi * static_cast<double>(safeCutoff) * invSr);
            else
                cachedF = 2.0 * std::sin(juce::MathConstants<double>::pi * static_cast<double>(safeCutoff) * invSr);
        }

        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            double drive        = 0.5 + accent * 0.5;
            double inputRaw     = static_cast<double>(data[i]) * drive;
            double inputClipped = std::tanh(inputRaw);

            if (filterMode >= 3) {
                double g = juce::jlimit(0.0, moogGMaxLimit, cachedG);
                const double resonance = juce::jlimit(0.0, moogResLimit, static_cast<double>(qAmt));

                double u = inputClipped - resonance * 4.0 * V[ch][3];

                double V0 = V[ch][0];
                double V1 = V[ch][1];
                double V2 = V[ch][2];
                double V3 = V[ch][3];

                double tanhV0 = std::tanh(V0);
                double dV0 = g * (std::tanh(u) - tanhV0) / (1.0 + g * (1.0 - tanhV0 * tanhV0));
                V0 += dV0;

                double tanhV1 = std::tanh(V1);
                double dV1 = g * (std::tanh(V0) - tanhV1) / (1.0 + g * (1.0 - tanhV1 * tanhV1));
                V1 += dV1;

                // Fixed: was using tanhV1 in denominator instead of tanhV2 (dead code + bug)
                double tanhV2 = std::tanh(V2);
                double dV2 = g * (std::tanh(V1) - tanhV2) / (1.0 + g * (1.0 - tanhV2 * tanhV2));
                V2 += dV2;

                double tanhV3 = std::tanh(V3);
                double dV3 = g * (std::tanh(V2) - tanhV3) / (1.0 + g * (1.0 - tanhV3 * tanhV3));
                V3 += dV3;

                V[ch][0] = V0;
                V[ch][1] = V1;
                V[ch][2] = V2;
                V[ch][3] = V3;

                // Slope-aware Moog output
                double moogOut;
                if (filterMode == 3)       // MOOG-LP: klassischer V3-Ausgang
                    moogOut = V3;
                else if (filterMode == 4)  // MOOG-BP: Differenz zwischen Stufe 3 und 4
                    moogOut = (V2 - V3) * 2.0;
                else                       // MOOG-HP: Komplementär (Eingang minus Tiefpass)
                    moogOut = u - V3;

                // Resonanz-Lautstärkekompensation: hohe Q-Werte zehren Bassvolumen auf
                const double resonanceComp = 1.0 + resonance * 0.65;
                data[i] = static_cast<float>(moogOut * outputGain * resonanceComp);
            } else {
                double f = juce::jlimit<double>(0.0, svfBandLimit, cachedF);
                double q = 1.4 - static_cast<double>(qAmt) * 1.35;

                double low  = svfLow[ch]  + f * svfBand[ch];
                double high = inputClipped - low - q * svfBand[ch];
                double band = std::tanh(f * high + svfBand[ch]);

                svfLow[ch]  = low;
                svfBand[ch] = band;

                double out = (filterMode == 1) ? band : (filterMode == 2) ? high : low;
                data[i] = static_cast<float>(out * outputGain);
            }
        }
    }
}

// --- State Persistence (I/O) ---
void PCFProcessor::getStateInformation(juce::MemoryBlock& state) {
  auto stateTree = apvts.copyState();
  stateTree.setProperty("presetIsDirty", isDirty.load(), nullptr);
  std::unique_ptr<juce::XmlElement> xml(stateTree.createXml());
  copyXmlToBinary(*xml, state);
}

void PCFProcessor::setStateInformation(const void* stateInformation, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(stateInformation, sizeInBytes));
  if (xml != nullptr) {
    auto stateTree = juce::ValueTree::fromXml(*xml);

    // Restore the dirty flag before replacing state so we know whether the
    // user had unsaved modifications when the project was saved.
    bool wasDirty = (bool)stateTree.getProperty("presetIsDirty", false);

    // replaceState restores all APVTS parameters exactly as the user left them,
    // including any modifications made after loading a preset.
    apvts.replaceState(stateTree);

    // Restore the active preset index only – do NOT call loadPresetByIndex()
    // here because that would overwrite the user's modifications with the
    // clean preset data.
    activePresetIndex = loadActivePresetIndexFromState();
    isDirty.store(wasDirty);
  }
}

// --- Preset Loading & Application ---
void PCFProcessor::loadFactoryPreset(int index) {
  SequencerPreset preset;
  if (presetManager.getPresetByIndex(index, true, preset)) {
    applyPresetToApvts(preset);
  }
}

void PCFProcessor::loadUserPreset(int index) {
  SequencerPreset preset;
  if (presetManager.getPresetByIndex(index, false, preset)) {
    applyPresetToApvts(preset);
  }
}

void PCFProcessor::applyPresetToApvts(const SequencerPreset& preset) {
  {
    struct LoadingGuard {
        std::atomic<bool>& flag;
        LoadingGuard(std::atomic<bool>& f) : flag(f) { flag.store(true); }
        ~LoadingGuard() { flag.store(false); }
    } guard(isInternalLoading);

    if (auto* param = apvts.getParameter("patternLength")) {
      param->setValueNotifyingHost(apvts.getParameterRange("patternLength").convertTo0to1((float)preset.patternLength));
    }

    for (int i = 0; i < 16; ++i) {
      juce::String idx = juce::String(i);
      if (auto* pParam = apvts.getParameter("stepPitch" + idx)) {
        pParam->setValueNotifyingHost(apvts.getParameterRange("stepPitch" + idx).convertTo0to1(preset.pitches[i]));
      }
      if (auto* gParam = apvts.getParameter("stepGate" + idx)) {
        gParam->setValueNotifyingHost(apvts.getParameterRange("stepGate" + idx).convertTo0to1((float)preset.gates[i]));
      }
    }
  } // LoadingGuard released here

  updateSequencerTiming();
  isDirty.store(false);

  // Sync the sequencer pending buffer with the freshly loaded preset values,
  // then commit as the randomization anchor. Must happen AFTER the loading guard
  // is released so setStep calls aren't blocked.
  for (int i = 0; i < 16; ++i) {
    juce::String idx = juce::String(i);
    const float pitch = apvts.getRawParameterValue("stepPitch" + idx)->load();
    const bool  gate  = apvts.getRawParameterValue("stepGate"  + idx)->load() > 0.5f;
    stepSequencer.setStep(i, pitch, stepSequencer.getAccentForStep(i),
                            stepSequencer.getSlideForStep(i), gate);
  }
  stepSequencer.commitBasePattern();
}

// --- Preset Saving & Overwriting ---
int PCFProcessor::saveCurrentAsUserPreset(const juce::String& presetName) {
  if (presetName.isEmpty()) return -1;

  struct SavingGuard {
      std::atomic<bool>& flag;
      SavingGuard(std::atomic<bool>& f) : flag(f) { flag.store(true); }
      ~SavingGuard() { flag.store(false); }
  } guard(isSaving);

  SequencerPreset newPreset;
  newPreset.name = presetName;
  newPreset.category = "User";
  newPreset.patternLength = static_cast<int>(apvts.getRawParameterValue("patternLength")->load());

  for (int i = 0; i < 16; ++i) {
    juce::String idx = juce::String(i);
    newPreset.pitches[i] = apvts.getRawParameterValue("stepPitch" + idx)->load();
    newPreset.gates[i] = apvts.getRawParameterValue("stepGate" + idx)->load() > 0.5f;
  }

  const int newIndex = presetManager.saveUserPreset(newPreset);
  isDirty.store(false);
  return newIndex;
}

bool PCFProcessor::overwriteCurrentUserPreset(int userIndex, const juce::String& presetName) {
  if (presetName.isEmpty() || userIndex < 0) return false;

  struct SavingGuard {
      std::atomic<bool>& flag;
      SavingGuard(std::atomic<bool>& f) : flag(f) { flag.store(true); }
      ~SavingGuard() { flag.store(false); }
  } guard(isSaving);

  SequencerPreset preset;
  preset.name = presetName;
  preset.category = "User";
  preset.patternLength = static_cast<int>(apvts.getRawParameterValue("patternLength")->load());

  for (int i = 0; i < 16; ++i) {
    juce::String idx = juce::String(i);
    preset.pitches[i] = apvts.getRawParameterValue("stepPitch" + idx)->load();
    preset.gates[i] = apvts.getRawParameterValue("stepGate" + idx)->load() > 0.5f;
  }

  const bool ok = presetManager.overwriteUserPreset(userIndex, preset);
  if (ok) isDirty.store(false);
  return ok;
}

// --- Editor & Plugin Creation ---
juce::AudioProcessorEditor* PCFProcessor::createEditor() { return new PCFEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PCFProcessor(); }

// --- Preset Index Management ---
void PCFProcessor::loadPresetByIndex(int index) {
    auto entries = presetManager.getCombinedEntries();
    int total = static_cast<int>(entries.size());

    if (total > 0) {
        index = juce::jlimit(0, total - 1, index);
        activePresetIndex = index;

        const auto& entry = entries[index];
        if (entry.isFactory) loadFactoryPreset(entry.index);
        else                 loadUserPreset(entry.index);

        syncActivePresetIndexToState();
    }
}

void PCFProcessor::setActivePresetIndex(int index) {
    activePresetIndex = index;
    loadPresetByIndex(index);
    sendChangeMessage(); // Notify UI immediately when index changes
}

void PCFProcessor::syncActivePresetIndexToState() {
    apvts.state.setProperty("activePresetIndex", activePresetIndex, nullptr);
}

int PCFProcessor::loadActivePresetIndexFromState() {
    return static_cast<int>(apvts.state.getProperty("activePresetIndex", 0));
}

void PCFProcessor::deleteUserPreset(int index) {
    presetManager.deleteUserPreset(index);
    int total = static_cast<int>(presetManager.getCombinedEntries().size());
    activePresetIndex = juce::jlimit(0, total > 0 ? total - 1 : 0, activePresetIndex);
    syncActivePresetIndexToState();
}
