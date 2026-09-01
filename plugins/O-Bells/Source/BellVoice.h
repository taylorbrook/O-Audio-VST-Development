/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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

    BellVoice.h
    O-Bells - Physical Modeling Bell Synthesizer
    Modal synthesis voice implementation

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "BellSound.h"
#include "NoteExpression.h"  // modules/tuning/note-expression

class TuningEngine;

class BellVoice : public juce::SynthesiserVoice
{
public:
    BellVoice();

    // SynthesiserVoice interface
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int, int) override {}

    // v3.0.0: Tuning engine integration
    void setTuningEngine(TuningEngine* engine) { tuningEngine = engine; }

    // VST3 Note Expression (kTuningTypeID) — module-owned pending table source.
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source)
    {
        pendingTuningSource = source;
    }
    void renderNextBlock(juce::AudioBuffer<float>&, int startSample, int numSamples) override;

    // Prepare DSP components
    void prepare(double sampleRate, int samplesPerBlock);

    // v3.1.2: High Fidelity mode toggle (disables voice culling)
    void setHighFidelity(bool enabled) { currentHighFidelity = enabled; }

    // Parameter update (called from processor's prepareToPlay or processBlock)
    void updateParameters(float inharmonicity, float damping, float overtoneBrightness, float acousticBrightness,
                         float airAbsorption, float airAbsorptionTime,
                         float strikePosition, float malletHardness, float material,
                         float bloomSpeed, float bloomAmount,
                         bool bloomFineEnabled,
                         float bloomSpeedLow, float bloomSpeedMid, float bloomSpeedHigh,
                         float bloomAmountLow, float bloomAmountMid, float bloomAmountHigh,
                         float shimmer,
                         int unisonCount, float unisonDetune,
                         float octaveBlendSub, float octaveBlendOct, float stereoSpread,
                         float partialTuning, float pitchEnvelope, float pitchEnvTime,
                         int velocityCurve, float nonlinearEffects,
                         int strikeNoiseChar, float attackLevel, float outputGain,
                         float strikeTime, float brilliance, float bodyTime, float humSustain,
                         float humanize);

