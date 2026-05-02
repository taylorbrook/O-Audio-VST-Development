/*
  ==============================================================================

    BowedContrabassVoice.cpp
    O-Contrabass - 4-String Bank MPESynthesiserVoice (Phase 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "BowedContrabassVoice.h"
#include "DSP/DispersionFilter.h"
#include "DSP/SchellengCalibration.h"
#include "DSP/SubHarmonicBias.h"
#include "TuningEngine.h"
#include <cmath>

namespace schelleng     = ouaricon::contrabass::schelleng;
namespace sub_harmonics = ouaricon::contrabass::sub_harmonics;

// Phase 2.4a HR-7 — harness-side wedge-math bypass for --matrix-stability mode.
// Production builds: weak default returns false (defined in PluginProcessor.cpp).
// Harness binary:    strong override in main.cpp returns g_matrixStabilityMode.
extern "C" bool isMatrixStabilityModeActive() noexcept;

namespace
{
    // Phase 2.2 locked constants (PLAN rev-6 preamble + RESEARCH §15.5/§15.6).
    constexpr float kOpenStringFrequencyHz[4] = { 41.20f, 55.00f, 73.42f, 98.00f };
    constexpr float kBOpen[4]                 = { 1.0e-4f, 7.0e-5f, 5.0e-5f, 3.0e-5f };
    constexpr int   kMPerString[4]            = { 4, 3, 2, 1 };
    constexpr const char* kDetuneParamIds[4]  = { "DETUNE_E", "DETUNE_A", "DETUNE_D", "DETUNE_G" };

    // Phase 2.3 locked constants (PLAN rev-7 preamble + RESEARCH §16).
    // Phase 2.4a R34d removed kSchellengZ / kSchellengR / kSchellengDMu — closed-form
    // wedge math replaced by schelleng::safeDepthForString trilinear lookup.
    constexpr float kVibratoRampSec      = 0.3f;                       // architecture line 125
    constexpr float kVibratoFadeOutSec   = 0.150f;                     // architecture line 127
    constexpr float kPressureLagRad      = 0.4014f;                    // 23° in radians
    constexpr float kAntiCorrPerDepth    = 0.13f;                      // Q5 anti-correlation guard
    constexpr float kVibFactorScale      = -0.69314718056f / 1200.0f;  // -ln(2)/1200
}

BowedContrabassVoice::BowedContrabassVoice (juce::AudioProcessorValueTreeState* apvts,
                                            TuningEngine* engine)
    : parameters (apvts), tuningEngine (engine)
{
}

//==============================================================================
// MPE Voice Callbacks
//==============================================================================

void BowedContrabassVoice::noteStarted()
{
    auto note = getCurrentlyPlayingNote();
    const int   midiNote = note.initialNote;
    const float velocity = note.noteOnVelocity.asUnsignedFloat();

    // 1. Resolve frequency through TuningEngine (Phase 2.6b R40b Site A).
    //    ESCALATION-RP1 LOCK Option B — voice-side ratio multiplication on top
    //    of the engine's absolute-Hz return. Engine's internal A4 stays at
    //    440 Hz (we never call setMasterTune); the REFERENCE_PITCH APVTS slider
    //    scales the result here, honoring the Stage-1 220–880 Hz contract
    //    without touching the module clamp (400–480 Hz). At REFERENCE_PITCH
    //    = 440 Hz the ratio is 1.0 → bit-equivalent to the previous
    //    getMidiNoteInHertz path (Gate 8b inv #1 algebraic identity at 12-TET
    //    default; PLAN §23.6.6 proof). Q17 LOCK: cache the post-ratio base
    //    frequency so notePitchbendChanged re-uses it without re-polling
    //    TuningEngine mid-sustain. nullptr fallback is defensive only —
    //    production path always passes a valid pointer (R40a addVoice site).
    const double tuneFreqHz = (tuningEngine != nullptr)
                                ? tuningEngine->getFrequency (midiNote)
                                : juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const float  refPitchHz = parameters->getRawParameterValue ("REFERENCE_PITCH")->load();
    tuningEngineBaseFreqHz  = tuneFreqHz * (static_cast<double> (refPitchHz) / 440.0);

    const float bend = static_cast<float> (note.totalPitchbendInSemitones);
    double freq = tuningEngineBaseFreqHz;
    if (std::abs (bend) > 0.001f)
        freq *= std::pow (2.0, bend / 12.0);
    currentFrequency = static_cast<float> (freq);

    // 2. Resolve target string (closed-form ladder + ACTIVE_STRINGS clamp).
    const int activeStrings = static_cast<int> (parameters->getRawParameterValue ("ACTIVE_STRINGS")->load());
    const int newStringIndex = mapMidiNoteToStringIndex (midiNote, activeStrings);

    // 3. Decide trigger semantics.
    const bool isFirstNote    = (activeStringIndex < 0);
    const bool needsCrossfade = (! isFirstNote)
                              && bowModel.isActive()
                              && (newStringIndex != activeStringIndex);

    if (needsCrossfade)
    {
        previousStringIndex       = activeStringIndex;
        activeStringIndex         = newStringIndex;
        crossfadeRemainingSamples = crossfadeTotalSamples;
    }
    else
    {
        previousStringIndex       = -1;
        activeStringIndex         = newStringIndex;
        crossfadeRemainingSamples = 0;
    }

    // 4. Configure new string's delay-line immediately (avoid lag at old freq).
    const float detuneCents   = readDetuneForString (newStringIndex);
    const float targetSamples = computeDelaySamples (currentFrequency, detuneCents);
    detuneSmoothed[newStringIndex].setCurrentAndTargetValue (targetSamples);
    strings[newStringIndex].trigger (currentFrequency);
    strings[newStringIndex].setDelaySamples (targetSamples);

    // 5. Engage bow.
    bowModel.startBow (velocity);
    oversampling.reset();

    // Phase 2.3 — re-arm vibrato S-curve onset; exit any prior fade-out (Q3).
    // vibratoPhase NOT reset (sine-phase-carry contract); only the per-note
    // onset envelope re-arms.
    vibratoOnsetTimerSeconds   = 0.0f;
    noteOffFadeOutTimerSeconds = -1.0f;
}

void BowedContrabassVoice::noteStopped (bool allowTailOff)
{
    if (allowTailOff)
    {
        // Bow lifts; release ramp begins, voice stays active until energy decays.
        bowModel.stopBow();

        // Phase 2.3 — start vibrato 150 ms linear fade-out. The
        // `vibratoOnsetGateAtNoteOff` snapshot is captured one-shot in the
        // per-sample loop the first time it sees the timer at 0.0f (pin #8).
        noteOffFadeOutTimerSeconds = 0.0f;
    }
    else
    {
        // Hard stop — immediate silence.
        clearCurrentNote();
        for (auto& s : strings)
            s.reset();
        bowModel.reset();
        oversampling.reset();
        previousStringIndex       = -1;
        crossfadeRemainingSamples = 0;

        // Phase 2.3 — reset modulator state on hard stop.
        vibratoOnsetTimerSeconds   = 0.0f;
        vibratoOnsetGateAtNoteOff  = 0.0f;
        noteOffFadeOutTimerSeconds = -1.0f;
        lastSafeDepth.store (0.0f, std::memory_order_relaxed);

        // Phase 2.5 — reset body resonator + bow noise generator state on hard
        // stop (filter state continues across blocks during sustain; only
        // resets on voice-stop per ARCHITECTURE §"Body Resonator" "Filter
        // state continues across blocks (no per-block reset)" intent).
        bodyResonator.reset();
        bowNoiseGenerator.reset();
        lastFundamentalHz = 0.0f;
    }
}

void BowedContrabassVoice::notePitchbendChanged()
{
    auto note = getCurrentlyPlayingNote();

    // Phase 2.6b R40b Site B — Q17 LOCK: re-use cached tuningEngineBaseFreqHz
    // from noteStarted; do NOT re-poll TuningEngine mid-sustain. Live
    // retuning takes effect at the NEXT note-on. Bend is multiplicative
    // pow2(bend/12) on the cached value — ESCALATION-MPE1 ±24 semi range
    // tracks click-free via the existing 20 ms detuneSmoothed delay-line
    // ramp; no new smoother on currentFrequency (PLAN §23.5.5 verification).
    const float bend = static_cast<float> (note.totalPitchbendInSemitones);
    double freq = tuningEngineBaseFreqHz;
    if (std::abs (bend) > 0.001f)
        freq *= std::pow (2.0, bend / 12.0);
    currentFrequency = static_cast<float> (freq);

    if (activeStringIndex >= 0)
    {
        const float detuneCents   = readDetuneForString (activeStringIndex);
        const float targetSamples = computeDelaySamples (currentFrequency, detuneCents);
        detuneSmoothed[activeStringIndex].setTargetValue (targetSamples);
    }
}

void BowedContrabassVoice::notePressureChanged()
{
    // Pressure modulation will be wired in Phase 2.6 (Note Expression / MPE-Z).
}

void BowedContrabassVoice::noteTimbreChanged()
{
    // Timbre modulation (CC74 / MPE-Y) will be wired in Phase 2.6.
}

void BowedContrabassVoice::noteKeyStateChanged()
{
    // No special handling needed for sustain/sostenuto in Phase 2.2.
}

//==============================================================================
// Lifecycle
//==============================================================================

void BowedContrabassVoice::prepareToPlay (double hostSampleRate, int maxBlockSize)
{
    currentMaxBlockSize = maxBlockSize;
    sr_internal         = hostSampleRate * 2.0;

    // 2× oversampler — initProcessing sized to maxBlockSize (host-rate block size).
    oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
    oversampling.reset();

    // Mono scratch: maxBlockSize × 2 covers the upsampled buffer footprint.
    voiceBuffer.setSize (1, maxBlockSize * 2, /*keepExistingContent*/ false,
                         /*clearExtraSpace*/ true, /*avoidReallocating*/ false);
    voiceBuffer.clear();

    // Phase 2.2 — per-slot prepare. Each string gets its own M (E=4, A=3, D=2, G=1)
    // configured via R22's setDispersionActiveSections pass-through. Slot 0 (E):
    // prepare()'s internal setActiveSections(4) is followed by R22 setter to 4
    // (no-op re-set) — slot-0 final state matches Phase 2.1c bit-exactly.
    for (int s = 0; s < 4; ++s)
    {
        strings[s].prepare (sr_internal, maxBlockSize * 2);
        strings[s].setDispersionActiveSections (kMPerString[s]);
        detuneSmoothed[s].reset (sr_internal, 0.020);
        detuneSmoothed[s].setCurrentAndTargetValue (
            static_cast<float> (sr_internal) / kOpenStringFrequencyHz[s]);
    }

    bowModel.prepare (sr_internal);

    // Phase 2.1b — bass-string friction defaults (module init = treble defaults).
    frictionModel.setStaticFrictionCoefficient  (0.85f);
    frictionModel.setDynamicFrictionCoefficient (0.25f);

    // Crossfade ramp precompute (RESEARCH §15.3 — 5 ms equal-power, two array
    // loads/sample, ~3.5 KiB at 88.2 k internal SR).
    crossfadeTotalSamples = static_cast<int> (std::ceil (0.005 * sr_internal));
    crossfadeRamp.resize (static_cast<size_t> (crossfadeTotalSamples + 1));
    const float invN   = 1.0f / static_cast<float> (crossfadeTotalSamples);
    const float halfPi = juce::MathConstants<float>::halfPi;
    for (int i = 0; i <= crossfadeTotalSamples; ++i)
    {
        const float t = static_cast<float> (i) * invN;
        crossfadeRamp[static_cast<size_t> (i)] = { std::cos (t * halfPi), std::sin (t * halfPi) };
    }

    activeStringIndex         = -1;
    previousStringIndex       = -1;
    crossfadeRemainingSamples = 0;

    // ─── Phase 2.3 — modulator + macro state + smoothers ──────────────────────
    // pin #4 — fresh notes get full onset envelope (per-note semantics; first
    // note after prepareToPlay sees the configured VIBRATO_ONSET delay).
    vibratoPhase                  = 0.0f;
    vibratoOnsetTimerSeconds      = 0.0f;
    vibratoOnsetGateAtNoteOff     = 0.0f;
    noteOffFadeOutTimerSeconds    = -1.0f;          // sentinel: not in fade
    slowLfoPhase                  = 0.0f;

    // pin #9 — three SmoothedValues init at currentValue == targetValue == 0.0
    // so getNextValue() returns 0.0f exactly until any non-zero target lands.
    // This is the HR-3 IEEE 754 identity-arithmetic invariant: at modulators-
    // off, x + 0.0 == x and x * 1.0 == x bit-exactly for all finite x.
    macroSmoothed.reset (sr_internal, 0.020);
    macroSmoothed.setCurrentAndTargetValue (0.0f);
    slowLfoSpeedSmoothed.reset (sr_internal, 0.020);
    slowLfoSpeedSmoothed.setCurrentAndTargetValue (0.0f);
    slowLfoPressureSmoothed.reset (sr_internal, 0.020);
    slowLfoPressureSmoothed.setCurrentAndTargetValue (0.0f);

    lastSafeDepth.store (0.0f, std::memory_order_relaxed);

    // ─── Phase 2.4b — sub-harmonic bias state (HR-9 strict-default) ───────────
    // 30 ms ramp time per CONTEXT rev-7 Q30 + architecture §457 implementation
    // note. setCurrentAndTargetValue(0.0f) is the HR-9 strict-default precondition:
    // at APVTS default 0.0, getNextValue() returns exact 0.0f → caller-side
    // short-circuit fires → 10 carry-forward goldens reproduce byte-identical.
    subHarmonicsSmoothed.reset (sr_internal, 0.030);
    subHarmonicsSmoothed.setCurrentAndTargetValue (0.0f);
    voiceBowForceUpliftThisBlock = 1.0f;             // HR-9 reset
    lastSubAmount.store (0.0f, std::memory_order_relaxed);

    // ─── Phase 2.5 — Body resonator + bow noise generator ─────────────────────
    // Body runs at host rate (post 2× downsample); noise generator likewise
    // operates at host rate. 30 ms ramps (CONTEXT line 152 + ARCHITECTURE §152).
    // Defaults match parameter-spec.md (SIZE=0.75 / DAMPING=0.40 / MIX=0.80 /
    // NOISE=0.35).
    bodyResonator.prepare (hostSampleRate, maxBlockSize);
    bowNoiseGenerator.prepare (hostSampleRate, /*voiceIndex=*/ 0); // monophonic (RESEARCH §21.6)

    bodySizeSmoothed.reset    (hostSampleRate, 0.030);
    bodyDampingSmoothed.reset (hostSampleRate, 0.030);
    bodyMixSmoothed.reset     (hostSampleRate, 0.030);
    bowNoiseSmoothed.reset    (hostSampleRate, 0.030);
    bodySizeSmoothed.setCurrentAndTargetValue    (0.75f);
    bodyDampingSmoothed.setCurrentAndTargetValue (0.40f);
    bodyMixSmoothed.setCurrentAndTargetValue     (0.80f);
    bowNoiseSmoothed.setCurrentAndTargetValue    (0.35f);

    lastFundamentalHz = 0.0f;
}

