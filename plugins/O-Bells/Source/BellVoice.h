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

class BellVoice : public juce::SynthesiserVoice
{
public:
    BellVoice();

    // SynthesiserVoice interface
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>&, int startSample, int numSamples) override;

    // Prepare DSP components
    void prepare(double sampleRate, int samplesPerBlock);

    // Parameter update (called from processor's prepareToPlay or processBlock)
    void updateParameters(float inharmonicity, float damping, float brightness,
                         float strikePosition, float malletHardness, float material,
                         int unisonCount, float unisonDetune,
                         float octaveBlendSub, float octaveBlendOct, float stereoSpread,
                         float partialTuning, float pitchEnvelope, float pitchEnvTime,
                         int decayShape, int velocityCurve, float nonlinearEffects,
                         int strikeNoiseChar, float outputGain);

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

    // Material decay multipliers (from ARCHITECTURE.md)
    static constexpr float MATERIAL_DECAY_BRONZE = 1.0f;
    static constexpr float MATERIAL_DECAY_STEEL = 1.4f;
    static constexpr float MATERIAL_DECAY_GLASS = 2.5f;
    static constexpr float MATERIAL_DECAY_CRYSTAL = 5.0f;

    // Modal partial state
    struct ModalPartial
    {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float amplitude = 0.0f;
        float targetAmplitude = 0.0f;
        float frequency = 0.0f;
        float decayRate = 0.0f;
        bool active = false;
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
    };

    // Voice state
    UnisonVoice fundamentalVoices[MAX_UNISON];
    UnisonVoice subOctaveVoices[MAX_UNISON];
    UnisonVoice upperOctaveVoices[MAX_UNISON];
    StrikeExciter strikeNoise;

    // Current parameters (updated from processor)
    float currentInharmonicity = 0.5f;
    float currentDamping = 0.7f;
    float currentBrightness = 0.5f;
    float currentStrikePosition = 0.5f;
    float currentMalletHardness = 0.5f;
    float currentMaterial = 0.25f;
    int currentUnisonCount = 1;
    float currentUnisonDetune = 10.0f;
    float currentOctaveBlendSub = 0.0f;
    float currentOctaveBlendOct = 0.0f;
    float currentStereoSpread = 0.5f;
    float currentPartialTuning = 0.0f;
    float currentPitchEnvelope = 0.0f;
    float currentPitchEnvTime = 50.0f;
    int currentDecayShape = 1;  // Exponential
    int currentVelocityCurve = 0;  // Linear
    float currentNonlinearEffects = 0.0f;
    int currentStrikeNoiseChar = 0;  // Click
    float currentOutputGain = 0.0f;

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

    // Helper functions
    float calculatePartialFrequency(int partialIndex, float fundamental, float inharmonicity);
    float calculateStrikePositionGain(int partialIndex, float position);
    float calculatePartialAmplitude(int partialIndex, float brightness);
    float applyVelocityCurve(float velocity, int curve);
    float calculateMaterialDecayMultiplier(float material);
    void calculateUnisonDetunes(int count, float detuneAmount);
    void initializePartials(float fundamental, float velocity);
    float generateStrikeNoise();
    float processPitchEnvelope();
    float processPartial(ModalPartial& partial);
};
