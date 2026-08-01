/*
   This file is part of the Ouaricon Audio scala-tuning-engine module.
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
// ===== scala-tuning-engine - APVTS Parameters =====
// Add these parameters to your createParameterLayout() function.
// Prefix can be customized (default: "tuning_")

// In your PluginProcessor.cpp createParameterLayout():

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ═══════════════════════════════════════════════════════════════════
    // TUNING PARAMETERS
    // ═══════════════════════════════════════════════════════════════════

    // Reference pitch (A4) - 400-480 Hz, default 440
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tuning_masterTune", 1),
        "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // Tuning mode: 0=12-TET, 1=Custom/Scala, 2=MTS-ESP
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("tuning_tuningMode", 1),
        "Tuning Mode",
        juce::StringArray{"12-TET", "Custom", "MTS-ESP"},
        0));

    // Octave stretch for physical modeling (0.95-1.25, default 1.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tuning_octaveStretch", 1),
        "Octave Stretch",
        juce::NormalisableRange<float>(0.95f, 1.25f, 0.001f),
        1.0f));

    // Pitch bend range in semitones (1-48, default 2)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tuning_pitchBendRange", 1),
        "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    // Built-in temperament preset
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("tuning_temperamentPreset", 1),
        "Temperament",
        juce::StringArray{
            "Equal 12-TET",
            "Pythagorean",
            "Zarlino",
            "Meantone (1/4)",
            "Werckmeister III",
            "Kirnberger III",
            "Vallotti",
            "Well Tempered",
            "Just Intonation",
            "Bohlen-Pierce",
            "Custom"
        },
        0));

    // ... add your other plugin parameters here ...

    return { params.begin(), params.end() };
}


// ═══════════════════════════════════════════════════════════════════
// PARAMETER LISTENERS (Optional)
// ═══════════════════════════════════════════════════════════════════
// If you want parameters to update TuningEngine in real-time:

void YourProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "tuning_masterTune")
    {
        tuningEngine.setMasterTune(static_cast<double>(newValue));
    }
    else if (parameterID == "tuning_octaveStretch")
    {
        tuningEngine.setOctaveStretch(newValue);
    }
    else if (parameterID == "tuning_pitchBendRange")
    {
        tuningEngine.setPitchBendRange(newValue);
    }
    else if (parameterID == "tuning_tuningMode")
    {
        int mode = static_cast<int>(newValue);
        tuningEngine.setMode(static_cast<TuningEngine::Mode>(mode));
    }
    else if (parameterID == "tuning_temperamentPreset")
    {
        int preset = static_cast<int>(newValue);
        tuningEngine.setBuiltInPreset(static_cast<TuningEngine::BuiltInPreset>(preset));
    }
}
