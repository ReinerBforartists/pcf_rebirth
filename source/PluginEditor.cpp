// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PluginEditor.cpp
// @brief   Implementation of the PCF ReBirth editor.
// @details Handles UI construction, parameter attachment wiring, timer-driven
//          visual updates, manual layout calculations, and preset browser logic.
// ============================================================================

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "StepSequencer.h"
#include <JuceHeader.h>
#include <BinaryData.h>
#include <cmath>
#include <algorithm>

static juce::String pitchToNoteName(int semitones) {
  const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
  int note   = ((semitones % 12) + 12) % 12;
  int octave = 4 + static_cast<int>(std::floor(semitones / 12.0));
  return juce::String(names[note]) + juce::String(octave);
}

PCFEditor::PCFEditor(PCFProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      freqAttach         (p.apvts, "freq",          freqKnob),
      qAmtAttach        (p.apvts, "qAmt",         qAmtKnob),
      envModAttach      (p.apvts, "envMod",       envModKnob),
      slewTimeAttach    (p.apvts, "slewTime",     slewTimeKnob),
      gainAttach        (p.apvts, "gain",         gainKnob),
      syncToHostAttach  (p.apvts, "syncToHost",  syncToHostButton),
      bypassAttach      (p.apvts, "bypass",        bypassButton),
      sequencerRunAttach(p.apvts, "sequencerRun", sequencerRunButton),
      patternLengthAttach(p.apvts, "patternLength", patternLengthSlider)
{
  setLookAndFeel(&myLookAndFeel);

  // --- About Dialog Setup (Embedded Panel) ---
  aboutButton.setButtonText("About");
  aboutButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff26263a));
  aboutButton.onClick = [this] {
      if (aboutDialog == nullptr) {
          aboutDialog = std::make_unique<AboutDialog>();
          addChildComponent(aboutDialog.get());
      }
      aboutDialog->setVisible(!aboutDialog->isVisible());
  };
  addAndMakeVisible(aboutButton);

  // --- Load Logo ---
  const char* svgData = BinaryData::pcf_rebirth_logo_svg;
  const int   svgSize = BinaryData::pcf_rebirth_logo_svgSize;

  if (svgData != nullptr) {
      juce::String svgString = juce::String::fromUTF8(svgData, svgSize);
      auto xml = juce::XmlDocument::parse(svgString);
      if (xml != nullptr) {
          logoDrawable = juce::Drawable::createFromSVG(*xml);
      }
  }

  // --- Setup Knobs ---
  for (auto* knob : { &freqKnob, &qAmtKnob, &envModKnob, &slewTimeKnob, &gainKnob }) {
    knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    knob->setPopupMenuEnabled(false); // HostContextKnob::mouseDown handles right-click
    knob->setColour(juce::Slider::textBoxTextColourId,       juce::Colour(0xffc8c8d8));
    knob->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff26263a));
    knob->setColour(juce::Slider::textBoxOutlineColourId,    juce::Colour(0xff35354d));
    knob->setColour(juce::Slider::textBoxHighlightColourId,  juce::Colour(0x663d8eff));
    addAndMakeVisible(knob);
  }
  slewTimeKnob.setTextValueSuffix(" ms");
  gainKnob.setTextValueSuffix(" dB");

  // --- Bypass Button ---
  bypassButton.setButtonText("BYPASS");
  bypassButton.setClickingTogglesState(true);
  bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff3b5c));
  bypassButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff26263a));
  addAndMakeVisible(bypassButton);

  // --- Sync & Tempo ---
  syncToHostButton.setButtonText("SYNC");
  syncToHostButton.setClickingTogglesState(true);
  addAndMakeVisible(syncToHostButton);

  tempoDisplay.setEditable(true);
  tempoDisplay.setJustificationType(juce::Justification::centred);
  tempoDisplay.setColour(juce::Label::backgroundColourId,            juce::Colour(0xff26263a));
  tempoDisplay.setColour(juce::Label::outlineColourId,               juce::Colour(0xff35354d));
  tempoDisplay.setColour(juce::Label::textColourId,                  juce::Colour(0xffc8c8d8));
  tempoDisplay.setColour(juce::TextEditor::backgroundColourId,       juce::Colour(0xff26263a));
  tempoDisplay.setColour(juce::TextEditor::outlineColourId,          juce::Colour(0xff35354d));
  tempoDisplay.setColour(juce::TextEditor::focusedOutlineColourId,   juce::Colour(0xb23d8eff));
  tempoDisplay.setColour(juce::TextEditor::textColourId,             juce::Colour(0xffc8c8d8));
  tempoDisplay.setText("120", juce::dontSendNotification);
  tempoDisplay.onTextChange = [this]() {
    if (processor.apvts.getRawParameterValue("syncToHost")->load() > 0.5f) return;
    float val = juce::jlimit(40.0f, 200.0f, tempoDisplay.getText().getFloatValue());
    if (auto* param = processor.apvts.getParameter("tempo"))
      param->setValueNotifyingHost(processor.apvts.getParameterRange("tempo").convertTo0to1(val));
  };
  addAndMakeVisible(tempoDisplay);

  // --- Sequencer Run ---
  sequencerRunButton.setButtonText("RUN");
  sequencerRunButton.setClickingTogglesState(true);
  addAndMakeVisible(sequencerRunButton);

  // --- Length Label ---
  lengthLabel.setJustificationType(juce::Justification::centred);
  lengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9ab0));
  lengthLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
  lengthLabel.setText("LENGTH", juce::dontSendNotification);
  addAndMakeVisible(lengthLabel);

  // --- Filter Mode Buttons (Architektur + Slope) ---
  auto setupModeBtn = [&](juce::TextButton& btn, const juce::String& label, std::function<void()> cb) {
    btn.setButtonText(label);
    btn.setClickingTogglesState(false);
    btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff26263a));
    btn.onClick = cb;
    addAndMakeVisible(btn);
  };

  // Hilfsfunktionen: aktuelle Architektur (0=SVF, 1=MOOG) und Slope (0=LP,1=BP,2=HP) lesen
  auto getArch  = [this]() { return (int)processor.apvts.getRawParameterValue("filterMode")->load() / 3; };
  auto getSlope = [this]() { return (int)processor.apvts.getRawParameterValue("filterMode")->load() % 3; };

  auto setFilterMode = [this](int arch, int slope) {
    int mode = arch * 3 + slope;
    if (auto* param = processor.apvts.getParameter("filterMode"))
      param->setValueNotifyingHost(processor.apvts.getParameterRange("filterMode").convertTo0to1((float)mode));
  };

  setupModeBtn(filterArchSvf,  "SVF",  [=]() { setFilterMode(0, getSlope()); });
  setupModeBtn(filterArchMoog, "MOOG", [=]() { setFilterMode(1, getSlope()); });
  setupModeBtn(filterSlopeLp,  "LP",   [=]() { setFilterMode(getArch(), 0); });
  setupModeBtn(filterSlopeBp,  "BP",   [=]() { setFilterMode(getArch(), 1); });
  setupModeBtn(filterSlopeHp,  "HP",   [=]() { setFilterMode(getArch(), 2); });

  // --- Pattern Length Slider ---
  patternLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  patternLengthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 28, 18);
  patternLengthSlider.setRange(1.0, 16.0, 1.0);
  addAndMakeVisible(patternLengthSlider);

  // --- Randomization Controls ---
  randomEnableButton.setButtonText("RAND");
  randomEnableButton.setClickingTogglesState(true);
  randomEnableButton.setColour(juce::ToggleButton::textColourId,        juce::Colour(0xff9a9ab0));
  randomEnableButton.setColour(juce::ToggleButton::tickColourId,        juce::Colour(0xff3d8eff));
  randomEnableButton.setColour(juce::ToggleButton::tickDisabledColourId,juce::Colour(0xff5a5a7a));
  randomEnableButton.onStateChange = [this]() {
    processor.getStepSequencer().setRandomEnabled(randomEnableButton.getToggleState());
  };
  addAndMakeVisible(randomEnableButton);

  randomAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  randomAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
  randomAmountSlider.setRange(0.0, 48.0, 1.0);
  randomAmountSlider.setValue(3.0);
  randomAmountSlider.setColour(juce::Slider::trackColourId,            juce::Colour(0xff3d8eff));
  randomAmountSlider.setColour(juce::Slider::thumbColourId,            juce::Colour(0xff3d8eff));
  randomAmountSlider.setColour(juce::Slider::backgroundColourId,       juce::Colour(0xff26263a));
  randomAmountSlider.setColour(juce::Slider::textBoxTextColourId,      juce::Colour(0xffc8c8d8));
  randomAmountSlider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff1a1a24));
  randomAmountSlider.setColour(juce::Slider::textBoxOutlineColourId,   juce::Colours::transparentBlack);
  randomAmountSlider.textFromValueFunction = [](double v) {
    return juce::String((int)v);
  };
  randomAmountSlider.onValueChange = [this]() {
    // Normalize 0-24 range to 0.0-1.0 for the sequencer
    processor.getStepSequencer().setRandomAmount((float)randomAmountSlider.getValue() / 48.0f);
  };
  addAndMakeVisible(randomAmountSlider);

  // --- Step Gate Buttons & Pitch Sliders ---
  for (int i = 0; i < numSteps; ++i) {
    juce::String idx = juce::String(i);
    stepGateButtons[i].setButtonText(juce::String(i + 1));
    stepGateButtons[i].setClickingTogglesState(true);
    stepGateButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d97e));
    stepGateButtons[i].setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff26263a));
    stepGateButtons[i].setColour(juce::TextButton::textColourOnId,   juce::Colours::black.withAlpha(0.75f));
    addAndMakeVisible(stepGateButtons[i]);
    stepGateAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "stepGate" + idx, stepGateButtons[i]);

    stepPitchSliders[i].setSliderStyle(juce::Slider::LinearVertical);
    stepPitchSliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    stepPitchSliders[i].setRange(-24.0, 24.0, 1.0);
    stepPitchSliders[i].setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(stepPitchSliders[i]);
    stepPitchAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "stepPitch" + idx, stepPitchSliders[i]);
  }

  // --- Pitch Scale Label ---
  pitchScaleLabel.setText("+24 | -24", juce::dontSendNotification);
  pitchScaleLabel.setJustificationType(juce::Justification::centred);
  pitchScaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe8e8f0));
  pitchScaleLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
  addAndMakeVisible(pitchScaleLabel);

  // --- Preset Navigation ---
  prevPresetButton.setButtonText("<");
  prevPresetButton.onClick = [this]() {
    int currentIdx = processor.getCurrentPresetIndex();
    int total = processor.getPresetManager().getCombinedPresetCount();
    if (total > 0) {
      int prev = (currentIdx - 1 < 0) ? total - 1 : currentIdx - 1;
      processor.setActivePresetIndex(prev);
    }
  };
  addAndMakeVisible(prevPresetButton);

  nextPresetButton.setButtonText(">");
  nextPresetButton.onClick = [this]() {
    int currentIdx = processor.getCurrentPresetIndex();
    int total = processor.getPresetManager().getCombinedPresetCount();
    if (total > 0) {
      int next = (currentIdx + 1 >= total) ? 0 : currentIdx + 1;
      processor.setActivePresetIndex(next);
    }
  };
  addAndMakeVisible(nextPresetButton);

  // Preset Display Button Setup - With Multi-Column/Wrapped Height Logic
  presetMenuManager = std::make_unique<PresetBrowserPanel>(processor, 374); // panel height
  addChildComponent(presetMenuManager.get());

  presetDisplayButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff26263a));

  presetDisplayButton.onClick = [this]() {
      bool isVisible = presetMenuManager->isVisible();
      if (isVisible)
      {
          presetMenuManager->setVisible(false);
          return;
      }

      // Refresh the data before showing the panel
      presetMenuManager->refreshPresets();

      const int panelHeight = presetMenuManager->getRequiredHeight();
      const int panelWidth = getWidth() - 20;

      // The footer area starts at getHeight() - 36.
      // We want to place the bottom of the panel significantly higher than the footer.
      // Let's use a fixed margin from the very bottom of the component.
      const int marginFromBottom = 50; // Increased margin to push it up visibly
      const int bottomLimit = getHeight() - marginFromBottom;

      // Calculate Y position: Bottom of panel should be at bottomLimit
      int targetY = bottomLimit - panelHeight;

      // Ensure the top of the panel does not exit the editor bounds
      const int actualY = std::max(5, targetY);

      // Apply the new bounds
      presetMenuManager->setBounds(10, actualY, panelWidth, panelHeight);
      presetMenuManager->setVisible(true);
      presetMenuManager->toFront(false);
  };

  addAndMakeVisible(presetDisplayButton);

  // Register as listener for both Processor (preset changes) and PresetManager (save/delete)
  processor.addChangeListener(this);
  processor.getPresetManager().addChangeListener(this);
  updatePresetList();

  // --- Preset Name Input ---
  presetNameInput.setTextToShowWhenEmpty("New name...", juce::Colour(0xff5a5a7a));
  presetNameInput.setColour(juce::TextEditor::backgroundColourId,     juce::Colour(0xff26263a));
  presetNameInput.setColour(juce::TextEditor::textColourId,           juce::Colour(0xffc8c8d8));
  presetNameInput.setColour(juce::TextEditor::outlineColourId,        juce::Colour(0xff35354d));
  presetNameInput.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xb23d8eff));
  presetNameInput.setColour(juce::TextEditor::highlightColourId,      juce::Colour(0x663d8eff));
  presetNameInput.setColour(juce::TextEditor::highlightedTextColourId, juce::Colour(0xffc8c8d8));
  presetNameInput.onTextChange = [this]() {
    if (presetNameInput.getText().length() > 128)
      presetNameInput.setText(presetNameInput.getText().substring(0, 128), juce::dontSendNotification);
    updatePresetControls();
  };
  presetNameInput.addListener(this);
  addAndMakeVisible(presetNameInput);

  // --- Save/Delete Buttons ---
  savePresetButton.setButtonText("SAVE AS");
  savePresetButton.onClick = [this]() {
    int currentIdx = processor.getCurrentPresetIndex();
    const auto entries = processor.getPresetManager().getCombinedEntries();
    if (currentIdx < 0 || currentIdx >= (int)entries.size()) return;
    const auto& entry = entries[currentIdx];
    const juce::String typed = presetNameInput.getText().trim();
    if (!entry.isFactory && (typed.isEmpty() || typed == entry.name)) {
      processor.overwriteCurrentUserPreset(entry.index, entry.name);
      processor.clearDirtyFlag();
      showSavedFeedback();
      updatePresetList();
    } else {
      const juce::String saveName = typed.isNotEmpty() ? typed : entry.name;
      const int newIdx = processor.saveCurrentAsUserPreset(saveName);
      if (newIdx < 0) return;

      // Cache factory count to ensure a consistent offset during index calculation.
      // This prevents potential off-by-one mismatches if the factory list changes
      // between the save operation and the subsequent UI update.
      const int factoryPresetCount = static_cast<int>(processor.getPresetManager().getFactoryPresets().size());
      processor.setActivePresetIndex(factoryPresetCount + newIdx);

      processor.clearDirtyFlag();
      showSavedFeedback();
      updatePresetList();
    }
    presetNameInput.setText("", juce::dontSendNotification);
    updatePresetControls();
  };
  addAndMakeVisible(savePresetButton);

  deleteButton.setButtonText("DELETE");
  deleteButton.onClick = [this]() {
    int currentIdx = processor.getCurrentPresetIndex();
    const auto entries = processor.getPresetManager().getCombinedEntries();
    if (currentIdx < 0 || currentIdx >= (int)entries.size()) return;
    const auto& entry = entries[currentIdx];
    if (!entry.isFactory) {
      processor.deleteUserPreset(entry.index);
      updatePresetList();
    }
  };
  addAndMakeVisible(deleteButton);

  // --- Saved Feedback Label ---
  savedLabel.setJustificationType(juce::Justification::centredLeft);
  savedLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d97e));
  savedLabel.setVisible(false);
  addAndMakeVisible(savedLabel);

  setSize(760, 440);
  startTimerHz(30);

  // Wire up host context menus for the main knobs.
  struct KnobParam { HostContextKnob* knob; const char* id; };
  for (auto& kp : { KnobParam{&freqKnob,    "freq"},
                    KnobParam{&qAmtKnob,     "qAmt"},
                    KnobParam{&envModKnob,   "envMod"},
                    KnobParam{&slewTimeKnob, "slewTime"},
                    KnobParam{&gainKnob,     "gain"} })
  {
      kp.knob->setEditor(this);
      kp.knob->setLinkedParameter(p.apvts.getParameter(kp.id));
  }
}

