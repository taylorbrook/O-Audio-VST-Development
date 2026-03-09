/*
  ==============================================================================

    WavetableVoice.h
    SynthesiserVoice subclass with wavetable oscillator and ADSR envelope
    Phase 2.2: Multiple sub-oscillators (12 chord voices per main voice)

    Oscillator Count
    ----------------
    Each WavetableVoice holds 12 sub-voices, each with 6 WavetableOscillator
    instances (base/spacing/inversion × oscA/oscB). The PluginProcessor
    allocates 8 voices, giving a total of:

        8 voices × 12 sub-voices × 6 oscillators = 576 WavetableOscillator instances

    Memory / cache implications:
    - Each oscillator holds a wavetable pointer, phase state, and interpolation
      scratch data. At ~64 bytes per oscillator this is ~36 KB of hot state.
    - Sub-voices within a voice are contiguous in the arrays, so per-voice
      iteration is cache-friendly. Cross-voice iteration (e.g. global parameter
      sweeps) will stride across voice objects and may incur cache misses.
    - The wavetable data itself is shared (read-only pointers into WavetableSound),
      so the 576 oscillators do NOT duplicate the actual sample data.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include "WavetableOscillator.h"
#include "WavetableSound.h"
#include "ChordGenerator.h"

class TuningEngine;  // Forward declaration

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
    void prepare(int maxBlockSize);

    // Parameter setters (called from audio thread via atomic reads)
    void setWavetableBank(int bankIndex);
    void setWavetablePosition(float pos);
    void setWavetableBank2(int bankIndex);
    void setWavetablePosition2(float pos);
    void setGainA(float gain);
    void setGainB(float gain);
    void setEnvelopeParameters(float attack, float decay, float sustain, float release);
    void setChordGenerationParams(int voiceCount, float complexity, int keyRoot,
                                   const std::vector<int>& enabledDegrees, int scaleDegreeCount,
                                   float spacing, float inversion,
                                   float detuneRandom, float timingRandom,
                                   ChordGenerator* chordGen, class TuningEngine* tuning,
                                   juce::Random* random);
    void setStereoSpread(float spread);
    void setWavetablePositionWithLFO(float basePos, float lfoPhase, float lfoDepth);
    void setWavetablePosition2WithLFO(float basePos, float lfoPhase, float lfoDepth);

    // UI data access (read from message thread)
    int getActiveSubVoiceCount() const { return activeSubVoices; }

    // Base note info and gain
    const SubVoiceInfo& getSubVoiceInfo(int index) const { return subVoiceInfos[static_cast<size_t>(index)]; }
    float getSubVoiceBaseGain(int index) const
    {
        auto idx = static_cast<size_t>(index);
        return subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx]
               * (1.0f - subVoiceSpacingGains[idx]) * (1.0f - subVoiceInversionGains[idx]);
    }

    // Spacing (up) note info and gain
    const SubVoiceInfo& getSubVoiceSpacingInfo(int index) const { return subVoiceSpacingInfos[static_cast<size_t>(index)]; }
    float getSubVoiceSpacingGain(int index) const
    {
        auto idx = static_cast<size_t>(index);
        return subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx] * subVoiceSpacingGains[idx];
    }

    // Inversion (down) note info and gain
    const SubVoiceInfo& getSubVoiceInversionInfo(int index) const { return subVoiceInversionInfos[static_cast<size_t>(index)]; }
    float getSubVoiceInversionGain(int index) const
    {
        auto idx = static_cast<size_t>(index);
        return subVoiceComplexityGains[idx] * subVoiceVoiceCountGains[idx] * subVoiceInversionGains[idx];
    }

private:
    static constexpr int MAX_SUB_VOICES = 12;

    // Scratch buffers for block-based rendering (heap-allocated in prepare())
    std::vector<float> scratchL;
    std::vector<float> scratchR;
    int preparedBlockSize = 0;

    // Oscillator Set A (primary)
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceOscillators;          // base pitch
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceSpacingOscillators;   // pitched UP (1-3 octaves)
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceInversionOscillators; // pitched DOWN (1-3 octaves)

    // Oscillator Set B (v1.8.0: second wavetable layer)
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceOscillators2;
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceSpacingOscillators2;
    std::array<WavetableOscillator, MAX_SUB_VOICES> subVoiceInversionOscillators2;
    std::array<SubVoiceInfo, MAX_SUB_VOICES> subVoiceInfos{};                     // base note info
    std::array<SubVoiceInfo, MAX_SUB_VOICES> subVoiceSpacingInfos{};              // spacing note info
    std::array<SubVoiceInfo, MAX_SUB_VOICES> subVoiceInversionInfos{};            // inversion note info
    int activeSubVoices = 1;

    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParams;

    double currentSampleRate = 44100.0;
    float currentVelocity = 0.0f;

    // Chord generation context (set each processBlock, used on note-on)
    int cachedVoiceCount = 5;
    float cachedComplexity = 0.5f;
    int cachedKeyRoot = 0;
    std::vector<int> cachedEnabledDegrees;
    int cachedScaleDegreeCount = 12;
    float cachedSpacing = 0.0f;
    float cachedInversion = 0.3f;
    float cachedDetuneRandom = 5.0f;
    float cachedTimingRandom = 10.0f;  // ms
    ChordGenerator* chordGeneratorPtr = nullptr;
    TuningEngine* tuningEnginePtr = nullptr;
    juce::Random* randomPtr = nullptr;

    // Per-sub-oscillator timing delays (in samples)
    std::array<int, MAX_SUB_VOICES> subVoiceDelays{};
    std::array<int, MAX_SUB_VOICES> subVoiceDelayCounters{};

    // Per-sub-voice gain fading (three independent smoothed components)
    std::array<float, MAX_SUB_VOICES> subVoiceComplexityThresholds{};
    std::array<float, MAX_SUB_VOICES> subVoiceComplexityGains{};
    std::array<float, MAX_SUB_VOICES> subVoiceVoiceCountGains{};

    // Per-sub-voice spacing crossfade (base pitch -> up-shifted pitch)
    std::array<float, MAX_SUB_VOICES> subVoiceSpacingThresholds{};   // random per note-on (0-1)
    std::array<float, MAX_SUB_VOICES> subVoiceSpacingGains{};        // smoothed 0=off, 1=active

    // Per-sub-voice inversion crossfade (base pitch -> down-shifted pitch)
    std::array<float, MAX_SUB_VOICES> subVoiceInversionThresholds{};  // random per note-on (0-1)
    std::array<float, MAX_SUB_VOICES> subVoiceInversionGains{};       // smoothed 0=off, 1=active

    float gainSmoothCoeff = 0.001f;

    // v1.9.0: Independent gain per oscillator
    float cachedGainA = 1.0f;
    float cachedGainB = 0.0f;
    float smoothedGainA = 1.0f;
    float smoothedGainB = 0.0f;

    // v1.13.0: Per-sub-voice stereo panning
    float cachedStereoSpread = 0.5f;
    std::array<float, MAX_SUB_VOICES> subVoicePanFactor{};  // Set in startNote, includes random offset
    std::array<float, MAX_SUB_VOICES> smoothedPan{};         // Smoothed pan position (-1 to +1)

    // v1.14.0: Per-sub-voice LFO phase offsets for organic ensemble movement
    std::array<float, MAX_SUB_VOICES> subVoiceLFOPhaseOffsets{};

    // Initialize all 6 oscillators + infos for one sub-voice slot
    void initializeSingleSubVoice(size_t idx,
                                   int baseMidiNote, float baseFreq,
                                   int spacingMidiNote, float spacingFreq,
                                   int inversionMidiNote, float inversionFreq);

    // Resolve MIDI note to frequency using tuning engine (or 12-TET fallback) with cent offset
    float resolveFrequency(int midiNote, double centOffset) const;

    // Multi-octave shift helper (weighted: 60% = 1oct, 30% = 2oct, 10% = 3oct)
    static int getRandomOctaveShift(juce::Random* rng);
};
