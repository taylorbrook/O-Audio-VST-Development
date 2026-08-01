/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    PresetManager.h
    v1.3.0: Preset system for saving/loading complete patch state

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Forward declaration
class TuningEngine;

/**
 * Manages preset save/load operations for O-Marimba.
 *
 * Presets are stored as JSON files in:
 *   ~/Library/Application Support/O-Marimba/Presets/
 *
 * Each preset contains:
 *   - All APVTS parameters
 *   - Tuning intervals (any scale size)
 *   - Scale name
 *   - Tonic note (transposition)
 */
class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvts, TuningEngine& tuning);
    ~PresetManager() = default;

    // Preset operations
    bool savePreset(const juce::String& presetName);
    bool loadPreset(const juce::String& presetName);
    bool deletePreset(const juce::String& presetName);

    // Preset listing
    juce::StringArray getPresetList() const;
    juce::String getCurrentPresetName() const { return currentPresetName; }
    void setCurrentPresetName(const juce::String& name) { currentPresetName = name; }

    // Navigation (for ◀ ▶ buttons)
    juce::String getNextPreset() const;
    juce::String getPreviousPreset() const;

    // State serialization (for DAW session save/restore)
    // Returns complete state including tuning as XML
    std::unique_ptr<juce::XmlElement> getStateAsXml() const;
    void setStateFromXml(const juce::XmlElement* xml);

    // Factory preset initialization
    void initializeFactoryPresets();

    // Check if preset is factory (read-only)
    bool isFactoryPreset(const juce::String& presetName) const;

private:
    juce::AudioProcessorValueTreeState& parameters;
    TuningEngine& tuningEngine;
    juce::String currentPresetName { "Default" };

    // Get presets directory (creates if needed)
    juce::File getPresetsDirectory() const;
    juce::File getFactoryPresetsDirectory() const;
    juce::File getUserPresetsDirectory() const;

    // JSON serialization helpers
    juce::var createPresetJson() const;
    bool applyPresetJson(const juce::var& presetData);

    // Factory preset data
    struct FactoryPreset
    {
        juce::String name;
        std::vector<double> intervals;
        juce::String scaleName;
        int tonic;
        // Parameter values (normalized 0-1)
        float malletHardness;
        float barMaterial;
        float resonance;
        // v1.6.0: New timbre parameters
        float strikePosition;   // 0-1 (0.5 = center)
        float overtoneDamping;  // 0-1 (0.5 = natural)
        float tone;             // 0-1 (0.75 = default)
        int tuningMode;  // 0=12TET, 1=Scala, 2=MTS
        float referencePitch;  // Hz
        float velCurve;
        float outputGain;  // dB
    };

    static const std::vector<FactoryPreset>& getFactoryPresetData();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
