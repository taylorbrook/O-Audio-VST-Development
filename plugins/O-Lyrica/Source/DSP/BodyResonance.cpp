/*
  ==============================================================================

    BodyResonance.cpp
    OuariconLyrica - Body Resonance Module Implementation

  ==============================================================================
*/

#include "BodyResonance.h"

// v1.3.1: Named constants for mix coefficients (previously magic numbers)
namespace
{
    // Maximum dry signal reduction at bodyAmount=1.0
    // Keeps some dry signal even at maximum body resonance
    constexpr float MAX_DRY_REDUCTION = 0.6f;

    // Wet signal gain multiplier (v1.1.5: Increased from 0.3 for audible effect)
    constexpr float WET_GAIN_MULTIPLIER = 0.7f;
}

BodyResonance::BodyResonance()
{
    // Initialize mode amplitudes to default values
    modeAmplitudes = { 1.0f, 0.8f, 0.6f, 0.4f, 0.3f };
}

void BodyResonance::prepare(double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused(maxBlockSize);
    currentSampleRate = sampleRate;

    // Prepare all body mode filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1; // Mono processing per voice

    // CR-04: Establish each mode's coefficient buffer (order 2, capacity ≥8) and size the filter
    // state OFF the audio thread. Assigning an ArrayCoefficients std::array uses
    // Coefficients::operator=(std::array) → assignImpl, which grows the buffer once here;
    // updateFilterCoefficients() then reuses it with NO heap alloc/free on the audio thread.
    for (int i = 0; i < NUM_MODES; ++i)
    {
        float seedFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45),
                                      BASE_FREQUENCIES[i]);
        *bodyModes[i].coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
            currentSampleRate, seedFreq,
            getQForWoodType(currentWoodType), getGainForWoodType(currentWoodType));
        bodyModes[i].prepare(spec);   // sizes filter state for order 2 (one-time alloc here)
        bodyModes[i].reset();
    }

    // Initialize filter coefficients
    updateFilterCoefficients();
}

void BodyResonance::setBodyParameters(float size, WoodType type, float amount)
{
    // v1.32.6: Thread-safe — store parameters atomically for deferred application on audio thread
    bodyAmount.store(juce::jlimit(0.0f, 1.0f, amount), std::memory_order_relaxed);

    float clampedSize = juce::jlimit(0.0f, 1.0f, size);
    bool needsUpdate = false;

    if (std::abs(pendingBodySize.load(std::memory_order_relaxed) - clampedSize) > 0.001f)
    {
        pendingBodySize.store(clampedSize, std::memory_order_relaxed);
        needsUpdate = true;
    }

    if (static_cast<WoodType>(pendingWoodType.load(std::memory_order_relaxed)) != type)
    {
        pendingWoodType.store(static_cast<int>(type), std::memory_order_relaxed);
        needsUpdate = true;
    }

    if (needsUpdate)
        filterUpdatePending.store(true, std::memory_order_release);
}

void BodyResonance::setModeSpread(float spread)
{
    // v1.32.6: Thread-safe — store atomically for deferred application on audio thread
    float newSpread = juce::jlimit(-1.0f, 1.0f, spread);

    if (std::abs(newSpread - pendingModeSpread.load(std::memory_order_relaxed)) < 0.01f)
        return;

    pendingModeSpread.store(newSpread, std::memory_order_relaxed);
    filterUpdatePending.store(true, std::memory_order_release);
}

float BodyResonance::process(float input)
{
    // IN-06: pending coefficient updates are now applied once per block via applyPendingUpdates()
    // (called from the processor before the per-sample loop), not per sample here.

    // OPTIMIZED: Unrolled 5-mode filter bank (no loop overhead)
    float resonantOutput =
        bodyModes[0].processSample(input) * modeAmplitudes[0] +
        bodyModes[1].processSample(input) * modeAmplitudes[1] +
        bodyModes[2].processSample(input) * modeAmplitudes[2] +
        bodyModes[3].processSample(input) * modeAmplitudes[3] +
        bodyModes[4].processSample(input) * modeAmplitudes[4];

    // IN-01: NaN/Inf guard. A transient non-finite value from the synth/crosstalk feed would
    // otherwise latch all 5 biquad states to NaN permanently (sticky silence). Reset the bank
    // and drop this sample to silence.
    if (! std::isfinite(resonantOutput))
    {
        reset();
        resonantOutput = 0.0f;
    }

    float currentAmount = bodyAmount.load(std::memory_order_relaxed);
    float dryAmount = 1.0f - (currentAmount * MAX_DRY_REDUCTION);
    return input * dryAmount + resonantOutput * (WET_GAIN_MULTIPLIER * currentAmount);
}

void BodyResonance::applyPendingFilterUpdates()
{
    // v1.32.6: Apply coefficient updates on audio thread (called from process())
    if (filterUpdatePending.load(std::memory_order_acquire))
    {
        bodySize = pendingBodySize.load(std::memory_order_relaxed);
        currentWoodType = static_cast<WoodType>(pendingWoodType.load(std::memory_order_relaxed));
        modeSpread = pendingModeSpread.load(std::memory_order_relaxed);

        updateFilterCoefficients();

        filterUpdatePending.store(false, std::memory_order_relaxed);
    }
}

