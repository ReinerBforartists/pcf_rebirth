// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PluginEditor.h
// @brief   Main editor component for the PCF ReBirth plugin.
// @details Handles UI layout, parameter attachments, timer-driven updates,
//          preset browser panel, about dialog integration, and custom drawing.
// ============================================================================

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>
#include "PluginProcessor.h"
#include "AboutDialog.h"
#include "PresetBrowserPanel.h"

class PCFProcessor;

// --- Custom LookAndFeel for dark theme styling ---
class PCF_LookAndFeel : public juce::LookAndFeel_V4
{
public:
    PCF_LookAndFeel()
    {
        setColour(juce::Label::textColourId,                       juce::Colour(0xffc8c8d8));
        setColour(juce::Label::backgroundColourId,                 juce::Colours::transparentBlack);
        setColour(juce::Label::outlineColourId,                    juce::Colours::transparentBlack);
        setColour(juce::Label::textWhenEditingColourId,            juce::Colour(0xffc8c8d8));

        setColour(juce::TextEditor::backgroundColourId,            juce::Colour(0xff26263a));
        setColour(juce::TextEditor::textColourId,                  juce::Colour(0xffc8c8d8));
        setColour(juce::TextEditor::highlightColourId,             juce::Colour(0x663d8eff));
        setColour(juce::TextEditor::highlightedTextColourId,       juce::Colour(0xffc8c8d8));
        setColour(juce::TextEditor::outlineColourId,               juce::Colour(0xff35354d));
        setColour(juce::TextEditor::focusedOutlineColourId,        juce::Colour(0xb23d8eff));
        setColour(juce::TextEditor::shadowColourId,                juce::Colours::transparentBlack);

        setColour(juce::ComboBox::backgroundColourId,              juce::Colour(0xff26263a));
        setColour(juce::ComboBox::textColourId,                    juce::Colour(0xffc8c8d8));
        setColour(juce::ComboBox::outlineColourId,                 juce::Colour(0xff35354d));
        setColour(juce::ComboBox::buttonColourId,                  juce::Colour(0xff26263a));
        setColour(juce::ComboBox::arrowColourId,                   juce::Colour(0xff5a5a7a));
        setColour(juce::ComboBox::focusedOutlineColourId,          juce::Colour(0xb23d8eff));

        setColour(juce::PopupMenu::backgroundColourId,             juce::Colour(0xff1e1e26));
        setColour(juce::PopupMenu::textColourId,                   juce::Colour(0xffc8c8d8));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,  juce::Colour(0x403d8eff));
        setColour(juce::PopupMenu::highlightedTextColourId,        juce::Colours::white);

        setColour(juce::Slider::backgroundColourId,                juce::Colour(0xff26263a));
        setColour(juce::Slider::trackColourId,                     juce::Colour(0xff3d8eff));
        setColour(juce::Slider::thumbColourId,                     juce::Colour(0xff3d8eff));
        setColour(juce::Slider::textBoxTextColourId,               juce::Colour(0xffc8c8d8));
        setColour(juce::Slider::textBoxBackgroundColourId,         juce::Colour(0xff26263a));
        setColour(juce::Slider::textBoxHighlightColourId,          juce::Colour(0x663d8eff));
        setColour(juce::Slider::textBoxOutlineColourId,            juce::Colour(0xff35354d));

        setColour(juce::TextButton::buttonColourId,                juce::Colour(0xff26263a));
        setColour(juce::TextButton::buttonOnColourId,              juce::Colour(0xff3d8eff));
        setColour(juce::TextButton::textColourOffId,               juce::Colour(0xffc8c8d8));
        setColour(juce::TextButton::textColourOnId,                juce::Colours::white);

