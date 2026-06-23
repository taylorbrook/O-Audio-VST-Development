/*
  ==============================================================================

    O-FreqPulse - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"
#include <array>
#include <atomic>

class OFreqPulseAudioProcessor : public juce::AudioProcessor
{
public:
    OFreqPulseAudioProcessor();
    ~OFreqPulseAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-FreqPulse"; }

    // Thread-safe access for GUI timer
    int getBandStep(int band) const { return bandStepAtomics[band].load(); }
    bool getHasAudioSignal() const { return hasAudioSignal.load(); }

    // v1.5.0: Tooltip system state (saved with plugin state)
    bool getTooltipsEnabled() const { return tooltipsEnabled.load(std::memory_order_acquire); }
    void setTooltipsEnabled(bool enabled) { tooltipsEnabled.store(enabled, std::memory_order_release); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // v1.6.0: Preset management
    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;

private:
    // Per-band parameter cache structure
    struct BandParams
    {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* depth = nullptr;
        std::atomic<float>* rate = nullptr;     // v1.7.0: Per-band rate override (0=Global)
        std::atomic<float>* eucOn = nullptr;
        std::atomic<float>* eucSteps = nullptr;
        std::atomic<float>* eucPulses = nullptr;
        std::atomic<float>* eucOffset = nullptr;
        std::atomic<float>* phaseOffset = nullptr;  // v1.14.0: Per-band phase offset (0-31)
        std::atomic<float>* bandSteps = nullptr;   // v1.15.0: Per-band step count (0=global, 2-32)
        std::array<std::atomic<float>*, 32> stepStates;  // 32 step parameters per band
    };

    // Cached parameter pointers (real-time safe access)
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* stepsParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* swingParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;

    // Crossover frequency cached pointers (3 crossover points for 4 bands)
    std::atomic<float>* crossover1Param = nullptr;
    std::atomic<float>* crossover2Param = nullptr;
    std::atomic<float>* crossover3Param = nullptr;

    std::array<BandParams, 4> bandParams;  // 4 bands

    // Linkwitz-Riley crossover filter bank (LR4, -24 dB/oct)
    // Binary tree topology: split at c2 → split low-half at c1, split high-half at c3
    // Produces 4 bands: Sub [DC..c1], Low [c1..c2], Mid [c2..c3], High [c3..Nyquist]
    juce::dsp::LinkwitzRileyFilter<float> crossoverMid;   // splits at c2
    juce::dsp::LinkwitzRileyFilter<float> crossoverLow;   // splits low-half at c1
    juce::dsp::LinkwitzRileyFilter<float> crossoverHigh;  // splits high-half at c3

    // Dry/Wet Mixer
    juce::dsp::DryWetMixer<float> dryWetMixer { 10 };  // 10 samples max delay for dry/wet alignment

    // Per-band attack/release envelope (replaces SmoothedValue for asymmetric ramps)
    struct BandEnvelope
    {
        float current = 1.0f;
        float target = 1.0f;
        float attackRate = 0.0f;   // gain increase per sample
        float releaseRate = 0.0f;  // gain decrease per sample

        void setRates(double sampleRate, float attackMs, float releaseMs)
        {
            float aSamples = std::max(1.0f, attackMs * 0.001f * static_cast<float>(sampleRate));
            float rSamples = std::max(1.0f, releaseMs * 0.001f * static_cast<float>(sampleRate));
            attackRate = 1.0f / aSamples;
            releaseRate = 1.0f / rSamples;
        }

        void setTargetValue(float t) { target = t; }
        void setCurrentAndTargetValue(float v) { current = target = v; }

        float getNextValue()
        {
            if (current < target)
                current = std::min(current + attackRate, target);
            else if (current > target)
                current = std::max(current - releaseRate, target);
            return current;
        }
    };

    std::array<BandEnvelope, 4> bandEnvelopes;
    std::array<float, 4> bandGainFiltered = { 1.0f, 1.0f, 1.0f, 1.0f };  // One-pole LPF on gain to soften ramp corners
    float gainFilterCoeff = 0.0f;  // One-pole coefficient, set in prepareToPlay

    // Euclidean Patterns
    std::array<std::array<bool, 32>, 4> euclideanPatterns;

    // Step Sequencer State
    int currentStep = 0;
    double lastPpqPosition = -1.0;

    // Per-band step positions (for independent per-band playhead highlighting)
    std::array<std::atomic<int>, 4> bandStepAtomics { { {0}, {0}, {0}, {0} } };

    // Audio signal detection (gates playhead movement)
    std::atomic<bool> hasAudioSignal { false };

    // v1.5.0: Tooltip enabled state (saved with plugin state)
    std::atomic<bool> tooltipsEnabled { false };

    // Cached parameter values (for change detection)
    float lastCrossovers[3] = { 0.0f, 0.0f, 0.0f };
    int lastEuclideanParams[4][3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };  // [band][steps/pulses/offset]
    int lastBandSteps[4] = { 0, 0, 0, 0 };  // v1.15.0: Per-band step count change detection
    float lastAttackMs = -1.0f;   // Track attack parameter to avoid recompute every block
    float lastReleaseMs = -1.0f;  // Track release parameter to avoid recompute every block

    // DSP state
    double currentSampleRate = 44100.0;

    // Factory Presets
    int currentProgram = 0;
    static constexpr int numPresets = 12;
    void loadPreset(int presetIndex);
    void initializeFactoryPresets();

    // Helper Methods
    void updateCrossoverFrequencies();
    std::array<bool, 32> generateEuclidean(int steps, int pulses, int offset);
    void updateEuclideanPatterns();
    int calculateCurrentStep(double ppq, int numSteps, int rateIndex, float swing);
    float getTargetGainForBand(int bandIndex, int currentStep, int numSteps);

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreqPulseAudioProcessor)
};
