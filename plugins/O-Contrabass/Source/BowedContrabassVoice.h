/*
  ==============================================================================

    BowedContrabassVoice.h
    O-Contrabass - 4-String Bank MPESynthesiserVoice (Phase 2.2)
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.2 expands the single-string E1 voice to a 4-string E/A/D/G bank
    (open frequencies 41.20 / 55.00 / 73.42 / 98.00 Hz) with per-string
    M=4/3/2/1 dispersion sections (B prefactors 1e-4 / 7e-5 / 5e-5 / 3e-5),
    per-string detune ±1200¢ ramps (20 ms SmoothedValue<Linear> in
    delay-samples space), MIDI→string mapping (28/33/38/43 thresholds with
    ACTIVE_STRINGS clamp), and 5 ms equal-power crossfade at voice mix-bus
    on note-on string transitions. Idle strings tick on every sample with
    zero-input excitation; only the active string receives bow friction
    injection. HARD RULES §15.9.5 govern slot-0 bit-exact preservation
    against the Phase 2.1c regression preset.

  ==============================================================================
*/

#pragma once
#include <array>
#include <atomic>
#include <utility>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/WaveguideString.h"
#include "DSP/BodyResonator.h"
#include "DSP/BowNoiseGenerator.h"
#include "BowModel.h"
#include "HyperbolicFriction.h"
#include "NoteExpression.h"   // Phase 2.6c R41b — Steinberg-free module header (safe in voice TU)

class TuningEngine;     // Phase 2.6b R40b forward decl — full include in .cpp

class BowedContrabassVoice : public juce::MPESynthesiserVoice
{
public:
    explicit BowedContrabassVoice (juce::AudioProcessorValueTreeState* apvts,
                                   TuningEngine* engine);
    ~BowedContrabassVoice() override = default;

    // MPE voice callbacks
    void noteStarted() override;
    void noteStopped (bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    // Lifecycle
    void prepareToPlay (double hostSampleRate, int maxBlockSize);

    // Render
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Latency reporting (processor reads in prepareToPlay → setLatencySamples)
    float getOversamplingLatency() const noexcept { return oversampling.getLatencyInSamples(); }

    // CC11 expression macro — set by OContrabassMPESynthesiser. Cached now,
    // wired into the bow envelope in Phase 2.6 alongside Note Expression.
    void setExpression (float value) noexcept { mpeExpression = value; }

    // Phase 2.6c R41b — VST3 Note Expression pending-table source (module-owned
    // table, D-09 pattern; processor wires this in its ctor addVoice loop).
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* t) noexcept
    {
        pendingTuningSource = t;
    }

    // Phase 2.3 R29 — Schelleng wedge instrumentation hook (single-writer audio
    // thread / single-reader harness; relaxed memory order — no cross-thread
    // ordering required for this scalar audit value).
    float getLastSafeDepth() const noexcept
    {
        return lastSafeDepth.load (std::memory_order_relaxed);
    }

    // Phase 2.4b R35d — sub-harmonic bias instrumentation hook (mirrors HR-4
    // lastSafeDepth pattern; written per-block at top of Step 2.5 — pre-gate
    // store(0.0) then post-bias store(subAmount) for active-string biased path).
    float getLastSubAmount() const noexcept
    {
        return lastSubAmount.load (std::memory_order_relaxed);
    }

    // ------------------------------------------------------------------------
    // Stage 3 Task 13 (D5) — Schelleng-wedge viz taps. READ-ONLY relaxed
    // atomics published per block from the render path (same single-writer
    // pattern as lastSafeDepth). Never feeds back into the signal path —
    // goldens-safe by construction. Reader: processor getBowStateViz()
    // (message thread, editor timer @30 Hz).
    //   vizBowSpeed/Pressure = effective post-LFO/macro/MPE values pushed to
    //   the bow model in Step 6; vizBowBeta = effectivePosition (post-MPE Y).
    //   vizStartOrdinal orders voices so the processor can pick the
    //   most-recently-started active voice (fixes the voice-0 hardcode).
    // ------------------------------------------------------------------------
    bool         getVizActive()       const noexcept { return vizActive.load (std::memory_order_relaxed); }
    juce::uint32 getVizStartOrdinal() const noexcept { return vizStartOrdinal.load (std::memory_order_relaxed); }
    float        getVizBowSpeed()     const noexcept { return vizBowSpeed.load (std::memory_order_relaxed); }
    float        getVizBowPressure()  const noexcept { return vizBowPressure.load (std::memory_order_relaxed); }
    float        getVizBowBeta()      const noexcept { return vizBowBeta.load (std::memory_order_relaxed); }

private:
    void updateParametersFromAPVTS();

