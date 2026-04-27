/*
  ==============================================================================

    OContrabassMPESynthesiser.h
    O-Contrabass - MPESynthesiser subclass with CC11 Expression handling
    Ouaricon Audio
    Developer: Taylor Brook

    Patterned on O-Bowed/Source/BowedMPESynthesiser.h. Dispatches CC11
    (Expression) to active voices on the matching channel. Base class
    handleController is empty -- no need to call it.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "BowedContrabassVoice.h"

namespace OContrabassMPEConstants
{
    constexpr float inv127 = 1.0f / 127.0f;
}

class OContrabassMPESynthesiser : public juce::MPESynthesiser
{
public:
    void handleController (int midiChannel, int controllerNumber, int controllerValue) override
    {
        if (controllerNumber == 11) // Expression
        {
            const float normalised = static_cast<float> (controllerValue) * OContrabassMPEConstants::inv127;
            setExpressionForChannel (midiChannel, normalised);
        }
    }

private:
    void setExpressionForChannel (int midiChannel, float normalisedValue)
    {
        for (int i = 0; i < getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<BowedContrabassVoice*> (getVoice (i)))
            {
                if (voice->isActive())
                {
                    auto note = voice->getCurrentlyPlayingNote();
                    if (note.midiChannel == midiChannel)
                        voice->setExpression (normalisedValue);
                }
            }
        }
    }
};
