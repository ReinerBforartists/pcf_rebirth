// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    AboutDialog.cpp
// @brief   Implementation of the embedded about dialog.
// @details Handles SVG loading, UI wiring, visibility toggling, centering,
//          and custom painting for the backdrop, panel, and logo.
// ============================================================================

#include "AboutDialog.h"
#include <BinaryData.h> // Required to access the embedded resources

AboutDialog::AboutDialog()
    : logoDrawable(nullptr)
{
    setOpaque(false);
    setAlwaysOnTop(true);
    setVisible(false);
    setupUI();
}

void AboutDialog::setupUI()
{
    // --- Load SVG Logo from Binary Data ---
    const char* svgData = BinaryData::pcf_rebirth_logo_svg;
    const int   svgSize = BinaryData::pcf_rebirth_logo_svgSize;

    if (svgData != nullptr) {
        juce::String svgString = juce::String::fromUTF8(svgData, svgSize);
        auto xml = juce::XmlDocument::parse(svgString);
        if (xml != nullptr) {
            logoDrawable = juce::Drawable::createFromSVG(*xml);
        }
    }

    // --- Setup Close Button ---
    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this] { setVisible(false); };

    // --- Setup GitHub Button ---
    addAndMakeVisible(githubButton);
    githubButton.setButtonText("GitHub Repo");
    githubButton.onClick = []() {
        juce::URL("https://github.com/ReinerBforartists/pcf_rebirth").launchInDefaultBrowser();
    };

    // --- Setup Info Label (Multi-line) ---
    addAndMakeVisible(infoLabel);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc8c8d8));
    infoLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    juce::String infoText;
    infoText << "Version 1.0.0\n\n"
             << "Created by: Reiner Prokein\n"
             << "License: MIT License\n"
             << "Copyright © 2026 Reiner Prokein\n\n"
             << "This software is free to use and open source\n"
             << "It is inspired by the PCF module from Rebirth 338\n";

    infoLabel.setText(infoText, juce::dontSendNotification);
}

void AboutDialog::setVisible(bool visible)
{
    if (isVisible() == visible)
        return;

    juce::Component::setVisible(visible);
    if (visible)
        centreRelativeToParent();
}

void AboutDialog::centreRelativeToParent()
{
    auto* parent = getParentComponent();
    if (parent == nullptr)
        return;

    // Use getWidth()/getHeight() directly – these are already in the parent's
    // local coordinate space, so no getX()/getY() offset needed.
    const int panelWidth  = 400;
    const int panelHeight = 350;

    setBounds(
        (parent->getWidth()  - panelWidth)  / 2,
        (parent->getHeight() - panelHeight) / 2,
        panelWidth, panelHeight
    );
}

void AboutDialog::paint(juce::Graphics& g)
{
    // --- Semi-transparent backdrop ---
    g.setColour(juce::Colour(0x80141418));
    g.fillRect(getLocalBounds());

    // --- Panel background ---
    g.setColour(juce::Colour(0xff1e1e26));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f);
    g.setColour(juce::Colour(0xff35354d));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.0f);

    // --- Draw the logo if it exists ---
    if (logoDrawable != nullptr) {
        auto bounds = logoDrawable->getBounds();
        if (!bounds.isEmpty()) {
            juce::Rectangle<float> targetArea(
                (getWidth() - 150) * 0.5f,
                20.0f,
                150.0f,
                40.0f
            );

            float scaleX = targetArea.getWidth() / bounds.getWidth();
            float scaleY = targetArea.getHeight() / bounds.getHeight();
            float scale  = std::min(scaleX, scaleY);

            juce::AffineTransform transform = juce::AffineTransform::scale(scale)
                .translated(targetArea.getX(), targetArea.getY());

            logoDrawable->draw(g, 1.0f, transform);
        }
    }
}

void AboutDialog::resized()
{
    auto area = getLocalBounds().reduced(30);

    // Leave space at the top for the logo (approx 70 pixels)
    area.removeFromTop(70);

    int infoHeight = static_cast<int>(area.getHeight() * 0.6f);
    infoLabel.setBounds(area.removeFromTop(infoHeight));

    auto buttonArea = area.removeFromBottom(30);
    githubButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(5, 0));
    closeButton.setBounds(buttonArea.reduced(5, 0));
}
