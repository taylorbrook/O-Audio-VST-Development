/*
  ==============================================================================

    MicrotonalSamplerSound.h
    Microtonal Sample Engine - Synthesiser Sound
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class MicrotonalSamplerSound : public juce::SynthesiserSound
{
public:
    MicrotonalSamplerSound() {}

    bool appliesToNote   (int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