PCFEditor::~PCFEditor() {
  setLookAndFeel(nullptr);
  processor.removeChangeListener(this);
  processor.getPresetManager().removeChangeListener(this);
  presetNameInput.removeListener(this);
  stopTimer();
}

void PCFEditor::hidePresetBrowser() {
  if (presetMenuManager != nullptr)
      presetMenuManager->setVisible(false);
}

void PCFEditor::mouseDown(const juce::MouseEvent& e) {
  if (presetMenuManager != nullptr && presetMenuManager->isVisible()) {
      if (!presetMenuManager->getBounds().contains(e.getPosition()))
          presetMenuManager->setVisible(false);
  }
}

void PCFEditor::textEditorFocusLost(juce::TextEditor&) {
  updatePresetControls();
}

void PCFEditor::changeListenerCallback(juce::ChangeBroadcaster* source) {
  // React to changes from both the Processor (preset selection) and PresetManager (save/delete)
  if (source == &processor || source == &processor.getPresetManager())
    updatePresetList();
}

void PCFEditor::updatePresetControls() {
  int currentIdx = processor.getCurrentPresetIndex();
  const auto entries = processor.getPresetManager().getCombinedEntries();
  if (currentIdx < 0 || currentIdx >= (int)entries.size()) {
    savePresetButton.setEnabled(false);
    deleteButton.setEnabled(false);
    return;
  }
  const auto& entry = entries[currentIdx];
  const juce::String typedName = presetNameInput.getText().trim();
  if (entry.isFactory) {
    savePresetButton.setButtonText("SAVE AS");
    savePresetButton.setEnabled(typedName.isNotEmpty());
    deleteButton.setEnabled(false);
  } else {
    const bool isOverwrite = typedName.isEmpty() || typedName == entry.name;
    savePresetButton.setButtonText(isOverwrite ? "OVERWRITE" : "SAVE AS");
    savePresetButton.setEnabled(true);
    deleteButton.setEnabled(true);
  }
}

