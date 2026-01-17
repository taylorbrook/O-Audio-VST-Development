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
#include <unordered_map>
#include <vector>

/**
 * Sympathetic Resonance Engine (Optimized)
 *
 * Processor-level component that tracks all active voices and computes
 * sympathetic coupling between harmonically related strings. When one
 * string vibrates, nearby harmonic frequencies are excited through
 * acoustic coupling, creating authentic harp shimmer and bloom.
 *
 * OPTIMIZATION: Precomputes harmonic coupling matrix on voice registration
 * to avoid O(n²) calculations per sample. Uses unordered_map for O(1) lookup.
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
    static constexpr int MAX_VOICES = 16;

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

    /**
     * Precomputed coupling between two voices
     * Computed once on voice registration, used every sample
     */
    struct CouplingPair
    {
        int otherVoiceId;           // ID of the coupled voice
        float totalCoupling;        // Precomputed: harmonic * material * intensity scale
    };

    std::unordered_map<int, VoiceInfo> activeVoices;  // Registry of active voices (O(1) lookup)
    std::unordered_map<int, std::vector<CouplingPair>> couplingMatrix; // Precomputed couplings per voice
    std::vector<int> activeVoiceIds;                  // Fast iteration list
    float intensity;                                   // Global intensity parameter
    double sampleRate;                                 // Current sample rate

    /**
     * Rebuild coupling matrix for all active voices
     * Called on voice registration/unregistration
     */
    void rebuildCouplingMatrix();

    /**
     * Compute coupling strength between two frequencies
     * Detects harmonic relationships: unison, octave, fifth, third
     * OPTIMIZED: Avoids log2 in hot path by using ratio-based detection
     *
     * @param freq1 First frequency
     * @param freq2 Second frequency
     * @return Coupling strength (0.0-1.0)
     */
    float computeCouplingStrength(double freq1, double freq2) const;

    /**
     * Check if two frequencies form a harmonic interval
     * OPTIMIZED: Uses ratio comparison instead of cents calculation
     * @param freq1 First frequency
     * @param freq2 Second frequency
     * @param ratio Target frequency ratio (e.g., 2.0 for octave)
     * @param toleranceRatio Frequency tolerance as ratio (e.g., 1.006 for ~10 cents)
     * @return Coupling strength (0.0-1.0) based on proximity to exact ratio
     */
    float checkHarmonicIntervalFast(double freq1, double freq2, double ratio, double toleranceRatio) const;

    /**
     * Design resonator filter for a given frequency
     * @param filter Filter to configure
     * @param frequency Center frequency in Hz
     * @param Q Quality factor (bandwidth)
     */
    void designResonatorFilter(juce::dsp::IIR::Filter<float>& filter, double frequency, float Q);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SympatheticResonanceEngine)
};
