// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PresetBrowserPanel.cpp
// @brief   Implementation of the multi-column preset browser.
// @details Handles column wrapping logic, hit-testing, hover state tracking,
//          and custom drawing for headers and preset items.
// ============================================================================

#include "PresetBrowserPanel.h"

// --- PresetBrowserPanel Implementation ---

// Initializes the viewport and delegates content management to the inner class.
PresetBrowserPanel::PresetBrowserPanel(PCFProcessor& p, int maxHeight)
    : processor(p)
{
    content = std::make_unique<ContentComponent>(processor, maxHeight);

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(content.get(), false);
    // Harmonize horizontal scrollbar with primary blue, keeping it subtle for dark mode
    viewport.getHorizontalScrollBar().setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xFF3D8EFF).withAlpha(0.35f));
    viewport.getHorizontalScrollBar().setColour(juce::ScrollBar::backgroundColourId, juce::Colour(0xFF1A1A24));

    // Goal: No vertical scroll (we wrap content horizontally), only horizontal if needed.
    viewport.setScrollBarsShown(false, true);

    refreshPresets();
}

PresetBrowserPanel::~PresetBrowserPanel() {}

void PresetBrowserPanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff35354d));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);
}

void PresetBrowserPanel::resized()
{
    viewport.setBounds(getLocalBounds());
}

void PresetBrowserPanel::refreshPresets()
{
    content->refresh();
}

int PresetBrowserPanel::getRequiredHeight() const
{
    return content ? content->getHeight() : 0;
}

// --- ContentComponent Implementation ---

// Initializes the content component with processor reference and height constraint.
PresetBrowserPanel::ContentComponent::ContentComponent(PCFProcessor& p, int maxHeight)
    : processor(p), maxColumnHeight(maxHeight)
{}

// Distributes preset entries into vertically wrapped columns based on height limits.
void PresetBrowserPanel::ContentComponent::updateColumns()
{
    columns.clear();
    const auto combined = processor.getPresetManager().getCombinedEntries();
    if (combined.empty())
    {
        setSize(colWidth, 0);
        return;
    }

    // 1. Create a flat list of all elements (Headers and Items)
    std::vector<VisualElement> allElements;
    juce::String lastCat = "";
    for (const auto& e : combined)
    {
        if (e.category != lastCat)
        {
            allElements.push_back({ VisualElement::Type::Header, e.category.toUpperCase(), -1 });
            lastCat = e.category;
        }
        allElements.push_back({ VisualElement::Type::Item, e.name, e.index });
    }

    // 2. Distribute elements into columns using the wrapping logic
    std::vector<VisualElement> currentColumn;
    int currentColumnHeight = 0;

    for (const auto& el : allElements)
    {
        int elH = (el.type == VisualElement::Type::Header ? headerH : rowH);

        // If adding this element exceeds the max height, start a new column
        if (!currentColumn.empty() && (currentColumnHeight + elH > maxColumnHeight))
        {
            columns.push_back(currentColumn);
            currentColumn.clear();
            currentColumnHeight = 0;
        }

        currentColumn.push_back(el);
        currentColumnHeight += elH;
    }

    if (!currentColumn.empty())
        columns.push_back(currentColumn);

    // 3. Set the total size of the content component (Width = cols * width, Height = max height)
    // FIX: Added +6 pixels buffer to avoid horizontal scrollbar overlapping last row.
    setSize(static_cast<int>(columns.size()) * colWidth, maxColumnHeight + 6);
}

void PresetBrowserPanel::ContentComponent::refresh()
{
    updateColumns();
    repaint();
}

void PresetBrowserPanel::ContentComponent::resized() {}

