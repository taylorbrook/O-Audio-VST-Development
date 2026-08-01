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
// ===== scala-tuning-engine - State Persistence =====
// Add these to your PluginProcessor for DAW session save/restore.

// ═══════════════════════════════════════════════════════════════════
// SAVE STATE (getStateInformation)
// ═══════════════════════════════════════════════════════════════════

void YourProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Get APVTS state
    auto state = apvts.copyState();

    // ===== Add tuning custom state =====
    auto tuningState = state.getOrCreateChildWithName("tuningEngine", nullptr);

    // Save intervals
    auto intervals = tuningEngine.getIntervals();
    juce::String intervalsStr;
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (i > 0) intervalsStr += ",";
        intervalsStr += juce::String(intervals[i], 6);
    }
    tuningState.setProperty("intervals", intervalsStr, nullptr);

    // Save scale name
    tuningState.setProperty("scaleName", tuningEngine.getActiveTuningName(), nullptr);

    // Save tonic
    tuningState.setProperty("tonic", tuningEngine.getTonicNote(), nullptr);

    // Save preset (if using built-in)
    tuningState.setProperty("preset",
        static_cast<int>(tuningEngine.getBuiltInPreset()), nullptr);

    // Convert to XML and save
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}


// ═══════════════════════════════════════════════════════════════════
// RESTORE STATE (setStateInformation)
// ═══════════════════════════════════════════════════════════════════

void YourProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        apvts.replaceState(state);

        // ===== Restore tuning custom state =====
        auto tuningState = state.getChildWithName("tuningEngine");
        if (tuningState.isValid())
        {
            // Restore intervals
            juce::String intervalsStr = tuningState.getProperty("intervals", "");
            if (intervalsStr.isNotEmpty())
            {
                std::vector<double> intervals;
                juce::StringArray tokens;
                tokens.addTokens(intervalsStr, ",", "");

                for (const auto& token : tokens)
                {
                    intervals.push_back(token.getDoubleValue());
                }

                juce::String scaleName = tuningState.getProperty("scaleName", "Custom");
                tuningEngine.setCustomIntervals(intervals, scaleName);
            }

            // Restore tonic
            int tonic = tuningState.getProperty("tonic", 0);
            tuningEngine.setTonicNote(tonic);

            // Note: Built-in preset is restored via APVTS parameter,
            // but custom intervals take precedence if saved
        }
    }
}


// ═══════════════════════════════════════════════════════════════════
// ALTERNATIVE: Minimal Persistence (APVTS only)
// ═══════════════════════════════════════════════════════════════════
// If you only need preset-based tunings (not custom intervals),
// you can use the simpler APVTS-only approach:

void YourProcessor::getStateInformation_Simple(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void YourProcessor::setStateInformation_Simple(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        // Apply preset from restored parameter
        auto* presetParam = apvts.getRawParameterValue("tuning_temperamentPreset");
        if (presetParam)
        {
            int preset = static_cast<int>(*presetParam);
            tuningEngine.setBuiltInPreset(
                static_cast<TuningEngine::BuiltInPreset>(preset));
        }
    }
}
