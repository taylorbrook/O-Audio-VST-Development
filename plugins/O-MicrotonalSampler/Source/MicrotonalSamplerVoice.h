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
    static constexpr int kRrCounterSize = 128 * 4;
    using RrCounterArray = std::array<std::atomic<uint8_t>, (size_t) kRrCounterSize>;

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

private:
    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    std::shared_ptr<SampleMap>*                   sampleMapSource     = nullptr;
    RrCounterArray*                               rrCounters          = nullptr;

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
