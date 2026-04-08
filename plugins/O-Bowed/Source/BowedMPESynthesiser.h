/*
  ==============================================================================

    BowedMPESynthesiser.h
    O-Bowed - MPESynthesiser subclass with CC11 Expression handling
    Ouaricon Audio
    Developer: Taylor Brook

    Dispatches CC11 (Expression) to active voices on the matching channel.
    Base class handleController is empty -- no need to call it.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "BowedStringVoice.h"

// Normalization constants for CC values
namespace MPEConstants
{
    constexpr float inv127 = 1.0f / 127.0f;
}

class BowedMPESynthesiser : public juce::MPESynthesiser
{
public:
    void handleController (int midiChannel, int controllerNumber, int controllerValue) override
    {
        if (controllerNumber == 11) // Expression
        {
            float normalizedValue = static_cast<float> (controllerValue) * MPEConstants::inv127;
            setExpressionForChannel (midiChannel, normalizedValue);
        }
    }

private:
    void setExpressionForChannel (int midiChannel, float normalizedValue)
    {
        for (int i = 0; i < getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<BowedStringVoice*> (getVoice (i)))
            {
                if (voice->isActive())
                {
                    auto note = voice->getCurrentlyPlayingNote();
                    if (note.midiChannel == midiChannel)
                        voice->setExpression (normalizedValue);
                }
            }
        }
    }
};
