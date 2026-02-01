/*
  ==============================================================================

    WavetableSound.h
    SynthesiserSound subclass for wavetable synth

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class WavetableSound final : public juce::SynthesiserSound
{
public:
    WavetableSound() = default;

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
