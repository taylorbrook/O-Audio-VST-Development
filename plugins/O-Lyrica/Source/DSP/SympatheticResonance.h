/*
  ==============================================================================

    SympatheticResonance.h
    Sympathetic Resonance Engine - Phase 2.7 (Thread-Safe v1.3.2)
    Ouaricon Audio
    Developer: Taylor Brook

    Models acoustic coupling between strings. Tracks all active voices and
    their fundamental frequencies, identifies harmonically related voices
    (unison, octave, fifth, third), and applies damped coupling based on
    frequency relationships and material properties.

    THREAD SAFETY (v1.3.2):
    - Uses fixed-size arrays instead of dynamic maps (no allocation on audio thread)
    - Double-buffered coupling matrix for lock-free reads during audio processing
    - Atomic flags for voice active status
    - Coupling matrix rebuilt at block boundaries, not per-voice registration

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "StringMaterial.h"
#include <array>
#include <atomic>

/**
 * Sympathetic Resonance Engine (Thread-Safe)
 *
 * Processor-level component that tracks all active voices and computes
 * sympathetic coupling between harmonically related strings. When one
 * string vibrates, nearby harmonic frequencies are excited through
 * acoustic coupling, creating authentic harp shimmer and bloom.
 *
 * THREAD SAFETY: Uses fixed arrays and double-buffered coupling matrix
 * for lock-free audio thread access. No dynamic allocation during processing.
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
     * @return Slot index for this voice (pass to computeSympatheticContribution), or -1 if full
     */
    int registerVoice(int voiceId, double frequency, const StringMaterial& material);

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
     * Set resonator Q (sharpness) for sympathetic filters (v1.3.0)
     * @param Q Quality factor (0.1 = broad/diffuse, 20.0 = sharp/ringing)
     * Higher Q = more defined resonant peaks, more "shimmer"
     * Lower Q = broader resonance, more diffuse coupling
     */
    void setResonatorQ(float Q);

    /**
     * Compute sympathetic contribution for a specific voice
     * Called per-sample during voice rendering to add coupled energy
     * from other harmonically related strings.
     *
     * @param slotIndex Slot index returned by registerVoice()
     * @param voiceOutput Current sample output from this voice
     * @return Sympathetic contribution to add to voice output
     */
    float computeSympatheticContribution(int slotIndex, float voiceOutput);

    /**
     * Reset all resonators and clear voice registry
     */
    void reset();

    /**
     * Synchronize coupling matrix before processing a block
     * Call this at the START of each processBlock to safely update the read buffer
     * if any voices have been registered/unregistered since last block.
     */
    void syncBeforeBlock();

private:
    static constexpr int MAX_VOICES = 16;
    static constexpr int MAX_COUPLINGS_PER_VOICE = 15;  // Max couplings = MAX_VOICES - 1

    /**
     * Voice information tracked by the engine (fixed-size slot)
     */
    struct VoiceSlot
    {
        std::atomic<bool> active{false};                // Thread-safe active flag
        int voiceId = -1;                               // External voice ID
        double frequency = 440.0;                       // Fundamental frequency
        float materialCoupling = 0.5f;                  // Material coupling coefficient (0-1)
        juce::dsp::IIR::Filter<float> resonatorFilter;  // Bandpass filter tuned to frequency
        float lastSample = 0.0f;                        // Last sample for feedback
        float energyDecay = 0.9995f;                    // Per-sample energy decay factor
    };

    /**
     * Precomputed coupling between two voices
     * Computed once on voice registration, used every sample
     */
    struct CouplingEntry
    {
        int slotIndex = -1;         // Index into voiceSlots array
        float totalCoupling = 0.0f; // Precomputed: harmonic * material * intensity scale
    };

    /**
     * Coupling data for one voice slot (fixed-size)
     */
    struct SlotCouplings
    {
        std::array<CouplingEntry, MAX_COUPLINGS_PER_VOICE> couplings;
        int count = 0;  // Number of valid couplings
    };

    /**
     * Double-buffered coupling matrix for thread-safe access
     */
    struct CouplingBuffer
    {
        std::array<SlotCouplings, MAX_VOICES> slotCouplings;
    };

    // Fixed-size voice slot array (no dynamic allocation)
    std::array<VoiceSlot, MAX_VOICES> voiceSlots;

    // Double-buffered coupling matrices
    std::array<CouplingBuffer, 2> couplingBuffers;
    std::atomic<int> readBufferIndex{0};    // Audio thread reads from this buffer
    std::atomic<bool> rebuildPending{false}; // Flag indicating rebuild needed

    // Parameters
    float intensity = 0.3f;          // Global intensity parameter
    float resonatorQ = 5.0f;         // v1.3.0: Resonator Q (sharpness)
    double sampleRate = 44100.0;     // Current sample rate

    /**
     * Find slot index for a voice ID, or -1 if not found
     */
    int findSlotForVoice(int voiceId) const;

    /**
     * Find first inactive slot, or -1 if all full
     */
    int findInactiveSlot() const;

    /**
     * Rebuild coupling matrix into the write buffer
     * Called from syncBeforeBlock when rebuildPending is true
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
     */
    float checkHarmonicIntervalFast(double freq1, double freq2, double ratio, double toleranceRatio) const;

    /**
     * Design resonator filter for a given frequency
     */
    void designResonatorFilter(juce::dsp::IIR::Filter<float>& filter, double frequency, float Q);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SympatheticResonanceEngine)
};
