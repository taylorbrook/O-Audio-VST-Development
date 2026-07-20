#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

class ScatterAudioProcessor : public juce::AudioProcessor
{
public:
    ScatterAudioProcessor();
    ~ScatterAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Scatter"; }
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

    // Phase 4.2: Grain visualization data structure
    struct GrainVisualizationData
    {
        float x;      // Time position in delay buffer (0.0-1.0)
        float y;      // Pitch shift amount (-1.0 to +1.0, representing -7 to +7 semitones)
        float pan;    // Pan position (0.0-1.0)
    };

    // Phase 4.2: Public accessor for grain positions (thread-safe read)
    std::vector<GrainVisualizationData> getActiveGrainPositions() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Phase 3.1: Core Granular Engine Components

    // Grain voice structure
    struct GrainVoice
    {
        float readPosition = 0.0f;      // Position in delay buffer (fractional samples)
        float windowPosition = 0.0f;    // Position in window envelope (0.0-1.0)
        int grainSizeSamples = 0;       // Duration of this grain in samples
        float playbackRate = 1.0f;      // Playback speed (pitch shift)
        float pan = 0.5f;               // Phase 3.3: Pan position (0.0 = left, 1.0 = right)
        bool reverse = false;           // Phase 3.3: Reverse playback flag
        bool active = false;            // Is this voice currently playing?
    };

    // DSP components (declare BEFORE parameters for initialization order)
    juce::dsp::ProcessSpec spec;

    // Granular delay buffer (Lagrange3rd interpolation for future pitch shifting)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayBuffer;

    // Grain voice pool (64 pre-allocated voices)
    static constexpr int maxGrainVoices = 64;
    std::array<GrainVoice, maxGrainVoices> grainVoices;

    // Grain scheduler state
    int grainSpawnCounter = 0;         // Sample counter for grain spawning
    int lastGrainSpawnInterval = 0;    // Cached spawn interval

    // Window function lookup table (Hann window)
    std::vector<float> hannWindow;
    int windowTableSize = 0;

    // Sample rate tracking
    double currentSampleRate = 44100.0;
    int currentDelayBufferSize = 0;

    // Phase 3.2: Scale quantization lookup tables
    static constexpr int numScales = 5;
    std::array<std::vector<int>, numScales> scaleIntervals;

    // Phase 3.3: Spatial + Reverse + Feedback components
    juce::dsp::DryWetMixer<float> dryWetMixer;
    juce::AudioBuffer<float> feedbackBuffer;

    // Helper methods
    void spawnNewGrain(float grainSizeMs, float pitchRandomPercent, float panRandomPercent, int scaleIndex, int rootNote);
    void updateGrainScheduler(float densityPercent, float grainSizeMs, float pitchRandomPercent, float panRandomPercent, int scaleIndex, int rootNote);
    void processGrainVoices(juce::AudioBuffer<float>& buffer);
    void generateHannWindow(int sizeInSamples);
    void initializeScaleTables();
    int quantizePitchToScale(float pitchSemitones, int scaleIndex, int rootNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScatterAudioProcessor)
};