// Draws headers, items, hover states, and active preset indicators.
void PresetBrowserPanel::ContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a24));
    const int activeIdx = processor.getCurrentPresetIndex();

    for (int c = 0; c < static_cast<int>(columns.size()); ++c)
    {
        int x = c * colWidth;
        int currentY = 0;
        const auto& column = columns[c];

        for (int r = 0; r < static_cast<int>(column.size()); ++r)
        {
            const auto& el = column[r];
            const int h = (el.type == VisualElement::Type::Header ? headerH : rowH);

            // Hover effect
            if (c == hoveredCol && r == hoveredRow)
            {
                g.setColour(juce::Colour(0x1a3d8eff));
                g.fillRect(x, currentY, colWidth, h);
            }
            else if (el.type == VisualElement::Type::Header)
            {
                // Header Styling
                g.setColour(juce::Colour(0xff26263a));
                g.fillRect(x, currentY, colWidth, h);
                g.setColour(juce::Colour(0xff35354d));
                g.drawHorizontalLine(currentY + h - 1, (float)x, (float)(x + colWidth));
            }

            if (el.type == VisualElement::Type::Header)
            {
                g.setColour(juce::Colour(0xFF6A9EFF)); // +160 grayscale units lighter than bg, blue-tinted to match UI
                g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
                g.drawText(el.text, x + 8, currentY, colWidth - 8, h, juce::Justification::centredLeft);
            }
            else
            {
                // Item Styling
                bool isActive = (el.combinedIndex == activeIdx);

                if (isActive)
                {
                    g.setColour(juce::Colour(0xff3d8eff));
                    g.fillRoundedRectangle((float)x + 2.0f, (float)currentY + (rowH * 0.25f), 4.0f, (float)rowH * 0.5f, 1.0f);
                    g.setColour(juce::Colours::white);
                }
                else if (c == hoveredCol && r == hoveredRow)
                {
                    g.setColour(juce::Colour(0xffddddf0));
                }
                else
                {
                    g.setColour(juce::Colour(0xffb0b0c8));
                }

                g.setFont(juce::Font(juce::FontOptions(12.0f)));
                g.drawText(el.text, x + 12, currentY, colWidth - 15, h, juce::Justification::centredLeft);
            }
            currentY += h;
        }

        // Vertical separator between columns
        if (c < static_cast<int>(columns.size()) - 1)
        {
            g.setColour(juce::Colour(0xff2a2a3a));
            g.drawVerticalLine(x + colWidth, 0.0f, (float)getHeight());
        }
    }
}

// Tracks cursor position to update hover states and trigger repaint.
void PresetBrowserPanel::ContentComponent::mouseMove(const juce::MouseEvent& e)
{
    if (columns.empty()) return;
    int c = e.x / colWidth;
    int r = -1;
    if (c >= 0 && c < static_cast<int>(columns.size()))
    {
        int cumulativeY = 0;
        for (int i = 0; i < static_cast<int>(columns[c].size()); ++i)
        {
            int h = (columns[c][i].type == VisualElement::Type::Header ? headerH : rowH);
            if (e.y >= cumulativeY && e.y < cumulativeY + h)
            {
                r = i;
                break;
            }
            cumulativeY += h;
        }
    }

    if (c != hoveredCol || r != hoveredRow)
    {
        hoveredCol = c;
        hoveredRow = r;
        repaint();
    }
}

// Resets hover state when cursor leaves the component bounds.
void PresetBrowserPanel::ContentComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredCol = -1;
    hoveredRow = -1;
    repaint();
}

// Hit-tests columns and rows to apply the selected preset index.
void PresetBrowserPanel::ContentComponent::mouseDown(const juce::MouseEvent& e)
{
    int c = e.x / colWidth;
    if (c >= 0 && c < static_cast<int>(columns.size()))
    {
        int cumulativeY = 0;
        for (int i = 0; i < static_cast<int>(columns[c].size()); ++i)
        {
            int h = (columns[c][i].type == VisualElement::Type::Header ? headerH : rowH);
            if (e.y >= cumulativeY && e.y < cumulativeY + h)
            {
                if (columns[c][i].type == VisualElement::Type::Item)
                    processor.setActivePresetIndex(columns[c][i].combinedIndex);
                break;
            }
            cumulativeY += h;
        }
    }
}