void PCFEditor::updatePresetList() {
  if (presetMenuManager != nullptr)
      presetMenuManager->refreshPresets();

  refreshPresetDisplayName();
  updatePresetControls();
}

void PCFEditor::showSavedFeedback() {
    const int footerY = getHeight() - 36;
    const int rowSpacing = 6;

    // Position on-screen only when feedback is active
    savedLabel.setBounds(deleteButton.getBounds().getRight() + rowSpacing, footerY, 80, 22);
    savedLabel.setText("SAVED!", juce::dontSendNotification);
    savedLabel.setVisible(true);
    savedLabel.repaint();

    juce::Component::SafePointer<PCFEditor> safeThis(this);
    juce::Timer::callAfterDelay(2000, [safeThis]() {
        if (safeThis == nullptr) return;
        safeThis->savedLabel.setVisible(false);
        // Move off-screen to prevent click interception when inactive
        safeThis->savedLabel.setBounds(-100, 0, 80, 22);
    });
}

void PCFEditor::timerCallback() {
  const bool syncOn = processor.apvts.getRawParameterValue("syncToHost")->load() > 0.5f;
  const float tempo  = processor.currentTempo.load();

  // 1. Update Tempo Display
  if (!tempoDisplay.isBeingEdited())
    tempoDisplay.setText(juce::String((int)std::round((double)tempo)), juce::dontSendNotification);

  tempoDisplay.setEditable(!syncOn);
  tempoDisplay.setColour(juce::Label::textColourId, syncOn ? juce::Colour(0xff5a5a7a) : juce::Colour(0xffc8c8d8));
  tempoDisplay.setColour(juce::Label::outlineColourId, syncOn ? juce::Colour(0x6635354d) : juce::Colour(0xff35354d));

  // 2. Update Sequencer Run State
  sequencerRunButton.setToggleState(processor.apvts.getRawParameterValue("sequencerRun")->load() > 0.5f, juce::dontSendNotification);

  // 3. Update Filter Mode Highlights
  const int mode  = (int)processor.apvts.getRawParameterValue("filterMode")->load();
  const int arch  = mode / 3;
  const int slope = mode % 3;
  auto highlight = [this](juce::TextButton& btn, bool active) {
    btn.setColour(juce::TextButton::buttonColourId, active ? juce::Colour(0xBF3d8eff) : juce::Colour(0xff26263a));
  };
  highlight(filterArchSvf,  arch  == 0);
  highlight(filterArchMoog, arch  == 1);
  highlight(filterSlopeLp,  slope == 0);
  highlight(filterSlopeBp,  slope == 1);
  highlight(filterSlopeHp,  slope == 2);

  // 4. Update Step Visuals (Active step & Alpha)
  const int currentStep = processor.getStepSequencer().getCurrentStep();
  const int patLen      = (int)patternLengthSlider.getValue();
  for (int i = 0; i < numSteps; ++i) {
    stepGateButtons[i].setColour(juce::TextButton::buttonOnColourId, i == currentStep ? juce::Colour(0xffffb020) : juce::Colour(0xff00d97e));
    const float alpha = (i < patLen) ? 1.0f : 0.25f;
    stepGateButtons[i].setAlpha(alpha);
    stepPitchSliders[i].setAlpha(alpha);

    double val = stepPitchSliders[i].getValue();
    stepPitchSliders[i].setTooltip(pitchToNoteName((int)val) + " (" + juce::String(val, 1) + ")");
  }

  // 5. Update Preset Button Text & Dirty Flag (Visual Only)
  // Note: Button enable/disable states are handled via event listener now
  int currentIdx = processor.getCurrentPresetIndex();
  const auto entries = processor.getPresetManager().getCombinedEntries();
  if (currentIdx >= 0 && currentIdx < (int)entries.size()) {
    const auto& entry = entries[currentIdx];
    juce::String name = entry.isFactory ? entry.name : "[U] " + entry.name;
    if (presetDisplayButton.getButtonText() != name)
      presetDisplayButton.setButtonText(name);

    bool isDirty = processor.isPresetDirty();
    presetDisplayButton.setColour(juce::TextButton::textColourOffId, isDirty ? juce::Colour(0xffffb020) : juce::Colour(0xffc8c8d8));
  }

  repaint();
}

