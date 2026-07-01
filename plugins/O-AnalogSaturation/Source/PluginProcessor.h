/*
  ==============================================================================

    O-AnalogSaturation - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <atomic>

class OAnalogSaturationAudioProcessor : public juce::AudioProcessor
{
public:
    OAnalogSaturationAudioProcessor();
    ~OAnalogSaturationAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-AnalogSaturation"; }
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

    // Oversampling (MID=2x, HIGH=4x, LOW=none)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingMid;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplingHigh;

    int currentQuality = 1;  // Track current quality mode (0=LOW, 1=MID, 2=HIGH)

    // TRANSFORMER model filters and parameters
    std::vector<juce::dsp::IIR::Filter<float>> transformerLFBumpFilters;
    std::vector<juce::dsp::IIR::Filter<float>> transformerHFSheenFilters;
    static constexpr float TRANSFORMER_CORE_SATURATION = 0.8f;

    // TUBE model filters
    std::vector<juce::dsp::IIR::Filter<float>> tubePresenceFilters;

    // MAGNETIC model (Jiles-Atherton hysteresis)
    std::vector<juce::dsp::IIR::Filter<float>> magneticHeadBumpFilters;
    std::vector<juce::dsp::IIR::Filter<float>> magneticHFRolloffFilters;

    // CR-01: These tone filters run INSIDE the oversampled nonlinear path, so their
    // coefficients must be designed at the rate that path actually executes at
    // (base * osFactor). Precompute one immutable coefficient set per Quality
    // (index 0=LOW/1x, 1=MID/2x, 2=HIGH/4x) and swap the active set when Quality
    // changes. Assigning a Coefficients::Ptr is a ref-count op (no allocation), so
    // the swap is real-time safe. Coefficient objects are shared across channels.
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> transformerLFBumpCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> transformerHFSheenCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> tubePresenceCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> magneticHeadBumpCoeffs;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 3> magneticHFRolloffCoeffs;
    void applyQualityToneCoeffs(int quality);
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
    double sampleRateHz = 48000.0;
    static constexpr float AUTOGAIN_TIME_CONSTANT_SECONDS = 0.1f;  // 100 ms
    // CR-02: the RMS envelopes update once per block, so the one-pole coefficient
    // must be derived from the ACTUAL block length (not a per-sample constant),
    // keeping the realized time constant ~100 ms regardless of host block size.
    float autoGainBlockCoeff(int numSamples) const;

    // Processing helpers
    float calculatePeakDB(const juce::AudioBuffer<float>& buffer);
    void captureInputRMS(const juce::AudioBuffer<float>& buffer);
    void processSaturationDirect(juce::AudioBuffer<float>& buffer, int model, float intensity);
    void processSaturationBlock(juce::dsp::AudioBlock<float>& block, int model, float intensity);
    float processSample(float input, int model, float intensity, int channel);
    void applyAutoGain(juce::AudioBuffer<float>& buffer, bool enabled);

    // Saturation model implementations
    float processDiodeSample(float input, float intensity);
    float processTransformerSample(float input, float intensity, int channel);
    float processTubeSample(float input, float intensity, int channel);
    float processMagneticSample(float input, float intensity, int channel);
    float langevinFunction(float x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OAnalogSaturationAudioProcessor)
};
