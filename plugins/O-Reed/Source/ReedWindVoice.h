/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    ReedWindVoice.h
    O-Reed - Physical Modeling Reed Wind Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/ReedModel.h"
#include "DSP/BoreWaveguide.h"
#include "DSP/BreathEnvelope.h"
#include "DSP/BreathNoise.h"
#include "DSP/MouthpieceChamber.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (PendingTuningTable + helpers)

class TuningEngine;  // Forward declare, avoid header dependency

class ReedWindVoice : public juce::MPESynthesiserVoice
{
public:
    ReedWindVoice(int voiceIndex = 0);

    void setAPVTS(juce::AudioProcessorValueTreeState* apvts);

    // Called from PluginProcessor::prepareToPlay
    void prepare(double sampleRate, int maxBlockSize);

    // --- MPESynthesiserVoice pure virtual overrides ---
    void noteStarted() override;
    void noteStopped(bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;

    // TuningEngine injection (processor-owned, voice holds non-owning pointer)
    void setTuningEngine(TuningEngine* engine) noexcept { tuningEngine = engine; }

    // VST3 Note Expression source (Phase 24): processor-owned, voice holds non-owning pointer.
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source) noexcept
    {
        pendingTuningSource = source;
    }

    float getOversamplingLatency() const noexcept { return oversampling2x.getLatencyInSamples(); }

private:
    // Tuning-aware frequency helper
    float getBaseFrequencyFromTuning(int midiNote) const;
    juce::dsp::Oversampling<float>& getActiveOversampling() noexcept;

    // --- Per-voice oversampling (mono, two instances for runtime 2x/4x switching) ---
    juce::dsp::Oversampling<float> oversampling2x { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    juce::dsp::Oversampling<float> oversampling4x { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    juce::AudioBuffer<float> voiceBuffer;
    int currentOsFactor = 1;  // 1=2x, 2=4x (matches Oversampling factor param)

    // TuningEngine (processor-owned, voice holds non-owning pointer)
    TuningEngine* tuningEngine = nullptr;

    // VST3 Note Expression pending-tuning table (Phase 24): module-owned, voice holds non-owning pointer.
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    // --- DSP Components ---
    ReedModel reedModel;
    BoreWaveguide bore;
    BoreWaveguide bore2;       // Phase 3.4: second parallel bore for dual bore mode
    BreathEnvelope breathEnv;
    BreathNoise breathNoise;
    MouthpieceChamber chamber;

    // Previous bore output for feedback loop
    float prevBoreMinus = 0.0f;
    float prevBore2Minus = 0.0f;  // Phase 3.4: second bore feedback

    // Previous reed flow for noise scaling
    float prevUReed = 0.0f;

    // Per-voice LFO phase state (expression modifiers)
    float vibratoPhase = 0.0f;
    float growlPhase   = 0.0f;
    float flutterPhase = 0.0f;
    float sr = 44100.0f;  // cached sample rate for LFO phase increment

    // Smoothing for mouthpiece volume parameter (to detect active transitions)
    float smoothedMouthpieceVol = 0.0f;
    float paramSmoothCoeff = 0.001f;
    bool chamberWasActive = false;

    // Post-release bore damping: accelerates bore decay after breath envelope goes Off
    float releaseDampGain = 1.0f;       // Current per-sample damping multiplier (1.0 = no damping)
    float releaseDampCoeff = 0.9998f;   // Per-sample decay coefficient (~50ms TC at 88.2kHz)

    // --- APVTS ---
    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // --- Cached APVTS parameter pointers (set once in setAPVTS) ---

    // Primary Controls
    std::atomic<float>* pBreathPressure     = nullptr;
    std::atomic<float>* pEmbouchure         = nullptr;
    std::atomic<float>* pReedHardness       = nullptr;
    std::atomic<float>* pBoreCharacter      = nullptr;
    // instrumentPreset is Choice -- read via getRawParameterValue

    // Secondary Controls
    std::atomic<float>* pReedOpening        = nullptr;
    std::atomic<float>* pBellSize           = nullptr;
    std::atomic<float>* pAirNoise           = nullptr;
    std::atomic<float>* pDoubleReed         = nullptr;
    std::atomic<float>* pBoreDiameter       = nullptr;

    // Advanced / Sound Design
    std::atomic<float>* pReedMass           = nullptr;
    std::atomic<float>* pReedDamping        = nullptr;
    std::atomic<float>* pMouthpieceVol      = nullptr;
    std::atomic<float>* pToneHoleCutoff     = nullptr;
    std::atomic<float>* pRegisterHole       = nullptr;
    std::atomic<float>* pBoreLength         = nullptr;
    // boreProfile is Choice

    // Expressive Controls
    std::atomic<float>* pVibratoDepth       = nullptr;
    std::atomic<float>* pVibratoRate        = nullptr;
    // vibratoSource is Choice
    std::atomic<float>* pGrowlAmount        = nullptr;
    std::atomic<float>* pFlutterTongue      = nullptr;
    std::atomic<float>* pSubtone            = nullptr;
    std::atomic<float>* pAttackChiff        = nullptr;

    // Impossible Physics
    std::atomic<float>* pInfiniteSustain    = nullptr;
    std::atomic<float>* pReverseBore        = nullptr;
    std::atomic<float>* pDualBore           = nullptr;  // Bool stored as float
    std::atomic<float>* pDronePitch         = nullptr;
    std::atomic<float>* pFeedbackPath       = nullptr;

    // Tuning
    std::atomic<float>* pReferencePitch     = nullptr;
    // tuningSystem is Choice

    // Voice Configuration
    // polyMode is Choice
    std::atomic<float>* pMaxVoices          = nullptr;
    // oversampling is Choice

    // Output
    std::atomic<float>* pOutputGain         = nullptr;

    // Choice params (stored as float by APVTS)
    std::atomic<float>* pInstrumentPreset   = nullptr;
    std::atomic<float>* pBoreProfile        = nullptr;
    std::atomic<float>* pVibratoSource      = nullptr;
    std::atomic<float>* pTuningSystem       = nullptr;
    std::atomic<float>* pPolyMode           = nullptr;
    std::atomic<float>* pOversampling       = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReedWindVoice)
};
