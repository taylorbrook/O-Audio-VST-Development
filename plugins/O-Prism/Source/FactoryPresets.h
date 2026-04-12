/*
  ==============================================================================

    FactoryPresets.h
    O-Prism - Factory preset library (96 presets across 10 categories)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the full factory preset vector.
        Values in each preset are stored as normalized [0,1] after conversion
        from raw parameter ranges via APVTS param ranges.
        Tuning parameters are never included (they're excluded in PresetManager). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
