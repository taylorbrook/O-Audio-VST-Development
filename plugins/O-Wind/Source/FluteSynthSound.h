/*
  ==============================================================================

    FluteSynthSound.h
    O-Wind - Physical Modeling Flute Synthesizer
    Simple SynthesiserSound implementation

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class FluteSynthSound : public juce::SynthesiserSound
{
public:
    FluteSynthSound() {}

    // All MIDI notes trigger flute sounds (full keyboard range)
    bool appliesToNote(int) override { return true; }

    // All MIDI channels trigger flute sounds
    bool appliesToChannel(int) override { return true; }
};
