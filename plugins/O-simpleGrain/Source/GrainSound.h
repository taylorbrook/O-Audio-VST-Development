/*
  ==============================================================================

    O-simpleGrain - Grain Sound

    The single shared juce::SynthesiserSound. Applies to every note/channel, so
    every GrainVoice can play it (canPlaySound checks dynamic_cast<GrainSound*>).

    Mirrors O-simpleFM FMSound.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
class GrainSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};
