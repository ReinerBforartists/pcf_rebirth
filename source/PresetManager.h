// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PresetManager.h
// @brief   Preset persistence and management for factory/user presets.
// @details Handles loading/saving XML files, name sanitization, index mapping,
//          and aggregation of factory/user entries for UI consumption.
// ============================================================================

#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>

// Represents an entry in the combined preset list for the UI.
struct PresetEntry {
    juce::String name;
    juce::String category; // Required for grouping in Multirow Browser
    bool isFactory;
    int index; // Index within factoryPresets or userPresets
};

// Holds the state of a single sequencer pattern.
struct SequencerPreset {
    juce::String name;
    juce::String category; // Required for grouping in Multirow Browser
    float pitches[16];
    bool gates[16];
    int patternLength;
    bool randomEnabled = false;
    float randomAmount = 0.0f;
};

//Manages factory presets (in-memory) and user presets (individual XML files).
class PresetManager : public juce::ChangeBroadcaster {
public:
    PresetManager();
    void initialize(); // Explicit initialization after full construction
    ~PresetManager() = default;

    std::vector<PresetEntry> getCombinedEntries() const;
    int getCombinedPresetCount() const { return static_cast<int>(factoryPresets.size() + userPresets.size()); }

    const std::vector<SequencerPreset>& getFactoryPresets() const { return factoryPresets; }
    const std::vector<SequencerPreset>& getUserPresets()    const { return userPresets; }

    bool loadUserPreset(int index, SequencerPreset& outPreset) const;
    void deleteUserPreset(int index);
    bool renameUserPreset(int index, const juce::String& newName);
    bool getPresetByIndex(int index, bool isFactory, SequencerPreset& outPreset) const;

    int saveUserPreset(const SequencerPreset& preset);
    bool overwriteUserPreset(int userIndex, const SequencerPreset& preset);

    int clampActivePresetIndex(int idx) const;
    bool isNameTaken(const juce::String& name, int exemptUserIndex = -1) const;

private:
    void initializeFactoryPresets();
    void loadUserPresetsFromDisk();
    juce::File getUserPresetsDirectory() const;
    juce::File getUserPresetFile(const juce::String& name) const;
    juce::String sanitizeFileName(const juce::String& name) const;

    std::vector<SequencerPreset> factoryPresets;
    std::vector<SequencerPreset> userPresets;

    PresetManager(const PresetManager&) = delete;
    PresetManager& operator=(const PresetManager&) = delete;
};
