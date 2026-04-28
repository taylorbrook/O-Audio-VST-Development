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
#include "BowModel.h"
#include "HyperbolicFriction.h"

class BowedContrabassVoice : public juce::MPESynthesiserVoice
{
public:
    explicit BowedContrabassVoice (juce::AudioProcessorValueTreeState* apvts);
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

    // Phase 2.3 R29 — Schelleng wedge instrumentation hook (single-writer audio
    // thread / single-reader harness; relaxed memory order — no cross-thread
    // ordering required for this scalar audit value).
    float getLastSafeDepth() const noexcept
    {
        return lastSafeDepth.load (std::memory_order_relaxed);
    }

private:
    void updateParametersFromAPVTS();

    int   mapMidiNoteToStringIndex (int midiNote, int activeStrings) const noexcept;
    float readDetuneForString      (int s) const noexcept;
    float computeDelaySamples      (float playedFreqHz, float detuneCents) const noexcept;

    juce::AudioProcessorValueTreeState* parameters = nullptr;

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
    float outputGainLinear = 1.0f;
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
};