void PCFEditor::refreshPresetDisplayName() {
  int currentIdx = processor.getCurrentPresetIndex();
  const auto entries = processor.getPresetManager().getCombinedEntries();
  if (currentIdx >= 0 && currentIdx < (int)entries.size()) {
    const auto& entry = entries[currentIdx];
    juce::String name = entry.isFactory ? entry.name : "[U] " + entry.name;
    presetDisplayButton.setButtonText(name);
  }
}

void PCFEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(0xff141418));

  g.setColour(juce::Colour(0x8035354d));
  g.drawHorizontalLine(112, 8.0f, (float)(getWidth() - 8));
  g.drawHorizontalLine(getHeight() - 54, 8.0f, (float)(getWidth() - 8));

  const float topLabelY = 6.0f;
  g.setColour(juce::Colour(0xff9a9ab0));
  g.setFont(juce::Font(juce::FontOptions(10.5f)));

  auto label = [&](const juce::String& text, const juce::Component& c) {
    if (c.getBounds().isEmpty()) return;
    g.drawText(text, c.getX(), (int)topLabelY, c.getWidth(), 16, juce::Justification::centred);
  };

  label("FREQ",  freqKnob);
  label("Q AMT", qAmtKnob);
  label("MOD",   envModKnob);
  label("SLEW",  slewTimeKnob);
  label("GAIN",  gainKnob);
  label("BPM",   tempoDisplay);

  // ALGO über SVF/MOOG-Spalte, MODE über LP/BP/HP-Spalte
  g.drawText("ALGO", filterArchSvf.getX(), (int)topLabelY, filterArchSvf.getWidth(), 16, juce::Justification::centred);
  g.drawText("MODE", filterSlopeLp.getX(), (int)topLabelY, filterSlopeLp.getWidth(), 16, juce::Justification::centred);

  if (logoDrawable != nullptr) {
      auto bounds = logoDrawable->getBounds();
      if (!bounds.isEmpty()) {
          float scaleX = static_cast<float>(logoBounds.getWidth()) / static_cast<float>(bounds.getWidth());
          float scaleY = static_cast<float>(logoBounds.getHeight()) / static_cast<float>(bounds.getHeight());
          float scale  = std::min(scaleX, scaleY);

          juce::AffineTransform transform = juce::AffineTransform::scale(scale)
              .translated(juce::Point<float>(static_cast<float>(logoBounds.getX()), static_cast<float>(logoBounds.getY())));

          logoDrawable->draw(g, 1.0f, transform);
      }
  }

  if (pitchMidLineY > 0.0f) {
    g.setColour(juce::Colour(0x8035354d));
    g.drawHorizontalLine((int)pitchMidLineY, 10.0f, (float)(getWidth() - 10));
  }

  g.setColour(juce::Colour(0x4d35354d));
  const int seqLeft  = 10;
  const int stepW    = (getWidth() - 20) / numSteps;
  const int gateH    = 24;
  const int scaleH   = 14;
  const int pitchY   = (int)gateYCoord + gateH + 4 + scaleH;
  const int footerDivY = getHeight() - 54;
  const int pitchH   = footerDivY - pitchY - 8;
  const int pitchMid = pitchY + pitchH / 2;
  for (int i = 0; i < numSteps; ++i) {
    const int x = seqLeft + i * stepW;
    g.drawHorizontalLine(pitchMid, (float)(x + 2), (float)(x + stepW - 3));
  }

  // Randomized pitch overlay — shown when RAND is active.
  // Draws a small blue tick at the randomized pitch position for each step.
  if (randomEnableButton.getToggleState()) {
    const float pitchMin = -24.0f;
    const float pitchMax =  24.0f;
    const float pitchRange = pitchMax - pitchMin;
    const int patLen = (int)patternLengthSlider.getValue();

    for (int i = 0; i < numSteps; ++i) {
      if (i >= patLen) continue;

      const float randPitch = processor.getStepSequencer().getRandomizedPitchForStep(i);
      // LinearVertical: top = max, bottom = min
      const float norm = 1.0f - (randPitch - pitchMin) / pitchRange;
      const int x = seqLeft + i * stepW;
      const int tickY = pitchY + (int)(norm * pitchH);
      const int tickX = x + 2;
      const int tickW = stepW - 4;

      // Filled blue tick — 3px high, full step width
      g.setColour(juce::Colour(0xCC3d8eff));
      g.fillRect(tickX, tickY - 1, tickW, 3);
    }
  }
}

