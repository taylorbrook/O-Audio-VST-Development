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
 * Bass frequencies are typically mono-summed to:
 * - Maintain phase coherence for sub frequencies
 * - Ensure vinyl/club system compatibility
 * - Maximize low-end punch and clarity
 */
class MonoSummer {
public:
    MonoSummer() = default;
    ~MonoSummer() = default;

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
     * Expand mono signal back to stereo.
     * @param monoInput Mono buffer (1 channel)
     * @param stereoOutput Pre-allocated stereo buffer (2 channels)
     */
    void expandToStereo(const juce::AudioBuffer<float>& monoInput,
                        juce::AudioBuffer<float>& stereoOutput);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonoSummer)
};
