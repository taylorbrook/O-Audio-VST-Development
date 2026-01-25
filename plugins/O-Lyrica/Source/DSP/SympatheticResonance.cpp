/*
  ==============================================================================

    SympatheticResonance.cpp
    Sympathetic Resonance Engine - Phase 2.7 (Thread-Safe v1.3.2)
    Ouaricon Audio
    Developer: Taylor Brook

    THREAD SAFETY (v1.3.2):
    - Fixed-size arrays replace dynamic maps (no allocation on audio thread)
    - Double-buffered coupling matrix for lock-free audio processing
    - Atomic flags for voice active status
    - rebuildCouplingMatrix deferred to block boundaries via syncBeforeBlock()

  ==============================================================================
*/

#include "SympatheticResonance.h"
#include <algorithm>

// v1.3.2: Named constants for DSP thresholds and tuning parameters
namespace
{
    // Energy decay constants
    constexpr float ENERGY_DECAY_BASE = 0.995f;         // Base decay coefficient
    constexpr float ENERGY_DECAY_MODIFIER = 0.0048f;    // Material-dependent decay range

    // Coupling matrix constants
    constexpr float COUPLING_SCALE_FACTOR = 0.05f;      // Global coupling intensity scaling
    constexpr float INTENSITY_CHANGE_THRESHOLD = 0.01f; // Min change to trigger rebuild
    constexpr float Q_CHANGE_THRESHOLD = 0.05f;         // Min Q change to update filters

    // Soft clipping constants
    constexpr float SOFT_CLIP_THRESHOLD = 0.1f;         // Signal level to start soft clipping
    constexpr float SOFT_CLIP_HEADROOM = 0.05f;         // Extra headroom after tanh

    // Harmonic coupling strengths (relative weights for musical intervals)
    constexpr float UNISON_COUPLING = 0.9f;             // 1:1 ratio - strongest
    constexpr float OCTAVE_COUPLING = 0.7f;             // 2:1 ratio
    constexpr float FIFTH_COUPLING = 0.5f;              // 3:2 ratio
    constexpr float THIRD_COUPLING = 0.3f;              // 5:4 ratio - weakest
}

SympatheticResonanceEngine::SympatheticResonanceEngine()
{
    // Voice slots are default-initialized with active = false
}

SympatheticResonanceEngine::~SympatheticResonanceEngine()
{
}

void SympatheticResonanceEngine::prepare(double newSampleRate, int /*maxBlockSize*/)
{
    sampleRate = newSampleRate;

    // Reset all voice slots with new sample rate
    for (auto& slot : voiceSlots)
    {
        if (slot.active.load(std::memory_order_relaxed))
        {
            juce::dsp::ProcessSpec spec;
            spec.sampleRate = sampleRate;
            spec.maximumBlockSize = 1;
            spec.numChannels = 1;

            slot.resonatorFilter.prepare(spec);
            designResonatorFilter(slot.resonatorFilter, slot.frequency, resonatorQ);
        }
    }

    // Mark rebuild needed
    rebuildPending.store(true, std::memory_order_release);
}

int SympatheticResonanceEngine::findSlotForVoice(int voiceId) const
{
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (voiceSlots[i].active.load(std::memory_order_relaxed) &&
            voiceSlots[i].voiceId == voiceId)
        {
            return i;
        }
    }
    return -1;
}

int SympatheticResonanceEngine::findInactiveSlot() const
{
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (!voiceSlots[i].active.load(std::memory_order_relaxed))
        {
            return i;
        }
    }
    return -1;
}

