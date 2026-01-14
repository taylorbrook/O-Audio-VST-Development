/*
  ==============================================================================

    CompressorUnit - Embeddable Dynamics Compressor Module
    Ouaricon Audio Module System v1.2.1

    Compact compressor with 4 essential parameters:
    - Threshold, Ratio, Attack, Release
    Fixed 6dB soft knee for musical response.

    v1.2.1 Changes:
    - Added gain smoothing to prevent clicks at high gain reduction

    Based on Ouaricon Compressor DSP.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>

class CompressorUnit
{
public:
    // Parameter prefix for APVTS registration
    static constexpr const char* PARAM_PREFIX = "comp_";

    // Fixed internal values
    static constexpr float FIXED_KNEE_DB = 6.0f;
    static constexpr float GAIN_SMOOTH_COEFF = 0.005f;  // Smooth gain changes to prevent clicks

    CompressorUnit() = default;
    ~CompressorUnit() = default;

    //==========================================================================
    // Parameter Layout - call from host's createParameterLayout()
    //==========================================================================
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& prefix = PARAM_PREFIX)
    {
        // Enabled (bypass toggle)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "enabled", 1 },
            "Compressor Enabled",
            true  // ON by default
        ));

        // Threshold
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "threshold", 1 },
            "Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -20.0f,
            "dB"
        ));

        // Ratio
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "ratio", 1 },
            "Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            2.0f,
            ":1"
        ));

        // Attack
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "attack", 1 },
            "Attack",
            juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f),
            10.0f,
            "ms"
        ));

        // Release
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "release", 1 },
            "Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f),
            100.0f,
            "ms"
        ));

        // Autogain (makeup gain = gain reduction)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "autogain", 1 },
            "Auto Gain",
            false  // OFF by default
        ));
    }

    //==========================================================================
    // Initialization - call from host's prepareToPlay()
    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        envelopeDB = -60.0f;

        // Initialize coefficients with defaults
        updateCoefficients(10.0f, 100.0f);
    }

    void reset()
    {
        envelopeDB = -60.0f;
        smoothedGainReduction = 0.0f;
        smoothedGainLinear = 1.0f;  // Unity gain
        gainReductionDB.store(0.0f);
    }

    //==========================================================================
    // Processing - call from host's processBlock()
    //==========================================================================
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& prefix = PARAM_PREFIX)
    {
        // Check bypass
        auto* enabledParam = apvts.getRawParameterValue(prefix + "enabled");
        if (enabledParam->load() < 0.5f)
        {
            gainReductionDB.store(0.0f);
            return;  // Bypassed
        }

        // Read parameters
        auto* thresholdParam = apvts.getRawParameterValue(prefix + "threshold");
        auto* ratioParam = apvts.getRawParameterValue(prefix + "ratio");
        auto* attackParam = apvts.getRawParameterValue(prefix + "attack");
        auto* releaseParam = apvts.getRawParameterValue(prefix + "release");
        auto* autogainParam = apvts.getRawParameterValue(prefix + "autogain");

        float thresholdDB = thresholdParam->load();
        float ratio = ratioParam->load();
        float attackTimeMs = attackParam->load();
        float releaseTimeMs = releaseParam->load();
        bool autogainEnabled = autogainParam->load() > 0.5f;

        // Update coefficients
        updateCoefficients(attackTimeMs, releaseTimeMs);

        // Process audio
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        float peakGainReduction = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Stereo-linked detection: use max of all channels
            float maxInputLevel = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float inputLevel = std::abs(buffer.getSample(channel, sample));
                maxInputLevel = std::max(maxInputLevel, inputLevel);
            }

            // Convert to dB
            float inputLevelDB = juce::Decibels::gainToDecibels(maxInputLevel, -60.0f);

            // Envelope follower
            if (inputLevelDB > envelopeDB)
            {
                envelopeDB += (inputLevelDB - envelopeDB) * attackCoeff;
            }
            else
            {
                envelopeDB += (inputLevelDB - envelopeDB) * releaseCoeff;
            }

            // Calculate gain reduction
            float gr = calculateGainReduction(envelopeDB, thresholdDB, ratio, FIXED_KNEE_DB);
            peakGainReduction = std::max(peakGainReduction, gr);

            // Convert to linear gain
            float targetGainLinear = juce::Decibels::decibelsToGain(-gr);

            // Smooth the gain to prevent clicks at high GR
            smoothedGainLinear += (targetGainLinear - smoothedGainLinear) * GAIN_SMOOTH_COEFF;

            // Apply smoothed gain
            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer(channel);
                channelData[sample] *= smoothedGainLinear;
            }
        }

        // Update atomic meter value
        gainReductionDB.store(peakGainReduction);

        // Apply autogain (smoothed, conservative makeup gain)
        if (autogainEnabled && peakGainReduction > 0.0f)
        {
            // Smooth the autogain envelope (slower than compression to avoid pumping)
            const float autogainCoeff = 0.0005f;  // Very slow follower
            smoothedGainReduction += (peakGainReduction - smoothedGainReduction) * autogainCoeff;

            // Apply only 60% of the smoothed gain reduction to avoid clipping
            float makeupDB = smoothedGainReduction * 0.6f;
            float makeupGainLinear = juce::Decibels::decibelsToGain(makeupDB);
            buffer.applyGain(makeupGainLinear);
        }
    }

    //==========================================================================
    // Metering - thread-safe access for UI
    //==========================================================================
    float getGainReductionDB() const { return gainReductionDB.load(); }

private:
    double currentSampleRate = 44100.0;
    float envelopeDB = -60.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float smoothedGainReduction = 0.0f;  // For conservative autogain
    float smoothedGainLinear = 1.0f;     // Smoothed gain to prevent clicks

    std::atomic<float> gainReductionDB { 0.0f };

    //==========================================================================
    // DSP Helpers
    //==========================================================================
    float calculateGainReduction(float inputLevel, float thresholdDB,
                                  float ratio, float kneeDB)
    {
        float x = inputLevel - thresholdDB;

        if (x < -kneeDB / 2.0f)
        {
            // Below knee - no compression
            return 0.0f;
        }
        else if (x > kneeDB / 2.0f)
        {
            // Above knee - full compression
            return x - (x / ratio);
        }
        else
        {
            // Inside knee - smooth transition
            float kneeFactor = (x + kneeDB / 2.0f) / kneeDB;
            float fullReduction = x - (x / ratio);
            return kneeFactor * kneeFactor * fullReduction;
        }
    }

    void updateCoefficients(float attackTimeMs, float releaseTimeMs)
    {
        attackCoeff = 1.0f - std::exp(-1.0f / (attackTimeMs * static_cast<float>(currentSampleRate) / 1000.0f));
        releaseCoeff = 1.0f - std::exp(-1.0f / (releaseTimeMs * static_cast<float>(currentSampleRate) / 1000.0f));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorUnit)
};