        setColour(juce::ToggleButton::textColourId,                juce::Colour(0xffc8c8d8));
        setColour(juce::ToggleButton::tickColourId,                juce::Colour(0xff3d8eff));
        setColour(juce::ToggleButton::tickDisabledColourId,        juce::Colour(0xff5a5a7a));
    }

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float radius = juce::jmin(width, height) * 0.5f - 4.0f;
        const float arcThickness = 3.5f;
        const float arcR = radius - arcThickness * 0.5f;

        juce::Path trackArc;
        trackArc.addCentredArc(cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff35354d));
        g.strokePath(trackArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        juce::Path valueArc;
        valueArc.addCentredArc(cx, cy, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xff3d8eff));
        g.strokePath(valueArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float knobR = radius - arcThickness - 3.0f;
        g.setColour(juce::Colour(0xff1e1e26));
        g.fillEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);
        g.setColour(juce::Colour(0xff35354d));
        g.drawEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

        const float pDist = knobR * 0.35f;
        const float pLen  = knobR * 0.55f;
        g.setColour(juce::Colour(0xff3d8eff));
        g.drawLine(cx + std::sin(angle) * pDist,
                   cy - std::cos(angle) * pDist,
                   cx + std::sin(angle) * (pDist + pLen),
                   cy - std::cos(angle) * (pDist + pLen), 2.0f);
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox(slider);
        label->setColour(juce::Label::textColourId,                juce::Colour(0xffc8c8d8));
        label->setColour(juce::Label::backgroundColourId,          juce::Colour(0xff26263a));
        label->setColour(juce::Label::outlineColourId,             juce::Colour(0xff35354d));
        label->setColour(juce::TextEditor::backgroundColourId,     juce::Colour(0xff26263a));
        label->setColour(juce::TextEditor::textColourId,           juce::Colour(0xffc8c8d8));
        label->setColour(juce::TextEditor::outlineColourId,        juce::Colour(0xff35354d));
        label->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xb23d8eff));
        return label;
    }

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isHighlighted,
                              bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        juce::Colour fill = backgroundColour;
        if (isDown)        fill = fill.darker(0.2f);
        else if (isHighlighted) fill = fill.brighter(0.08f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xff35354d));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawToggleButton(juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool isHighlighted,
                          bool) override
    {
        const float boxSize = 14.0f;
        const float boxX    = 2.0f;
        const float boxY    = (button.getHeight() - boxSize) * 0.5f;

        g.setColour(juce::Colour(0xff26263a));
        g.fillRoundedRectangle(boxX, boxY, boxSize, boxSize, 3.0f);
        g.setColour(isHighlighted ? juce::Colour(0xff3d8eff) : juce::Colour(0xff35354d));
        g.drawRoundedRectangle(boxX, boxY, boxSize, boxSize, 3.0f, 1.0f);

        if (button.getToggleState()) {
            g.setColour(juce::Colour(0xff3d8eff));
            g.fillRoundedRectangle(boxX + 3.5f, boxY + 3.5f, boxSize - 7.0f, boxSize - 7.0f, 1.5f);
        }

        g.setColour(juce::Colour(0xffc8c8d8));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(button.getButtonText(),
                   static_cast<int>(boxX + boxSize + 5), 0,
                   button.getWidth() - static_cast<int>(boxSize) - 8,
                   button.getHeight(),
                   juce::Justification::centredLeft);
    }

    void drawLinearSlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos, float, float,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            const float cx     = x + width * 0.5f;
            const float trackW = 2.0f;
            const float thumbR = 5.0f;
            g.setColour(juce::Colour(0xff35354d));
            g.fillRoundedRectangle(cx - trackW * 0.5f, static_cast<float>(y), trackW, static_cast<float>(height), 1.0f);
            g.setColour(juce::Colour(0xff3d8eff));
            g.fillEllipse(cx - thumbR, sliderPos - thumbR, thumbR * 2.0f, thumbR * 2.0f);
            g.setColour(juce::Colour(0xff1e1e26));
            g.fillEllipse(cx - 2.0f, sliderPos - 2.0f, 4.0f, 4.0f);
        }
        else
        {
            juce::ignoreUnused(slider);
            const float trackY = y + height * 0.5f;
            const float trackH = 3.0f;
            const float thumbR = 6.0f;
            g.setColour(juce::Colour(0xff35354d));
            g.fillRoundedRectangle(static_cast<float>(x), trackY - trackH * 0.5f, static_cast<float>(width), trackH, 1.5f);
            g.setColour(juce::Colour(0xff3d8eff));
            g.fillRoundedRectangle(static_cast<float>(x), trackY - trackH * 0.5f, sliderPos - x, trackH, 1.5f);
            g.setColour(juce::Colour(0xff3d8eff));
            g.fillEllipse(sliderPos - thumbR, trackY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
            g.setColour(juce::Colour(0xff1e1e26));
            g.fillEllipse(sliderPos - thumbR + 2.5f, trackY - thumbR + 2.5f, (thumbR - 2.5f) * 2.0f, (thumbR - 2.5f) * 2.0f);
        }
    }

    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool, int, int, int, int, juce::ComboBox& box) override
    {
        juce::Rectangle<float> bounds(0.5f, 0.5f, width - 1.0f, height - 1.0f);
        g.setColour(juce::Colour(0xff26263a));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(box.hasKeyboardFocus(false) ? juce::Colour(0xb23d8eff) : juce::Colour(0xff35354d));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        const float ax = width - 18.0f, ay = height * 0.5f - 3.0f;
        juce::Path arrow;
        arrow.startNewSubPath(ax, ay);
        arrow.lineTo(ax + 6.0f, ay);
        arrow.lineTo(ax + 3.0f, ay + 5.0f);
        arrow.closeSubPath();
        g.setColour(juce::Colour(0xff5a5a7a));
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(juce::FontOptions(12.0f));
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        g.setColour(juce::Colour(0xff1e1e26));
        g.fillRect(0, 0, width, height);
        g.setColour(juce::Colour(0xff35354d));
        g.drawRect(0, 0, width, height, 1);
    }

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool, bool, const juce::String& text,
                           const juce::String&, const juce::Drawable*, const juce::Colour*) override
    {
        if (isSeparator) {
            g.setColour(juce::Colour(0xff35354d));
            g.fillRect(area.getX() + 8, area.getCentreY(), area.getWidth() - 16, 1);
            return;
        }
        if (isHighlighted && isActive) {
            g.setColour(juce::Colour(0x403d8eff));
            g.fillRoundedRectangle(area.toFloat().reduced(2.0f, 1.0f), 3.0f);
        }
        g.setColour(isActive ? juce::Colour(0xffc8c8d8) : juce::Colour(0xff5a5a7a));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText(text, area.getX() + 10, area.getY(),
                   area.getWidth() - 14, area.getHeight(),
                   juce::Justification::centredLeft);
    }
};

