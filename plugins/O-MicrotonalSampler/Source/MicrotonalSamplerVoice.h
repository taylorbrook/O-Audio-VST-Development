/*
  ==============================================================================

    MicrotonalSamplerVoice.h
    Microtonal Sample Engine - Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1 surface: cubic-Hermite varispeed read + ADSR + NE consumption.
    Phase 2.3: dual-slot equal-power velocity-layer crossfade.
    Phase 2.4: voice-steal tail-ramp scratch buffers (5 ms linear-down).
    Loop wrap (2.5) lands later.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>
#include "TuningEngine.h"          // global namespace (D-4)
#include "NoteExpression.h"        // resolved via ouaricon_add_module include path
#include "SampleMap.h"
#include "MicrotonalSamplerSound.h"

class MicrotonalSamplerVoice : public juce::SynthesiserVoice
{
public:
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

    // Phase 2.1: prepare ADSR sample-rate before audio runs (RESEARCH pitfall #1).
    // Called from OMicrotonalSamplerAudioProcessor::prepareToPlay.
    void prepareToPlay (double sampleRate, int samplesPerBlock);

    // JUCE's Synthesiser may also reset the playback sample rate via this hook.
    // Override to keep ADSR's sample rate synchronized (RESEARCH pitfall #1).
    void setCurrentPlaybackSampleRate (double newRate) override;

    // Wiring setters (called once per voice from PluginProcessor ctor).
    void setAPVTS               (juce::AudioProcessorValueTreeState* p) { parameters = p; }
    void setTuningEngine        (TuningEngine* engine)                  { tuningEngine = engine; }
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* src) { pendingTuningSource = src; }
    void setSampleMapSource     (std::shared_ptr<SampleMap>* src)       { sampleMapSource = src; }

private:
    // Wiring (from setters)
    juce::AudioProcessorValueTreeState*           parameters          = nullptr;
    TuningEngine*                                 tuningEngine        = nullptr;  // D-4: global namespace
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;
    std::shared_ptr<SampleMap>*                   sampleMapSource     = nullptr;

    // Per-voice DSP state (Phase 2.1 + 2.3 dual-slot crossfade)
    juce::ADSR                  adsr;
    double                      currentFrequency   = 0.0;
    int                         currentMidiNote    = -1;
    std::shared_ptr<SampleMap>  currentMap;  // lifetime owner snapshot (RESEARCH pitfall #5)

    // Phase 2.3: dual-slot equal-power velocity-layer crossfade.
    // When slotHigh == nullptr the voice plays slotLow at full weight (single-
    // layer path, identical to Phase 2.1). When both are set, the renderer
    // mixes them per cubicInterp(slotLow,...) * wLow + cubicInterp(slotHigh,...) * wHigh.
    // posLow/posHigh and playRateLow/playRateHigh are independent because the
    // two slots may differ in sourceSampleRate (defensive — usually identical).
    const SampleSlot*           slotLow            = nullptr;
    const SampleSlot*           slotHigh           = nullptr;
    float                       layerWeightLow     = 1.0f;
    float                       layerWeightHigh    = 0.0f;
    double                      posLow             = 0.0;
    double                      posHigh            = 0.0;
    double                      playRateLow        = 1.0;
    double                      playRateHigh       = 1.0;

    // Phase 2.4: voice-steal tail-ramp scratch buffers. When startNote detects
    // an already-active voice, renderTailRamp captures kMaxStealRamp samples
    // of OLD-note audio × linear-down ramp into these buffers; renderNextBlock
    // then mixes them additively on top of the new note for stealTailSamplesRemaining
    // blocks. Sized in prepareToPlay (message-thread alloc, RESEARCH pitfall #6).
    std::vector<float> stealTailBufferL;
    std::vector<float> stealTailBufferR;
    int                stealTailSamplesRemaining = 0;
    int                kMaxStealRamp             = 0;

    void renderTailRamp (int rampSamples) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrotonalSamplerVoice)
};
