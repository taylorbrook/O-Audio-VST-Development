/*
  ==============================================================================

    MarimbaSound.h
    Simple SynthesiserSound subclass for MicroMarimba
    Indicates that all notes can be played by MarimbaVoice

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class MarimbaSound : public juce::SynthesiserSound
{
public:
    MarimbaSound() {}

    // All MIDI notes can trigger this sound
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }

    // All channels can play this sound (omni mode)
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};
