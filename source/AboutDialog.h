// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    AboutDialog.h
// @brief   Embedded about dialog component.
// @details Displays plugin version, credits, license info, and external links.
//          Inherits from juce::Component to avoid DAW sandbox/window-manager
//          conflicts. Positioned relative to the parent editor bounds.
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AboutDialog : public juce::Component
{
public:
    AboutDialog();
    ~AboutDialog() override = default;

    void setVisible(bool visible) override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Exposed for editor layout updates
    void centreRelativeToParent();

private:
    // --- UI Elements ---
    std::unique_ptr<juce::Drawable> logoDrawable;      // Rendered SVG logo
    juce::TextButton closeButton;                      // Closes the dialog
    juce::TextButton githubButton;                     // Opens GitHub repo in browser
    juce::Label infoLabel;                             // Multi-line text display

    void setupUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutDialog)
};