void SympatheticResonanceEngine::registerVoice(int voiceId, double frequency, const StringMaterial& material)
{
    // First check if voice is already registered
    int existingSlot = findSlotForVoice(voiceId);
    if (existingSlot >= 0)
    {
        // Update existing slot
        auto& slot = voiceSlots[existingSlot];
        slot.frequency = frequency;
        slot.materialCoupling = material.sympatheticCoupling;
        slot.lastSample = 0.0f;
        slot.energyDecay = ENERGY_DECAY_BASE + (1.0f - material.dampingCoeff) * ENERGY_DECAY_MODIFIER;

        // Prepare resonator filter
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 1;
        spec.numChannels = 1;
        slot.resonatorFilter.prepare(spec);
        designResonatorFilter(slot.resonatorFilter, frequency, resonatorQ);

        // Mark rebuild needed
        rebuildPending.store(true, std::memory_order_release);
        return;
    }

    // Find an inactive slot
    int slotIndex = findInactiveSlot();
    if (slotIndex < 0)
    {
        // No available slots - voice limit reached
        DBG("SympatheticResonanceEngine: No available slots for voice " << voiceId);
        return;
    }

    // Configure the slot (before setting active)
    auto& slot = voiceSlots[slotIndex];
    slot.voiceId = voiceId;
    slot.frequency = frequency;
    slot.materialCoupling = material.sympatheticCoupling;
    slot.lastSample = 0.0f;
    slot.energyDecay = ENERGY_DECAY_BASE + (1.0f - material.dampingCoeff) * ENERGY_DECAY_MODIFIER;

    // Prepare resonator filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 1;
    spec.numChannels = 1;
    slot.resonatorFilter.prepare(spec);
    designResonatorFilter(slot.resonatorFilter, frequency, resonatorQ);

    // Atomically activate the slot (write barrier ensures all data visible)
    slot.active.store(true, std::memory_order_release);

    // Mark rebuild needed (will be processed at next syncBeforeBlock)
    rebuildPending.store(true, std::memory_order_release);
}

void SympatheticResonanceEngine::unregisterVoice(int voiceId)
{
    int slotIndex = findSlotForVoice(voiceId);
    if (slotIndex >= 0)
    {
        // Atomically deactivate the slot
        voiceSlots[slotIndex].active.store(false, std::memory_order_release);
        voiceSlots[slotIndex].voiceId = -1;

        // Mark rebuild needed
        rebuildPending.store(true, std::memory_order_release);
    }
}

void SympatheticResonanceEngine::setIntensity(float newIntensity)
{
    float oldIntensity = intensity;
    intensity = juce::jlimit(0.0f, 1.0f, newIntensity);

    // If intensity changed significantly, rebuild coupling matrix
    if (std::abs(intensity - oldIntensity) > INTENSITY_CHANGE_THRESHOLD)
    {
        rebuildPending.store(true, std::memory_order_release);
    }
}

void SympatheticResonanceEngine::setResonatorQ(float Q)
{
    float newQ = juce::jlimit(0.1f, 20.0f, Q);

    // Only update if Q changed significantly
    if (std::abs(newQ - resonatorQ) < Q_CHANGE_THRESHOLD)
        return;

    resonatorQ = newQ;

    // Update all existing resonator filters with new Q
    for (auto& slot : voiceSlots)
    {
        if (slot.active.load(std::memory_order_relaxed))
        {
            designResonatorFilter(slot.resonatorFilter, slot.frequency, resonatorQ);
        }
    }
}

void SympatheticResonanceEngine::syncBeforeBlock()
{
    // Check if rebuild is needed (acquire to see all prior writes)
    if (!rebuildPending.load(std::memory_order_acquire))
        return;

    // Rebuild into the write buffer (opposite of read buffer)
    rebuildCouplingMatrix();

    // Swap buffers atomically
    int oldIndex = readBufferIndex.load(std::memory_order_relaxed);
    readBufferIndex.store(1 - oldIndex, std::memory_order_release);

    // Clear rebuild flag
    rebuildPending.store(false, std::memory_order_release);
}