void BodyResonance::reset()
{
    for (auto& filter : bodyModes)
        filter.reset();
}

void BodyResonance::updateFilterCoefficients()
{
    float Q = getQForWoodType(currentWoodType);
    float gain = getGainForWoodType(currentWoodType);

    for (int i = 0; i < NUM_MODES; ++i)
    {
        float scaledFreq = scaleFrequency(BASE_FREQUENCIES[i], i);

        // Clamp to valid range
        scaledFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), scaledFreq);

        // CR-04: RT-safe coefficient update. Assigning an ArrayCoefficients std::array uses
        // Coefficients::operator=(std::array) → assignImpl, reusing the pre-seeded buffer
        // (capacity ≥8) — NO heap alloc/free on the audio thread.
        // v1.1.4: Fixed - was using unity gain (1.0) which caused no audible effect
        *bodyModes[i].coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
            currentSampleRate,
            scaledFreq,
            Q,
            gain  // Resonance boost based on wood type
        );

        // Update mode amplitude based on wood type
        modeAmplitudes[i] = getModeAmplitude(i, currentWoodType);
    }
}

float BodyResonance::getQForWoodType(WoodType type) const
{
    // Q factor controls resonance sharpness
    // Higher Q = sharper, more resonant peaks
    // Lower Q = broader, more damped peaks

    switch (type)
    {
        case WoodType::Spruce:
            return 3.0f;  // Traditional, moderate resonance

        case WoodType::Maple:
            return 2.5f;  // Slightly less resonant, warmer

        case WoodType::Exotic:
            return 4.0f;  // More resonant, exotic character

        case WoodType::Synthetic:
            return 5.0f;  // Sharp, defined modes (less natural)

        default:
            return 3.0f;
    }
}

float BodyResonance::getGainForWoodType(WoodType type) const
{
    // v1.1.5: Increased gain values significantly for audible wood type differences
    // Gain controls how much the resonant frequencies are boosted
    // Linear gain values (not dB) for makePeakFilter

    switch (type)
    {
        case WoodType::Spruce:
            return 3.5f;  // ~10.9 dB - Traditional, balanced resonance

        case WoodType::Maple:
            return 2.8f;  // ~8.9 dB - Warmer, rounder body

        case WoodType::Exotic:
            return 4.5f;  // ~13.1 dB - Pronounced, rich resonance

        case WoodType::Synthetic:
            return 5.5f;  // ~14.8 dB - Sharp, defined peaks

        default:
            return 3.5f;
    }
}

float BodyResonance::getModeAmplitude(int modeIndex, WoodType type) const
{
    // Base amplitude decreases with higher modes
    float baseAmp = 1.0f - (static_cast<float>(modeIndex) * 0.15f);

    // Wood type affects mode emphasis
    switch (type)
    {
        case WoodType::Spruce:
            // Balanced across all modes
            return baseAmp;

        case WoodType::Maple:
            // Emphasize lower modes (warmer)
            return baseAmp * (modeIndex < 2 ? 1.2f : 0.8f);

        case WoodType::Exotic:
            // Emphasize mid-range modes (unique character)
            return baseAmp * (modeIndex == 2 || modeIndex == 3 ? 1.3f : 0.9f);

        case WoodType::Synthetic:
            // Emphasize higher modes (brighter, artificial)
            return baseAmp * (modeIndex > 2 ? 1.3f : 0.7f);

        default:
            return baseAmp;
    }
}

float BodyResonance::scaleFrequency(float baseFreq, int modeIndex) const
{
    // Body size scales from 0.0 (small) to 1.0 (large)
    // Small body = higher frequencies
    // Large body = lower frequencies

    // Map bodySize (0.0-1.0) to scaling factor (2.0-0.5)
    // bodySize = 0.0 → scale = 2.0 (small, high pitch)
    // bodySize = 0.5 → scale = 1.25 (medium)
    // bodySize = 1.0 → scale = 0.5 (large, low pitch)

    float scaleFactor = 2.0f - (bodySize * 1.5f);

    // v1.3.0: Apply mode spread
    // modeSpread > 0: Higher modes spread further apart (stretch)
    // modeSpread < 0: Modes compress together (squish)
    // modeSpread = 0: Original uniform scaling
    //
    // Mode 2 (center mode) is the pivot point - it stays relatively fixed
    // Modes 0,1 shift down/up and modes 3,4 shift up/down based on spread

    float modeOffset = static_cast<float>(modeIndex) - 2.0f;  // -2, -1, 0, +1, +2
    float spreadMultiplier = 1.0f + (modeSpread * modeOffset * 0.15f);  // ±30% max at extremes

    return baseFreq * scaleFactor * spreadMultiplier;
}
