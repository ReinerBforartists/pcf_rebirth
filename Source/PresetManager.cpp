// Copyright (c) 2026 Reiner Prokein
// PCF Rebirth | MIT License
// ============================================================================
// @file    PresetManager.cpp
// @brief   Implementation of preset persistence and management.
// @details Handles factory preset initialization, XML file I/O, name
//          sanitization, index offset mapping, and change notification.
// ============================================================================

#include "PresetManager.h"
#include <algorithm>

// Initializes factory presets and loads user presets from disk.
PresetManager::PresetManager() {
    initializeFactoryPresets();
    loadUserPresetsFromDisk();
}

// Safely notifies listeners after all internal data is fully loaded.
void PresetManager::initialize() {
    sendChangeMessage();
}

// Returns the user application data directory for preset storage.
juce::File PresetManager::getUserPresetsDirectory() const {
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("PCF_ReBirth")
                   .getChildFile("presets")
                   .getChildFile("user");
    if (!dir.exists()) dir.createDirectory();
    return dir;
}

/**
 * Sanitizes a filename by replacing illegal characters and truncating to 128 chars.
 */
juce::String PresetManager::sanitizeFileName(const juce::String& name) const {
    juce::String sanitized;
    for (auto c : name.trim()) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|')  {
            sanitized += '_';
        } else {
            sanitized += c;
        }
    }

    // Truncate to 128 characters as a safety measure for OS limits.
    const int maxNameLength = 128;
    if (sanitized.length() > maxNameLength) {
        sanitized = sanitized.substring(0, maxNameLength);
    }

    return sanitized;
}

// Constructs the full file path for a given preset name.
juce::File PresetManager::getUserPresetFile(const juce::String& name) const {
    return getUserPresetsDirectory().getChildFile(sanitizeFileName(name) + ".xml");
}