private:
    // Modal synthesis configuration
    static constexpr int NUM_PARTIALS = 8;
    static constexpr int MAX_UNISON = 4;

    // Partial ratio tables (from ARCHITECTURE.md)
    static constexpr float harmonicRatios[NUM_PARTIALS] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
    static constexpr float bellRatios[NUM_PARTIALS] = {0.5f, 1.0f, 2.4f, 3.0f, 4.0f, 5.2f, 6.0f, 8.0f};
    static constexpr float gamelanRatios[NUM_PARTIALS] = {0.5f, 1.0f, 2.1f, 3.5f, 4.8f, 5.8f, 7.2f, 9.5f};

    // Decay multipliers per partial (higher partials decay faster)
    static constexpr float DECAY_MULTIPLIERS[NUM_PARTIALS] = {1.2f, 1.0f, 0.85f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f};

    // Material properties (v1.2.0 research-based system)
    struct MaterialProperties
    {
        float decayMultiplier;      // Overall sustain (1.0 = baseline)
        float brightnessOffset;     // Spectral tilt (-0.2 to +0.2)
        float inharmonicity;        // Partial stretch (0.0 = harmonic, higher = more inharmonic)
    };

    // Research-based material definitions (5 materials) - v1.3.0 exaggerated for audible differentiation
    static constexpr MaterialProperties MATERIAL_BRONZE     = {1.0f,  0.0f,   0.0f};    // Baseline warm
    static constexpr MaterialProperties MATERIAL_BRASS      = {0.7f,  +0.20f, +0.08f};  // Bright, short, jazzy
    static constexpr MaterialProperties MATERIAL_STEEL      = {2.0f,  +0.25f, -0.05f};  // Very bright, long sustain
    static constexpr MaterialProperties MATERIAL_ALUMINUM   = {0.5f,  +0.30f, +0.12f};  // Very bright, short, thin
    static constexpr MaterialProperties MATERIAL_CAST_IRON  = {1.5f,  -0.25f, +0.15f};  // Dark, long, gamelan-like

    // Modal partial state
    struct ModalPartial
    {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float amplitude = 0.0f;
        float targetAmplitude = 0.0f;
        float frequency = 0.0f;
        bool active = false;

        // Bloom state (v1.2.0)
        float bloomPhase = 0.0f;          // 0.0 to 1.0 during bloom
        float bloomRate = 0.0f;           // Increment per sample
        float initialAmplitude = 0.0f;    // Starting amplitude
        float peakAmplitude = 0.0f;       // Peak after bloom

        // Shimmer state (v1.2.0)
        float shimmerLFOPhase = 0.0f;     // LFO phase accumulator
        float shimmerLFORate = 0.0f;      // LFO frequency (Hz)
        float shimmerDepth = 0.0f;        // Modulation depth in cents
    };

    // Unison voice state
    struct UnisonVoice
    {
        ModalPartial partials[NUM_PARTIALS];
        float detuneAmount = 0.0f;  // In cents
        float panPosition = 0.0f;   // -1.0 (left) to +1.0 (right)
    };

    // Strike exciter state with filtering
    struct StrikeExciter
    {
        float amplitude = 0.0f;
        float decayRate = 0.0f;
        bool active = false;

        // One-pole filter state for noise shaping
        float filterState = 0.0f;
        float filterCoeff = 0.0f;  // 0 = no filter, positive = lowpass, negative = highpass

        // Resonant bandpass state (for Ping)
        float bp1 = 0.0f;
        float bp2 = 0.0f;
        float resonance = 0.0f;
        float centerFreq = 0.0f;

        // Resonant filter bank for impulse response (v1.3.0)
        static constexpr int NUM_RESONATORS = 4;
        juce::dsp::StateVariableTPTFilter<float> resonators[NUM_RESONATORS];
        float resonatorGains[NUM_RESONATORS] = {1.0f, 1.0f, 1.0f, 1.0f};
        int impulseSamplesRemaining = 0;
        float impulseAmplitude = 1.0f;
    };

    // Stereo enhancement state (v1.2.0)
    struct PartialStereoState
    {
        float panLFOPhase = 0.0f;    // Slow LFO for pan movement
        float panLFORate = 0.0f;     // LFO frequency (Hz)
        float basePan = 0.0f;        // Static pan position
    };

    // Voice state
    UnisonVoice fundamentalVoices[MAX_UNISON];
    UnisonVoice subOctaveVoices[MAX_UNISON];
    UnisonVoice upperOctaveVoices[MAX_UNISON];
    StrikeExciter strikeNoise;

    // Stereo enhancement
    PartialStereoState partialStereo[NUM_PARTIALS];

    // Current parameters (updated from processor)
    float currentInharmonicity = 0.5f;
    float currentDamping = 0.7f;
    float currentOvertoneBrightness = 0.5f;   // v2.0.0: renamed from currentBrightness
    float currentAcousticBrightness = 0.7f;   // v2.0.0: new - controls HF decay rate
    float currentAirAbsorption = 0.0f;       // v2.1.0: time-varying lowpass filter
    float currentAirAbsorptionTime = 2.0f;   // v2.2.0: independent time in seconds
    float currentStrikePosition = 0.5f;
    float currentMalletHardness = 0.5f;
    float currentMaterial = 0.0f;  // Default to Bronze (matches parameter default)
    float currentBloomSpeed = 0.0f;    // v1.4.0: Split bloom into speed/amount
    float currentBloomAmount = 0.0f;
    // v1.5.0: Bloom fine controls (per-band override)
    bool currentBloomFineEnabled = false;
    float currentBloomSpeedLow = 0.5f;
    float currentBloomSpeedMid = 0.5f;
    float currentBloomSpeedHigh = 0.5f;
    float currentBloomAmountLow = 0.0f;
    float currentBloomAmountMid = 0.0f;
    float currentBloomAmountHigh = 0.0f;
    float currentShimmer = 0.2f;
    int currentUnisonCount = 1;
    float currentUnisonDetune = 10.0f;
    float currentOctaveBlendSub = 0.0f;
    float currentOctaveBlendOct = 0.0f;
    float currentStereoSpread = 0.5f;
    float currentPartialTuning = 0.0f;
    float currentPitchEnvelope = 0.0f;
    float currentPitchEnvTime = 50.0f;
    int currentVelocityCurve = 0;  // Linear
    float currentNonlinearEffects = 0.0f;
    int currentStrikeNoiseChar = 0;  // Click
    float currentAttackLevel = 0.5f;  // v1.3.0
    float currentOutputGain = 0.0f;
    // Multi-stage envelope parameters (always active in v1.2.0)
    float currentStrikeTime = 30.0f;      // ms
    float currentBrilliance = 50.0f;      // %
    float currentBodyTime = 1500.0f;      // ms
    float currentHumSustain = 50.0f;      // %

    // Humanize parameter (v2.4.0)
    float currentHumanize = 0.3f;         // 0-1, per-note variation amount

    // Per-note variation state (calculated in startNote, applied during note)
    // These represent variation factors (1.0 = no change)
    float noteVariationStrikePos = 1.0f;      // ±5% variation
    float noteVariationMalletHardness = 1.0f; // ±10% variation
    float noteVariationDecay = 1.0f;          // ±15% variation
    float noteVariationAttack = 1.0f;         // ±20% variation
    float noteVariationInharmonicity = 1.0f;  // ±3% variation

    // v3.0.0: Tuning engine (owned by processor)
    TuningEngine* tuningEngine = nullptr;

    // VST3 Note Expression pending-tuning source (owned by processor's vst3Extensions)
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    // v3.1.2: Voice-level silence gating
    bool currentHighFidelity = false;
    int silentSampleCount = 0;
    static constexpr float VOICE_SILENCE_THRESHOLD = 0.0005f;
    static constexpr int SILENCE_SAMPLES_TO_KILL = 1024;  // ~23ms at 44.1kHz

    // Voice state
    int currentMidiNote = 0;
    float currentVelocity = 0.0f;
    double currentSampleRate = 44100.0;
    bool noteActive = false;
    bool tailOff = false;

    // Release envelope for gradual fade on note-off
    float releaseEnvelope = 1.0f;
    float releaseRate = 1.0f;  // Multiplier per sample (close to 1.0 = slow release)

    // Pitch envelope state
    float pitchEnvelopePhase = 0.0f;
    float pitchEnvelopeDecayRate = 0.0f;

    // Multi-stage envelope state
    int samplesSinceNoteOn = 0;
    // Pre-calculated decay coefficients per partial (computed once in startNote).
    // IN-13: default-initialized defensively — always written before use, but {} avoids
    // any read of indeterminate memory should the invariant ever change.
    float strikeDecayCoeffs[NUM_PARTIALS] {};
    float bodyDecayCoeffs[NUM_PARTIALS] {};
    float humDecayCoeffs[NUM_PARTIALS] {};

    // Shimmer decay progress tracking (0.0 = start, 1.0 = fully decayed)
    float decayProgress = 0.0f;

    // Mallet attack ramp state (v1.2.0)
    int attackRampSamples = 0;       // Total samples for attack ramp
    int attackRampPosition = 0;      // Current position in ramp

    // Air absorption lowpass filter state (v2.1.0)
    // Two-pole filter (12dB/octave) - cascaded one-poles
    float airLPFilterStateL1 = 0.0f;  // Left channel filter state (pole 1)
    float airLPFilterStateL2 = 0.0f;  // Left channel filter state (pole 2)
    float airLPFilterStateR1 = 0.0f;  // Right channel filter state (pole 1)
    float airLPFilterStateR2 = 0.0f;  // Right channel filter state (pole 2)
    int airAbsorptionElapsedSamples = 0;  // v2.2.0: independent time tracking

    // Helper functions
    float calculatePartialFrequency(int partialIndex, float fundamental, float inharmonicity);
    float calculateStrikePositionGain(int partialIndex, float position);
    float calculatePartialAmplitude(int partialIndex, float brightness);
    float applyVelocityCurve(float velocity, int curve);
    MaterialProperties getMaterialProperties(float material);
    void calculateUnisonDetunes(int count, float detuneAmount);
    void initializePartials(float fundamental, float velocity);
    void initializeBloom(ModalPartial& partial, int partialIndex);
    void applyBloom(ModalPartial& partial);
    void initializeShimmer(ModalPartial& partial, int partialIndex);
    float applyShimmer(ModalPartial& partial, float basePhaseInc);
    void initializeMalletAttack();
    float getPartialPan(int partialIndex);
    void initializeStereoMovement();
    float getModulatedPan(int partialIndex);
    float generateStrikeNoise();
    float processPitchEnvelope();
    void calculateMultiStageCoefficients(float fundamental);
    void applyMultiStageDecay(ModalPartial& partial, int partialIndex);

    // IN-10: per-voice RNG (seeded once in the constructor) instead of the shared
    // juce::Random::getSystemRandom() singleton. startNote is serialized by the
    // Synthesiser's lock so voices never raced each other, but this removes the
    // audio-thread touch of a shared, documented-non-thread-safe generator.
    juce::Random voiceRandom;

    // Gaussian random number generator (CLT approximation)
    float gaussianApprox()
    {
        // Sum of 3 uniform randoms approximates Gaussian (Central Limit Theorem)
        float sum = 0.0f;
        for (int i = 0; i < 3; ++i)
            sum += voiceRandom.nextFloat() * 2.0f - 1.0f;
        return sum / 1.73f;  // Scale to ~unit variance
    }
};
