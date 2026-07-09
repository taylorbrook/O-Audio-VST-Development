/*
  ==============================================================================

    WaveguideString.h
    O-Contrabass - Split-Rail Digital Waveguide for Bass-Range Bowed String
    Ouaricon Audio
    Developer: Taylor Brook

    Bass-adapted reimplementation. Phase 2.1a-recovery (rev-3) structural
    choices, locked by ARCHITECTURE.md + RESEARCH.md §11:

    - Split-rail topology (canonical Smith two-port scattering): a
      bridgeDelay + neckDelay pair, each a juce::dsp::DelayLine<float,
      Lagrange3rd> at 8192-sample capacity (covers E1 −1200 cents at
      88.2 kHz internal SR). The bow point sits between the two rails;
      v_string_incoming is the sum of the two boundary reflections.
    - Bridge LP one-pole, F2-corrected recurrence:
          y[n] = g·(1−p)·x[n] + p·y[n−1] + leak
      DC gain = g exactly (RESEARCH §11.1 B2 fix; do NOT multiply g into
      the feedback term). g is clamped to [0.997, 0.9999999] via quadratic
      skew g = 0.997 + 0.00295 · INFINITE_SUSTAIN².
    - Bridge LP HF damping pole p clamped to [0.05, 0.95].
    - In-loop algebraic saturator: x / sqrt(1 + x²), applied per rail on
      the WRITE path (RESEARCH §1.3 — NOT tanh).
    - F3 deviation from ARCHITECTURE.md §"DC Blocker": NO in-loop DC
      blocker. Once F2 fixes the bridge LP DC gain to g, the LP attenuates
      DC by (1−g) per round trip and an in-loop DCB is redundant AND
      harmful (transient response zeros cold-start sticking-regime
      injection — see RESEARCH §11.1 B3, §11.6). If long-form drone
      surfaces DC drift in Phase 2.4, an output-path DCB at
      BowedContrabassVoice::renderNextBlock (post processSamplesDown) is
      the correct placement; it does not interfere with bootstrapping.
    - Constant −1.0e-20f denormal leak added to the bridge LP y ONLY when
      INFINITE_SUSTAIN < 0.95 (drone-mode parameter check, not hysteresis).
    - Defensive recovery: if (!std::isfinite(state)) state = 0.0f at the
      bridge LP entry (RESEARCH §5 pitfall #9).
    - Per-sample bridge-rail loop order (HARD CONTRACT):
          dispersion → bridge LP → −1 boundary → sum at bow
          → friction injection → in-loop saturator (per rail) → delay write.
      Phase 2.1c: dispersion runs on the bridge rail between popSample and
      bridge LP. Mirrors O-Bowed bridge-rail-only chain. Per
      ARCHITECTURE.md §"Cascaded Allpass Dispersion" + §"Processing Order"
      and RESEARCH §14.4.
    - Sign convention (HARD CONTRACT, RESEARCH §11.4):
          bridgeReflection = -bridgeFiltered  (LP, then −1 boundary)
          nutReflection    = -neckRaw         (rigid nut, no LP)
          v_string_incoming = bridgeReflection + nutReflection
          toBridge = nutReflection    + newVelocity
          toNeck   = bridgeReflection + newVelocity
          output   = toBridge (post per-rail saturator)
    - 20 ms SmoothedValue<Linear> on STRING_STIFFNESS; stiffness cached per
      block. Dispersion wiring deferred to Phase 2.1c (bridge rail only).

  ==============================================================================
*/

#pragma once
#include <juce_dsp/juce_dsp.h>
#include "DispersionFilter.h"

class HyperbolicFriction;