    int   mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept;
    float readDetuneForString      (int s) const noexcept;
    float computeDelaySamples      (float playedFreqHz, float detuneCents) const noexcept;

    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // Phase 2.6b R40b — TuningEngine pointer (constructor-injected by processor)
    // + cache of the per-note resolved base frequency (post-TuningEngine lookup
    // × REFERENCE_PITCH ratio, pre-MPE-bend application). Q17 LOCK: voice
    // resolves f_target ONCE at noteStarted; per-block MPE bend applies as
    // multiplicative pow2(bend/12) on the cached value — NO re-poll mid-sustain.
    TuningEngine* tuningEngine        = nullptr;
    double        tuningEngineBaseFreqHz { 0.0 };

    // Phase 2.6c R41b — pending NE tuning table (owned by the processor's
    // VST3Extensions member; nullptr until setPendingTuningSource is called).
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    // DSP composition — Phase 2.2 4-string bank (slots 0=E, 1=A, 2=D, 3=G).
    std::array<WaveguideString, 4> strings;
    BowModel bowModel;
    HyperbolicFriction frictionModel;

    // Phase 2.2 — per-string detune ramp (20 ms linear, in delay-samples space).
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> detuneSmoothed;

    // Phase 2.2 — string-bank state machine.
    int   activeStringIndex          = -1;     // -1 = no string yet (first noteStarted)
    int   previousStringIndex        = -1;     // valid only while crossfadeRemainingSamples > 0
    int   crossfadeRemainingSamples  = 0;
    int   crossfadeTotalSamples      = 0;      // = ceil(0.005 * sr_internal); cached at prepare
    std::vector<std::pair<float, float>> crossfadeRamp;  // (oldGain, newGain) per sample
    double sr_internal               = 88200.0;
    // WR-08 / WR-09: host (pre-oversampling) sample rate. Per-block quantities that
    // step at the host block size — the slow-bow LFO phase delta and the macro /
    // sub-harmonic smoother ramps — must use this, not the 2× internal rate.
    double sr_host                   = 44100.0;

    // Per-voice 2× oversampler — RESEARCH §3.1 / §Q4. PolyphaseIIR, max quality,
    // useIntegerLatency=false (default).
    juce::dsp::Oversampling<float> oversampling
    {
        /*numChannels*/ 1,
        /*factor*/      1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        /*isMaxQuality*/ true
    };

    // Mono scratch — sized to maxBlockSize × 2 in prepareToPlay so the
    // upsampled AudioBlock fits without further allocation in renderNextBlock.
    juce::AudioBuffer<float> voiceBuffer;

    // Cached host & oversampled block size (for AudioBlock setup)
    int currentMaxBlockSize = 0;

    // Cached per-block state read from APVTS
    float currentFrequency = 41.2f;        // E1 default
    float effectivePosition = 0.10f;
    // Phase 2.6a R39d — outputGainLinear member REMOVED; OUTPUT_GAIN is now
    // applied processor-side post-StereoWidth (ARCHITECTURE §258 final stage).
    float mpeExpression = 1.0f;

