/*
  ==============================================================================

    FactoryPresets.h
    O-simpleFM — factory preset library (6 presets).

    Default + the five pedagogical "lesson" sounds (E-Piano, Tubular Bell, Brass,
    Clarinet, Clang Bell). Raw parameter values are converted to APVTS-normalized
    [0,1] at build time via each parameter's NormalisableRange. Materialized to
    JSON under ~/Library/O-simpleFM/Presets/Factory/ by OuariconPresetManager.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the factory preset vector (normalized values). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
