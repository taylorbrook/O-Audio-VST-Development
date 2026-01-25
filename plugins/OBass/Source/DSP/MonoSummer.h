#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * MonoSummer handles bass band mono conversion and stereo expansion.
 *
 * Signal flow:
 * 1. sumToMono(): Stereo bass band -> Mono (L+R)/2
 * 2. [Enhancement processing happens on mono signal]
 * 3. expandToStereo(): Mono enhanced -> Stereo output
 *
 * Stereo modes:
 * - Mono: Output same signal to L and R (default, most common for bass)
 * - MatchOriginal: Restore original L/R balance ratio
 */
class MonoSummer {
public:
    enum class StereoMode { Mono, MatchOriginal };

    MonoSummer() = default;
    ~MonoSummer() = default;

    // Configuration
    void setStereoMode(StereoMode mode) { stereoMode = mode; }
    StereoMode getStereoMode() const { return stereoMode; }

    // Lifecycle
    void prepare(int maxBlockSize);
    void reset();

    /**
     * Sum stereo buffer to mono.
     * @param stereoInput Stereo input buffer (2 channels)
     * @param monoOutput Pre-allocated mono buffer (1 channel, same sample count)
     */
    void sumToMono(const juce::AudioBuffer<float>& stereoInput,
                   juce::AudioBuffer<float>& monoOutput);

    /**
     * Capture original stereo balance before summing.
     * Call this BEFORE sumToMono() if using MatchOriginal mode.
     * @param stereoInput The original stereo signal
     */
    void captureBalance(const juce::AudioBuffer<float>& stereoInput);

    /**
     * Expand mono signal back to stereo.
     * @param monoInput Mono buffer (1 channel)
     * @param stereoOutput Pre-allocated stereo buffer (2 channels)
     */
    void expandToStereo(const juce::AudioBuffer<float>& monoInput,
                        juce::AudioBuffer<float>& stereoOutput);

private:
    StereoMode stereoMode = StereoMode::Mono;

    // Balance capture for MatchOriginal mode
    // Stores per-sample L/(L+R) ratio - 0.5 = centered
    std::vector<float> balanceRatios;
    bool balanceCaptured = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonoSummer)
};
