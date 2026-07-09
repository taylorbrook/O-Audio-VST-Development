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
#include "DSP/SympatheticResonance.h"
#include "DSP/TuningEngine.h"
#include "DSP/GlissandoController.h"
#include "NoteExpression.h"  // modules/tuning/note-expression (PendingTuningTable + helpers)

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
     * v1.35.0: Per-voice output buffer for string crosstalk processing
     */
    const juce::AudioBuffer<float>& getVoiceOutputBuffer() const { return voiceOutputBuffer; }
    void clearVoiceOutputBuffer() { voiceOutputBuffer.clear(); }

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
     * Set pointer to the module-owned pending-tuning table (128 MIDI slots,
     * semitones). Voice reads-and-clears its slot in startNote() to apply Dorico's
     * VST3 Note Expression tuning delta before the first sample renders.
     */
    void setPendingTuningSource(Ouaricon::NoteExpression::PendingTuningTable* source);

    /**
     * Get unique voice ID for sympathetic resonance tracking
     */
    int getVoiceId() const;

private:
    /**
     * Update DSP components from APVTS parameters (called each render block)
     * Enables real-time parameter modulation during note playback
     * @param numSamples Size of current render block, used to advance the
     *                   material-change crossfade at block rate (v2.1.9)
     */
    void updateParametersFromAPVTS(int numSamples);

    // Physical modeling string (Phase 2.2 - Bidirectional Waveguide)
    WaveguideString stringModel;

    // Phase 2.9: Glissando Controller
    GlissandoController glissandoController;

    // APVTS reference for parameter access
    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // WR-09: cached raw APVTS parameter pointers, resolved once in setAPVTS(). The per-block
    // updateParametersFromAPVTS() and the per-sample glissando loop previously did string-keyed
    // hashmap lookups (getRawParameterValue("id")) on the audio thread; this mirrors the effects
    // fxCache and reads the atomics directly.
    struct ParamCache
    {
        std::atomic<float>* brightness       = nullptr;
        std::atomic<float>* timbre           = nullptr;
        std::atomic<float>* decayTime        = nullptr;
        std::atomic<float>* stringStiffness  = nullptr;
        std::atomic<float>* stringTension    = nullptr;
        std::atomic<float>* stringGauge      = nullptr;
        std::atomic<float>* stringLength     = nullptr;
        std::atomic<float>* attackNoise      = nullptr;
        std::atomic<float>* bridgeBrightness = nullptr;
        std::atomic<float>* stringMaterial   = nullptr;
    } paramCache;

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

    // VST3 Note Expression: pending tuning deltas (semitones) — module-owned table
    Ouaricon::NoteExpression::PendingTuningTable* pendingTuningSource = nullptr;

    double currentFrequency = 440.0;
    int currentMidiNote = -1; // Current MIDI note number (for pitch bend)
    int glissUpdateCounter = 0; // IN-05: decimates string-model updates during glissando
    int voiceId = -1; // Unique ID for sympathetic tracking (v1.3.2: generated from atomic counter)
    int cachedSympatheticSlot = -1; // v1.32.5: Cached slot index from registerVoice() to avoid per-sample lookup

    // v1.3.2: Static atomic counter for guaranteed unique voice IDs
    static std::atomic<int> nextVoiceId;

    // Phase 2.7: Current material (needed for sympathetic registration)
    StringMaterial currentMaterial;

    // Track current material type to avoid unnecessary DSP updates
    MaterialType currentMaterialType = MaterialType::Nylon;

    // v2.1.9: Material parameter crossfade state
    // When stringMaterial changes mid-note, damping/brightness/stiffness/coupling/noise
    // are interpolated from crossfadeFromMaterial to crossfadeTargetMaterial over ~50ms
    // at block rate, driving stringModel.setMaterial() on each block. Advancing via
    // crossfadeSamplesRemaining (decremented by numSamples each block).
    StringMaterial crossfadeFromMaterial;
    StringMaterial crossfadeTargetMaterial;
    int materialCrossfadeSamplesTotal = 0;
    int materialCrossfadeSamplesRemaining = 0;
    double currentSampleRate = 44100.0;

    // v1.35.0: Per-voice output buffer for crosstalk processing
    juce::AudioBuffer<float> voiceOutputBuffer;

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

    // v1.32.3: Extracted from startNote() for readability
    void setupScaleLockedGlissando(int midiNoteNumber);
    void setupFreeGlissando();
    void applyDirectionExcitation(float& pluckPosition, float& fingerHardness);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarpSynthVoice)
};
