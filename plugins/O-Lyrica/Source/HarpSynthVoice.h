/*
  ==============================================================================

    HarpSynthVoice.h
    Physical Modeling Harp Synthesizer Voice
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "HarpSynthSound.h"
#include "DSP/WaveguideString.h"
#include "DSP/StringMaterial.h"
#include "DSP/BodyResonance.h"
#include "DSP/SympatheticResonance.h"
#include "DSP/TuningEngine.h"
#include "DSP/GlissandoController.h"

class HarpSynthVoice : public juce::SynthesiserVoice
{
public:
    HarpSynthVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

    /**
     * Prepare voice for playback
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Set APVTS reference for parameter access
     */
    void setAPVTS(juce::AudioProcessorValueTreeState* apvts);

    /**
     * Set sympathetic resonance engine reference (Phase 2.7)
     */
    void setSympatheticEngine(SympatheticResonanceEngine* engine);

    /**
     * Set tuning engine reference (Phase 2.8)
     */
    void setTuningEngine(TuningEngine* engine);

    /**
     * v1.30.0: Set pointer to active glissando mode atomic (owned by processor)
     */
    void setActiveGlissandoMode(std::atomic<int>* modePtr);

    /**
     * v1.30.0: Set pointer to custom degree bitmask (owned by processor)
     */
    void setCustomDegreeMask(std::atomic<uint64_t>* maskPtr);

    /**
     * v1.31.0: Set pointer to host BPM atomic (owned by processor)
     */
    void setHostBpm(std::atomic<double>* bpmPtr);

    /**
     * Get unique voice ID for sympathetic resonance tracking
     */
    int getVoiceId() const;

private:
    /**
     * Update DSP components from APVTS parameters (called each render block)
     * Enables real-time parameter modulation during note playback
     */
    void updateParametersFromAPVTS();

    // Physical modeling string (Phase 2.2 - Bidirectional Waveguide)
    WaveguideString stringModel;

    // Phase 2.6: Body Resonance (modal synthesis)
    BodyResonance bodyResonance;

    // Phase 2.9: Glissando Controller
    GlissandoController glissandoController;

    // APVTS reference for parameter access
    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // Phase 2.7: Sympathetic Resonance Engine (shared, owned by processor)
    SympatheticResonanceEngine* sympatheticEngine = nullptr;

    // Phase 2.8: Tuning Engine (shared, owned by processor)
    TuningEngine* tuningEngine = nullptr;

    // v1.30.0: Active glissando mode atomic (owned by processor)
    std::atomic<int>* activeGlissandoModePtr = nullptr;

    // v1.30.0: Custom degree bitmask pointer (owned by processor)
    std::atomic<uint64_t>* customDegreeMaskPtr = nullptr;

    // v1.31.0: Host BPM pointer (owned by processor, for tempo sync)
    std::atomic<double>* hostBpmPtr = nullptr;

    double currentFrequency = 440.0;
    double previousFrequency = 440.0; // For glissando start point
    float currentVelocity = 0.0f;
    int currentMidiNote = -1; // Current MIDI note number (for pitch bend)
    int voiceId = -1; // Unique ID for sympathetic tracking (v1.3.2: generated from atomic counter)

    // v1.3.2: Static atomic counter for guaranteed unique voice IDs
    static std::atomic<int> nextVoiceId;

    // Phase 2.7: Current material (needed for sympathetic registration)
    StringMaterial currentMaterial;

    // Track current material type to avoid unnecessary DSP updates
    MaterialType currentMaterialType = MaterialType::Nylon;

    // v1.19.0: Random generator for humanization (per-note variation)
    juce::Random humanizeRandom;

    /**
     * v1.19.0: Apply humanization offset to a parameter value
     * @param baseValue Original parameter value (0.0-1.0)
     * @param maxOffset Maximum offset range (e.g., 0.05 = ±5%)
     * @param humanizeAmount Humanize intensity (0.0 = none, 1.0 = full)
     * @return Humanized value clamped to 0.0-1.0
     */
    float applyHumanization(float baseValue, float maxOffset, float humanizeAmount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarpSynthVoice)
};