    // ─── Phase 2.3 — Vibrato + Slow-LFO + EXPRESSION_MACRO state ───────────────
    //
    // Vibrato sine-LFO phase counter — continuous across notes (Q3 sine-phase-
    // carry contract; HR-1 short-circuits the cents write at zero depth but
    // ALWAYS advances the phase to keep timekeeping monotonic across re-arms).
    float vibratoPhase                = 0.0f;
    // Per-note onset timer — re-armed in noteStarted() so every fresh note
    // sees the configured VIBRATO_ONSET delay before the half-cosine S-curve
    // ramp engages.
    float vibratoOnsetTimerSeconds    = 0.0f;
    // One-shot snapshot of `vibratoOnsetGate` at the moment note-off fade
    // engages (pin #8) — captured on the per-sample iteration that first
    // sees `noteOffFadeOutTimerSeconds == 0.0f`. Used as the fade-out start
    // value for the 150 ms linear ramp.
    float vibratoOnsetGateAtNoteOff   = 0.0f;
    // Note-off linear fade-out timer — sentinel −1 means "not in fade".
    // Set to 0 by noteStopped(allowTailOff=true); advances per-sample once
    // engaged.
    float noteOffFadeOutTimerSeconds  = -1.0f;
    // Slow-Bow LFO sine phase counter — voice-level (single phase across the
    // 4-string bank; HR-2 gates phase advance at zero depth so the regression
    // bar is unaffected when SLOW_LFO_DEPTH=0).
    float slowLfoPhase                = 0.0f;

    // Phase 2.3 — voice-level macro source smoother (one source, four
    // destinations: bow speed, bow pressure, vibrato depth, brightness offset).
    // 20 ms linear ramp per RESEARCH §16.5 (Δp ≈ 0.015/block on the bridge-LP
    // pole — well below zipper threshold).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> macroSmoothed;
    // Slow-LFO output smoothers (architecture line 112 — 20 ms each).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> slowLfoSpeedSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> slowLfoPressureSmoothed;

    // Phase 2.3 — Schelleng wedge instrumentation atom. Written per-block in
    // step 2 of the 7-step evaluation order (always written, even at zero LFO
    // depth, so the harness read view is well-defined every block — pin #4).
    std::atomic<float> lastSafeDepth { 0.0f };

    // ─── Phase 2.4b — Sub-harmonic bias state (HR-9 + HR-10) ──────────────────
    // 30 ms ramp; HR-9 strict-default precondition: setCurrentAndTargetValue(0)
    // in prepareToPlay; UNCONDITIONAL setTargetValue per block (pin #11 precedent).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> subHarmonicsSmoothed;
    // Instrumentation atom (mirrors lastSafeDepth pattern — pre-gate store(0)
    // then optional post-bias store(subAmount)).
    std::atomic<float> lastSubAmount { 0.0f };
    // F_bow uplift factor consumed at Step 6 setBowPressure call.
    // HR-9 reset to 1.0f at top of every renderNextBlock (BEFORE Step 2.5) so
    // short-circuit path leaves Step 6 bit-exact identical to Phase 2.4a.
    float voiceBowForceUpliftThisBlock { 1.0f };

    // ─── Phase 2.5 — Body Resonator + Bow Noise Generator ─────────────────────
    // 8-mode parallel bandpass body resonator (mono single-bank, runs at host
    // rate AFTER 2× downsample at line 688). 3-band BPF + period-heuristic
    // slip-burst bow noise generator summed AFTER body. Both per-block-pushed
    // via 30 ms SmoothedValue<Linear> ramps (CONTEXT line 152 + ARCHITECTURE
    // §152 + parameter-spec.md defaults: SIZE=0.75 / DAMPING=0.40 / MIX=0.80
    // / NOISE=0.35).
    BodyResonator        bodyResonator;
    BowNoiseGenerator    bowNoiseGenerator;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bodySizeSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bodyDampingSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bodyMixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bowNoiseSmoothed;
    // For slip-trigger 5-cent change detection (push setFundamentalHz only on
    // note-start or > 5 cent change to avoid resetting slip counter every block).
    float lastFundamentalHz = 0.0f;

    // ─── Stage 3 Task 13 (D5) — Schelleng viz taps (see public accessors) ─────
    // Single-writer (audio thread) relaxed stores: Step 6 of renderNextBlock
    // publishes speed/pressure/beta; noteStarted sets active + ordinal;
    // both clearCurrentNote sites clear active. Read-only — never consumed by
    // the signal path (goldens-safe).
    std::atomic<float>        vizBowSpeed     { 0.0f };
    std::atomic<float>        vizBowPressure  { 0.0f };
    std::atomic<float>        vizBowBeta      { 0.10f };
    std::atomic<bool>         vizActive       { false };
    std::atomic<juce::uint32> vizStartOrdinal { 0 };
    // Shared monotonic note-start ordinal across all voices (viz ordering only).
    static inline std::atomic<juce::uint32> sVizOrdinalCounter { 0 };
};