//==============================================================================
// Render
//==============================================================================

void BowedContrabassVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                             int startSample, int numSamples)
{
    // Defensive double-cover (also lives at processor processBlock entry).
    juce::ScopedNoDenormals noDenormals;

    if (numSamples <= 0)
        return;

    // 1. Read APVTS atomics into block-cached locals.
    updateParametersFromAPVTS();

    // Voice cleanup — bow released and all 4 strings decayed: clear the slot.
    if (! bowModel.isActive())
    {
        bool anyActive = false;
        for (auto& s : strings)
            if (s.isActive()) { anyActive = true; break; }

        if (! anyActive)
        {
            clearCurrentNote();
            return;
        }
    }

    // Phase 2.4b HR-9 reset — the F_bow uplift factor MUST be 1.0f at the
    // start of every block so the HR-9 short-circuit path leaves Step 6's
    // setBowPressure call bit-exact identical to Phase 2.4a.
    voiceBowForceUpliftThisBlock = 1.0f;

    // ─── Phase 2.3 — 7-Step Per-Block Evaluation Order (RESEARCH §16.6) ──────
    //
    // Step 1 — read raw APVTS atomics for the modulator + macro layer. The
    // existing updateParametersFromAPVTS() already cached MPE position and
    // configured frictionModel/strings; the 10 reads below feed the new
    // modulator math without interfering with the existing cached state.
    const float rawVibratoDepth   = parameters->getRawParameterValue ("VIBRATO_DEPTH")->load();
    const float rawVibratoRate    = parameters->getRawParameterValue ("VIBRATO_RATE")->load();
    const float rawVibratoOnsetMs = parameters->getRawParameterValue ("VIBRATO_ONSET")->load();
    const float rawSlowLfoRate    = parameters->getRawParameterValue ("SLOW_LFO_RATE")->load();
    const float rawSlowLfoDepth   = parameters->getRawParameterValue ("SLOW_LFO_DEPTH")->load();
    const float rawMacro          = parameters->getRawParameterValue ("EXPRESSION_MACRO")->load();
    const float rawBowSpeed       = parameters->getRawParameterValue ("BOW_SPEED")->load();
    const float rawBowPressure    = parameters->getRawParameterValue ("BOW_PRESSURE")->load();
    const float rawBowPos         = parameters->getRawParameterValue ("BOW_POSITION")->load();
    const float rawBrightness     = parameters->getRawParameterValue ("BRIGHTNESS")->load();
    const float rawRosin          = parameters->getRawParameterValue ("ROSIN")->load();
    const float rawSubHarmonics   = parameters->getRawParameterValue ("SUB_HARMONICS")->load();

    // Step 2 — Schelleng wedge fMin/fMax/headroom/safeDepth/vibAntiCorr.
    // pin #4 — write lastSafeDepth UNCONDITIONALLY before the HR-4 gate so the
    // harness read view is well-defined every block.
    lastSafeDepth.store (0.0f, std::memory_order_relaxed);

    float safeDepth   = 0.0f;
    float vibAntiCorr = 0.0f;
    if (rawSlowLfoDepth > 0.0f)                                              // HR-4 gate
    {
        if (isMatrixStabilityModeActive())                                   // HR-7 bypass
        {
            // Phase 2.4a — harness matrix-stability render bypasses wedge math
            // entirely (safeDepth = rawSlowLfoDepth) so per-combo stability is
            // measured at full LFO amplitude. Production builds: weak default
            // returns false; this branch is unreachable.
            safeDepth   = rawSlowLfoDepth;
            vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;
            lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
        }
        else
        {
            // Phase 2.4a — empirical calibration table (RESEARCH §17.3 / §17.10).
            // 27-point grid trilinear interpolation per string; values populated by
            // tools/schelleng-fit/emit_table.py from --matrix-stability render data.
            // Architecture-spec'd pass_breathingAudible (rmsByDecadePeakToPeakPct ≥ 0.20)
            // restored at R34e.
            const float beta = juce::jlimit (0.02f, 0.25f, rawBowPos);
            safeDepth   = juce::jlimit (0.0f, rawSlowLfoDepth,
                                        schelleng::safeDepthForString (activeStringIndex,
                                                                       rawBowSpeed,
                                                                       rawBowPressure,
                                                                       beta));
            vibAntiCorr = kAntiCorrPerDepth * rawSlowLfoDepth;                   // Q5 carry-forward
            lastSafeDepth.store (safeDepth, std::memory_order_relaxed);
        }
    }

    // ─── Phase 2.4b R35d — Step 2.5 — Sub-harmonic bias (HR-9 + HR-10) ───────
    //
    // Friction-junction parameter biasing toward Schelleng F_max regime per
    // ARCHITECTURE §457. HR-9 caller-side short-circuit at subAmount==0.0f or
    // non-active-string preserves bit-exact regression bar for 10 carry-forward
    // goldens. HR-10 friction module ABI preservation: bias mutations pushed
    // back via existing setRosin / setStaticFrictionCoefficient setters.
    //
    // setRosin(rawRosin) RELOCATED here from updateParametersFromAPVTS:
    //   - HR-9 path: setRosin(rawRosin) runs alone → friction state matches
    //     Phase 2.4a → bit-exact preserved.
    //   - Bias path: setRosin(rawRosin) runs first; then else-branch overwrites
    //     with setRosin(rosinEq) via inverse algebraic identity.
    frictionModel.setRosin (rawRosin);

    subHarmonicsSmoothed.setTargetValue (rawSubHarmonics);              // pin #11 UNCONDITIONAL
    const float subAmount = subHarmonicsSmoothed.getNextValue();
    subHarmonicsSmoothed.skip (juce::jmax (0, numSamples - 1));         // pin #7 jmax guard

    lastSubAmount.store (0.0f, std::memory_order_relaxed);              // HR-9 pre-gate (mirrors HR-4)

    if (subAmount != 0.0f && activeStringIndex >= 0 && activeStringIndex < 4)
    {
        const auto noteForBias = getCurrentlyPlayingNote();
        const float mpePressureBlockEntry = noteForBias.pressure.asUnsignedFloat();

        const float v_b_voice = bowModel.getBowVelocity();
        const float beta_v    = juce::jlimit (0.02f, 0.25f, rawBowPos);
        // ROSIN→v_0 forward map (architecture-spec'd; mirrors HyperbolicFriction.h
        // setRosin internals at v1.0).
        float v_0_pre   = 0.1f * std::exp (-4.6f * rawRosin);
        float mu_s_pre  = 0.85f;                                         // bass default
        constexpr float mu_d_const = 0.25f;                              // bass default
        // F_bow_pre = bow force entering the friction junction at block entry,
        // matching Step 6's `effectiveBowPressure * (0.5f + mpePressure * 1.5f)`
        // expression at modulators-off (raw APVTS feeds effectiveBowPressure
        // unchanged at HR-1/HR-2/HR-3 short-circuits).
        const float F_bow_baseline = rawBowPressure
                                   * (0.5f + 1.5f * mpePressureBlockEntry);
        float F_bow_post = F_bow_baseline;

        const float safeDepthSub = schelleng::safeDepthForString (
            activeStringIndex, rawBowSpeed, rawBowPressure, beta_v);

        sub_harmonics::applyBias (subAmount, activeStringIndex,
                                  v_b_voice, beta_v, safeDepthSub,
                                  F_bow_post, v_0_pre, mu_s_pre, mu_d_const);

        // HR-10 push v_0 via ROSIN inverse algebraic identity.
        const float rosinEq = juce::jlimit (
            0.0f, 1.0f,
            -std::log (10.0f * juce::jmax (1.0e-6f, v_0_pre)) / 4.6f);
        frictionModel.setRosin (rosinEq);
        frictionModel.setStaticFrictionCoefficient (mu_s_pre);

        // F_bow uplift factor consumed at Step 6 — RATIO of post-bias to
        // pre-bias F_bow so that Step 6's existing `* (0.5 + p*1.5)` macro
        // expression composes correctly: post = pre × uplift.
        voiceBowForceUpliftThisBlock = F_bow_post
                                     / juce::jmax (1.0e-6f, F_bow_baseline);

        lastSubAmount.store (subAmount, std::memory_order_relaxed);
    }

    // Step 3 — Slow-LFO phase advance + sin (HR-2 gate).
    float slowLfoSpeedMod = 0.0f, slowLfoPressureMod = 0.0f;
    if (rawSlowLfoDepth > 0.0f)                                              // HR-2 gate
    {
        slowLfoPhase += juce::MathConstants<float>::twoPi * rawSlowLfoRate
                      * (static_cast<float> (numSamples) / static_cast<float> (sr_internal));
        if (slowLfoPhase > juce::MathConstants<float>::twoPi)
            slowLfoPhase -= juce::MathConstants<float>::twoPi;

        slowLfoSpeedMod    = safeDepth * std::sin (slowLfoPhase);
        slowLfoPressureMod = safeDepth * std::sin (slowLfoPhase + kPressureLagRad);
    }

    // Step 4 — apply slow-LFO multiplicatively to bow speed/pressure.
    // At HR-2 zero depth, both mods are 0 → IEEE 754 identity-arithmetic.
    const float bowSpeedAfterLfo    = rawBowSpeed    * (1.0f + 0.6f * slowLfoSpeedMod);
    const float bowPressureAfterLfo = rawBowPressure * (1.0f + 0.5f * slowLfoPressureMod);

    // Step 5 — layer EXPRESSION_MACRO multiplicatively (HR-3 IEEE 754 identity).
    // pin #11 — setTargetValue UNCONDITIONAL each block (avoids state-machine
    // complexity; covers 0=0 case via x+0=x exact / x*1=x exact).
    // pin #7 — getNextValue() advances by one sample-tick; skip(jmax(0, n-1))
    // catches up the rest of the block. jmax(0,...) guards numSamples==1
    // (skip(-1) would underflow the smoother counter).
    macroSmoothed.setTargetValue (rawMacro);
    const float macroNow = macroSmoothed.getNextValue();
    macroSmoothed.skip (juce::jmax (0, numSamples - 1));

    const float effectiveBowSpeed        = bowSpeedAfterLfo    * (1.0f + 0.4f * macroNow);
    const float effectiveBowPressure     = bowPressureAfterLfo * (1.0f + 0.6f * macroNow);
    const float effectiveVibratoDepth    = rawVibratoDepth     * (1.0f + 0.3f * macroNow);
    const float effectiveBrightnessHz    = rawBrightness + 500.0f * macroNow;
    const float effectiveVibratoRate     = rawVibratoRate + vibAntiCorr;
    const float effectiveVibratoOnsetSec = 0.001f * rawVibratoOnsetMs;

    // Step 6 — push to bowModel + all-strings brightness. MPE pressure/timbre
    // is consumed here (not in updateParametersFromAPVTS) so the macro layer
    // is the single source of truth for the bowModel's setBowSpeed/Pressure
    // pushes. At modulators-off this is byte-identical to Phase 2.2:
    //   effectiveBowSpeed * mpeExpression == rawBowSpeed * mpeExpression
    //   effectiveBowPressure * (0.5 + p*1.5) == rawBowPressure * (0.5 + p*1.5)
    //   effectiveBrightnessHz == rawBrightness
    {
        const auto note = getCurrentlyPlayingNote();
        const float mpePressure = note.pressure.asUnsignedFloat();
        bowModel.setBowSpeed    (effectiveBowSpeed * mpeExpression);
        // Phase 2.4b R35d — multiply by voiceBowForceUpliftThisBlock (HR-9 path:
        // 1.0f → IEEE 754 identity → bit-exact preserved).
        bowModel.setBowPressure (effectiveBowPressure * (0.5f + mpePressure * 1.5f)
                                 * voiceBowForceUpliftThisBlock);
        for (auto& s : strings)
            s.setBrightness (effectiveBrightnessHz);
    }

    // Suppress unused-variable warnings for fields not consumed at this stage
    // (Step 7 per-sample loop uses effectiveVibratoDepth / effectiveVibratoRate /
    //  effectiveVibratoOnsetSec).
    juce::ignoreUnused (rawVibratoOnsetMs);

    // 2. Per-block dispersion + detune update sequence (RESEARCH §15.4).
    const float stringStiffness = parameters->getRawParameterValue ("STRING_STIFFNESS")->load();
    for (int s = 0; s < 4; ++s)
        strings[s].setStringStiffness (stringStiffness);

    for (int s = 0; s < 4; ++s)
        strings[s].advanceStiffnessSmootherBy (numSamples);

    for (int s = 0; s < 4; ++s)
    {
        const float currentStiffness = strings[s].getCurrentSmoothedStiffness();
        const float B = kBOpen[s] * juce::jlimit (0.0f, 1.0f, currentStiffness);
        const int   M = kMPerString[s];

        // Active + previous slots use played frequency (post-MPE-bend); idle
        // slots use their open-string default (no bend applied to silent strings).
        const float f0 = (s == activeStringIndex || s == previousStringIndex)
                       ? juce::jlimit (20.0f, 5000.0f, currentFrequency)
                       : juce::jlimit (20.0f, 5000.0f, kOpenStringFrequencyHz[s]);

        // Short-circuit at STRING_STIFFNESS=0 to preserve bit-exact regression bar.
        float a = (currentStiffness <= 0.0f)
                ? 0.0f
                : DispersionFilter<4>::computeAllpassCoefficient (f0, B, M);
        if (! std::isfinite (a)) a = 0.0f;
        strings[s].setDispersionCoefficient (a);
    }

    // Per-block detune ramp targets — only update active + previous; idle slots
    // stay at their last-set target (open-string default since prepare()).
    if (activeStringIndex >= 0)
    {
        const float dc = readDetuneForString (activeStringIndex);
        const float ds = computeDelaySamples (currentFrequency, dc);
        detuneSmoothed[activeStringIndex].setTargetValue (ds);
    }
    if (previousStringIndex >= 0)
    {
        const float dcPrev = readDetuneForString (previousStringIndex);
        const float dsPrev = computeDelaySamples (currentFrequency, dcPrev);
        detuneSmoothed[previousStringIndex].setTargetValue (dsPrev);
    }

    // 3. Clear the host-rate slice of voiceBuffer.
    voiceBuffer.clear (0, 0, numSamples);

    // 4. Wrap voiceBuffer in an AudioBlock for the host-rate slice.
    juce::dsp::AudioBlock<float> hostBlock (voiceBuffer);
    auto block = hostBlock.getSubBlock (0, static_cast<size_t> (numSamples));

    // 5. Upsample to 2× rate (silence → 2× scratch buffer).
    auto upBlock = oversampling.processSamplesUp (block);
    auto numUp = static_cast<int> (upBlock.getNumSamples());
    auto* upData = upBlock.getChannelPointer (0);

    // 6. Per-sample friction + 4-string bank loop at 2× rate.
    //    HARD RULE §15.9.5: use early-return on activeStringIndex (NOT
    //    unconditional sum) to preserve slot-0 bit-exact mix path.
    //
    //    Phase 2.3 — Step 7 of the 7-step evaluation order. Active-string-only
    //    vibrato modulation per Q2 (idle strings cold-decoupled). HR-1 gates
    //    the cents write at zero depth → modulators-off codepath byte-identical
    //    to Phase 2.2. Vibrato phase counter + onset timer ALWAYS advance
    //    (Q3 sine-phase-carry contract; HR-1 only gates the mix-write).
    const float kVibPhaseDelta = juce::MathConstants<float>::twoPi
                               * effectiveVibratoRate / static_cast<float> (sr_internal);
    const float kPiOverRamp    = juce::MathConstants<float>::pi / kVibratoRampSec;
    const float kInvSrInternal = 1.0f / static_cast<float> (sr_internal);

    for (int i = 0; i < numUp; ++i)
    {
        bowModel.updateEnvelope();
        const float v_bow = bowModel.getBowVelocity();
        const float F_bow = bowModel.getBowForce();

        // ─── Step 7a — vibrato cents (active string only; HR-1 gated) ────────
        float vibCents = 0.0f;
        if (effectiveVibratoDepth > 0.0f)                                    // HR-1 gate
        {
            // pin #8 — one-shot capture of onset gate at fade-entry. The
            // capture happens the first per-sample iteration the timer is
            // exactly 0.0f (set by noteStopped(allowTailOff=true)). After
            // capture the timer advances each sample; subsequent iterations
            // use the linear-fade branch.
            float gate;
            if (noteOffFadeOutTimerSeconds == 0.0f)
            {
                // Compute current onset gate FIRST so the snapshot is correct.
                const float elapsed = vibratoOnsetTimerSeconds - effectiveVibratoOnsetSec;
                if      (elapsed <= 0.0f)              gate = 0.0f;
                else if (elapsed >= kVibratoRampSec)   gate = 1.0f;
                else                                   gate = 0.5f - 0.5f
                                                            * std::cos (kPiOverRamp * elapsed);
                vibratoOnsetGateAtNoteOff = gate;
            }
            else if (noteOffFadeOutTimerSeconds > 0.0f
                  && noteOffFadeOutTimerSeconds < kVibratoFadeOutSec)
            {
                // Linear 150 ms fade-out from snapshot value to 0.
                const float k = juce::jlimit (0.0f, 1.0f,
                                              noteOffFadeOutTimerSeconds / kVibratoFadeOutSec);
                gate = vibratoOnsetGateAtNoteOff * (1.0f - k);
            }
            else if (noteOffFadeOutTimerSeconds >= kVibratoFadeOutSec)
            {
                // Fade complete — vibrato silenced until next note-on re-arm.
                gate = 0.0f;
            }
            else
            {
                // Standard onset path (sentinel −1 / not in fade).
                const float elapsed = vibratoOnsetTimerSeconds - effectiveVibratoOnsetSec;
                if      (elapsed <= 0.0f)              gate = 0.0f;
                else if (elapsed >= kVibratoRampSec)   gate = 1.0f;
                else                                   gate = 0.5f - 0.5f
                                                            * std::cos (kPiOverRamp * elapsed);
            }

            vibCents = effectiveVibratoDepth * gate * std::sin (vibratoPhase);
        }

        // Phase counter + timers ALWAYS advance regardless of HR-1 gate.
        vibratoPhase += kVibPhaseDelta;
        if (vibratoPhase > juce::MathConstants<float>::twoPi)
            vibratoPhase -= juce::MathConstants<float>::twoPi;
        vibratoOnsetTimerSeconds += kInvSrInternal;
        if (noteOffFadeOutTimerSeconds >= 0.0f)
            noteOffFadeOutTimerSeconds += kInvSrInternal;

        float mixedSample = 0.0f;

        if (crossfadeRemainingSamples > 0 && previousStringIndex >= 0)
        {
            const int idx = juce::jlimit (0, crossfadeTotalSamples,
                                          crossfadeTotalSamples - crossfadeRemainingSamples);
            const auto gains = crossfadeRamp[static_cast<size_t> (idx)];
            const float oldGain = gains.first;
            const float newGain = gains.second;

            // Phase 2.3 — vibrato modulates active slot in delay-samples space.
            // Structure mirrors Phase 2.2: vibrato branch first (early-out at
            // vibCents==0 keeps idle slots on the carry-forward isSmoothing
            // path); branches DO NOT call getNextValue twice per iteration
            // (HR-1 bit-exact: each branch consumes the smoother counter
            // exactly once, matching Phase 2.2 verbatim at modulators-off).
            for (int s = 0; s < 4; ++s)
            {
                if (s == activeStringIndex && vibCents != 0.0f)
                {
                    const float detuneSamples  = detuneSmoothed[s].getNextValue();
                    const float vibFactor      = std::exp (vibCents * kVibFactorScale);
                    const float modulatedDelay = detuneSamples * vibFactor;
                    strings[s].setDelaySamples (modulatedDelay);
                }
                else if (detuneSmoothed[s].isSmoothing())
                {
                    strings[s].setDelaySamples (detuneSmoothed[s].getNextValue());
                }
                else
                {
                    detuneSmoothed[s].getNextValue();
                }
            }

            float oldOut = 0.0f, newOut = 0.0f;
            for (int s = 0; s < 4; ++s)
            {
                if (s == previousStringIndex)
                    oldOut = strings[s].processSample (0.0f, 0.0f, frictionModel);
                else if (s == activeStringIndex)
                    newOut = strings[s].processSample (v_bow, F_bow, frictionModel);
                else
                    (void) strings[s].processSample (0.0f, 0.0f, frictionModel);
            }

            mixedSample = oldOut * oldGain + newOut * newGain;
            --crossfadeRemainingSamples;
            if (crossfadeRemainingSamples == 0)
                previousStringIndex = -1;
        }
        else
        {
            // Standard path — slot-0 bit-exact mix path: when activeStringIndex==0
            // AND vibCents==0 (HR-1 short-circuit) AND idle slots not smoothing,
            // mixedSample = strings[0].processSample(v_bow, F_bow, friction)
            // verbatim against Phase 2.2 regression bar.
            for (int s = 0; s < 4; ++s)
            {
                if (s == activeStringIndex && vibCents != 0.0f)
                {
                    const float detuneSamples  = detuneSmoothed[s].getNextValue();
                    const float vibFactor      = std::exp (vibCents * kVibFactorScale);
                    const float modulatedDelay = detuneSamples * vibFactor;
                    strings[s].setDelaySamples (modulatedDelay);
                }
                else if (detuneSmoothed[s].isSmoothing())
                {
                    strings[s].setDelaySamples (detuneSmoothed[s].getNextValue());
                }
                else
                {
                    detuneSmoothed[s].getNextValue();
                }

                if (s == activeStringIndex)
                    mixedSample = strings[s].processSample (v_bow, F_bow, frictionModel);
                else
                    (void) strings[s].processSample (0.0f, 0.0f, frictionModel);
            }
        }

        upData[i] = mixedSample;
    }

    // 7. Downsample back to host rate (writes into `block` which aliases voiceBuffer).
    oversampling.processSamplesDown (block);

    // === PHASE 2.5 NEW Step 8: Body resonator (8-mode parallel bandpass + 35 Hz HP dry path + wet/dry mix) ===
    // Skip-bump SmoothedValue: advance smoother by numSamples and read the
    // end-of-block value (per-block recompute pattern; coefficients re-derived
    // once per block via recomputeCoefficients() inside processBlock).
    // jmax guard handles numSamples==1 (skip(0) is a no-op return of current).
    {
        const float sz = bodySizeSmoothed.skip    (juce::jmax (1, numSamples - 1));
        const float dp = bodyDampingSmoothed.skip (juce::jmax (1, numSamples - 1));
        const float mx = bodyMixSmoothed.skip     (juce::jmax (1, numSamples - 1));
        bodyResonator.setSize    (sz);
        bodyResonator.setDamping (dp);
        bodyResonator.setMix     (mx);
        bodyResonator.processBlock (voiceBuffer.getWritePointer (0), numSamples);
    }

    // === PHASE 2.5 NEW Step 9: Bow noise (3-band BPF + period-heuristic slip bursts; sums after body) ===
    {
        // bowEnergy = clamp(0, 1, |v_bow| · F_bow / (v_ref · F_ref)) per
        // ARCHITECTURE §164 (v_ref = 0.3 m/s, F_ref = 2.0 N).
        constexpr float kVRef = 0.3f;
        constexpr float kFRef = 2.0f;
        const float vBow = std::abs (bowModel.getBowVelocity());
        const float fBow = bowModel.getBowForce();
        const float bowEnergyVal = juce::jlimit (0.0f, 1.0f,
                                                 (vBow * fBow) / (kVRef * kFRef));
        bowNoiseGenerator.setBowEnergy (bowEnergyVal);

        // Push active-string fundamental on note-start or > 5 cent change
        // (period-heuristic slip trigger). currentFrequency is the played
        // freq for the active string (post-MPE-bend); idle slot has
        // activeStringIndex < 0 so we skip the push entirely until a note
        // arrives.
        if (activeStringIndex >= 0)
        {
            const float f0 = juce::jlimit (20.0f, 5000.0f, currentFrequency);
            const float cents = (lastFundamentalHz > 0.0f)
                              ? 1200.0f * std::log2 (f0 / lastFundamentalHz)
                              : 0.0f;
            if (lastFundamentalHz <= 0.0f || std::abs (cents) > 5.0f)
            {
                bowNoiseGenerator.setFundamentalHz (f0);
                lastFundamentalHz = f0;
            }
        }

        // BOW_NOISE smoothed level (skip-bump end-of-block read).
        const float noiseLvl = bowNoiseSmoothed.skip (juce::jmax (1, numSamples - 1));
        bowNoiseGenerator.setNoiseLevel (noiseLvl);

        float* mono = voiceBuffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
            mono[i] += bowNoiseGenerator.processSample();
    }
    // === END PHASE 2.5 ===

    // 10. Mix host-rate voiceBuffer into the output buffer (mono → stereo split).
    constexpr float kVoiceNorm = 0.35f;
    const int numOutChans = outputBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Phase 2.6a R39d — OUTPUT_GAIN multiplier REMOVED; relocated
        // processor-side post-StereoWidth (ARCHITECTURE §258 final stage).
        float s = voiceBuffer.getSample (0, i) * kVoiceNorm;
        s = juce::jlimit (-1.0f, 1.0f, s);

        outputBuffer.addSample (0, startSample + i, s);
        if (numOutChans >= 2)
            outputBuffer.addSample (1, startSample + i, s);
    }
}

