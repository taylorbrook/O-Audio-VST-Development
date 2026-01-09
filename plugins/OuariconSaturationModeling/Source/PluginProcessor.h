/*
  ==============================================================================

    OuariconSaturationModeling - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class OuariconSaturationModelingAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconSaturationModelingAudioProcessor();
    ~OuariconSaturationModelingAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Saturation Modeling"; }
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

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (Phase 2.1: Oversampling + DIODE model)
    // --------------------------------------------------------

    // Oversampling system (3 quality levels)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingLow;   // No oversampling (factor=1)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingMid;   // 2x oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingHigh;  // 4x oversampling

    juce::dsp::ProcessSpec spec;
    int currentQuality = 1;  // Track current quality mode (0=LOW, 1=MID, 2=HIGH)

    // DIODE model state (Newton-Raphson solver)
    // Per-channel previous voltage (for warm start)
    std::vector<float> diodePrevVoltage;

    // DIODE model parameters (fixed, from architecture.md)
    static constexpr float DIODE_IS = 2.52e-9f;   // Saturation current (1N914 typical)
    static constexpr float DIODE_N = 1.752f;      // Ideality factor
    static constexpr float DIODE_VT = 0.026f;     // Thermal voltage

    // TRANSFORMER model components (Phase 2.2)
    // -----------------------------------------

    // Frequency response filters (per-channel)
    std::vector<juce::dsp::IIR::Filter<float>> transformerLFBumpFilters;   // 60Hz peak filter
    std::vector<juce::dsp::IIR::Filter<float>> transformerHFSheenFilters;  // 8kHz high shelf

    // TRANSFORMER model parameters (fixed, from architecture.md)
    static constexpr float TRANSFORMER_CORE_SATURATION = 0.8f;  // Saturation threshold

    // Helper functions
    float processDiodeSample(float input, float intensity, int iterations, float& prevVoltage);
    float processTransformerSample(float input, float intensity, int channel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessor)
};
