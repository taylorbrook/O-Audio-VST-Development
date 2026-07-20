#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// Grain voice structure for polyphonic grain management
struct GrainVoice
{
    float readPosition = 0.0f;      // Position in delay buffer (samples)
    float windowPosition = 0.0f;    // Progress through envelope (0.0-1.0)
    float playbackRate = 1.0f;      // Pitch shift as playback rate
    float pan = 0.5f;               // Stereo position (0=left, 1=right)
    int grainLengthSamples = 0;     // Length of this grain in samples
    int pitchSemitones = 0;         // Pitch shift in semitones
    bool active = false;            // Whether this voice is currently playing
};

class AngelGrainAudioProcessor : public juce::AudioProcessor
{
public:
    AngelGrainAudioProcessor();
    ~AngelGrainAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AngelGrain"; }
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
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components
    juce::dsp::ProcessSpec spec;

    // Grain buffer (circular delay line)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> grainBuffer;
    static constexpr int maxDelaySeconds = 2;
    int writePosition = 0;

    // Grain voice engine (32 polyphonic voices)
    static constexpr int maxGrainVoices = 32;
    std::array<GrainVoice, maxGrainVoices> grainVoices;

    // Grain scheduler
    int samplesSinceLastGrain = 0;
    int nextGrainInterval = 0;

    // Hann window table for grain envelopes
    static constexpr int windowTableSize = 4096;
    std::array<float, windowTableSize> hannWindow;

    // Note: Using manual linear dry/wet mixing for intuitive 50% behavior

    // Random number generator
    juce::Random random;

    // Current sample rate for calculations
    double currentSampleRate = 44100.0;

    // Pre-allocated buffers for real-time safety
    juce::AudioBuffer<float> wetBuffer;
    juce::AudioBuffer<float> dryBuffer;

    // Feedback buffer for feedback loop (stereo)
    float feedbackSampleL = 0.0f;
    float feedbackSampleR = 0.0f;

    // Helper methods
    void spawnGrain();
    float getWindowSample(float normalizedPosition, float tukeyAlpha);
    int findFreeVoice();
    int selectPitchShift(float chaosAmount);
    float calculatePlaybackRate(int semitones);
    float quantizeDelayTimeToTempo(float delayTimeMs, double bpm);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AngelGrainAudioProcessor)
};