// Populates the in-memory factory preset list by category.
void PresetManager::initializeFactoryPresets() {
    factoryPresets.clear();

    // Helper lambda to add presets with category
    auto addPreset = [&](const juce::String& cat, const juce::String& name, int len, const float* p, const bool* g) {
        SequencerPreset preset;
        preset.category = cat;
        preset.name = name;
        preset.patternLength = len;
        for (int i = 0; i < 16; ++i) {
            preset.pitches[i] = (p && i < 16) ? p[i] : 0.0f;
            preset.gates[i] = (g && i < 16) ? g[i] : false;
        }
        factoryPresets.push_back(preset);
    };

    // --- CATEGORY: ACID ---
    float acidP1[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  acidG1[] = { true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false };
    addPreset("Acid", "Basic Pulse", 16, acidP1, acidG1);

    float acidP2[] = { 0, 0, 12, 0, 7, 0, -5, 0, 0, 0, 12, 0, 7, 0, -5, 0 };
    bool  acidG2[] = { true, true, true, false, true, true, true, false, true, true, true, false, true, true, true, false };
    addPreset("Acid", "Acid Classic", 16, acidP2, acidG2);

    float acidP3[] = { 0, -12, 0, 12, 0, -12, 0, 7, 0, -12, 0, 12, 0, -12, 0, 5 };
    bool  acidG3[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Acid", "Acid Roller", 16, acidP3, acidG3);

    float acidP4[] = { 0, 0, 0, 7, 0, 0, 12, 0, 0, 0, 0, -5, 0, 0, 0, -12 };
    bool  acidG4[] = { true, false, true, true, false, true, true, false, true, false, true, true, false, true, true, false };
    addPreset("Acid", "Techno Drive", 16, acidP4, acidG4);

    float acidP5[] = { 0, 12, 0, 12, 7, 0, 5, 0, 0, 12, 0, 12, -5, 0, -12, 0 };
    bool  acidG5[] = { true, true, false, true, true, false, true, false, true, true, false, true, true, false, true, false };
    addPreset("Acid", "Dark Acid", 16, acidP5, acidG5);

    float acidP6[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  acidG6[] = { true, false, false, true, false, true, false, false, true, false, false, true, false, true, false, false };
    addPreset("Acid", "Syncopated", 16, acidP6, acidG6);

    float acidP7[] = { 0, 0, 7, 0, 12, 0, 7, 0, 0, 0, -5, 0, -12, 0, -5, 0 };
    bool  acidG7[] = { true, true, true, false, true, true, true, false, true, true, true, false, true, true, true, false };
    addPreset("Acid", "Acid Sequence A", 16, acidP7, acidG7);

    float acidP8[] = { 0, 5, 0, 12, 0, 5, 0, 12, 0, 5, 0, 12, 0, 5, 0, 12 };
    bool  acidG8[] = { true, false, true, true, false, true, true, false, true, false, true, true, false, true, true, false };
    addPreset("Acid", "Acid Sequence B", 16, acidP8, acidG8);

    float acidP9[] = { 0, -5, 0, 7, 0, -5, 0, 12, 0, -5, 0, 7, 0, -5, 0, 0 };
    bool  acidG9[] = { true, true, false, true, true, true, false, true, true, true, false, true, true, true, false, true };
    addPreset("Acid", "Funky Acid", 16, acidP9, acidG9);

    float acidP10[] = { 0, 0, -5, 0, 0, 7, 0, -5, 0, 0, -5, 0, 0, 12, 0, -5 };
    bool  acidG10[] = { true, false, true, true, false, true, false, true, true, false, true, false, true, true, false, true };
    addPreset("Acid", "Sliding Acid", 16, acidP10, acidG10);

    float acidP11[] = { 0, 0, 7, 7, 0, 0, 5, 5, 0, 0, 3, 3, 0, 0, -5, -5 };
    bool  acidG11[] = { true, true, true, false, true, true, true, false, true, true, true, false, true, true, true, false };
    addPreset("Acid", "Stab Pattern", 16, acidP11, acidG11);

    float acidP12[] = { 0, 12, -12, 0, 7, 12, -5, 0, 0, 12, -12, 0, 5, 12, -7, 0 };
    bool  acidG12[] = { true, true, true, false, true, true, true, false, true, true, true, false, true, true, true, false };
    addPreset("Acid", "Acid Jump", 16, acidP12, acidG12);

    // --- CATEGORY: TECHNO & HOUSE ---
    float techP1[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  techG1[] = { true, false, true, false, true, false, true, false, false, true, false, true, false, true, false, true };
    addPreset("Techno", "Off-Beat", 16, techP1, techG1);

    float techP2[] = { 0, 0, 0, 0, 12, 12, 0, 0, 0, 0, 0, 0, 12, 12, 0, 0 };
    bool  techG2[] = { true, false, true, false, true, true, false, true, true, false, true, false, true, true, false, true };
    addPreset("Techno", "Octave Jump", 16, techP2, techG2);

    float techP3[] = { -12, -12, 0, -12, 0, -12, 0, -12, -12, -12, 0, -12, 0, -12, 0, -12 };
    bool  techG3[] = { true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false };
    addPreset("Techno", "Sub Rumble", 16, techP3, techG3);

    float techP4[] = { 0, 0, -2, 0, 0, 0, -2, 0, 0, 0, -2, 0, 0, 0, -2, 0 };
    bool  techG4[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Techno", "Driving Bass", 16, techP4, techG4);

    float techP5[] = { 0, 0, 0, 0, 0, -5, 0, 0, 0, 0, 0, -5, 0, 0, 0, 0 };
    bool  techG5[] = { true, false, true, false, true, true, false, true, true, false, true, true, false, true, true, false };
    addPreset("Techno", "Warehouse Groove", 16, techP5, techG5);

    float techP6[] = { 0, 7, 0, 7, 0, 7, 0, 7, 0, 7, 0, 7, 0, 7, 0, 7 };
    bool  techG6[] = { true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false };
    addPreset("Techno", "Housey Bounce", 16, techP6, techG6);

    float techP7[] = { 0, 0, 0, -12, 0, 0, -12, 0, 0, 0, 0, -12, 0, -12, 0, 0 };
    bool  techG7[] = { true, false, false, true, true, false, true, false, true, false, false, true, true, true, false, false };
    addPreset("Techno", "Detroit Roll", 16, techP7, techG7);

    float techP8[] = { 0, 0, 0, 0, -5, 0, 0, -5, 0, 0, 0, 0, -5, 0, -5, 0 };
    bool  techG8[] = { true, true, false, true, true, false, true, false, true, true, false, true, true, false, true, true };
    addPreset("Techno", "Minimal Funk", 16, techP8, techG8);

    float techP9[] = { -12, 0, -12, 0, -12, 0, 7, 0, -12, 0, -12, 0, -12, 0, 5, 0 };
    bool  techG9[] = { true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false };
    addPreset("Techno", "Sub Pulse", 16, techP9, techG9);

    // --- CATEGORY: AMBIENT & CINEMATIC ---
    float ambP1[] = { 0, 2, 4, 5, 7, 8, 10, 12, 10, 8, 7, 5, 4, 2, 0, -2 };
    bool  ambG1[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Ambient", "The Ladder", 16, ambP1, ambG1);

    float ambP2[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  ambG2[] = { true, true, false, true, false, true, true, false };
    addPreset("Ambient", "Short Loop", 8, ambP2, ambG2);

    float ambP3[] = { 0, 7, 12, 19, 24, 19, 12, 7 };
    bool  ambG3[] = { true, false, true, false, true, false, true, false };
    addPreset("Ambient", "Siren", 8, ambP3, ambG3);

    float ambP4[] = { 0, -12, 0, -24, 0, -12, 0, -24 };
    bool  ambG4[] = { true, true, true, true, true, true, true, true };
    addPreset("Ambient", "Deep Dive", 8, ambP4, ambG4);

    float ambP5[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  ambG5[] = { true, false, true, false, true, false, true, false };
    addPreset("Ambient", "Minimalist", 8, ambP5, ambG5);

    float ambP6[] = { 0, 12, 24, 36, 24, 12, 0, -12 };
    bool  ambG6[] = { true, true, true, true, true, true, true, true };
    addPreset("Ambient", "Cosmic Sweep", 8, ambP6, ambG6);

    float ambP7[] = { 0, 5, 0, -5, 0, 12, 0, -12 };
    bool  ambG7[] = { true, false, true, false, true, false, true, false };
    addPreset("Ambient", "Ethereal Wind", 8, ambP7, ambG7);

    float ambP8[] = { 0, 4, 7, 11, 14, 11, 7, 4, 0, -5, 0, 4, 7, 4, 0, -5 };
    bool  ambG8[] = { true, true, true, true, true, true, true, true, true, true, false, true, true, true, true, true };
    addPreset("Ambient", "Pentatonic Dream", 16, ambP8, ambG8);

    float ambP9[] = { 0, 0, 12, 0, 0, 7, 0, 0 };
    bool  ambG9[] = { true, false, false, true, false, false, true, false };
    addPreset("Ambient", "Slow Pulse", 8, ambP9, ambG9);

    // --- CATEGORY: INDUSTRIAL & NOISE ---
    float indP1[] = { 0, 1, 0, 6, 0, 1, 0, 6, 0, 1, 0, 6, 0, 1, 0, 6 };
    bool  indG1[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Industrial", "Steel Factory", 16, indP1, indG1);

    float indP2[] = { 0, 13, -1, 12, 0, 13, -1, 12, 0, 13, -1, 12, 0, 13, -1, 12 };
    bool  indG2[] = { true, true, false, true, true, true, false, true, true, true, false, true, true, true, false, true };
    addPreset("Industrial", "Rhythmic Noise", 16, indP2, indG2);

    float indP3[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  indG3[] = { true, true, true, true, true, true, true, true };
    addPreset("Industrial", "Distorted Pulse", 8, indP3, indG3);

    float indP4[] = { 0, -1, 0, 1, 0, -1, 0, 1 };
    bool  indG4[] = { true, true, true, true, true, true, true, true };
    addPreset("Industrial", "Gritty Texture", 8, indP4, indG4);

    float indP5[] = { 0, 24, -12, 12, 0, 24, -12, 12 };
    bool  indG5[] = { true, true, true, true, true, true, true, true };
    addPreset("Industrial", "Glitchy", 8, indP5, indG5);

    float indP6[] = { 0, 0, 1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 0, 6, -1, 0 };
    bool  indG6[] = { true, true, true, false, true, true, true, false, true, true, true, false, true, true, true, true };
    addPreset("Industrial", "Machine Grind", 16, indP6, indG6);

    float indP7[] = { 0, -12, 0, 0, -12, 0, 0, -12, 0, 0, 12, 0, 0, -12, 0, 0 };
    bool  indG7[] = { true, true, false, true, true, false, true, true, true, false, true, false, true, true, false, true };
    addPreset("Industrial", "Stomper", 16, indP7, indG7);

    float indP8[] = { 0, 13, 0, -1, 6, 13, 0, -1 };
    bool  indG8[] = { true, true, true, true, true, true, true, true };
    addPreset("Industrial", "Rusty Cog", 8, indP8, indG8);

    // --- CATEGORY: EXPERIMENTAL ---
    float expP1[] = { 0, -2, 0, 2, 0, -2, 0, 2, 0, -2, 0, 2, 0, -2, 0, 2 };
    bool  expG1[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Experimental", "Micro-Tuning", 16, expP1, expG1);

    float expP2[] = { 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0 };
    bool  expG2[] = { true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false };
    addPreset("Experimental", "The Gap", 16, expP2, expG2);

    float expP3[] = { 0, 7, 12, 5, 0, -5, -12, -7, 0, 7, 12, 5, 0, -5, -12, -7 };
    bool  expG3[] = { true, true, false, true, true, true, false, true, true, true, false, true, true, true, false, true };
    addPreset("Experimental", "Full Circle", 16, expP3, expG3);

    float expP4[] = { 0, 2, 5, 9, 14, 21, 28, 37 };
    bool  expG4[] = { true, true, true, true, true, true, true, true };
    addPreset("Experimental", "The Ascent", 8, expP4, expG4);

    float expP5[] = { 0, -12, 7, -5, 12, -7, 5, -12 };
    bool  expG5[] = { true, false, true, true, false, true, true, false };
    addPreset("Experimental", "Chaos Theory", 8, expP5, expG5);

    float expP6[] = { 0, 3, 7, 10, 14, 10, 7, 3, 0, -3, -7, -10, -7, -3, 0, 3 };
    bool  expG6[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Experimental", "Sine Imitation", 16, expP6, expG6);

    float expP7[] = { 0, 0, 5, 0, 7, 0, 5, 0, 0, 0, 3, 0, 5, 0, 3, 0 };
    bool  expG7[] = { true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false };
    addPreset("Experimental", "Even Steps", 16, expP7, expG7);

    // --- CATEGORY: UTILITY ---
    float utilP1[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  utilG1[] = { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    addPreset("Utility", "Init", 16, utilP1, utilG1);

    float utilP2[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  utilG2[] = { true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
    addPreset("Utility", "Single Hit", 16, utilP2, utilG2);

    float utilP3[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  utilG3[] = { true, true, false, false, true, true, false, false, true, true, false, false, true, true, false, false };
    addPreset("Utility", "Basic 4/4", 16, utilP3, utilG3);

    float utilP4[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  utilG4[] = { true, false, false, false, true, false, false, false, true, false, false, false, true, false, false, false };
    addPreset("Utility", "Quarter Notes", 16, utilP4, utilG4);

    float utilP5[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    bool  utilG5[] = { true, false, true, false, true, false, true, false };
    addPreset("Utility", "Eight Notes", 8, utilP5, utilG5);
}

// Scans the user presets directory and parses XML files.
void PresetManager::loadUserPresetsFromDisk() {
    userPresets.clear();
    auto dir = getUserPresetsDirectory();
    if (!dir.isDirectory()) return;

    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : files) {
        SequencerPreset preset;
        preset.name = file.getFileNameWithoutExtension();
        preset.category = "User"; // User presets are grouped together
        preset.patternLength = 16;
        std::fill(std::begin(preset.pitches), std::end(preset.pitches), 0.0f);
        std::fill(std::begin(preset.gates), std::end(preset.gates), false);

        std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || !xml->hasTagName("PRESET")) continue;

        preset.patternLength  = xml->getIntAttribute("len", 16);

        auto* pitchesNode = xml->getChildByName("PITCHES");
        if (pitchesNode != nullptr) {
            juce::String valsStr = pitchesNode->getStringAttribute("vals");
            juce::StringArray vals;
            vals.addTokens(valsStr, ",", "");
            for (int i = 0; i < 16 && i < vals.size(); ++i)
                preset.pitches[i] = vals[i].getFloatValue();
        }

        auto* gatesNode = xml->getChildByName("GATES");
        if (gatesNode != nullptr) {
            juce::String valsStr = gatesNode->getStringAttribute("vals");
            juce::StringArray vals;
            vals.addTokens(valsStr, ",", "");
            for (int i = 0; i < 16 && i < vals.size(); ++i)
                preset.gates[i] = vals[i].getIntValue() > 0;
        }
        userPresets.push_back(preset);
    }
    // Removed sendChangeMessage() here to prevent premature notification during construction
}

// Saves a new preset to disk with automatic name collision handling.
int PresetManager::saveUserPreset(const SequencerPreset& preset) {
    juce::String baseName = sanitizeFileName(preset.name);
    juce::String finalName = baseName;
    int attempt = 1;

    while (attempt < 1000 && (getUserPresetFile(finalName).existsAsFile() || isNameTaken(finalName, -1))) {
        finalName = baseName + " " + juce::String(attempt++);
    }
    if (attempt >= 1000) return -1;

    juce::File targetFile = getUserPresetFile(finalName);
    juce::XmlElement root("PRESET");
    root.setAttribute("name", finalName);
    root.setAttribute("len", preset.patternLength);

    auto* pitchesNode = root.createNewChildElement("PITCHES");
    juce::String pVals;
    for (int i = 0; i < 16; ++i) pVals += juce::String(preset.pitches[i]) + (i < 15 ? "," : "");
    pitchesNode->setAttribute("vals", pVals);

    auto* gatesNode = root.createNewChildElement("GATES");
    juce::String gVals;
    for (int i = 0; i < 16; ++i) gVals += juce::String(preset.gates[i] ? 1 : 0) + (i < 15 ? "," : "");
    gatesNode->setAttribute("vals", gVals);

    if (root.writeTo(targetFile)) {
        SequencerPreset newPreset = preset;
        newPreset.name = finalName;
        userPresets.push_back(newPreset);
        sendChangeMessage();
        return static_cast<int>(userPresets.size()) - 1;
    }
    return -1;
}

// Updates an existing user preset file and updates the in-memory list.
bool PresetManager::overwriteUserPreset(int index, const SequencerPreset& preset) {
    // Handle combined indices passed from UI (index = factoryCount + userIndex)
    int factoryCount = static_cast<int>(factoryPresets.size());
    int userIndex = index - factoryCount;

    if (userIndex < 0 || userIndex >= static_cast<int>(userPresets.size()))
        return false;

    juce::File targetFile = getUserPresetFile(preset.name);
    juce::XmlElement root("PRESET");
    root.setAttribute("name", preset.name);
    root.setAttribute("len", preset.patternLength);

    auto* pitchesNode = root.createNewChildElement("PITCHES");
    juce::String pVals;
    for (int i = 0; i < 16; ++i) pVals += juce::String(preset.pitches[i]) + (i < 15 ? "," : "");
    pitchesNode->setAttribute("vals", pVals);

    auto* gatesNode = root.createNewChildElement("GATES");
    juce::String gVals;
    for (int i = 0; i < 16; ++i) gVals += juce::String(preset.gates[i] ? 1 : 0) + (i < 15 ? "," : "");
    gatesNode->setAttribute("vals", gVals);

    bool success = root.writeTo(targetFile);
    if (success) {
        userPresets[userIndex] = preset;
        sendChangeMessage();
    }
    return success;
}

// Renames a user preset file and updates the in-memory list.
bool PresetManager::renameUserPreset(int index, const juce::String& newName) {
    // Handle combined indices passed from UI
    int factoryCount = static_cast<int>(factoryPresets.size());
    int userIndex = index - factoryCount;

    if (userIndex < 0 || userIndex >= static_cast<int>(userPresets.size()) || newName.isEmpty())
        return false;

    juce::String sanitizedName = sanitizeFileName(newName);
    juce::File currentFile = getUserPresetFile(userPresets[userIndex].name);
    juce::File newFile = getUserPresetFile(sanitizedName);

    if (newFile.existsAsFile() || isNameTaken(sanitizedName)) return false;

    bool success = currentFile.moveFileTo(newFile);
    if (success) {
        userPresets[userIndex].name = sanitizedName;
        sendChangeMessage();
    }
    return success;
}

// Removes a user preset file and updates the in-memory list.
void PresetManager::deleteUserPreset(int index) {
    // Handle combined indices passed from UI (index = factoryCount + userIndex)
    int factoryCount = static_cast<int>(factoryPresets.size());
    int userIndex = index - factoryCount;

    if (userIndex >= 0 && userIndex < static_cast<int>(userPresets.size())) {
        juce::File file = getUserPresetFile(userPresets[userIndex].name);
        if (file.existsAsFile()) file.deleteFile();
        userPresets.erase(userPresets.begin() + userIndex);
        sendChangeMessage();
    }
}

// Retrieves a preset by index, handling both factory and user lists.
bool PresetManager::getPresetByIndex(int index, bool isFactory, SequencerPreset& outPreset) const {
    if (isFactory) {
        if (index < 0 || index >= static_cast<int>(factoryPresets.size())) return false;
        outPreset = factoryPresets[index];
    } else {
        if (index < 0 || index >= static_cast<int>(userPresets.size())) return false;
        outPreset = userPresets[index];
    }
    return true;
}

// Aggregates factory and user presets into a single UI-ready list.
std::vector<PresetEntry> PresetManager::getCombinedEntries() const {
    std::vector<PresetEntry> entries;

    // 1. Add factory presets with their original index
    for (int i = 0; i < static_cast<int>(factoryPresets.size()); ++i) {
        entries.push_back({ factoryPresets[i].name, factoryPresets[i].category, true, i });
    }

    // 2. Add user presets with an offset index to ensure uniqueness
    // We add the size of factoryPresets so that the first user preset
    // starts at index (factoryPresets.size()) instead of 0.
    int factoryCount = static_cast<int>(factoryPresets.size());
    for (int i = 0; i < static_cast<int>(userPresets.size()); ++i) {
        entries.push_back({ userPresets[i].name, userPresets[i].category, false, i + factoryCount });
    }

    return entries;
}

// Ensures the active preset index stays within valid bounds.
int PresetManager::clampActivePresetIndex(int idx) const {
    int total = static_cast<int>(factoryPresets.size() + userPresets.size());
    return juce::jlimit(0, total > 0 ? total - 1 : 0, idx);
}

// Checks if a sanitized name already exists in factory or user lists.
bool PresetManager::isNameTaken(const juce::String& name, int exemptUserIndex) const {
    juce::String sanitized = sanitizeFileName(name);
    for (const auto& p : factoryPresets)
        if (p.name == sanitized) return true;

    for (int i = 0; i < static_cast<int>(userPresets.size()); ++i) {
        if (i == exemptUserIndex) continue;
        if (userPresets[i].name == sanitized) return true;
    }
    return false;
}

// Loads a user preset from the in-memory list by index.
bool PresetManager::loadUserPreset(int index, SequencerPreset& outPreset) const {
    if (index < 0 || index >= static_cast<int>(userPresets.size())) return false;
    outPreset = userPresets[index];
    return true;
}