// --- Host-aware Knob ---
// Extends juce::Slider to show the VST3 host context menu on right-click.
// This gives FL Studio (and other VST3 hosts) the chance to inject their own
// entries like "Create automation clip" or "MIDI learn".
// Falls back to the standard JUCE popup menu if no host context is available.
class HostContextKnob : public juce::Slider
{
public:
    // Set after construction, once the editor exists.
    void setEditor(juce::AudioProcessorEditor* e) { editor = e; }

    // Set after the SliderAttachment is created so we know which parameter
    // this knob is bound to.
    void setLinkedParameter(juce::AudioProcessorParameter* p) { linkedParam = p; }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            showHostOrFallbackMenu(e);
            return;
        }
        juce::Slider::mouseDown(e);
    }

private:
    juce::AudioProcessorEditor*    editor      = nullptr;
    juce::AudioProcessorParameter* linkedParam = nullptr;

    void showHostOrFallbackMenu(const juce::MouseEvent& e)
    {
        if (editor != nullptr && linkedParam != nullptr)
        {
            if (auto* hostContext = editor->getHostContext())
            {
                if (auto hostMenu = hostContext->getContextMenuForParameter(linkedParam))
                {
                    // FL Studio does not DPI-virtualise its plugin window on Windows,
                    // so it expects raw physical pixels. JUCE's localPointToGlobal
                    // returns logical pixels (already scaled by the OS scale factor).
                    // Dividing by the display scale factor converts back to physical pixels.
                    // FL Studio expects coordinates relative to the plugin window.
                    auto pos = e.getEventRelativeTo(editor).getPosition();
                    hostMenu->showNativeMenu(pos);
                    return;
                }
            }
        }

        setPopupMenuEnabled(true);
        juce::Slider::mouseDown(e);
        setPopupMenuEnabled(false);
    }
};

