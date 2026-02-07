/*
  ==============================================================================

    OuariconTremolo - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "OuariconPresetManager.h"
#if OUARICON_LICENSING_ENABLED
  #include "OuariconLicense.h"
#endif

class OuariconTremoloAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconTremoloAudioProcessor();
    ~OuariconTremoloAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Tremolo"; }
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

    // Public access to parameters for editor
    juce::AudioProcessorValueTreeState parameters;

    // Preset management
    OuariconPresetManager presetManager;

#if OUARICON_LICENSING_ENABLED
    OuariconLicense& getLicenseManager() { return *licenseManager; }
#endif

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (declared BEFORE parameters for initialization order)
    // LFO state
    float lfoPhase = 0.0f;
    float lfoPhaseIncrement = 0.0f;
    double currentSampleRate = 44100.0;

    // Smoothing filter state (separate for L/R when Pan Sync enabled)
    float smoothedLFO_L = 0.0f;
    float smoothedLFO_R = 0.0f;

    // Random number generator for noise waveform
    juce::Random random;

    // Sample-and-hold state for noise waveform
    float noiseHeldValue = 0.0f;
    float noisePrevHeldValue = 0.0f;  // Previous value for smooth transitions
    int noiseLastQuarter = -1;

    // Cached parameter pointers (valid for processor lifetime)
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* waveformParam = nullptr;
    std::atomic<float>* smoothingParam = nullptr;
    std::atomic<float>* panSyncParam = nullptr;
    std::atomic<float>* tempoSyncParam = nullptr;

    // Helper methods
    float generateWaveform(float phase, int waveformType, float mainLfoPhase);
    float applySmoothingFilter(float rawLFO, float& prevSmoothed, float coefficient);
    float smoothTransition(float t);  // Polynomial smoothing for sharp transitions

#if OUARICON_LICENSING_ENABLED
    std::unique_ptr<OuariconLicense> licenseManager;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconTremoloAudioProcessor)
};
