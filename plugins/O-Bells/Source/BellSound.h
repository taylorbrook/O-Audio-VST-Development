/*
  ==============================================================================

    BellSound.h
    O-Bells - Physical Modeling Bell Synthesizer
    Simple SynthesiserSound implementation

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class BellSound : public juce::SynthesiserSound
{
public:
    BellSound() {}

    // All MIDI notes trigger bell sounds (full keyboard range)
    bool appliesToNote(int) override { return true; }

    // All MIDI channels trigger bell sounds
    bool appliesToChannel(int) override { return true; }
};
