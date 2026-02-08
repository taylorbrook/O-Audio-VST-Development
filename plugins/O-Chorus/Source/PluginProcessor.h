/*
  ==============================================================================

    O-Chorus - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DSP/ChorusEngine.h"

#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicense.h"
#endif

class OChorusAudioProcessor : public juce::AudioProcessor
{
public:
    OChorusAudioProcessor();
    ~OChorusAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Chorus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

#if OUARICON_LICENSING_ENABLED
    OuariconLicense& getLicenseManager() { return *licenseManager; }
#endif

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    ChorusEngine chorusEngine;
    double currentSampleRate = 44100.0;

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicense> licenseManager;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OChorusAudioProcessor)
};
