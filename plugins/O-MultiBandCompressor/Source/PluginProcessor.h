/*
  ==============================================================================

    O-MultiBandCompressor - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "DSP/MultiBandProcessor.h"

class OMultiBandCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    OMultiBandCompressorAudioProcessor();
    ~OMultiBandCompressorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-MultiBandCompressor"; }
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

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Access to gain reduction meters for UI (all 4 bands)
    float getLowBandGainReduction() const { return lowBandGainReduction.load(std::memory_order_relaxed); }
    float getLoMidBandGainReduction() const { return loMidBandGainReduction.load(std::memory_order_relaxed); }
    float getHiMidBandGainReduction() const { return hiMidBandGainReduction.load(std::memory_order_relaxed); }
    float getHighBandGainReduction() const { return highBandGainReduction.load(std::memory_order_relaxed); }

    // Access to input/output level meters for UI (Phase 5.3)
    float getInputLevelL() const { return inputLevelL.load(std::memory_order_relaxed); }
    float getInputLevelR() const { return inputLevelR.load(std::memory_order_relaxed); }
    float getOutputLevelL() const { return outputLevelL.load(std::memory_order_relaxed); }
    float getOutputLevelR() const { return outputLevelR.load(std::memory_order_relaxed); }

private:
    // DSP Components (BEFORE parameters for initialization order)
    juce::dsp::ProcessSpec spec;

    // Phase 4.2: Multiband processor (4-band crossover + compressors)
    MultiBandProcessor multibandProcessor;

    // Gain stages
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;

    // Phase 4.3: Dry/wet mixer for parallel compression
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Gain reduction metering (atomic for thread-safe UI access)
    std::atomic<float> lowBandGainReduction { 0.0f };
    std::atomic<float> loMidBandGainReduction { 0.0f };
    std::atomic<float> hiMidBandGainReduction { 0.0f };
    std::atomic<float> highBandGainReduction { 0.0f };

    // Input/output level metering (Phase 5.3)
    std::atomic<float> inputLevelL { 0.0f };
    std::atomic<float> inputLevelR { 0.0f };
    std::atomic<float> outputLevelL { 0.0f };
    std::atomic<float> outputLevelR { 0.0f };

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMultiBandCompressorAudioProcessor)
};
