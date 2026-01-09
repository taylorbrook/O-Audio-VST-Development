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

    // TUBE model components (Phase 2.3)
    // ----------------------------------

    // Frequency response filter (per-channel)
    std::vector<juce::dsp::IIR::Filter<float>> tubePresenceFilters;  // 3kHz peak filter

    // Per-channel previous plate voltage (for warm start)
    std::vector<float> tubePrevPlateVoltage;

    // TUBE model parameters (fixed, from architecture.md)
    static constexpr float TUBE_MU = 100.0f;       // Amplification factor (12AX7 typical)
    static constexpr float TUBE_KP = 600.0f;       // Plate coefficient
    static constexpr float TUBE_EX = 1.4f;         // Plate current exponent
    static constexpr float TUBE_KG1 = 1060.0f;     // Grid coefficient
    static constexpr float TUBE_VSUPPLY = 250.0f;  // Plate supply voltage (fixed)
    static constexpr float TUBE_RLOAD = 100000.0f; // Load resistance (100kΩ)

    // Helper functions
    float processDiodeSample(float input, float intensity, int iterations, float& prevVoltage);
    float processTransformerSample(float input, float intensity, int channel);
    float processTubeSample(float input, float intensity, int iterations, int channel, float& prevPlateVoltage);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessor)
};
