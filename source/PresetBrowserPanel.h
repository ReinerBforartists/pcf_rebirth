// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PresetBrowserPanel.h
// @brief   Multi-column preset browser component.
// @details Wraps a juce::Viewport and manages a ContentComponent that
//          dynamically wraps presets into columns based on height constraints.
// ============================================================================

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// The PresetBrowserPanel acts as the frame (Viewport) for a wrapped multi-column preset list.
class PresetBrowserPanel : public juce::Component
{
public:
    // Constructor accepts maxHeight to control how many elements fit in one column before wrapping
    explicit PresetBrowserPanel(PCFProcessor& p, int maxHeight);
    ~PresetBrowserPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshPresets();

    // Returns the height of the panel itself (used for layout in Editor)
    int getRequiredHeight() const;

private:
    // --- Inner Class: Content Component ---
    // The ContentComponent handles the wrapped column logic.
    class ContentComponent : public juce::Component
    {
    public:
        ContentComponent(PCFProcessor& p, int maxHeight);
        ~ContentComponent() override {}

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseMove(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;

        void refresh();

    private:
        struct VisualElement
        {
            enum class Type { Header, Item };
            Type type;
            juce::String text;
            int combinedIndex = -1;
        };

        PCFProcessor& processor;
        int maxColumnHeight;

        // A list of columns, where each column is a vector of elements
        std::vector<std::vector<VisualElement>> columns;

        static constexpr int rowH       = 22;
        static constexpr int headerH    = 24;
        static constexpr int colWidth   = 150;

        int hoveredCol = -1;
        int hoveredRow = -1; // Index within the specific column

        void updateColumns();
    };

    PCFProcessor& processor;
    juce::Viewport viewport;
    std::unique_ptr<ContentComponent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserPanel)
};
