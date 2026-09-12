/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    StrataVoice.h
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "dsp/TerrainOscillator.h"
#include "dsp/SubOscillator.h"
#include "dsp/NoiseGenerator.h"
#include "dsp/GlideProcessor.h"
#include "dsp/SVFFilter.h"
#include "dsp/LFO.h"
#include "dsp/ModulationMatrix.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (PendingTuningTable + helpers)

class TuningEngine;
class StrataSound;
class OStrataAudioProcessor;

class StrataVoice : public juce::SynthesiserVoice
{
public:
    StrataVoice();

    void setAPVTS (juce::AudioProcessorValueTreeState* apvts);
    void setTuningEngine (TuningEngine* engine);
    void setProcessor (OStrataAudioProcessor* proc);
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

    /** Index in the processor's voice pool — folded into the harness phase seed (plan Decision 8). */
    void setVoiceIndex (int i) { voiceIndex = i; }

    /** Core 10 rings (one per oscillator). Every voice points at them; only the
        display voice (currentMidiNote == processor->getLastPlayedNote()) writes. */
    void setCaptureTargets (CycleCapture* a, CycleCapture* b) { captureA = a; captureB = b; }

    /** The processor's published Chebyshev sets / images for this block (plan
        Decision 33) — stored here, handed to the oscillators at block start. */
    void setPublished (const ChebyshevSet* a, const ChebyshevSet* b, const TerrainImage* ia, const TerrainImage* ib)
    {
        chebA = a; chebB = b; imgA = ia; imgB = ib;
    }

    // Row order of the 24 smoothed base values (plan Decision 6; reused by the
    // Phase 2.3 processor ramp rows): 0 Pos, 1 OrbAspect, 2 OrbRot, 3 OrbCX,
    // 4 OrbCY, 5 OrbMod, 6 TerFreq (log2), 7 TerModX, 8 TerModY, 9 OrbFeedback,
    // 10 TerSat for osc A; 11–21 the same for osc B; 22 ModWheel; 23 Aftertouch.
    static constexpr int kNumRamps = 24;
    static constexpr int kRampsPerOsc = 11;

private:
    juce::AudioProcessorValueTreeState* parameters = nullptr;
    TuningEngine* tuningEngine = nullptr;
    OStrataAudioProcessor* processor = nullptr;

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
    std::atomic<float>* pOscAPhase = nullptr;
    std::atomic<float>* pOscAUnison = nullptr;
    std::atomic<float>* pOscADetune = nullptr;
    std::atomic<float>* pOscAWidth = nullptr;
    std::atomic<float>* pOscAPos = nullptr;
    std::atomic<float>* pOscALevel = nullptr;
    std::atomic<float>* pOscAPan = nullptr;
    // Osc A warp
    std::atomic<float>* pOscAWarpType = nullptr;
    std::atomic<float>* pOscAWarpAmt = nullptr;
    // Osc A terrain / orbit (parameter-spec.md v2 rows 12–28; all 17 cached; Terrain Blur is read by the TerrainScheduler, not here)
    std::atomic<float>* pOscATerrain = nullptr;
    std::atomic<float>* pOscATerFreq = nullptr;
    std::atomic<float>* pOscATerModX = nullptr;
    std::atomic<float>* pOscATerModY = nullptr;
    std::atomic<float>* pOscATerTrack = nullptr;
    std::atomic<float>* pOscATerSat = nullptr;
    std::atomic<float>* pOscATerBlur = nullptr;
    std::atomic<float>* pOscATerEdge = nullptr;
    std::atomic<float>* pOscAOrbit = nullptr;
    std::atomic<float>* pOscAOrbAspect = nullptr;
    std::atomic<float>* pOscAOrbRot = nullptr;
    std::atomic<float>* pOscAOrbCX = nullptr;
    std::atomic<float>* pOscAOrbCY = nullptr;
    std::atomic<float>* pOscAOrbMod = nullptr;
    std::atomic<float>* pOscAOrbFeedback = nullptr;
    std::atomic<float>* pOscAOrbFbDamp = nullptr;
    std::atomic<float>* pOscAQuality = nullptr;
    // Osc B
    std::atomic<float>* pOscBCoarse = nullptr;
    std::atomic<float>* pOscBFine = nullptr;
    std::atomic<float>* pOscBPhase = nullptr;
    std::atomic<float>* pOscBUnison = nullptr;
    std::atomic<float>* pOscBDetune = nullptr;
    std::atomic<float>* pOscBWidth = nullptr;
    std::atomic<float>* pOscBPos = nullptr;
    std::atomic<float>* pOscBLevel = nullptr;
    std::atomic<float>* pOscBPan = nullptr;
    // Osc B warp
    std::atomic<float>* pOscBWarpType = nullptr;
    std::atomic<float>* pOscBWarpAmt = nullptr;
    // Osc B terrain / orbit
    std::atomic<float>* pOscBTerrain = nullptr;
    std::atomic<float>* pOscBTerFreq = nullptr;
    std::atomic<float>* pOscBTerModX = nullptr;
    std::atomic<float>* pOscBTerModY = nullptr;
    std::atomic<float>* pOscBTerTrack = nullptr;
    std::atomic<float>* pOscBTerSat = nullptr;
    std::atomic<float>* pOscBTerBlur = nullptr;
    std::atomic<float>* pOscBTerEdge = nullptr;
    std::atomic<float>* pOscBOrbit = nullptr;
    std::atomic<float>* pOscBOrbAspect = nullptr;
    std::atomic<float>* pOscBOrbRot = nullptr;
    std::atomic<float>* pOscBOrbCX = nullptr;
    std::atomic<float>* pOscBOrbCY = nullptr;
    std::atomic<float>* pOscBOrbMod = nullptr;
    std::atomic<float>* pOscBOrbFeedback = nullptr;
    std::atomic<float>* pOscBOrbFbDamp = nullptr;
    std::atomic<float>* pOscBQuality = nullptr;
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
    double voiceSampleRate = 44100.0;

    // Oscillators (live wave-terrain, Phase 2.1)
    TerrainOscillator oscA;
    TerrainOscillator oscB;
    int voiceIndex = 0;
    CycleCapture* captureA = nullptr;
    CycleCapture* captureB = nullptr;
    const ChebyshevSet* chebA = nullptr;
    const ChebyshevSet* chebB = nullptr;
    const TerrainImage* imgA = nullptr;
    const TerrainImage* imgB = nullptr;

    // Core 9 smoothing (QUAL-02) lives in the processor since Phase 2.3 (plan
    // Decision 5): 24 shared ramp rows filled per block, read here by absolute
    // sample index — every voice sees identical, always-current base values and
    // no note-on snap is needed.

    /** Per-sample oscillator feed: ramp rows + mod offsets → the nine per-sample setters. */
    void feedOscillator (TerrainOscillator& osc, int rowBase, int destBase, const float* rowValues);
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StrataVoice)
};