void SympatheticResonanceEngine::rebuildCouplingMatrix()
{
    // Write to the opposite buffer (not currently being read)
    int writeIndex = 1 - readBufferIndex.load(std::memory_order_relaxed);
    CouplingBuffer& writeBuffer = couplingBuffers[writeIndex];

    // Clear all couplings
    for (auto& slotCouplings : writeBuffer.slotCouplings)
    {
        slotCouplings.count = 0;
    }

    // Build coupling list for each active voice slot
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (!voiceSlots[i].active.load(std::memory_order_relaxed))
            continue;

        const auto& currentSlot = voiceSlots[i];
        SlotCouplings& couplings = writeBuffer.slotCouplings[i];
        couplings.count = 0;

        for (int j = 0; j < MAX_VOICES; ++j)
        {
            if (i == j)
                continue;

            if (!voiceSlots[j].active.load(std::memory_order_relaxed))
                continue;

            const auto& otherSlot = voiceSlots[j];

            // Compute harmonic coupling strength
            float harmonicCoupling = computeCouplingStrength(currentSlot.frequency, otherSlot.frequency);

            if (harmonicCoupling > 0.0f)
            {
                // Precompute total coupling factor
                float materialFactor = currentSlot.materialCoupling * otherSlot.materialCoupling;
                float totalCoupling = harmonicCoupling * materialFactor * intensity * COUPLING_SCALE_FACTOR;

                if (couplings.count < MAX_COUPLINGS_PER_VOICE)
                {
                    couplings.couplings[couplings.count].slotIndex = j;
                    couplings.couplings[couplings.count].totalCoupling = totalCoupling;
                    couplings.count++;
                }
            }
        }
    }
}

float SympatheticResonanceEngine::computeSympatheticContribution(int voiceId, float voiceOutput)
{
    // Fast path: if intensity is zero, skip all processing
    if (intensity <= 0.0f)
        return 0.0f;

    // Find the slot for this voice
    int slotIndex = findSlotForVoice(voiceId);
    if (slotIndex < 0)
        return 0.0f;

    // Read from the current read buffer (acquire to see all writes before buffer swap)
    int bufferIndex = readBufferIndex.load(std::memory_order_acquire);
    const SlotCouplings& couplings = couplingBuffers[bufferIndex].slotCouplings[slotIndex];

    if (couplings.count == 0)
        return 0.0f;

    float sympatheticSignal = 0.0f;

    // Iterate through precomputed couplings
    for (int i = 0; i < couplings.count; ++i)
    {
        const CouplingEntry& entry = couplings.couplings[i];

        // Check if the other voice is still active
        if (!voiceSlots[entry.slotIndex].active.load(std::memory_order_relaxed))
            continue;

        auto& otherSlot = voiceSlots[entry.slotIndex];

        // Apply energy decay
        otherSlot.lastSample *= otherSlot.energyDecay;

        // Excite resonator with precomputed coupling
        otherSlot.lastSample += voiceOutput * entry.totalCoupling;

        // Process through resonator filter
        float resonatorOutput = otherSlot.resonatorFilter.processSample(otherSlot.lastSample);

        sympatheticSignal += resonatorOutput;
    }

    // Soft clipping to prevent buildup
    if (sympatheticSignal > SOFT_CLIP_THRESHOLD)
        sympatheticSignal = SOFT_CLIP_THRESHOLD + std::tanh((sympatheticSignal - SOFT_CLIP_THRESHOLD) * 2.0f) * SOFT_CLIP_HEADROOM;
    else if (sympatheticSignal < -SOFT_CLIP_THRESHOLD)
        sympatheticSignal = -SOFT_CLIP_THRESHOLD + std::tanh((sympatheticSignal + SOFT_CLIP_THRESHOLD) * 2.0f) * SOFT_CLIP_HEADROOM;

    return sympatheticSignal;
}

void SympatheticResonanceEngine::reset()
{
    // Deactivate all voice slots
    for (auto& slot : voiceSlots)
    {
        slot.active.store(false, std::memory_order_relaxed);
        slot.voiceId = -1;
        slot.lastSample = 0.0f;
    }

    // Clear both coupling buffers
    for (auto& buffer : couplingBuffers)
    {
        for (auto& slotCouplings : buffer.slotCouplings)
        {
            slotCouplings.count = 0;
        }
    }

    rebuildPending.store(false, std::memory_order_relaxed);
}

