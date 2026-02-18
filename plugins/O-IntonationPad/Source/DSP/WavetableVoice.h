/*
  ==============================================================================

    WavetableVoice.h
    SynthesiserVoice subclass with wavetable oscillator and ADSR envelope
    Phase 2.2: Multiple sub-oscillators (12 chord voices per main voice)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include "WavetableOscillator.h"
#include "WavetableSound.h"
#include "ChordGenerator.h"

class TuningSystem;  // Forward declaration

struct SubVoiceInfo
{
    int midiNote = 0;
    float frequencyHz = 0.0f;
};

class WavetableVoice final : public juce::SynthesiserVoice
{
public:
    WavetableVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // Parameter setters (called from audio thread via atomic reads)
    void setWavetablePosition(float pos);
    void setEnvelopeParameters(float attack, float release);
    void setChordGenerationParams(int voiceCount, float complexity, int keyRoot, int keyScale,
                                   float inversionRandom, float detuneRandom, float timingRandom,
                                   ChordGenerator* chordGen, class TuningSystem* tuning,
                                   juce::Random* random);

    // UI data access (read from message thread)
    int getActiveSubVoiceCount() const { return activeSubVoices; }

    // Base (uninverted) note info and gain
    const SubVoiceInfo& getSubVoiceInfo(int index) const { return subVoiceInfos[static_cast<size_t>(index)]; }
    float getSubVoiceBaseGain(int index) const
    {
        auto idx = static_cast<size_t>(index);
        return subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx] * (1.0f - subVoiceInversionGains[idx]);
    }

    // Inverted note info and gain
    const SubVoiceInfo& getSubVoiceInvertedInfo(int index) const { return subVoiceInvertedInfos[static_cast<size_t>(index)]; }
    float getSubVoiceInvertedGain(int index) const
    {
        auto idx = static_cast<size_t>(index);
        return subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx] * subVoiceInversionGains[idx];
    }

private:
    static constexpr int MAX_SUB_VOICES = 12;

    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceOscillators;          // base pitch
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceInvertedOscillators; // inverted pitch (±1 octave)
    std::array<SubVoiceInfo, MAX_SUB_VOICES> subVoiceInfos{};                    // base note info
    std::array<SubVoiceInfo, MAX_SUB_VOICES> subVoiceInvertedInfos{};            // inverted note info
    int activeSubVoices = 1;

    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParams;

    double currentSampleRate = 44100.0;
    float currentVelocity = 0.0f;

    // Chord generation context (set each processBlock, used on note-on)
    int cachedVoiceCount = 5;
    float cachedComplexity = 0.5f;
    int cachedKeyRoot = 0;
    int cachedKeyScale = 0;
    float cachedInversionRandom = 0.3f;
    float cachedDetuneRandom = 5.0f;
    float cachedTimingRandom = 10.0f;  // ms
    ChordGenerator* chordGeneratorPtr = nullptr;
    TuningSystem* tuningSystemPtr = nullptr;
    juce::Random* randomPtr = nullptr;

    // Per-sub-oscillator timing delays (in samples)
    std::array<int, MAX_SUB_VOICES> subVoiceDelays{};
    std::array<int, MAX_SUB_VOICES> subVoiceDelayCounters{};

    // Per-sub-voice gain fading (three independent smoothed components)
    std::array<float, MAX_SUB_VOICES> subVoiceComplexityThresholds{};
    std::array<float, MAX_SUB_VOICES> subVoiceComplexityGains{};
    std::array<float, MAX_SUB_VOICES> subVoiceVoiceCountGains{};

    // Per-sub-voice inversion crossfade (base pitch <-> inverted pitch)
    std::array<float, MAX_SUB_VOICES> subVoiceInversionThresholds{};  // random per note-on (0-1)
    std::array<float, MAX_SUB_VOICES> subVoiceInversionGains{};       // smoothed 0=base, 1=inverted

    float gainSmoothCoeff = 0.001f;
};