void PCFEditor::resized() {
  const int H = getHeight();
  const int topY = 20;

  const int knobW  = 80;
  const int knobH  = 80;

  // Spacing between function knobs 10px
  freqKnob.setBounds    ( 10, topY, knobW, knobH);
  qAmtKnob.setBounds   (100, topY, knobW, knobH); // (10 + 80) + 10 = 100
  envModKnob.setBounds (190, topY, knobW, knobH); // (100 + 80) + 10 = 190
  slewTimeKnob.setBounds(280, topY, knobW, knobH); // (190 + 80) + 10 = 280

  // Filter-Section (ALGO/MODE)
  filterArchSvf.setBounds  (375, topY + 8,  44, 20);
  filterArchMoog.setBounds (375, topY + 34, 44, 20);
  filterSlopeLp.setBounds  (425, topY + 8,  36, 20);
  filterSlopeBp.setBounds  (425, topY + 34, 36, 20);
  filterSlopeHp.setBounds  (425, topY + 60, 36, 20);

  // Tempo-Section
  tempoDisplay.setBounds    (475, topY + 8 , 60, 20);
  syncToHostButton.setBounds(475, topY + 33, 70, 22);

  // Gain Knob
  gainKnob.setBounds        (552, topY, knobW, knobH);

  // Bypass & Logo
  bypassButton.setBounds    (659, topY + 8, 74, 44);
  logoBounds = juce::Rectangle<int>(645, 78, 98, 26);

  const int seqCtrlY = 122;
  sequencerRunButton.setBounds ( 10, seqCtrlY,  50, 22);
  lengthLabel.setBounds        ( 65, seqCtrlY,  45, 22);
  patternLengthSlider.setBounds(115, seqCtrlY, 160, 22);

  // RAND toggle + amount slider, right of pattern length
  randomEnableButton.setBounds (285, seqCtrlY,  70, 22);
  randomAmountSlider.setBounds (355, seqCtrlY, 140, 22);

  const int seqLeft = 10;
  const int stepW   = (getWidth() - 20) / numSteps;
  const int gateH   = 24;
  const int gateY   = 154;
  gateYCoord        = (float)gateY;

  const int scaleH  = 14;
  const int scaleY  = gateY + gateH + 4;

  const int footerDivY = H - 54;
  const int pitchY     = scaleY + scaleH;
  const int pitchH     = footerDivY - pitchY - 8;

  pitchMidLineY = (float)(pitchY + pitchH / 2);
  pitchScaleLabel.setBounds(getWidth() / 2 - 40, scaleY, 80, scaleH);

  for (int i = 0; i < numSteps; ++i) {
    const int x = seqLeft + i * stepW;
    stepGateButtons[i].setBounds (x + 1, gateY,  stepW - 2, gateH);
    stepPitchSliders[i].setBounds(x + 1, pitchY, stepW - 2, pitchH);
  }

  const int footerY = H - 36;
  const int navBtnW = 30;
  const int comboW  = 170;
  const int rowSpacing = 6;

  prevPresetButton.setBounds(10, footerY, navBtnW, 22);
  presetDisplayButton.setBounds(10 + navBtnW + rowSpacing, footerY, comboW, 22);
  nextPresetButton.setBounds(10 + navBtnW + rowSpacing + comboW + rowSpacing, footerY, navBtnW, 22);

  const int actionX = nextPresetButton.getRight() + 12;
  presetNameInput.setBounds (actionX,                       footerY, comboW, 22);
  savePresetButton.setBounds(actionX + comboW + rowSpacing, footerY,     80, 22);
  deleteButton.setBounds    (savePresetButton.getRight() + rowSpacing, footerY, 70, 22);

  // put label offscreen to prevent it to block clicks
  savedLabel.setBounds(-100, 0, 80, 22);

  // Fix absolute position for the About button (no longer relative to the label)
  aboutButton.setBounds(getWidth() - 80, footerY, 60, 22);

  // Keep AboutDialog centered relative to editor if visible
  if (aboutDialog != nullptr && aboutDialog->isVisible()) {
      aboutDialog->centreRelativeToParent();
  }
}
