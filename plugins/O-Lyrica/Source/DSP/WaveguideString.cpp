/*
  ==============================================================================

    WaveguideString.cpp
    Bidirectional Digital Waveguide String Model - Phase 2.2-2.5
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "WaveguideString.h"

WaveguideString::WaveguideString()
{
    // Initialize with default material (Nylon)
    currentMaterial = StringMaterial::fromType(MaterialType::Nylon);
}

WaveguideString::~WaveguideString()
{
}

void WaveguideString::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    // Maximum delay for lowest MIDI note (A0 = 27.5 Hz)
    // Each rail is half the total string length
    // Add safety margin for pitch bend
    int maxDelaySamples = static_cast<int>(sampleRate / 20.0) + 100;

    upperRail.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
    lowerRail.prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
    upperRail.setMaximumDelayInSamples(maxDelaySamples);
    lowerRail.setMaximumDelayInSamples(maxDelaySamples);

    // Prepare filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    bridgeFilter.prepare(spec);
    nutFilter.prepare(spec);
    loopDamping.prepare(spec);
    stiffnessFilter.prepare(sampleRate, maxBlockSize);

    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    stiffnessFilter.reset();

    // Prepare pluck exciter
    exciter.prepare(sampleRate, maxBlockSize);

    updateFilters();
    reset();
}

void WaveguideString::trigger(double frequency, float velocity, float position, float hardness)
{
    currentFrequency = frequency;
    pluckPosition = juce::jlimit(0.05f, 0.95f, position);

    // Calculate delay for each rail (half of full wavelength)
    float railDelay = calculateRailDelay();

    upperRail.setDelay(railDelay);
    lowerRail.setDelay(railDelay);

    // Clear delay lines for clean start
    upperRail.reset();
    lowerRail.reset();

    // Reset filters
    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    stiffnessFilter.reset();

    // Configure stiffness filter for this note
    stiffnessFilter.setParameters(frequency, stiffnessAmount);

    // v1.1.0: Calculate feedback coefficient for this frequency
    feedbackCoefficient = calculateFeedbackCoefficient();

    // Trigger pluck exciter
    exciter.trigger(velocity, position, hardness, frequency);

    // Initialize energy tracking
    currentEnergy = velocity;

    updateFilters();
}

float WaveguideString::processSample()
{
    // v1.7.10: Apply any pending filter updates (thread-safe)
    applyPendingFilterUpdates();

    // Generate excitation from PluckExciter
    float excitation = exciter.process();

    // Read from both delay lines
    float upperOut = upperRail.popSample(0);
    float lowerOut = lowerRail.popSample(0);

    // Bridge reflection: Filter upper rail output, reflect back to lower rail
    // The bridge end has frequency-dependent damping
    float bridgeReflection = bridgeFilter.processSample(upperOut);

    // Nut reflection: Invert lower rail output, reflect back to upper rail
    // The nut end inverts the wave (rigid boundary condition)
    float nutReflection = -nutFilter.processSample(lowerOut);

    // Apply loop damping once per round-trip (at bridge end only)
    // Note: Using same filter for both signals corrupts IIR state
    bridgeReflection = loopDamping.processSample(bridgeReflection);

    // Apply stiffness filter (Phase 2.4: creates inharmonicity/dispersion)
    // This adds frequency-dependent phase shift, making higher harmonics sharp
    bridgeReflection = stiffnessFilter.processSample(bridgeReflection);

    // v1.1.0: Apply feedback coefficient for decay time control
    // This provides uniform energy loss independent of frequency content
    bridgeReflection *= feedbackCoefficient;

    // Inject excitation at pluck position
    // This creates the comb filtering effect based on pluck position
    float excitationToUpper = excitation * pluckPosition;
    float excitationToLower = excitation * (1.0f - pluckPosition);

    // Feed reflected waves back into opposite rails
    upperRail.pushSample(0, nutReflection + excitationToUpper);
    lowerRail.pushSample(0, bridgeReflection + excitationToLower);

    // Output is sum of both traveling waves
    float output = (upperOut + lowerOut) * 0.5f;

    // v1.3.2: Denormal protection - flush tiny values to zero to prevent CPU spikes
    // IIR filter state can accumulate denormals during quiet passages
    if (std::abs(output) < 1e-15f)
        output = 0.0f;

    // Update energy estimate (for voice stealing and activity detection)
    currentEnergy = currentEnergy * energyDecayRate + std::abs(output) * (1.0f - energyDecayRate);

    return output;
}

bool WaveguideString::isActive() const
{
    return currentEnergy > ENERGY_THRESHOLD;
}

void WaveguideString::reset()
{
    upperRail.reset();
    lowerRail.reset();
    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    stiffnessFilter.reset();
    exciter.reset();
    currentEnergy = 0.0f;
}

void WaveguideString::setDamping(float damping)
{
    // v1.1.0: Renamed from "sustain" to "timbre" - controls tonal damping (brightness)
    // This preserves material-specific tonal character while allowing user adjustment
    userDampingModifier = juce::jlimit(0.0f, 1.0f, damping);
    dampingAmount = calculateFinalDamping();
    updateFilters();
}

void WaveguideString::setDecayTime(float decayTimeSeconds_)
{
    // v1.1.0: New parameter for true decay time control via feedback coefficient
    decayTimeSeconds = juce::jlimit(0.1f, 20.0f, decayTimeSeconds_);
    feedbackCoefficient = calculateFeedbackCoefficient();
}

void WaveguideString::setBrightness(float brightness)
{
    brightnessAmount = juce::jlimit(0.0f, 1.0f, brightness);
    updateFilters();

    // v1.1.1 FIX: Recalculate delay line length to compensate for changed filter group delay
    // Without this, changing brightness causes pitch bend (up to 1 semitone at brightness=0)
    if (currentFrequency > 20.0)
    {
        float railDelay = calculateRailDelay();
        upperRail.setDelay(railDelay);
        lowerRail.setDelay(railDelay);
    }
}

void WaveguideString::setBridgeBrightness(float bridgeBrightness)
{
    // v1.3.0: Direct control of bridge filter brightness
    // This is separate from general brightness, providing more waveguide-specific control
    bridgeBrightnessAmount = juce::jlimit(0.0f, 1.0f, bridgeBrightness);
    updateFilters();

    // Recalculate delay for pitch compensation (bridge filter affects group delay)
    if (currentFrequency > 20.0)
    {
        float railDelay = calculateRailDelay();
        upperRail.setDelay(railDelay);
        lowerRail.setDelay(railDelay);
    }
}

void WaveguideString::setAttackNoise(float noiseAmount)
{
    // v1.3.0: Independent attack noise control (overrides material default)
    exciter.setNoiseAmount(noiseAmount);
}

void WaveguideString::setPluckPosition(float position)
{
    pluckPosition = juce::jlimit(0.05f, 0.95f, position);
}

void WaveguideString::setTechnique(PlayingTechnique technique)
{
    exciter.setTechnique(technique);
}

void WaveguideString::setStiffness(float stiffness)
{
    // v1.0.3: User slider now acts as modifier, not overwrite
    // This preserves material-specific stiffness while allowing user adjustment
    userStiffnessModifier = juce::jlimit(0.0f, 1.0f, stiffness);
    stiffnessAmount = calculateFinalStiffness();
    // Update stiffness filter with current frequency and computed stiffness
    stiffnessFilter.setParameters(currentFrequency, stiffnessAmount);
}

void WaveguideString::setTension(float tension)
{
    // v1.2.0: String tension affects brightness and resonance
    // Higher tension = tighter string = brighter tone with more defined harmonics
    // Lower tension = looser string = darker, less resonant
    tensionAmount = juce::jlimit(0.0f, 1.0f, tension);
    updateFilters();
}

void WaveguideString::setGauge(float gauge)
{
    // v1.2.0: String gauge affects mass and damping characteristics
    // Higher gauge = thicker string = more mass = darker tone, heavier attack
    // Lower gauge = thinner string = less mass = brighter, quicker response
    gaugeAmount = juce::jlimit(0.0f, 1.0f, gauge);
    updateFilters();
}

void WaveguideString::setLength(float length)
{
    // v1.2.0: String length affects harmonic decay character (NOT pitch)
    // Longer strings have different energy distribution in harmonics
    // Affects the feedback/decay characteristics without changing fundamental
    lengthAmount = juce::jlimit(0.0f, 1.0f, length);
    // Length affects the feedback coefficient modifier
    feedbackCoefficient = calculateFeedbackCoefficient();
}

void WaveguideString::setMaterial(const StringMaterial& material)
{
    currentMaterial = material;

    // v1.0.4: Store material's base damping, then compute final with user modifier
    // This preserves material-specific decay while allowing user adjustment via sustain slider
    materialDamping = material.dampingCoeff;
    dampingAmount = calculateFinalDamping();

    // v1.0.3: Store material's base stiffness, then compute final with user modifier
    // This ensures different materials produce audibly different inharmonicity
    materialStiffness = material.stiffnessAmount;
    stiffnessAmount = calculateFinalStiffness();

    // Update pluck exciter with noise content
    exciter.setNoiseAmount(material.noiseContent);

    // Update all filters with new material properties
    updateFilters();

    // Update stiffness filter with computed stiffness
    stiffnessFilter.setParameters(currentFrequency, stiffnessAmount);

    // v1.1.2 FIX: Recalculate delay line length to compensate for changed filter group delay
    // Different materials have different brightnessCutoff and dampingCoeff values,
    // which change the filter cutoffs and thus the group delay. Without this,
    // changing materials causes pitch drift (e.g., Gut vs Crystal differs by ~3 samples).
    if (currentFrequency > 20.0)
    {
        float railDelay = calculateRailDelay();
        upperRail.setDelay(railDelay);
        lowerRail.setDelay(railDelay);
    }
}

void WaveguideString::setFrequency(double frequency)
{
    if (frequency > 20.0 && frequency < 20000.0)
    {
        currentFrequency = frequency;

        // Update delay line lengths for new frequency
        float railDelay = calculateRailDelay();
        upperRail.setDelay(railDelay);
        lowerRail.setDelay(railDelay);

        // Update stiffness filter for new frequency
        stiffnessFilter.setParameters(currentFrequency, stiffnessAmount);

        // v1.1.0: Recalculate feedback coefficient (depends on frequency)
        feedbackCoefficient = calculateFeedbackCoefficient();
    }
}

WaveguideString::FilterCutoffs WaveguideString::calculateFilterCutoffs() const
{
    // v1.3.1: Centralized filter cutoff calculation (was duplicated in updateFilters and calculateFilterGroupDelay)
    FilterCutoffs cutoffs;

    // Bridge Filter: Frequency-dependent reflection
    // Uses material's brightnessCutoff as base, modulated by brightness, tension, and bridge brightness
    float materialBrightness = currentMaterial.brightnessCutoff;

    // Tension modifier: tension=0 → 0.5x, tension=0.5 → 1.0x, tension=1.0 → 2.0x
    float tensionBrightnessModifier = 0.5f + tensionAmount * 1.5f;

    // Bridge brightness modifier: 0 → 0.3x, 0.5 → 1.0x, 1.0 → 2.0x
    float bridgeBrightnessModifier = 0.3f + bridgeBrightnessAmount * 1.7f;

    cutoffs.bridgeCutoffHz = materialBrightness * (0.5f + brightnessAmount * 0.8f) * tensionBrightnessModifier * bridgeBrightnessModifier;
    cutoffs.bridgeCutoffHz = juce::jlimit(300.0f, 20000.0f, cutoffs.bridgeCutoffHz);

    // Nut Filter: Higher cutoff (harder boundary than bridge)
    cutoffs.nutCutoffHz = materialBrightness * 1.2f * (0.7f + brightnessAmount * 0.5f) * tensionBrightnessModifier;
    cutoffs.nutCutoffHz = juce::jlimit(1000.0f, 20000.0f, cutoffs.nutCutoffHz);

    // Loop Damping Filter: Material + gauge modifier
    float gaugeDampingModifier = 0.5f + gaugeAmount * 1.5f;
    float effectiveDamping = dampingAmount * gaugeDampingModifier;
    float clampedDamping = juce::jlimit(0.0f, 1.0f, effectiveDamping);
    cutoffs.dampingCutoffHz = 200.0f + (1.0f - clampedDamping) * 14000.0f;
    cutoffs.dampingCutoffHz = juce::jlimit(200.0f, 14000.0f, cutoffs.dampingCutoffHz);

    return cutoffs;
}

void WaveguideString::updateFilters()
{
    // v1.3.1: Use shared cutoff calculation
    FilterCutoffs cutoffs = calculateFilterCutoffs();

    // v1.7.10 FIX: Thread-safe coefficient updates
    // Store cutoffs atomically for deferred application on audio thread
    // This prevents data races when parameters change from message thread
    // while processSample() is running on audio thread
    pendingBridgeCutoff.store(cutoffs.bridgeCutoffHz, std::memory_order_relaxed);
    pendingNutCutoff.store(cutoffs.nutCutoffHz, std::memory_order_relaxed);
    pendingDampingCutoff.store(cutoffs.dampingCutoffHz, std::memory_order_relaxed);
    filterUpdatePending.store(true, std::memory_order_release);
}

void WaveguideString::applyPendingFilterUpdates()
{
    // v1.7.10: Apply filter coefficient updates on audio thread
    // Only called from processSample() - safe to modify filter state
    if (filterUpdatePending.load(std::memory_order_acquire))
    {
        bridgeFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
            currentSampleRate, pendingBridgeCutoff.load(std::memory_order_relaxed));

        nutFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
            currentSampleRate, pendingNutCutoff.load(std::memory_order_relaxed));

        loopDamping.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
            currentSampleRate, pendingDampingCutoff.load(std::memory_order_relaxed));

        filterUpdatePending.store(false, std::memory_order_relaxed);
    }
}

float WaveguideString::calculateRailDelay() const
{
    // Total string delay = sampleRate / frequency
    float totalDelay = static_cast<float>(currentSampleRate / currentFrequency);

    // v1.1.1 FIX: Dynamic compensation for filter group delay
    // The filters (bridgeFilter, nutFilter, loopDamping, stiffnessFilter) add
    // group delay that effectively lengthens the delay line, lowering pitch.
    // Previously used a fixed constant (6.0f), but actual delay varies with
    // brightness parameter - causing pitch to bend when brightness changes.
    float filterGroupDelay = calculateFilterGroupDelay();
    float compensatedDelay = totalDelay - filterGroupDelay;

    // Each rail is half the total length
    return compensatedDelay * 0.5f;
}

float WaveguideString::calculateFilterGroupDelay() const
{
    // v1.1.1: Calculate actual group delay from all filters in the feedback loop
    // For first-order lowpass at DC: group_delay_samples = sampleRate / (2π * cutoffHz)
    // v1.3.1: Use shared cutoff calculation

    float twoPi = juce::MathConstants<float>::twoPi;

    // v1.3.1: Get cutoffs from shared calculation (was duplicated)
    FilterCutoffs cutoffs = calculateFilterCutoffs();

    // Group delay at DC for each first-order lowpass
    // v1.3.2: Added std::max guards for defense-in-depth (cutoffs already clamped to safe minimums)
    float bridgeDelay = static_cast<float>(currentSampleRate) / (twoPi * std::max(cutoffs.bridgeCutoffHz, 1.0f));
    float nutDelay = static_cast<float>(currentSampleRate) / (twoPi * std::max(cutoffs.nutCutoffHz, 1.0f));
    float dampingDelay = static_cast<float>(currentSampleRate) / (twoPi * std::max(cutoffs.dampingCutoffHz, 1.0f));

    // v1.1.3 FIX: Calculate actual stiffness filter group delay from allpass coefficients
    // Previous versions used a fixed 0.5f which caused pitch drift between materials
    // (e.g., Crystal at 0.70 stiffness vs Gut at 0.05 stiffness = ~14x different delay)
    float stiffnessDelay = 0.0f;

    if (stiffnessAmount > 0.001f)
    {
        // Replicate coefficient calculation from StiffnessFilter::updateCoefficients()
        // Frequency scaling: bass strings exhibit more stiffness (same as StiffnessFilter)
        constexpr double referenceFreq = 440.0;
        double freqRatio = currentFrequency / referenceFreq;
        float freqScaling = 1.0f / std::pow(static_cast<float>(freqRatio), 0.3f);
        freqScaling = juce::jlimit(0.5f, 2.0f, freqScaling);

        float baseCoefficient = stiffnessAmount * freqScaling;

        // Sum group delay from all 4 allpass stages
        constexpr int NUM_STAGES = 4;
        for (int i = 0; i < NUM_STAGES; ++i)
        {
            // Progressive scaling per stage (same as StiffnessFilter)
            float stageScaling = 1.0f - (static_cast<float>(i) / NUM_STAGES) * 0.5f;
            float coefficient = baseCoefficient * stageScaling * 0.8f;
            coefficient = juce::jlimit(-0.9f, 0.9f, coefficient);

            // Group delay at DC for first-order allpass: (1 - a) / (1 + a) samples
            if (std::abs(coefficient) > 0.001f)
            {
                stiffnessDelay += (1.0f - coefficient) / (1.0f + coefficient);
            }
        }
    }

    return bridgeDelay + nutDelay + dampingDelay + stiffnessDelay;
}
