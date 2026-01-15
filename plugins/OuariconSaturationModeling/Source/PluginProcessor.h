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
#include <atomic>

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

    // VU Meter levels (atomic for thread-safe access from editor)
    // Uses peak level like TapeAge (getMagnitude), not RMS
    std::atomic<float> inputLevelDB { -100.0f };   // Peak level in dB
    std::atomic<float> outputLevelDB { -100.0f };  // Peak level in dB

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Oversampling (LOW=none, MID=2x, HIGH=4x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingLow;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingMid;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingHigh;

    juce::dsp::ProcessSpec spec;
    int currentQuality = 1;  // Track current quality mode (0=LOW, 1=MID, 2=HIGH)

    // DIODE model state (per-channel previous voltage)
    std::vector<float> diodePrevVoltage;

    // TRANSFORMER model filters and parameters
    std::vector<juce::dsp::IIR::Filter<float>> transformerLFBumpFilters;
    std::vector<juce::dsp::IIR::Filter<float>> transformerHFSheenFilters;
    static constexpr float TRANSFORMER_CORE_SATURATION = 0.8f;

    // TUBE model filters and state
    std::vector<juce::dsp::IIR::Filter<float>> tubePresenceFilters;
    std::vector<float> tubePrevPlateVoltage;
    static constexpr float TUBE_VSUPPLY = 250.0f;

    // MAGNETIC model (Jiles-Atherton hysteresis)
    std::vector<juce::dsp::IIR::Filter<float>> magneticHeadBumpFilters;
    std::vector<juce::dsp::IIR::Filter<float>> magneticHFRolloffFilters;
    std::vector<float> magneticM;
    std::vector<float> magneticHPrev;
    static constexpr float MAGNETIC_MS = 1.0f;
    static constexpr float MAGNETIC_A = 0.4f;
    static constexpr float MAGNETIC_ALPHA = 0.01f;
    static constexpr float MAGNETIC_K = 0.2f;
    static constexpr float MAGNETIC_C = 0.8f;

    // Auto-gain RMS envelopes
    std::vector<float> inputRMSEnvelope;
    std::vector<float> outputRMSEnvelope;
    float autoGainCoeff = 0.0f;

    // Processing helpers (consolidated from processBlock)
    float calculatePeakDB(const juce::AudioBuffer<float>& buffer);
    void captureInputRMS(const juce::AudioBuffer<float>& buffer);
    void processSaturationDirect(juce::AudioBuffer<float>& buffer, int model, float intensity, int iterations);
    void processSaturationBlock(juce::dsp::AudioBlock<float>& block, int model, float intensity, int iterations);
    float processSample(float input, int model, float intensity, int iterations, int channel);
    void applyAutoGain(juce::AudioBuffer<float>& buffer, bool enabled);

    // Saturation model implementations
    float processDiodeSample(float input, float intensity, int iterations, float& prevVoltage);
    float processTransformerSample(float input, float intensity, int channel);
    float processTubeSample(float input, float intensity, int iterations, int channel, float& prevPlateVoltage);
    float processMagneticSample(float input, float intensity, int channel);
    float langevinFunction(float x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconSaturationModelingAudioProcessor)
};
