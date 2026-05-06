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

private:
    juce::AudioProcessorValueTreeState*           parameters             = nullptr;
    TuningEngine*                                 tuningEngine           = nullptr;
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource    = nullptr;
    std::shared_ptr<SampleMap>*                   sampleMapSource        = nullptr;
    RrCounterArray*                               rrCounters             = nullptr;
    std::atomic<int>*                             pendingTechniqueSource = nullptr;   // v1.14.0
    int                                           startTechnique         = 0;         // captured at startNote

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

    std::vector<float> stealTailBufferL;
    std::vector<float> stealTailBufferR;
    int                stealTailSamplesRemaining = 0;
    int                kMaxStealRamp             = 0;

    // v1.8.0: per-voice xorshift32 RNG state for the Random / RandomNoRepeat
    // selection modes. Seeded in prepareToPlay from the voice's identity hash;
    // advanced inline in selectVariantIndex (RT-safe — pure integer ops).
    uint32_t rngState = 0x12345678u;

    void renderTailRamp (int rampSamples) noexcept;

    // v1.8.0: pick a variant index for `cell` using mode + counter. Pure
    // index math + atomic ops. RT-safe. cell.variants must be non-empty.
    int selectVariantIndex (const SampleCell& cell, RoundRobinMode mode) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrotonalSamplerVoice)
};
