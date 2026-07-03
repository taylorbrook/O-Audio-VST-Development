/*
  ==============================================================================

    FactoryPresets.h
    O-simplePhysicalModelSynth — factory preset library (6 presets).

    The six concept-isolating presets named in the brief (Bright Steel, Muted
    Nylon, Koto/Harp, Struck Bar, Bell, Bowed String) — covering all 3 exciters
    (Pluck/Strike/Bow) and both resonators (String KS / Modal). Raw parameter
    values are converted to APVTS-normalized [0,1] at build time via each
    parameter's NormalisableRange. Materialized to JSON under
    ~/Library/O-simplePhysicalModelSynth/Presets/Factory/ by OuariconPresetManager.

    No "Default" preset: the plugin's power-on sound is already the
    createParameterLayout() defaults, and a Default that set `material` would
    stomp the damping/decay defaults via the macro listener (RESEARCH §2).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include "OuariconPresetManager.h"

namespace FactoryPresets
{
    /** Build the 6 factory presets (normalized values). */
    std::vector<OuariconPresetManager::FactoryPresetDef>
        build (juce::AudioProcessorValueTreeState& apvts);
}
