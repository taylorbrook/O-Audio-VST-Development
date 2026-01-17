/*
  ==============================================================================

    SympatheticResonance.h
    Sympathetic Resonance Engine - Phase 2.7
    Ouaricon Audio
    Developer: Taylor Brook

    Models acoustic coupling between strings. Tracks all active voices and
    their fundamental frequencies, identifies harmonically related voices
    (unison, octave, fifth, third), and applies damped coupling based on
    frequency relationships and material properties.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "StringMaterial.h"
#include <map>

/**
 * Sympathetic Resonance Engine
 *
 * Processor-level component that tracks all active voices and computes
 * sympathetic coupling between harmonically related strings. When one
 * string vibrates, nearby harmonic frequencies are excited through
 * acoustic coupling, creating authentic harp shimmer and bloom.
 */
class SympatheticResonanceEngine
{
public:
    SympatheticResonanceEngine();
    ~SympatheticResonanceEngine();

    /**
     * Prepare for playback
     * @param sampleRate Sample rate in Hz
     * @param maxBlockSize Maximum block size
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Register a voice when it starts playing
     * @param voiceId Unique identifier for this voice (typically index)
     * @param frequency Fundamental frequency in Hz
     * @param material String material (affects coupling strength)
     */
    void registerVoice(int voiceId, double frequency, const StringMaterial& material);

    /**
     * Unregister a voice when it stops playing
     * @param voiceId Voice identifier to remove
     */
    void unregisterVoice(int voiceId);

    /**
     * Set sympathetic resonance intensity
     * @param intensity Amount of coupling (0.0 = off, 1.0 = maximum)
     */
    void setIntensity(float intensity);

    /**
     * Compute sympathetic contribution for a specific voice
     * Called per-sample during voice rendering to add coupled energy
     * from other harmonically related strings.
     *
     * @param voiceId Voice requesting sympathetic contribution
     * @param voiceOutput Current sample output from this voice
     * @return Sympathetic contribution to add to voice output
     */
    float computeSympatheticContribution(int voiceId, float voiceOutput);

    /**
     * Reset all resonators and clear voice registry
     */
    void reset();

private:
    /**
     * Voice information tracked by the engine
     */
    struct VoiceInfo
    {
        double frequency;                           // Fundamental frequency
        float materialCoupling;                     // Material coupling coefficient (0-1)
        juce::dsp::IIR::Filter<float> resonatorFilter; // Bandpass filter tuned to frequency
        float lastSample;                           // Last sample for feedback
        float energyDecay;                          // Per-sample energy decay factor

        VoiceInfo()
            : frequency(440.0)
            , materialCoupling(0.5f)
            , lastSample(0.0f)
            , energyDecay(0.9995f)
        {}
    };

    std::map<int, VoiceInfo> activeVoices; // Registry of active voices
    float intensity;                        // Global intensity parameter
    double sampleRate;                      // Current sample rate

    /**
     * Compute coupling strength between two frequencies
     * Detects harmonic relationships: unison, octave, fifth, third
     *
     * @param freq1 First frequency
     * @param freq2 Second frequency
     * @return Coupling strength (0.0-1.0)
     */
    float computeCouplingStrength(double freq1, double freq2) const;

    /**
     * Check if two frequencies form a harmonic interval
     * @param freq1 First frequency
     * @param freq2 Second frequency
     * @param ratio Target frequency ratio (e.g., 2.0 for octave)
     * @param tolerance Frequency tolerance in cents
     * @return Coupling strength (0.0-1.0) based on proximity to exact ratio
     */
    float checkHarmonicInterval(double freq1, double freq2, double ratio, float tolerance) const;

    /**
     * Design resonator filter for a given frequency
     * @param filter Filter to configure
     * @param frequency Center frequency in Hz
     * @param Q Quality factor (bandwidth)
     */
    void designResonatorFilter(juce::dsp::IIR::Filter<float>& filter, double frequency, float Q);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SympatheticResonanceEngine)
};
