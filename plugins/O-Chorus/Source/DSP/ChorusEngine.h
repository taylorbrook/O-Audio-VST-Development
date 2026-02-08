/*
  ==============================================================================

    O-Chorus - Chorus DSP Engine
    Multi-voice BBD-style chorus with modulated delay lines
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ChorusEngine
{
public:
    ChorusEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void process (juce::AudioBuffer<float>& buffer,
                  float rate, float depth, int voices,
                  float width, float tone, float mix, float drive);

private:
    struct ChorusVoice
    {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 4410 };
        float lfoPhaseOffset = 0.0f;
        float depthVariation = 1.0f;
        float panPosition = 0.5f;
    };

    void setVoiceCount (int newCount);
    void updateToneFilter (float toneParam);

    static float saturate (float sample, float drive);
    static float mapToneParamToCutoff (float toneParam);

    static constexpr int maxVoices = 8;
    static constexpr float baseDelayMs = 10.0f;
    static constexpr float delayRangeMs = 5.0f;
    static constexpr float maxDelayMs = 50.0f;

    std::array<ChorusVoice, maxVoices> voices;

    // LFO state
    float lfoPhase = 0.0f;

    // Tone filter (stereo: one per channel)
    juce::dsp::IIR::Filter<float> toneFilterL, toneFilterR;
    float lastToneParam = 0.0f;

    // Voice crossfade
    int currentVoiceCount = 4;
    int targetVoiceCount = 4;
    float crossfadeProgress = 1.0f;
    static constexpr float crossfadeDurationMs = 50.0f;
    float crossfadeIncrement = 0.0f;

    // Smoothed parameters
    juce::SmoothedValue<float> smoothedRate;
    juce::SmoothedValue<float> smoothedDepth;
    juce::SmoothedValue<float> smoothedWidth;
    juce::SmoothedValue<float> smoothedMix;
    juce::SmoothedValue<float> smoothedDrive;
    juce::SmoothedValue<float> smoothedTone;

    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusEngine)
};
