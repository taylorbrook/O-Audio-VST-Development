/*
  ==============================================================================

    ModulationMatrix.h
    O-Prism - Microtonal Wavetable Synthesizer
    16-slot modulation routing matrix with per-sample evaluation

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>

// ═══════════════════════════════════════════════════════════════════
// Modulation Source / Destination Enums
// ═══════════════════════════════════════════════════════════════════

enum class ModSource
{
    None = 0,
    LFO1,
    LFO2,
    LFO3,
    LFO4,
    AmpEnv,
    FilterEnv,
    Velocity,
    NoteNum,
    ModWheel,
    Aftertouch,
    NumSources
};

enum class ModDest
{
    None = 0,
    OscAPos,
    OscBPos,
    FiltACutoff,
    FiltBCutoff,
    FiltARes,
    FiltBRes,
    OscMix,
    SubLevel,
    NoiseLevel,
    LFO1Rate,
    LFO2Rate,
    LFO3Rate,
    LFO4Rate,
    OscADetune,
    OscBDetune,
    OscAPan,
    OscBPan,
    ReverbMix,
    DelayMix,
    ChorusMix,
    DistMix,
    MasterVol,
    Pitch,
    NumDests
};

// String arrays for APVTS Choice parameters
inline juce::StringArray getModSourceNames()
{
    return { "None", "LFO1", "LFO2", "LFO3", "LFO4", "AmpEnv", "FilterEnv",
             "Velocity", "NoteNum", "ModWheel", "Aftertouch" };
}

inline juce::StringArray getModDestNames()
{
    return { "None", "OscA Pos", "OscB Pos", "FiltA Cut", "FiltB Cut",
             "FiltA Res", "FiltB Res", "Osc Mix", "Sub Level", "Noise Level",
             "LFO1 Rate", "LFO2 Rate", "LFO3 Rate", "LFO4 Rate",
             "OscA Detune", "OscB Detune",
             "OscA Pan", "OscB Pan", "Reverb Mix", "Delay Mix",
             "Chorus Mix", "Dist Mix", "Master Vol", "Pitch" };
}

// ═══════════════════════════════════════════════════════════════════
// Modulation Matrix
// ═══════════════════════════════════════════════════════════════════

class ModulationMatrix
{
public:
    static constexpr int kNumSlots = 16;
    static constexpr int kNumSources = static_cast<int> (ModSource::NumSources);
    static constexpr int kNumDests = static_cast<int> (ModDest::NumDests);

    ModulationMatrix() = default;

    /** Bind APVTS so slots can read their source/dest/amount/enabled params */
    void setAPVTS (juce::AudioProcessorValueTreeState* apvts);

    /** Read all slot configurations from APVTS (call once per block) */
    void updateFromAPVTS();

    /** Set a source value for the current sample (call before evaluate) */
    void setSourceValue (ModSource source, float value);

    /** Evaluate all active routes and accumulate per-destination offsets */
    void evaluate();

    /** Get the accumulated modulation offset for a destination */
    float getModOffset (ModDest dest) const;

    /** Reset all destination offsets to zero */
    void clearOffsets();

private:
    struct SlotState
    {
        int source = 0;
        int dest = 0;
        float amount = 0.0f;
        bool enabled = false;
    };

    struct CachedSlotParams
    {
        std::atomic<float>* src = nullptr;
        std::atomic<float>* dst = nullptr;
        std::atomic<float>* amt = nullptr;
        std::atomic<float>* on  = nullptr;
    };

    std::array<CachedSlotParams, kNumSlots> cachedParams {};
    std::array<SlotState, kNumSlots> slots {};
    std::array<float, kNumSources> sourceValues {};
    std::array<float, kNumDests> destOffsets {};
};
