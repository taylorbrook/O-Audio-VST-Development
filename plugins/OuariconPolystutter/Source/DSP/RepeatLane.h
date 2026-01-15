/*
  ==============================================================================

    RepeatLane.h
    Ouaricon Polystutter - Single-Lane Beat Repeater
    Phase 2.1: Core single-lane repeat engine with beat sync

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class RepeatLane
{
public:
    RepeatLane();
    ~RepeatLane() = default;

    // Prepare for playback
    void prepare(const juce::dsp::ProcessSpec& spec);

    // Reset state
    void reset();

    // Process a block of audio
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);

    // Trigger a new repeat capture
    void trigger();

    // Parameters
    void setEnabled(bool shouldBeEnabled);
    void setSubdivision(int subdivIndex);
    void setRepeats(int numRepeats);
    void setDecay(float decayPercent);
    void setVolume(float volumePercent);
    void setPan(float panPosition);           // -100 to +100
    void setProbability(float probPercent);   // 0 to 100%
    void setSwing(float swingPercent);        // 0 to 100%

    // Phase 2.4: Pitch shifting
    void setPitch(float pitchSemitones);      // -12 to +12 semitones

    // Phase 2.3: Advanced modes
    void setPingPong(bool shouldEnable);
    void setReverse(bool shouldEnable);
    void setFreeze(bool shouldEnable);
    void setManualTimeEnabled(bool shouldEnable);

    // Pattern sequencer
    void setPatternStep(int stepIndex, bool enabled);  // stepIndex: 0-15
    void updatePatternPosition(double ppqPosition, int subdivIndex);

    // Timing
    void updateTempo(double bpm, double sampleRate);

    // State
    bool isEnabled() const { return enabled; }
    bool isRepeating() const { return currentRepeat < maxRepeats && isTriggered; }

private:
    // State
    bool enabled = true;
    bool isTriggered = false;

    // Parameters
    int subdivisionIndex = 2;  // Default: 1/16
    int maxRepeats = 4;
    float decayAmount = 0.9f;  // 90%
    float volumeLevel = 1.0f;  // 100%
    float panPosition = 0.0f;  // -1.0 to +1.0 (center)
    float probabilityAmount = 1.0f;  // 0.0 to 1.0 (100% = always trigger)
    float swingAmount = 0.0f;  // 0.0 to 1.0 (0% to 100%)
    float pitchSemitones = 0.0f;  // -12.0 to +12.0 semitones
    float pitchRatio = 1.0f;  // Cached pitch ratio from semitones

    // Phase 2.3: Advanced mode parameters
    bool pingPongEnabled = false;
    bool reverseEnabled = false;
    bool freezeEnabled = false;
    bool manualTimeEnabled = false;

    // Timing
    double currentSampleRate = 48000.0;
    double currentBPM = 120.0;
    double subdivisionSamples = 0.0;

    // Repeat state
    int currentRepeat = 0;
    int samplesUntilNextRepeat = 0;
    float currentGain = 1.0f;

    // Phase 2.4: Pitch shifting state (fractional playback position)
    double fractionalPlaybackPosition = 0.0;

    // Capture buffer (using DelayLine for circular buffer)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineLeft;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineRight;

    int captureLength = 0;  // Length of captured audio in samples
    int playbackPosition = 0;

    // Phase 2.3: Freeze buffers (separate snapshots per lane)
    juce::AudioBuffer<float> freezeBuffer;
    bool freezeBufferReady = false;

    // Crossfade buffer for click-free looping
    int crossfadeSamples = 0;

    // Pattern sequencer state
    bool patternSteps[16] = { true, true, true, true, true, true, true, true,
                              true, true, true, true, true, true, true, true };  // All enabled by default
    int currentPatternStep = 0;
    double patternStartPPQ = 0.0;
    double lastPPQPosition = 0.0;

    // Random generator for probability (member to avoid lock in getSystemRandom)
    juce::Random randomGenerator;

    // Helper functions
    double calculateSubdivisionSamples(int subdivIndex, double bpm, double sampleRate);
    void startNewRepeat();
    float getCrossfadeGain(int sampleIndex, int fadeLength, bool fadeIn);
    int calculateSwingOffset(int repeatNumber) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RepeatLane)
};