//==============================================================================
// Parameter Reading
//==============================================================================

void BowedContrabassVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr)
        return;

    // Phase 2.3 NOTE: bow speed / pressure pushes to bowModel and brightness
    // push to strings have moved into renderNextBlock Step 6 (after the 7-step
    // modulator + macro evaluation order is run). This helper now reads the
    // raw APVTS values that don't participate in the macro layering, plus the
    // MPE-driven `effectivePosition` field still consumed by all 4 strings.

    // UPPER_SNAKE_CASE per parameter-spec.md (frozen contract).
    float bowPos       = parameters->getRawParameterValue ("BOW_POSITION")->load();
    float infSustain   = parameters->getRawParameterValue ("INFINITE_SUSTAIN")->load();
    // Phase 2.6a R39d — OUTPUT_GAIN read REMOVED here; relocated to processor
    // (ARCHITECTURE §258 final stage; voice writes voice-norm only).

    // MPE timbre modulates Y (bow position offset). MPE pressure (Z) is
    // consumed in renderNextBlock Step 6 alongside macro-lifted bow pressure.
    auto note = getCurrentlyPlayingNote();
    float mpeTimbre = note.timbre.asSignedFloat();

    effectivePosition = juce::jlimit (0.02f, 0.25f, bowPos + mpeTimbre * 0.05f);

    // Phase 2.4b R35d — frictionModel.setRosin RELOCATED to renderNextBlock
    // BEFORE Step 2.5 (HR-10 friction module ABI preservation): bias-path
    // overwrites with rosinEq via inverse algebraic identity; HR-9 path runs
    // setRosin(rawRosin) alone → bit-exact preserved.

    // Push bow position + sustain to all 4 strings — global parameters not
    // touched by the macro layer. Brightness is pushed in renderNextBlock
    // Step 6 (macro lifts it via +500·m Hz offset).
    for (auto& s : strings)
    {
        s.setBowPosition (effectivePosition);
        s.setInfiniteSustain (infSustain);
    }

    // Phase 2.6a R39d — outputGainLinear set REMOVED; OUTPUT_GAIN is now
    // applied processor-side post-StereoWidth (ARCHITECTURE §258 final stage).

    // Phase 2.5 — push body + bow-noise smoother targets. The smoothers ramp
    // at 30 ms; per-block recompute pattern (BodyResonator.recomputeCoefficients
    // is called inside processBlock).
    bodySizeSmoothed.setTargetValue    (parameters->getRawParameterValue ("BODY_SIZE")   ->load());
    bodyDampingSmoothed.setTargetValue (parameters->getRawParameterValue ("BODY_DAMPING")->load());
    bodyMixSmoothed.setTargetValue     (parameters->getRawParameterValue ("BODY_MIX")    ->load());
    bowNoiseSmoothed.setTargetValue    (parameters->getRawParameterValue ("BOW_NOISE")   ->load());
}

//==============================================================================
// Phase 2.2 Helpers
//==============================================================================

int BowedContrabassVoice::mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept
{
    int idx = 0;
    if      (midiNote >= 43) idx = 3;
    else if (midiNote >= 38) idx = 2;
    else if (midiNote >= 33) idx = 1;
    const int maxIdx = juce::jlimit (0, 3, activeStrings - 1);
    return juce::jmin (idx, maxIdx);
}

float BowedContrabassVoice::readDetuneForString (int s) const noexcept
{
    jassert (s >= 0 && s < 4);
    return parameters->getRawParameterValue (kDetuneParamIds[s])->load();
}

float BowedContrabassVoice::computeDelaySamples (float playedFreqHz, float detuneCents) const noexcept
{
    const float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
    const float detunedFreq = playedFreqHz * detuneRatio;
    return static_cast<float> (sr_internal) / juce::jmax (1.0f, detunedFreq);
}
