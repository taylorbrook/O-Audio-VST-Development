/*
  ==============================================================================

    OBass - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Bass enhancement plugin with crossover filtering and harmonic generation.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/CrossoverFilter.h"
#include "DSP/MonoSummer.h"
#include "DSP/CleanModeProcessor.h"

class OBassAudioProcessor : public juce::AudioProcessor
{
public:
    OBassAudioProcessor();
    ~OBassAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "OBass"; }
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

    // Public method for mode changes (called from UI/automation)
    void setLatencyMode(CrossoverFilter::Mode mode);

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP spec for processor configuration
    juce::dsp::ProcessSpec spec;

    // DSP Components
    CrossoverFilter crossover;
    MonoSummer monoSummer;
    CleanModeProcessor cleanModeProcessor;

    // Intermediate buffers (pre-allocated in prepareToPlay)
    juce::AudioBuffer<float> lowBandBuffer;   // Stereo low frequencies
    juce::AudioBuffer<float> highBandBuffer;  // Stereo high frequencies
    juce::AudioBuffer<float> monoBuffer;      // Mono bass for processing

    // State tracking
    CrossoverFilter::Mode lastReportedMode = CrossoverFilter::Mode::LowLatency;

    // Helper methods
    void updateLatencyReport();
    void recombineBands(juce::AudioBuffer<float>& output,
                        const juce::AudioBuffer<float>& lowBand,
                        const juce::AudioBuffer<float>& highBand);
    float calculateHighBandEnergy(const juce::AudioBuffer<float>& highBand);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OBassAudioProcessor)
};
