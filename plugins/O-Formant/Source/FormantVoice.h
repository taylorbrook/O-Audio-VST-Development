/*
  ==============================================================================

    FormantVoice.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "dsp/LFGlottalSource.h"
#include "dsp/AspirationNoise.h"
#include "dsp/FormantFilterBank.h"
#include "dsp/CascadeFormantBank.h"
#include "dsp/NasalPoleZero.h"
#include "dsp/VowelMorpher.h"
#include "dsp/GlottalWavetable.h"
#include "dsp/VibratoLFO.h"
#include "dsp/PitchGlide.h"
#include "dsp/ConsonantEngine.h"
#include "dsp/FricationFormantBank.h"
#include "dsp/LyricsEngine.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (PendingTuningTable + helpers)

class TuningEngine;

class FormantVoice : public juce::MPESynthesiserVoice
{
public:
    FormantVoice (int voiceIndex = 0);

    void setAPVTS (juce::AudioProcessorValueTreeState* apvts);
    void setWavetable (const GlottalWavetable* wt);
    void setTuningEngine (TuningEngine* te);
    void setLyricsEngine (LyricsEngine* le);
    void setPendingTuningSource (Ouaricon::NoteExpression::PendingTuningTable* source) noexcept { pendingTuningSource = source; }
    void prepare (double sampleRate);

    // --- MPESynthesiserVoice pure virtual overrides ---
    void noteStarted() override;
    void noteStopped (bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

private:
    // --- DSP Components ---
    LFGlottalSource glottalSource;
    AspirationNoise aspirationNoise;
    FormantFilterBank filterBank;
    CascadeFormantBank cascadeBank;
    NasalPoleZero nasalPoleZero;
    VowelMorpher vowelMorpher;
    VibratoLFO vibratoLFO;
    PitchGlide pitchGlide;
    ConsonantEngine consonantEngine;
    FricationFormantBank fricationBank;
    juce::ADSR adsr;

    // --- Voice state ---
    bool voiceActive = false;
    bool wasActive = false;
    int voiceIdx = 0;
    int sampleCounter = 0;
    int releaseSampleCount = -1; // -1 = not in release; >=0 = samples since noteOff

    // Per-voice MPE modulation state
    float mpeBreathOffset = 0.0f;
    float mpeVowelYOffset = 0.0f;
    float noteVelocity = 0.0f;

    // Smoothed Rd for dynamic vocal effort modulation (20ms ramp)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> rdSmoothed;

    // Spectral tilt one-pole filter state
    float spectralTiltPrev = 0.0f;

    // Source-filter coupling state (Titze 2008 harmonic reinforcement)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sourceFilterGain;
    float sourceFilterJitterBoost = 0.0f;

    // Block-rate formant data (updated every kCoeffUpdateInterval samples)
    static constexpr int kCoeffUpdateInterval = 32;
    float formantFreqs[5] = {};
    float formantBWs[5] = {};
    float formantGains[5] = {};

    // Consonant-to-vowel F2/F3 locus transition
    // (Delattre-Liberman-Cooper 1955; Kewley-Port 1982 τ=15ms exp. decay)
    bool  consonantTransitionActive = false;
    int   consonantTransitionSamples = 0;
    int   consonantTransitionMaxSamples = 0; // ~50ms in samples, set in prepare()
    float consonantTauSamples = 1.0f;        // τ=15ms in samples, set in prepare()
    float consonantF2Locus = 1800.0f;
    float consonantF3Locus = 2700.0f;
    float consonantTransitionAmount = 0.5f;  // cached per-block from APVTS

    // --- Tuning ---
    TuningEngine* tuningEnginePtr = nullptr;
    float tunedF0 = 440.0f; // cached tuned frequency for current note

    // VST3 Note Expression (Dorico microtonal — module-owned table)
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    // --- Lyrics ---
    LyricsEngine* lyricsEnginePtr = nullptr;
    LyricsEngine::SyllableTarget currentSyllable;
    bool lyricsActive = false;  // cached per-note: was lyrics enabled at note-on?

    // --- APVTS ---
    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // --- Cached APVTS parameter pointers (set once in setAPVTS) ---

    // Vowel Morph
    std::atomic<float>* pVowelX        = nullptr;
    std::atomic<float>* pVowelY        = nullptr;
    std::atomic<float>* pVowelFocus    = nullptr;

    // Glottal Source
    std::atomic<float>* pGlottalRd     = nullptr;
    std::atomic<float>* pBreathiness   = nullptr;
    std::atomic<float>* pVibratoRate   = nullptr;
    std::atomic<float>* pVibratoDepth  = nullptr;
    std::atomic<float>* pVibratoDelay  = nullptr;
    std::atomic<float>* pJitter        = nullptr;
    std::atomic<float>* pShimmer       = nullptr;
    std::atomic<float>* pRdModDepth    = nullptr;
    std::atomic<float>* pSpectralTilt  = nullptr;

    // Consonant / Noise
    std::atomic<float>* pConsonantLevel   = nullptr;
    std::atomic<float>* pConsonantTone    = nullptr;
    std::atomic<float>* pSibilance        = nullptr;
    std::atomic<float>* pConsonantVoicing = nullptr;
    std::atomic<float>* pAutoConsonant    = nullptr;
    std::atomic<float>* pConsonantAttack  = nullptr;
    std::atomic<float>* pConsonantHold    = nullptr;
    std::atomic<float>* pConsonantDecay   = nullptr;
    std::atomic<float>* pConsonantVOT     = nullptr;
    std::atomic<float>* pConsonantTransition = nullptr;

    // Envelope
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pDecay   = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;

    // Voice Character
    std::atomic<float>* pFormantTopology = nullptr;
    std::atomic<float>* pFormantShift  = nullptr;
    std::atomic<float>* pFormantSpread = nullptr;
    std::atomic<float>* pPitchGlide    = nullptr;
    std::atomic<float>* pTransitionTime = nullptr;
    std::atomic<float>* pSourceFilterCoupling = nullptr;
    std::atomic<float>* pSingersFormant = nullptr;

    // Nasal
    std::atomic<float>* pNasalCoupling = nullptr;
    std::atomic<float>* pNasalPlace    = nullptr;

    // Lyrics
    std::atomic<float>* pLyricsEnabled = nullptr;

    // Output
    std::atomic<float>* pOutputGain   = nullptr;
    std::atomic<float>* pStereoWidth  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FormantVoice)
};
