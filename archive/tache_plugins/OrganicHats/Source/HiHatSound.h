#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class HiHatSound : public juce::SynthesiserSound
{
public:
    HiHatSound() = default;

    bool appliesToNote(int midiNoteNumber) override
    {
        // C1 (36) = closed hi-hat, D1 (38) = open hi-hat
        return midiNoteNumber == 36 || midiNoteNumber == 38;
    }

    bool appliesToChannel(int) override { return true; }
};
