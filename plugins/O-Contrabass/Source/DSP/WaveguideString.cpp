/*
  ==============================================================================

    WaveguideString.cpp
    O-Contrabass - Split-Rail Digital Waveguide for Bass-Range Bowed String
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1a-recovery rev-3 — applies F1 (split-rail), F2 (bridge LP
    recurrence fix; DC gain = g exactly), F3 (no in-loop DC blocker) as
    a single coupled change. See PLAN rev-3 + RESEARCH §11.

  ==============================================================================
*/

#include "WaveguideString.h"
#include "HyperbolicFriction.h"
#include <cmath>

void WaveguideString::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;

    // Internal-rate ProcessSpec (caller passes sr already at 2× oversampled rate
    // and a doubled maxBlockSize, per BowedContrabassVoice::prepareToPlay).
    juce::dsp::ProcessSpec spec {
        sr,
        static_cast<juce::uint32> (maxBlockSize),
        1
    };

    bridgeDelay.prepare (spec);
    bridgeDelay.setMaximumDelayInSamples (8192);
    neckDelay.prepare (spec);
    neckDelay.setMaximumDelayInSamples (8192);

    // 20 ms linear ramp on STRING_STIFFNESS (RESEARCH §5 pitfall #6).
    stiffnessSmoothed.reset (sr, 0.020);
    stiffnessSmoothed.setCurrentAndTargetValue (cachedStringStiffness);

    filterDirty = true;
    reset();
}

void WaveguideString::trigger (float frequency)
{
    currentFrequency = frequency;

    // Clean start: clear delay lines + filter state to avoid artifacts from previous note.
    reset();

    updateDelayLengths();
    updateBridgeFilterCoeffs();
}

void WaveguideString::reset()
{
    bridgeDelay.reset();
    neckDelay.reset();
    bridgeY = 0.0f;
    energyEstimate = 0.0f;
}

void WaveguideString::updateDelayLengths()
{
    // Split-rail topology — total round-trip = sr / f0 minus bridge LP group delay,
    // partitioned at the bow contact point β.
    float totalDelay = static_cast<float> (sampleRate) / std::max (1.0f, currentFrequency);

    float pi = juce::MathConstants<float>::pi;
    float filterGroupDelay = static_cast<float> (sampleRate) / (2.0f * pi * std::max (1.0f, brightnessHz));
    float compensated = totalDelay - filterGroupDelay;

    float bridgeSamples = compensated * bowPosition;
    float neckSamples   = compensated * (1.0f - bowPosition);

    // Lagrange3rd needs ≥4 samples per rail (4-tap kernel).
    bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
    neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);

    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay   (neckSamples);
}

void WaveguideString::setDelaySamples (float totalSamples)
{
    // Per-sample total-delay setter (vibrato/detune ramps in 2.2/2.3). Splits
    // the total delay across the two rails using the current bowPosition.
    float bridgeSamples = totalSamples * bowPosition;
    float neckSamples   = totalSamples * (1.0f - bowPosition);
    bridgeSamples = juce::jlimit (4.0f, 8190.0f, bridgeSamples);
    neckSamples   = juce::jlimit (4.0f, 8190.0f, neckSamples);
    bridgeDelay.setDelay (bridgeSamples);
    neckDelay.setDelay   (neckSamples);
}

float WaveguideString::computeLoopGain (float infSustainParam01) noexcept
{
    // Quadratic skew per RESEARCH §Q5 — most perceptual range concentrated
    // in upper half of knob; hard-clamped to architectural ceiling.
    constexpr float kFloor   = 0.997f;
    constexpr float kSpan    = 0.00295f;        // 0.99995 − 0.997 at param=1
    constexpr float kCeiling = 0.9999999f;
    const float g = kFloor + kSpan * infSustainParam01 * infSustainParam01;
    return std::min (g, kCeiling);
}

void WaveguideString::updateBridgeFilterCoeffs()
{
    // g — loop gain, quadratic skew + ceiling clamp.
    bridgeG = computeLoopGain (infiniteSustain);

    // p — HF damping pole, clamped [0.05, 0.95] to ensure non-zero damping
    // in all states (ARCHITECTURE §"Bridge Filter").
    float pi = juce::MathConstants<float>::pi;
    float p = std::exp (-2.0f * pi * brightnessHz / static_cast<float> (sampleRate));
    bridgeP = juce::jlimit (0.05f, 0.95f, p);
    bridgeOneMinusP = 1.0f - bridgeP;

    // Drone-mode boundary uses the user-knob value (RESEARCH §Q5):
    // outside drone (INFINITE_SUSTAIN < 0.95), inject a constant −1e-20 into y
    // to keep the recursion above the subnormal range. In drone mode the leak
    // would slowly bleed energy away from a true sustain, so we disable it.
    denormalLeak = (infiniteSustain >= 0.95f) ? 0.0f : -1.0e-20f;

    filterDirty = false;
}

