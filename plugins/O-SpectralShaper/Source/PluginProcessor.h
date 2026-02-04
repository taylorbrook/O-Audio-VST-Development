/*
  ==============================================================================

    O-SpectralShaper - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "STFTProcessor.h"
#include <array>

class OSpectralShaperAudioProcessor : public juce::AudioProcessor
{
public:
    OSpectralShaperAudioProcessor();
    ~OSpectralShaperAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-SpectralShaper"; }
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

    // Public APVTS access for editor
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Curve access for UI (thread-safe via STFTProcessor)
    const std::array<float, 32>& getAttackCurve() const { return attackCurve; }
    const std::array<float, 32>& getSustainCurve() const { return sustainCurve; }
    void setAttackCurve(const std::array<float, 32>& curve);
    void setSustainCurve(const std::array<float, 32>& curve);

    // Visualization data structures (Phase 3.3)
    struct VisualizationFrame
    {
        std::array<float, STFTProcessor::NUM_BINS> fftMagnitudes {};
        std::array<float, STFTProcessor::NUM_BANDS> transientActivity {};
    };

    // Visualization FIFO access (read from GUI thread)
    juce::AbstractFifo& getVisualizationFifo() { return visualizationFifo; }
    const std::vector<VisualizationFrame>& getVisualizationBuffer() const { return visualizationBuffer; }

private:
    // DSP Components (declared BEFORE parameters for correct initialization order)
    STFTProcessor stftProcessor[2];  // L/R stereo
    juce::AudioBuffer<float> dryDelayBuffer;  // 512-sample latency matching
    int dryDelayWritePosition = 0;

    // Lookahead buffer (optional, toggleable via parameter)
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadWritePosition = 0;
    int lookaheadDelayLength = 0;  // In samples
    bool lookaheadEnabled = false;

    // Curve arrays (saved/loaded with plugin state)
    std::array<float, 32> attackCurve {};
    std::array<float, 32> sustainCurve {};

    // Sample rate (cached for lookahead calculation)
    double currentSampleRate = 44100.0;

    // Visualization FIFO (Phase 3.3)
    juce::AbstractFifo visualizationFifo { 60 };  // 60 frames buffered (~1 second at 60fps)
    std::vector<VisualizationFrame> visualizationBuffer { 60 };
    int hopCounter = 0;  // Track when to push frames (once per FFT hop)

    // APVTS (declared AFTER DSP components)
    juce::AudioProcessorValueTreeState parameters;

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Helper methods
    float getDryDelayedSample(int channel, float input);
    void advanceDryDelay();
    float getLookaheadDelayedSample(int channel, float input);
    void writeVisualizationFrame(const VisualizationFrame& frame);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSpectralShaperAudioProcessor)
};