float SympatheticResonanceEngine::computeCouplingStrength(double freq1, double freq2) const
{
    // OPTIMIZED: Use ratio-based tolerances instead of log2 cents calculation
    // Precomputed tolerance ratios (avoiding expensive log2 in hot path):
    // 10 cents ~ 2^(10/1200) ~ 1.00578
    // 15 cents ~ 2^(15/1200) ~ 1.00868
    // 20 cents ~ 2^(20/1200) ~ 1.01159
    // 25 cents ~ 2^(25/1200) ~ 1.01451

    // Unison (1/1) - strongest coupling
    float unisonCoupling = checkHarmonicIntervalFast(freq1, freq2, 1.0, 1.00578);
    if (unisonCoupling > 0.0f)
        return unisonCoupling * UNISON_COUPLING;

    // Octave (1/2, 2/1) - strong coupling
    float octaveDownCoupling = checkHarmonicIntervalFast(freq1, freq2, 0.5, 1.00868);
    float octaveUpCoupling = checkHarmonicIntervalFast(freq1, freq2, 2.0, 1.00868);
    float octaveCoupling = std::max(octaveDownCoupling, octaveUpCoupling);
    if (octaveCoupling > 0.0f)
        return octaveCoupling * OCTAVE_COUPLING;

    // Perfect Fifth (2/3, 3/2) - medium coupling
    float fifthDownCoupling = checkHarmonicIntervalFast(freq1, freq2, 2.0/3.0, 1.01159);
    float fifthUpCoupling = checkHarmonicIntervalFast(freq1, freq2, 3.0/2.0, 1.01159);
    float fifthCoupling = std::max(fifthDownCoupling, fifthUpCoupling);
    if (fifthCoupling > 0.0f)
        return fifthCoupling * FIFTH_COUPLING;

    // Major Third (4/5, 5/4) - weak coupling
    float thirdDownCoupling = checkHarmonicIntervalFast(freq1, freq2, 4.0/5.0, 1.01451);
    float thirdUpCoupling = checkHarmonicIntervalFast(freq1, freq2, 5.0/4.0, 1.01451);
    float thirdCoupling = std::max(thirdDownCoupling, thirdUpCoupling);
    if (thirdCoupling > 0.0f)
        return thirdCoupling * THIRD_COUPLING;

    return 0.0f;
}

float SympatheticResonanceEngine::checkHarmonicIntervalFast(double freq1, double freq2, double ratio, double toleranceRatio) const
{
    // OPTIMIZED: Direct ratio comparison instead of log2 cents calculation

    double actualRatio = freq2 / freq1;
    double targetRatio = ratio;

    // Compute deviation as a ratio (not cents)
    double deviationRatio = actualRatio / targetRatio;

    // Check if within tolerance (tolerance is already in ratio form)
    double invTolerance = 1.0 / toleranceRatio;

    if (deviationRatio >= invTolerance && deviationRatio <= toleranceRatio)
    {
        // Linear falloff based on how close to exact ratio
        // 1.0 at exact match, 0.0 at tolerance edge
        double maxDeviation = toleranceRatio - 1.0;
        double actualDeviation = std::abs(deviationRatio - 1.0);
        return 1.0f - static_cast<float>(actualDeviation / maxDeviation);
    }

    return 0.0f;
}

void SympatheticResonanceEngine::designResonatorFilter(juce::dsp::IIR::Filter<float>& filter, double frequency, float Q)
{
    double clampedFreq = juce::jlimit(20.0, sampleRate * 0.45, frequency);

    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRate,
        clampedFreq,
        Q
    );

    *filter.coefficients = *coefficients;
    filter.reset();
}
