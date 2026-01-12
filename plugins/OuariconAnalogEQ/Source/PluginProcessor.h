/*
  ==============================================================================

    Ouaricon Analog EQ - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class OuariconAnalogEQAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconAnalogEQAudioProcessor();
    ~OuariconAnalogEQAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Analog EQ"; }
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

    // VU Meter - output level for WebView (thread-safe)
    std::atomic<float> outputLevelDB { -100.0f };

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (declare BEFORE parameters for initialization order)
    juce::dsp::ProcessSpec spec;

    // EQ Band Filters (4 bands: LF shelf, LMF bell, HMF bell, HF shelf)
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefficients = juce::dsp::IIR::Coefficients<float>;

    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> lfFilter;   // Low shelf
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> lmfFilter;  // Low-mid bell
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> hmfFilter;  // High-mid bell
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> hfFilter;   // High shelf

    // Analog Saturation
    juce::dsp::WaveShaper<float> saturation;

    // Output Gain
    juce::dsp::Gain<float> outputGain;

    // Helper functions
    void updateFilterCoefficients();
    float getQValueFromChoice(int choiceIndex);

    // Previous parameter values (for change detection)
    float previousLfFreq = 0.0f, previousLfGain = 0.0f;
    float previousLmfFreq = 0.0f, previousLmfGain = 0.0f;
    int previousLmfQ = -1;
    float previousHmfFreq = 0.0f, previousHmfGain = 0.0f;
    int previousHmfQ = -1;
    float previousHfFreq = 0.0f, previousHfGain = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconAnalogEQAudioProcessor)
};
