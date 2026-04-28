/*
  ==============================================================================

    BassoonSound.h
    Modal Synthesis Bassoon - Synthesiser Sound
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class BassoonSound : public juce::SynthesiserSound
{
public:
    BassoonSound() {}

    bool appliesToNote   (int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
