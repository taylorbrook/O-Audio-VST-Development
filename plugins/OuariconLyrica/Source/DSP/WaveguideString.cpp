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

void WaveguideString::updateFilters()
{
    // Bridge Filter: Frequency-dependent reflection
    // Now uses material's brightnessCutoff as base, modulated by brightness parameter
    float materialBrightness = currentMaterial.brightnessCutoff;
    float bridgeCutoffHz = materialBrightness * (0.5f + brightnessAmount * 0.8f); // ±30% from material value
    bridgeCutoffHz = juce::jlimit(500.0f, 20000.0f, bridgeCutoffHz);

    bridgeFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, bridgeCutoffHz);

    // Nut Filter: Simple reflection with slight damping
    // The nut is typically a harder boundary than the bridge
    float nutCutoffHz = materialBrightness * 1.2f * (0.7f + brightnessAmount * 0.5f);
    nutCutoffHz = juce::jlimit(2000.0f, 20000.0f, nutCutoffHz);

    nutFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, nutCutoffHz);

    // Loop Damping Filter: Material-based energy loss + user sustain control
    // v1.0.4: dampingAmount is now pre-computed via calculateFinalDamping()
    // which combines materialDamping with userDampingModifier (0.5x to 1.5x range)
    // Higher damping = lower cutoff = faster high-freq decay = shorter sustain
    float clampedDamping = juce::jlimit(0.0f, 1.0f, dampingAmount);
    float dampingCutoffHz = 500.0f + (1.0f - clampedDamping) * 10000.0f; // 500Hz - 10.5kHz
    dampingCutoffHz = juce::jlimit(300.0f, 12000.0f, dampingCutoffHz);

    loopDamping.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        currentSampleRate, dampingCutoffHz);
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
    // This varies with brightness and damping settings.

    float twoPi = juce::MathConstants<float>::twoPi;

    // Bridge filter cutoff (same formula as updateFilters())
    float materialBrightness = currentMaterial.brightnessCutoff;
    float bridgeCutoffHz = materialBrightness * (0.5f + brightnessAmount * 0.8f);
    bridgeCutoffHz = juce::jlimit(500.0f, 20000.0f, bridgeCutoffHz);

    // Nut filter cutoff
    float nutCutoffHz = materialBrightness * 1.2f * (0.7f + brightnessAmount * 0.5f);
    nutCutoffHz = juce::jlimit(2000.0f, 20000.0f, nutCutoffHz);

    // Loop damping cutoff
    float clampedDamping = juce::jlimit(0.0f, 1.0f, dampingAmount);
    float dampingCutoffHz = 500.0f + (1.0f - clampedDamping) * 10000.0f;
    dampingCutoffHz = juce::jlimit(300.0f, 12000.0f, dampingCutoffHz);

    // Group delay at DC for each first-order lowpass
    float bridgeDelay = static_cast<float>(currentSampleRate) / (twoPi * bridgeCutoffHz);
    float nutDelay = static_cast<float>(currentSampleRate) / (twoPi * nutCutoffHz);
    float dampingDelay = static_cast<float>(currentSampleRate) / (twoPi * dampingCutoffHz);

    // Stiffness filter contributes additional phase delay (allpass cascade)
    // Use a small fixed approximation since it's relatively constant
    float stiffnessDelay = 0.5f;

    return bridgeDelay + nutDelay + dampingDelay + stiffnessDelay;
}
