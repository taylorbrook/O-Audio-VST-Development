/*
  ==============================================================================
    FactoryPresets.cpp - O-Bassoon factory preset definitions
  ==============================================================================
*/
#include "FactoryPresets.h"

std::vector<OuariconPresetManager::FactoryPresetDef>
FactoryPresets::build (juce::AudioProcessorValueTreeState& /*apvts*/)
{
    std::vector<OuariconPresetManager::FactoryPresetDef> presets = {
        {
            "Long Drone",
            {{"vibrato_rate", 0.45f}, {"vibrato_depth", 0.12f}, {"vibrato_onset", 0.40f},
             {"breath", 0.50f}, {"tone", 0.40f}, {"attack_character", 0.00f},
             {"attack_time", 0.60f}, {"release_time", 0.60f},
             {"voice_count", 0.20f}, {"output_gain", 0.70f}},
            juce::var()
        },
        {
            "Microtonal Pad",
            {{"vibrato_rate", 0.40f}, {"vibrato_depth", 0.25f}, {"vibrato_onset", 0.75f},
             {"breath", 0.65f}, {"tone", 0.50f}, {"attack_character", 0.00f},
             {"attack_time", 0.75f}, {"release_time", 0.7333f},
             {"voice_count", 0.4667f}, {"output_gain", 0.7333f}},
            juce::var()
        },
        {
            "Tongued Long Tone",
            {{"vibrato_rate", 0.55f}, {"vibrato_depth", 0.18f}, {"vibrato_onset", 0.15f},
             {"breath", 0.75f}, {"tone", 0.55f}, {"attack_character", 1.00f},
             {"attack_time", 0.04f}, {"release_time", 0.40f},
             {"voice_count", 0.20f}, {"output_gain", 0.7667f}},
            juce::var()
        },
        {
            "Bright Bassoon",
            {{"vibrato_rate", 0.60f}, {"vibrato_depth", 0.10f}, {"vibrato_onset", 0.20f},
             {"breath", 0.70f}, {"tone", 1.00f}, {"attack_character", 0.20f},
             {"attack_time", 0.10f}, {"release_time", 0.2667f},
             {"voice_count", 0.20f}, {"output_gain", 0.80f}},
            juce::var()
        }
    };
    return presets;
}
