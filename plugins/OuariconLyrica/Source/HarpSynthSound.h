/*
  ==============================================================================

    HarpSynthSound.h
    Physical Modeling Harp Synthesizer Sound
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class HarpSynthSound : public juce::SynthesiserSound
{
public:
    HarpSynthSound() {}

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