float WaveguideString::processSample (float v_bow, float F_bow,
                                       const HyperbolicFriction& friction)
{
    // Apply pending coefficient update (audio-thread only path).
    if (filterDirty)
        updateBridgeFilterCoeffs();

    // Step 1: Read both rails.
    float bridgeRaw = bridgeDelay.popSample (0);
    float neckRaw   = neckDelay.popSample   (0);

    // Step 2: Bridge LP — F2 fixed form: y = g·(1−p)·x + p·y_prev + leak.
    // (DROP the `g` from the feedback term so DC gain == g exactly. See
    //  RESEARCH §11.1 B2; mirrors O-Bowed WaveguideString.cpp:94-95
    //  coefficient form (b0, b1, a0, a1) = (g·(1−p), 0, 1, −p).)
    if (! std::isfinite (bridgeY))
        bridgeY = 0.0f;
    float bridgeFiltered = bridgeG * bridgeOneMinusP * bridgeRaw
                         + bridgeP * bridgeY
                         + denormalLeak;
    bridgeY = bridgeFiltered;

    // Step 3: Boundary reflections.
    //   Bridge: −1 boundary AFTER LP (rigid bridge; matches O-Bowed line 108).
    //   Nut:    −1 boundary, no LP (rigid nut).
    float bridgeReflection = -bridgeFiltered;
    float nutReflection    = -neckRaw;

    // Step 4: Sum at bow point (split-rail v_string_incoming).
    float v_string_incoming = bridgeReflection + nutReflection;
    float v_delta = v_bow - v_string_incoming;

    // Step 5: Friction (unchanged from rev-1 single-rail).
    float rho = friction.computeReflectionCoefficient (v_delta, F_bow);
    float clampedRho = std::min (rho, 0.85f);
    float frictionVelocity = 2.0f * clampedRho / (1.0f - clampedRho);
    float absVd = std::abs (v_delta);
    float injection = std::min (frictionVelocity, absVd);
    float newVelocity = (v_delta >= 0.0f) ? injection : -injection;

    // Step 6: Symmetric injection into both rails (canonical Smith two-port).
    // [Phase 2.1c placeholder] dispersion will run on the BRIDGE rail's
    //  outgoing wave only, BEFORE the algebraic saturator below.
    float toBridge = nutReflection    + newVelocity;
    float toNeck   = bridgeReflection + newVelocity;

    // Step 7: In-loop algebraic saturator on each rail (RESEARCH §1.3).
    toBridge = toBridge / std::sqrt (1.0f + toBridge * toBridge);
    toNeck   = toNeck   / std::sqrt (1.0f + toNeck   * toNeck);

    // Step 8: F3 — NO in-loop DC blocker. (If long-form drone surfaces DC
    //  drift in Phase 2.4/2.5, an output-path DCB at
    //  BowedContrabassVoice::renderNextBlock post-processSamplesDown is
    //  the correct placement; it does not interfere with bootstrapping.)

    bridgeDelay.pushSample (0, toBridge);
    neckDelay.pushSample   (0, toNeck);

    // Step 9: Output from bridge end (matches O-Bowed `output = toBridge`).
    float output = toBridge;
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs (output);
    return output;
}

void WaveguideString::setBowPosition (float beta)
{
    // Split-rail β: clamp [0.02, 0.98] (no-bow at ends would zero a delay rail).
    float clamped = juce::jlimit (0.02f, 0.98f, beta);
    if (std::abs (bowPosition - clamped) < 1e-6f)
        return;
    bowPosition = clamped;
    updateDelayLengths();
}

void WaveguideString::setBrightness (float cutoffHz)
{
    if (std::abs (brightnessHz - cutoffHz) < 0.5f)
        return;

    brightnessHz = cutoffHz;
    filterDirty = true;
    // Brightness affects bridge filter group delay → recompensate base delay.
    updateDelayLengths();
}

void WaveguideString::setInfiniteSustain (float amount)
{
    if (std::abs (infiniteSustain - amount) < 1e-6f)
        return;

    infiniteSustain = amount;
    filterDirty = true;
}

void WaveguideString::setStringStiffness (float amount)
{
    // Drives the 20 ms smoother. Phase 2.1a only caches the value — dispersion
    // is wired in 2.1c, at which point the voice's renderNextBlock will
    // advance the smoother by numSamples per block before reading the
    // current value to recompute the allpass coefficient `a`.
    cachedStringStiffness = juce::jlimit (0.0f, 1.0f, amount);
    stiffnessSmoothed.setTargetValue (cachedStringStiffness);
}

bool WaveguideString::isActive() const noexcept
{
    return energyEstimate > 1e-7f;
}
