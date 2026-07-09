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

    // v2.1.8: OnePoleLPF has no prepare() — only the stiffness filter needs it
    stiffnessFilter.prepare(sampleRate, maxBlockSize);

    bridgeFilter.reset();
    nutFilter.reset();
    loopDamping.reset();
    bridgeFilterShadow.reset();
    nutFilterShadow.reset();
    loopDampingShadow.reset();
    filterCrossfadeRemaining = 0;
    stiffnessFilter.reset();

    // Prepare pluck exciter
    exciter.prepare(sampleRate, maxBlockSize);

    // v1.33.0: Compute étouffé dampening rate for ~50ms decay to -60dB
    float dampeningSamples = static_cast<float>(sampleRate) * 0.05f;
    dampeningDecayRate = std::pow(0.001f, 1.0f / dampeningSamples);

    buzzFilter.reset();
    buzzEnvelope = 0.0f;
    buzzCapturedEnergy = 0.0f;
    buzzOnsetRamp = 0.0f;
    buzzOnsetPhase = false;

    updateFilters();
    reset();
}

void WaveguideString::trigger(double frequency, float velocity, float position, float hardness)
{
    // WR-02: validate the incoming frequency (setFrequency guards, but trigger previously trusted
    // it). A malformed Scala scale / out-of-range degree can make the tuning engine return 0/NaN;
    // that would yield a NaN/Inf rail delay (jlimit passes NaN through) → OOB delay-line read.
    frequency = juce::jlimit(20.0, 20000.0, std::isfinite(frequency) ? frequency : 440.0);

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
    // v2.1.8: Also clear shadows and cancel any in-flight crossfade for new notes
    bridgeFilterShadow.reset();
    nutFilterShadow.reset();
    loopDampingShadow.reset();
    filterCrossfadeRemaining = 0;
    stiffnessFilter.reset();

    // Configure stiffness filter for this note
    stiffnessFilter.setParameters(frequency, stiffnessAmount);

    // v1.1.0: Calculate feedback coefficient for this frequency
    feedbackCoefficient = calculateFeedbackCoefficient();

    // Trigger pluck exciter
    exciter.trigger(velocity, position, hardness, frequency);

    // v1.33.0: Reset dampening for new notes
    dampening = false;
    dampeningMultiplier = 1.0f;

    buzzEnvelope = 0.0f;
    buzzCapturedEnergy = 0.0f;
    buzzOnsetRamp = 0.0f;
    buzzOnsetPhase = false;
    buzzFilter.reset();

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

    // v2.1.8: Crossfaded bridge/nut/damping filtering.
    // During a coefficient transition we run the shadow filters (old coefs +
    // pre-update state) in parallel with the active filters (new coefs) and
    // linearly blend their outputs over FILTER_CROSSFADE_LENGTH samples. This
    // eliminates the click that previously occurred when brightness/tension/
    // material parameters caused a one-sample step in filter response inside
    // the waveguide feedback loop.
    float bridgeReflection;
    float nutReflection;

    if (filterCrossfadeRemaining > 0)
    {
        const float t = 1.0f - static_cast<float>(filterCrossfadeRemaining)
                               / static_cast<float>(FILTER_CROSSFADE_LENGTH);

        // Bridge
        const float newBridge = bridgeFilter.processSample(upperOut);
        const float oldBridge = bridgeFilterShadow.processSample(upperOut);
        bridgeReflection = oldBridge + (newBridge - oldBridge) * t;

        // Nut (rigid-boundary inversion)
        const float newNut = nutFilter.processSample(lowerOut);
        const float oldNut = nutFilterShadow.processSample(lowerOut);
        nutReflection = -(oldNut + (newNut - oldNut) * t);

        // Loop damping (applied once per round-trip at bridge end)
        const float newDamp = loopDamping.processSample(bridgeReflection);
        const float oldDamp = loopDampingShadow.processSample(bridgeReflection);
        bridgeReflection = oldDamp + (newDamp - oldDamp) * t;

        --filterCrossfadeRemaining;
    }
    else
    {
        // Bridge reflection: filter upper rail output, reflect back to lower rail
        bridgeReflection = bridgeFilter.processSample(upperOut);

        // Nut reflection: invert lower rail output, reflect back to upper rail
        nutReflection = -nutFilter.processSample(lowerOut);

        // Apply loop damping once per round-trip (at bridge end only)
        bridgeReflection = loopDamping.processSample(bridgeReflection);
    }

    // Apply stiffness filter (Phase 2.4: creates inharmonicity/dispersion)
    // This adds frequency-dependent phase shift, making higher harmonics sharp
    bridgeReflection = stiffnessFilter.processSample(bridgeReflection);

    // v1.1.0: Apply feedback coefficient for decay time control
    // This provides uniform energy loss independent of frequency content
    float effectiveFeedback = feedbackCoefficient;

    // v1.33.0: Étouffé dampening — rapidly reduce feedback when strings are muted
    if (dampening)
    {
        dampeningMultiplier *= dampeningDecayRate;
        effectiveFeedback *= dampeningMultiplier;
    }

    bridgeReflection *= effectiveFeedback;

    // Inject excitation at pluck position
    // This creates the comb filtering effect based on pluck position
    float excitationToUpper = excitation * pluckPosition;
    float excitationToLower = excitation * (1.0f - pluckPosition);

    if (dampening && (buzzOnsetPhase || buzzEnvelope > 1e-5f))
    {
        // v2.2.2: Onset ramp — gradual palm contact over ~8ms
        float effectiveEnvelope;
        if (buzzOnsetPhase)
        {
            buzzOnsetRamp += buzzOnsetIncrement;
            if (buzzOnsetRamp >= 1.0f)
            {
                buzzOnsetRamp = 1.0f;
                buzzOnsetPhase = false;
                buzzEnvelope = 1.0f;
            }
            effectiveEnvelope = buzzOnsetRamp;
        }
        else
        {
            buzzEnvelope *= buzzDecayRate;
            effectiveEnvelope = buzzEnvelope;
        }

        const float noise = buzzRandom.nextFloat() * 2.0f - 1.0f;
        const float filtered = buzzFilter.processSample(noise);

        constexpr float BUZZ_GAIN = 2.0f;
        const float buzzSample = filtered * effectiveEnvelope * buzzCapturedEnergy * BUZZ_GAIN;
        excitationToUpper += buzzSample * 0.5f;
        excitationToLower += buzzSample * 0.5f;
    }

    // WR-02: finite guard at the feedback push boundary. The denormal flush below (a) only touches
    // the READ output, not nutReflection/bridgeReflection pushed back into the rails, and (b) cannot
    // catch NaN (std::abs(NaN) < 1e-15 is false). If a non-finite value ever enters the loop it would
    // persist forever (currentEnergy → NaN → note stuck). Reset the SOURCE and silence this sample.
    if (! std::isfinite(nutReflection) || ! std::isfinite(bridgeReflection))
    {
        reset();
        exciter.reset();
        return 0.0f;
    }

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
    // v2.1.8: Clear crossfade shadows too
    bridgeFilterShadow.reset();
    nutFilterShadow.reset();
    loopDampingShadow.reset();
    filterCrossfadeRemaining = 0;
    stiffnessFilter.reset();
    exciter.reset();
    currentEnergy = 0.0f;

    buzzFilter.reset();
    buzzEnvelope = 0.0f;
    buzzCapturedEnergy = 0.0f;
    buzzOnsetRamp = 0.0f;
    buzzOnsetPhase = false;
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

void WaveguideString::setGlissandoExcitation(float amount)
{
    exciter.setGlissandoAmount(amount);
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

void WaveguideString::setDampening(bool active)
{
    dampening = active;
    if (active)
    {
        dampeningMultiplier = 1.0f;
        buzzCapturedEnergy = currentEnergy;

        // v2.2.2: Onset ramp — palm contact over ~8ms (not instant)
        buzzEnvelope = 0.0f;
        buzzOnsetRamp = 0.0f;
        buzzOnsetPhase = true;
        float onsetSamples = static_cast<float>(currentSampleRate) * 0.008f;
        buzzOnsetIncrement = 1.0f / onsetSamples;

        // v2.2.2: Pitch-coupled LPF — cutoff relative to fundamental
        // Loud strings: tighter filter (~3×f0) → more tonal character
        // Quiet strings: wider filter (~6×f0) → breathier character
        float energyFactor = juce::jlimit(0.0f, 1.0f, buzzCapturedEnergy * 15.0f);
        float harmonicMult = 6.0f - energyFactor * 3.0f; // quiet=6×f0, loud=3×f0
        float buzzCutoff = static_cast<float>(currentFrequency) * harmonicMult;
        buzzCutoff = juce::jlimit(200.0f, 8000.0f, buzzCutoff);
        buzzFilter.setCutoff(buzzCutoff, currentSampleRate);

        // v2.2.2: Frequency-dependent decay — high strings buzz shorter
        // Base duration ∝ 1/f0, clamped 10-60ms
        float baseDurationMs = juce::jlimit(10.0f, 60.0f,
            8000.0f / static_cast<float>(currentFrequency));
        // Amplitude-dependent: loud strings sustain up to 2× longer
        float energyScale = juce::jlimit(0.5f, 2.0f, buzzCapturedEnergy * 20.0f);
        float buzzDurationMs = baseDurationMs * energyScale;
        float buzzSamples = static_cast<float>(currentSampleRate) * buzzDurationMs * 0.001f;
        buzzDecayRate = std::pow(0.001f, 1.0f / buzzSamples);
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

    // WR-01: cap all three cutoffs below Nyquist (0.45·fs). The absolute-Hz clamps below assumed
    // ≥ 44.1 kHz; at fs ≤ 40 kHz a bright patch (e.g. Crystal, brightnessCutoff 16 kHz) could push
    // the cutoff to/above Nyquist, making tan(π·cutoff/fs) go negative/∞ → an unstable/NaN one-pole
    // sitting inside the waveguide feedback loop. jmin() with the Hz ceiling keeps 44.1/48 kHz
    // behavior unchanged.
    const float nyquistCeil = 0.45f * static_cast<float>(currentSampleRate);

    cutoffs.bridgeCutoffHz = materialBrightness * (0.5f + brightnessAmount * 0.8f) * tensionBrightnessModifier * bridgeBrightnessModifier;
    cutoffs.bridgeCutoffHz = juce::jlimit(300.0f, juce::jmin(20000.0f, nyquistCeil), cutoffs.bridgeCutoffHz);

    // Nut Filter: Higher cutoff (harder boundary than bridge)
    cutoffs.nutCutoffHz = materialBrightness * 1.2f * (0.7f + brightnessAmount * 0.5f) * tensionBrightnessModifier;
    cutoffs.nutCutoffHz = juce::jlimit(1000.0f, juce::jmin(20000.0f, nyquistCeil), cutoffs.nutCutoffHz);

    // Loop Damping Filter: Material + gauge modifier
    float gaugeDampingModifier = 0.5f + gaugeAmount * 1.5f;
    float effectiveDamping = dampingAmount * gaugeDampingModifier;
    float clampedDamping = juce::jlimit(0.0f, 1.0f, effectiveDamping);
    cutoffs.dampingCutoffHz = 200.0f + (1.0f - clampedDamping) * 14000.0f;
    cutoffs.dampingCutoffHz = juce::jlimit(200.0f, juce::jmin(14000.0f, nyquistCeil), cutoffs.dampingCutoffHz);

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
    // v2.1.8: Apply filter coefficient updates on the audio thread with a
    // crossfade to eliminate clicks on brightness/tension/material sweeps.
    //
    // Before overwriting the live coefficients we snapshot the current filter
    // (coefs + state) into a shadow copy via plain-value assignment — OnePoleLPF
    // is a trivially-copyable POD so this is allocation-free and RT-safe. The
    // shadow then continues running with the OLD response while the live filter
    // switches to the NEW response; processSample() blends their outputs over
    // FILTER_CROSSFADE_LENGTH samples (~64 samples ≈ 1.5 ms @ 44.1 kHz).
    //
    // If another update lands while a crossfade is still in flight, the shadow
    // is replaced by the current (mid-transition) state and the counter resets,
    // producing a smooth cascaded transition instead of compounding clicks.
    //
    // v1.7.10 thread-safety note preserved: this is the only place filter state
    // is mutated, and it's only called from processSample() on the audio thread.
    if (filterUpdatePending.load(std::memory_order_acquire))
    {
        // Snapshot current live filters into shadows (holds pre-update behavior)
        bridgeFilterShadow = bridgeFilter;
        nutFilterShadow = nutFilter;
        loopDampingShadow = loopDamping;

        // Install new cutoffs on live filters — allocation-free in-place write
        bridgeFilter.setCutoff(pendingBridgeCutoff.load(std::memory_order_relaxed),
                               currentSampleRate);
        nutFilter.setCutoff(pendingNutCutoff.load(std::memory_order_relaxed),
                            currentSampleRate);
        loopDamping.setCutoff(pendingDampingCutoff.load(std::memory_order_relaxed),
                              currentSampleRate);

        // Start (or restart) the crossfade
        filterCrossfadeRemaining = FILTER_CROSSFADE_LENGTH;

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

    // WR-03: floor the rail delay. Group-delay compensation subtracts an unbounded amount; for high
    // notes on a dark patch the sum of bridge/nut/damping group delays can exceed the string period,
    // driving compensatedDelay negative → DelayLine clamps to 0 → the loop short-circuits and the
    // string cannot resonate (dead/detuned top octave). ≥ 2 keeps 3rd-order Lagrange interpolation valid.
    // Each rail is half the total length.
    return juce::jmax(2.0f, compensatedDelay * 0.5f);
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
