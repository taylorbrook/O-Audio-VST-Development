/*
  ==============================================================================

    MicrotonalSamplerVoice.h
    Microtonal Sample Engine - Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1 surface: cubic-Hermite varispeed read + ADSR + NE consumption.
    Layer crossfade (2.3), voice-steal ramp (2.4), loop wrap (2.5) land later.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <memory>
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

    // Per-voice DSP state (Phase 2.1)
    juce::ADSR                  adsr;
    double                      pos                = 0.0;
    double                      playRate           = 1.0;
    double                      currentFrequency   = 0.0;
    int                         currentMidiNote    = -1;
    const SampleSlot*           currentSlot        = nullptr;
    std::shared_ptr<SampleMap>  currentMap;  // lifetime owner snapshot (RESEARCH pitfall #5)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrotonalSamplerVoice)
};