class WaveguideString
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void trigger (float frequency);       // set delay length for new note
    void reset();                         // clear delay lines + filter state

    // Core per-sample processing: takes bow signals + friction model, returns output sample.
    // Internally runs split-rail scattering + bridge LP + per-rail in-loop saturator.
    float processSample (float v_bow, float F_bow,
                         const HyperbolicFriction& friction);

    bool isActive() const noexcept;
    float getEnergyEstimate() const noexcept { return energyEstimate; }

    // Parameter setters (called once per block from voice)
    void setBowPosition (float beta);          // 0.02 - 0.98 clamped (split-rail β)
    void setBrightness (float cutoffHz);       // 80 - 12000
    void setInfiniteSustain (float amount);    // 0.0 - 1.0
    void setStringStiffness (float amount);    // 0.0 - 1.0 (drives smoother; dispersion lands in 2.1c)

    // Phase 2.1c — dispersion plumbing. Voice computes `a` from (f0, B, M)
    // once per block and pushes it here; voice also drives the smoother by
    // calling `advanceStiffnessSmootherBy(numSamples)` then reading the
    // current value via `getCurrentSmoothedStiffness()`.
    void setDispersionCoefficient (float a) noexcept;
    void advanceStiffnessSmootherBy (int numSamples) noexcept;
    float getCurrentSmoothedStiffness() const noexcept;

    /** Configures the per-instance dispersion section count. Phase 2.2: voice calls this
     *  once per slot at prepareToPlay (E=4, A=3, D=2, G=1 per ARCH §"String Waveguide
     *  Bank"). Pass-through to bridgeDispersion.setActiveSections.
     */
    void setDispersionActiveSections (int M) noexcept;

    // Per-sample total-delay setter — Lagrange3rd is stateless, so this is
    // safe to call mid-stream. Internally splits via bowPosition and updates
    // both rails (used by vibrato/detune ramps in Phase 2.2/2.3).
    void setDelaySamples (float totalSamples);

private:
    void updateDelayLengths();
    void updateBridgeFilterCoeffs();
    static float computeLoopGain (float infSustainParam01) noexcept;

    // WR-07: bridge-LP group-delay compensation, bounded to the filter's realizable
    // range. Shared by updateDelayLengths() and setDelaySamples() so both stay in sync.
    float bridgeGroupDelaySamples() const noexcept;

    double sampleRate = 88200.0;          // internal (oversampled) rate
    float currentFrequency = 41.2f;        // E1 default
    float bowPosition = 0.10f;             // β, clamped [0.02, 0.98]
    float brightnessHz = 4500.0f;
    float infiniteSustain = 0.0f;

    // Phase 2.1c bridge-rail dispersion. M=4 hardcoded for E1 (Phase 2.2 will
    // extend per-string M=4/3/2/1 via DispersionFilter::setActiveSections).
    // Placed BEFORE the stiffness smoother so the init-order in prepare() is
    // dispersion → smoother → bridge filter coeffs.
    DispersionFilter<4> bridgeDispersion;

    // Stiffness smoothing (20 ms linear). Dispersion 'a' coefficient is
    // computed from this in Phase 2.1c via voice-side per-block path:
    //  voice → advanceStiffnessSmootherBy(N) → getCurrentSmoothedStiffness()
    //        → DispersionFilter::computeAllpassCoefficient(f0, B, M)
    //        → setDispersionCoefficient(a).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> stiffnessSmoothed;
    float cachedStringStiffness = 0.30f;

    // Split-rail delay lines at 8192 samples each (covers E1 −1200 cents @ 88.2 kHz).
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> bridgeDelay { 8192 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> neckDelay   { 8192 };

    // Bridge LP state — explicit one-pole inlined recurrence (cheaper +
    // clearer than juce::dsp::IIR::Filter for the F2-corrected form).
    float bridgeY = 0.0f;       // bridge LP state y[n-1]

    // Cached per-block coefficients (recomputed on parameter change)
    float bridgeG = 0.997f;     // loop gain (quadratic skew of INFINITE_SUSTAIN)
    float bridgeP = 0.5f;       // HF damping pole (clamped [0.05, 0.95])
    float bridgeOneMinusP = 0.5f;
    float denormalLeak = -1.0e-20f;  // 0 in drone mode (INFINITE_SUSTAIN >= 0.95)

    // Energy tracking for voice cleanup
    float energyEstimate = 0.0f;

    bool filterDirty = true;
};
