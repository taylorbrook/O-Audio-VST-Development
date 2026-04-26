/*
  ==============================================================================

    PrismVoice.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "dsp/WavetableOscillator.h"
#include "dsp/SubOscillator.h"
#include "dsp/NoiseGenerator.h"
#include "dsp/GlideProcessor.h"
#include "dsp/SVFFilter.h"
#include "dsp/LFO.h"
#include "dsp/ModulationMatrix.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (PendingTuningTable + helpers)

class TuningEngine;
class PrismSound;
class OPrismAudioProcessor;
struct WavetableData;

class PrismVoice : public juce::SynthesiserVoice
{
public:
    PrismVoice();

    void setAPVTS (juce::AudioProcessorValueTreeState* apvts);
    void setTuningEngine (TuningEngine* engine);
    void setProcessor (OPrismAudioProcessor* proc);
    void prepare (double sampleRate, int samplesPerBlock);

    /** Set pointer to the module-owned pending-tuning table (128 MIDI slots,
        semitones). Voice reads-and-clears its slot in startNote() to apply
        Dorico's VST3 Note Expression tuning delta before the first sample. */
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* source)
    {
        pendingTuningSource = source;
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void setWavetableA (const WavetableData* table);
    void setWavetableB (const WavetableData* table);

private:
    juce::AudioProcessorValueTreeState* parameters = nullptr;
    TuningEngine* tuningEngine = nullptr;
    OPrismAudioProcessor* processor = nullptr;

    // VST3 Note Expression: pending tuning deltas (semitones) — module-owned table
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    // Modulation matrix (per-voice for per-sample evaluation)
    ModulationMatrix modMatrix;

    // ─── Cached APVTS parameter pointers (set once in setAPVTS) ─────
    // Glide
    std::atomic<float>* pGlideMode = nullptr;
    std::atomic<float>* pGlideTime = nullptr;
    // Osc A
    std::atomic<float>* pOscACoarse = nullptr;
    std::atomic<float>* pOscAFine = nullptr;
    std::atomic<float>* pOscAUnison = nullptr;
    std::atomic<float>* pOscADetune = nullptr;
    std::atomic<float>* pOscAWidth = nullptr;
    std::atomic<float>* pOscAPos = nullptr;
    std::atomic<float>* pOscALevel = nullptr;
    std::atomic<float>* pOscAPan = nullptr;
    // Osc A warp
    std::atomic<float>* pOscAWarpType = nullptr;
    std::atomic<float>* pOscAWarpAmt = nullptr;
    // Osc B
    std::atomic<float>* pOscBCoarse = nullptr;
    std::atomic<float>* pOscBFine = nullptr;
    std::atomic<float>* pOscBUnison = nullptr;
    std::atomic<float>* pOscBDetune = nullptr;
    std::atomic<float>* pOscBWidth = nullptr;
    std::atomic<float>* pOscBPos = nullptr;
    std::atomic<float>* pOscBLevel = nullptr;
    std::atomic<float>* pOscBPan = nullptr;
    // Osc B warp
    std::atomic<float>* pOscBWarpType = nullptr;
    std::atomic<float>* pOscBWarpAmt = nullptr;
    // Osc mix
    std::atomic<float>* pOscMix = nullptr;
    // Sub & Noise
    std::atomic<float>* pSubShape = nullptr;
    std::atomic<float>* pSubOctave = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pSubRouting = nullptr;
    std::atomic<float>* pNoiseType = nullptr;
    std::atomic<float>* pNoiseLevel = nullptr;
    // Filter A
    std::atomic<float>* pFiltAType = nullptr;
    std::atomic<float>* pFiltACutoff = nullptr;
    std::atomic<float>* pFiltARes = nullptr;
    std::atomic<float>* pFiltADrive = nullptr;
    std::atomic<float>* pFiltAKeyTrack = nullptr;
    // Filter B
    std::atomic<float>* pFiltBType = nullptr;
    std::atomic<float>* pFiltBCutoff = nullptr;
    std::atomic<float>* pFiltBRes = nullptr;
    std::atomic<float>* pFiltBDrive = nullptr;
    std::atomic<float>* pFiltBKeyTrack = nullptr;
    // Filter routing
    std::atomic<float>* pFiltRouting = nullptr;
    std::atomic<float>* pFiltAEnvDepth = nullptr;
    std::atomic<float>* pFiltBEnvDepth = nullptr;
    // Envelopes
    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;
    std::atomic<float>* pFiltAttack = nullptr;
    std::atomic<float>* pFiltDecay = nullptr;
    std::atomic<float>* pFiltSustain = nullptr;
    std::atomic<float>* pFiltRelease = nullptr;
    // LFOs
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo1Sync = nullptr;
    std::atomic<float>* pLfo1Division = nullptr;
    std::atomic<float>* pLfo1FreeRun = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;
    std::atomic<float>* pLfo2Sync = nullptr;
    std::atomic<float>* pLfo2Division = nullptr;
    std::atomic<float>* pLfo2FreeRun = nullptr;
    std::atomic<float>* pLfo3Rate = nullptr;
    std::atomic<float>* pLfo3Shape = nullptr;
    std::atomic<float>* pLfo3Sync = nullptr;
    std::atomic<float>* pLfo3Division = nullptr;
    std::atomic<float>* pLfo3FreeRun = nullptr;
    std::atomic<float>* pLfo4Rate = nullptr;
    std::atomic<float>* pLfo4Shape = nullptr;
    std::atomic<float>* pLfo4Sync = nullptr;
    std::atomic<float>* pLfo4Division = nullptr;
    std::atomic<float>* pLfo4FreeRun = nullptr;
    // Velocity
    std::atomic<float>* pVelocityCurve = nullptr;

    double currentFrequency = 0.0;
    float noteVelocity = 0.0f;
    int currentMidiNote = -1;
    int currentPitchWheel = 8192;
    double voiceSampleRate = 44100.0;

    // Oscillators
    WavetableOscillator oscA;
    WavetableOscillator oscB;
    double lastOscAOut = 0.0;
    double lastOscBOut = 0.0;

    // Sub & Noise
    SubOscillator subOsc;
    NoiseGenerator noiseGen;

    // Glide
    GlideProcessor glide;

    // Envelopes
    juce::ADSR ampEnvelope;
    juce::ADSR filterEnvelope;

    // Filters (separate L/R instances for true stereo processing)
    SVFFilter filterAL, filterAR;
    SVFFilter filterBL, filterBR;

    // LFOs (per-voice for smooth per-sample modulation)
    LFO lfo1, lfo2, lfo3, lfo4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismVoice)
};
