/*
  ==============================================================================

    O-simpleSampler - Sample Sound (shared SynthesiserSound)

    A trivial juce::SynthesiserSound that applies to every note and channel. The
    single shared instance lives on the juce::Synthesiser; each SampleVoice's
    canPlaySound() dynamic_casts to this type so the synth routes all MIDI to the
    sampler voices. No per-sound state — the actual audio source is published to
    the voices per block by the processor (atomic shared_ptr snapshot).

    NB: named SampleSound, NOT SamplerSound. juce::SamplerSound exists in
    juce_audio_formats, and the generated JuceHeader.h does `using namespace
    juce;`, so a class literally named `SamplerSound` is AMBIGUOUS at every
    unqualified use (same class of bug as the regionStart/regionEnd vs juce::end
    collision in PluginProcessor.h). Keep this name `Sampler`-free.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
class SampleSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};
