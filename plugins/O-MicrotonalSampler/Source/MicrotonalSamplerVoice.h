/*
  ==============================================================================

    MicrotonalSamplerVoice.h
    Microtonal Sample Engine - Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1 surface: cubic-Hermite varispeed read + ADSR + NE consumption.
    Phase 2.3: dual-cell equal-power velocity-layer crossfade.
    Phase 2.4: voice-steal tail-ramp scratch buffers (5 ms linear-down).
    Phase 2.5: 8-sample equal-power loop boundary crossfade + position wrap.
    v1.8.0: per-cell variant selection (round-robin) at startNote — pure
            atomic-counter + integer math; no allocation in audio path.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include "TuningEngine.h"          // global namespace (D-4)
#include "NoteExpression.h"        // resolved via ouaricon_add_module include path
#include "SampleMap.h"
#include "MicrotonalSamplerSound.h"

// v1.23.2 (W10): forward-declared so the pointer member below needs no
// juce_events include here — the definition lives in RetiredMapReaper.h,
// included only by the .cpp.
class RetiredMapReaper;

class MicrotonalSamplerVoice : public juce::SynthesiserVoice
{
public:
    // v1.8.0: round-robin selection mode. Mirrors the choice param indices in
    // PluginProcessor's APVTS (rr_mode). Default = RandomNoRepeat (industry
    // standard).
    enum class RoundRobinMode : int
    {
        Cycle           = 0,
        RandomNoRepeat  = 1,
        Random          = 2
    };

    // v1.8.0: per-cell counter array. 128 notes × 4 layers = 512 entries.
    // Index = midi * 4 + layer. Sentinel value 0xFF = "no variant yet"
    // (cleared by processor on ReplaceAll); both Cycle and RandomNoRepeat
    // interpret it as a no-exclusion start.
    //
    // v1.14.0: technique axis. The counter array now holds
    // 128 × 4 × kMaxTechniques (= 4096) entries to give every
    // (midi, layer, technique) cell its own RR cursor — counters from
    // different techniques must not interfere when the user switches
    // between them mid-session. Index =
    //     midi * 4 * kMaxTechniques + layer * kMaxTechniques + technique.
    static constexpr int kRrCounterSize = 128 * 4 * kMaxTechniques;   // 4096
    using RrCounterArray = std::array<std::atomic<uint8_t>, (size_t) kRrCounterSize>;

    // v1.16.10 (MEDIUM-02): pack a (midi, layer, technique) triple into the
    // flat rrCounters index. Layout coupling to kRrCounterSize lives here
    // and only here — call sites in PluginProcessor.cpp + this voice should
    // route through this helper instead of repeating the literal arithmetic.
    static constexpr int packRrCounterIndex (int midi, int layer, int tech) noexcept
    {
        return midi * 4 * kMaxTechniques + layer * kMaxTechniques + tech;
    }

    // v1.8.0: per-cell variant cap. The counter type above is uint8_t with
    // 0xFF reserved as "no variant yet" sentinel — therefore the cap MUST be
    // strictly less than 255 (so all valid indices fit in the lower 254 codes
    // and the sentinel is never produced as a real selection).
    //
    // v1.12.3 (HG-04): hoisted from PluginProcessor.cpp (where it was a
    // local constexpr in two places) into the same TU as the counter type,
    // so the static_assert below catches any future bump that would collide
    // with the sentinel. selectVariantIndex still defensively jlimits the
    // stored value to 254 — see MicrotonalSamplerVoice.cpp — but the assert
    // is the contract.
    static constexpr int kMaxVariantsPerCell = 64;
    static_assert (kMaxVariantsPerCell < 255,
                   "variant index must fit in uint8 with 0xFF sentinel reserved");

    MicrotonalSamplerVoice() = default;
    ~MicrotonalSamplerVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void prepareToPlay (double sampleRate, int samplesPerBlock);

    void setCurrentPlaybackSampleRate (double newRate) override;

    // Wiring setters (called once per voice from PluginProcessor ctor).
    void setAPVTS               (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine        (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }
    void setSampleMapSource     (std::shared_ptr<SampleMap>* src)       { sampleMapSource = src; }
    void setRrCounterArray      (RrCounterArray* a)                     { rrCounters = a; }

    // v1.14.0: pointer to the processor's atomic technique-select cursor. The
    // audio thread loads the cursor at startNote (memory_order_acquire) and
    // pairs that with the SampleMap snapshot to resolve the cell triplet.
    // KS / CC / PC events on the audio thread store into the same atomic.
    void setPendingTechniqueSource (std::atomic<int>* src)              { pendingTechniqueSource = src; }

    // v1.23.0: pointer to the processor-owned per-technique / per-(technique,
    // layer) loudness trim table. Read at note-on (RT-safe atomic loads) and
    // folded into the layer weights (velocity mode) / DynLayer gains (CC mode).
    void setTrimTableSource (const TrimTable* t)                         { trimTable = t; }

    // v1.23.2 (W10): pointer to the processor-owned message-thread reaper. When
    // set, startNote hands a retired `prevMap` (reload-boundary steal) to it so
    // the big SampleMap free never runs on the audio thread. Null in contexts
    // that construct a bare voice (e.g. unit tests) → falls back to letting
    // prevMap destruct in-scope, exactly as before.
    void setRetiredMapSink (RetiredMapReaper* r)                         { retiredMapSink = r; }

    // v1.23.8: pointer to the processor's audio-thread-direct CC 11 dynamics
    // atom (0..1). Preferred over the "expression" APVTS atom, which only
    // updates via a message-thread AsyncUpdater and therefore lags/coalesces
    // during offline export (Dorico export dynamics-jump bug). Null in bare
    // constructions → currentExpression() falls back to the APVTS atom.
    void setLiveExpressionSource (std::atomic<float>* src)               { liveExpressionSource = src; }

private:
    juce::AudioProcessorValueTreeState*           parameters             = nullptr;
    TuningEngine*                                 tuningEngine           = nullptr;
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource    = nullptr;
    std::shared_ptr<SampleMap>*                   sampleMapSource        = nullptr;
    RrCounterArray*                               rrCounters             = nullptr;
    std::atomic<int>*                             pendingTechniqueSource = nullptr;   // v1.14.0
    int                                           startTechnique         = 0;         // captured at startNote
    const TrimTable*                              trimTable              = nullptr;   // v1.23.0
    RetiredMapReaper*                             retiredMapSink         = nullptr;   // v1.23.2 (W10)

    // v1.11.3: cached ADSR atomic pointers. Resolved once in prepareToPlay so
    // startNote does not deref the result of getRawParameterValue without a
    // null-check. If any param is missing from APVTS at prepareToPlay, all
    // four are left null and startNote falls back to the prior ADSR setting.
    // (REVIEW DSP CRITICAL #2.)
    std::atomic<float>* attackParam  = nullptr;
    std::atomic<float>* decayParam   = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;

    juce::ADSR                  adsr;
    double                      currentFrequency   = 0.0;
    int                         currentMidiNote    = -1;
    std::shared_ptr<SampleMap>  currentMap;

    // v1.8.0: dual-cell crossfade — each cell has a selected variant for the
    // duration of the note. Variant pointers reference into the cell's
    // variants vector (held alive transitively through `currentMap`).
    const SampleCell*           cellLow            = nullptr;
    const SampleCell*           cellHigh           = nullptr;
    const SampleVariant*        variantLow         = nullptr;
    const SampleVariant*        variantHigh        = nullptr;
    float                       layerWeightLow     = 1.0f;
    float                       layerWeightHigh    = 0.0f;
    double                      posLow             = 0.0;
    double                      posHigh            = 0.0;
    double                      playRateLow        = 1.0;
    double                      playRateHigh       = 1.0;

    // v1.21.0: CC Crossfade dynamics. When `ccDynamicsActive`, ALL populated
    // velocity layers for (midi, technique) advance in lockstep each sample
    // (time-synced → click-free bracket entry) but only the two layers
    // bracketing the live, smoothed CC 11 dynamic position are summed into the
    // output. Independent of the velocity-crossfade path above — that path is
    // used only in Dynamics Mode = Velocity (back-compat, bit-identical).
    static constexpr int kMaxDynLayers = 4;   // SampleMap::numVelocityLayers <= 4

    struct DynLayer
    {
        const SampleVariant* variant  = nullptr;  // into currentMap (held for note)
        double               pos      = 0.0;
        double               playRate = 1.0;
        float                trimLin  = 1.0f;      // v1.23.0: technique×layer trim (linear)
    };
    std::array<DynLayer, (size_t) kMaxDynLayers> dynLayers {};
    int                        dynLayerCount    = 0;
    bool                       ccDynamicsActive = false;     // captured at note-on
    juce::SmoothedValue<float> dynamicsSmoother;             // d ∈ [0,1], per-sample

    // v1.21.0: cached param atoms (resolved in prepareToPlay, like the ADSR
    // pointers below). dynamicsModeParam: 0=Velocity, 1=CC Crossfade.
    // expressionParam: the CC 11 / Expression value (0..1) that drives the
    // dynamic crossfade position in CC mode.
    std::atomic<float>* dynamicsModeParam = nullptr;
    std::atomic<float>* expressionParam   = nullptr;
    // v1.22.0: CC-crossfade dynamic-range (dB span pp→ff). Read per block in
    // renderCcCrossfade / renderTailRampCc; drives the dB-linear loudness ramp
    // layered on top of the equal-power timbre morph. 0 dB = flat (v1.21.0).
    std::atomic<float>* dynamicRangeParam = nullptr;

    // v1.23.8: see setLiveExpressionSource.
    std::atomic<float>* liveExpressionSource = nullptr;

    // v1.23.8: the dynamics value (0..1) the render path should track. The
    // processor-fed live atom wins; the APVTS atom is the bare-voice fallback.
    float currentExpression() const noexcept
    {
        if (liveExpressionSource != nullptr)
            return liveExpressionSource->load (std::memory_order_relaxed);
        return (expressionParam != nullptr) ? expressionParam->load() : 1.0f;
    }

    std::vector<float> stealTailBufferL;
    std::vector<float> stealTailBufferR;
    int                stealTailSamplesRemaining = 0;
    int                kMaxStealRamp             = 0;

    // v1.8.0: per-voice xorshift32 RNG state for the Random / RandomNoRepeat
    // selection modes. Seeded in prepareToPlay from the voice's identity hash;
    // advanced inline in selectVariantIndex (RT-safe — pure integer ops).
    uint32_t rngState = 0x12345678u;

    void renderTailRamp (int rampSamples) noexcept;

    // v1.21.0: CC Crossfade render + its voice-steal tail. renderCcCrossfade
    // advances every `dynLayers` entry per sample but sums only the two
    // bracketing the live dynamic position. renderTailRampCc is the CC analogue
    // of renderTailRamp (frozen dynamic position over the 5 ms steal ramp).
    void renderCcCrossfade (juce::AudioBuffer<float>& out,
                            int startSample, int numSamples) noexcept;
    void renderTailRampCc  (int rampSamples) noexcept;

    // v1.8.0: pick a variant index for `cell` using mode + counter. Pure
    // index math + atomic ops. RT-safe. cell.variants must be non-empty.
    int selectVariantIndex (const SampleCell& cell, RoundRobinMode mode) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrotonalSamplerVoice)
};