class PCFEditor : public juce::AudioProcessorEditor,
                  private juce::Timer,
                  private juce::ChangeListener,
                  private juce::TextEditor::Listener
{
public:
  PCFEditor(PCFProcessor&);
  ~PCFEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  void timerCallback() override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;

  void changeListenerCallback(juce::ChangeBroadcaster* source) override;
  void textEditorFocusLost(juce::TextEditor& editor) override;

  // Public method to hide the browser panel from within the panel itself
  void hidePresetBrowser();

private:
  void updatePresetList();
  void showSavedFeedback();
  void updatePresetControls();
  void refreshPresetDisplayName();

  // --- Core References & LookAndFeel ---
  PCFProcessor& processor;
  PCF_LookAndFeel myLookAndFeel;

  // --- Parameter Knobs & Attachments ---
  HostContextKnob freqKnob, qAmtKnob, envModKnob, slewTimeKnob, gainKnob;
  juce::AudioProcessorValueTreeState::SliderAttachment freqAttach, qAmtAttach, envModAttach, slewTimeAttach, gainAttach;

  // --- Toggles & Display ---
  juce::ToggleButton syncToHostButton;
  juce::AudioProcessorValueTreeState::ButtonAttachment syncToHostAttach;
  juce::Label tempoDisplay;

  juce::ToggleButton sequencerRunButton;
  juce::AudioProcessorValueTreeState::ButtonAttachment sequencerRunAttach;

  juce::Label lengthLabel;

  static constexpr int numSteps = 16;
  juce::TextButton stepGateButtons[numSteps];
  juce::Slider     stepPitchSliders[numSteps];

  std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, numSteps> stepGateAttachments;
  std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numSteps> stepPitchAttachments;

  // --- Bypass & Filter Mode ---
  juce::TextButton bypassButton;
  juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttach;

  // Reihe 1: Filterarchitektur
  juce::TextButton filterArchSvf, filterArchMoog;
  // Reihe 2: Ausgangspol
  juce::TextButton filterSlopeLp, filterSlopeBp, filterSlopeHp;
  juce::Slider patternLengthSlider;
  juce::AudioProcessorValueTreeState::SliderAttachment patternLengthAttach;

  // --- Randomization ---
  juce::ToggleButton randomEnableButton;
  juce::Slider       randomAmountSlider;
  juce::TextButton presetDisplayButton;
  juce::TextButton prevPresetButton;
  juce::TextButton nextPresetButton;
  std::unique_ptr<PresetBrowserPanel> presetMenuManager;

  juce::TextEditor presetNameInput;
  juce::TextButton savePresetButton;
  juce::TextButton deleteButton;
  juce::Label savedLabel;

  juce::Label pitchScaleLabel;

  // --- Layout & Visual Helpers ---
  float gateYCoord   = 0.0f;
  float pitchMidLineY = 0.0f;

  // --- About Dialog ---
  juce::TextButton aboutButton;
  std::unique_ptr<AboutDialog> aboutDialog;

  // --- Logo Handling ---
  std::unique_ptr<juce::Drawable> logoDrawable;
  juce::Rectangle<int> logoBounds;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PCFEditor)
};
