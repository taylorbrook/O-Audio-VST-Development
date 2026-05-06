/*
  ==============================================================================
    FactoryPresets.h
    O-Bassoon - 4 factory preset definitions for v1.0.0
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the 4-preset factory vector for O-Bassoon v1.0.0.
        Values are stored as normalized [0,1] per OuariconPresetManager
        convention. Tuning state is not preset-persisted at v1.0
        (custom-state callback unused). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
