/*
  ==============================================================================

    O-simpleBeatmaker - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical TR-808/909-lineage step-sequencer drum machine. The internal
    sequencer (Stage 2) emits GM-mapped MIDI note-ons at sample-accurate offsets
    into the SAME MidiBuffer as host MIDI, so "the step grid and the piano roll
    are two views of one MIDI stream" is literally how the code works.

    Stage 1 (Foundation): a silent, loadable shell. Full 42-parameter APVTS plus
    a custom 6x32 step-grid (std::atomic<uint8_t>) persisted in a "PATTERN"
    ValueTree child. No DSP (first audio: Stage 2), no WebView (UI: Stage 3).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

#include "BeatmakerIDs.h"
#include "DrumVoiceEngine.h"
#include "UnifiedTriggerRouter.h"
#include "SequencerClock.h"
#include "TimingFeelEngine.h"
#include "VizAnalyzer.h"

//==============================================================================
class OSimpleBeatmakerAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleBeatmakerAudioProcessor();
    ~OSimpleBeatmakerAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleBeatmaker"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; } // longest voice tail

    //==========================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // APVTS access for the editor (and Stage-2 DSP param push).
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    //==========================================================================
    // Viz tap access (Stage-3 editor Timer drains this; Stage 2 just feeds it).
    OSimpleBeatmaker::VizAnalyzer& getVizAnalyzer() noexcept { return viz; }

#if OUARICON_BUILD_TESTS
    //==========================================================================
    // Render-harness test hooks (compiled ONLY into the offline render-test —
    // the shipping plugin target never defines OUARICON_BUILD_TESTS, so there is
    // no allocation/hook in the real audio path).
    struct TestEmittedHit
    {
        int         voiceIndex, stepIndex, velocity, source;
        juce::int64 blockStartAbs;          // absolute sample of the emitting block
        int         finalOffsetInBlock;     // EXACT arg passed to sequencerMidi.addEvent
        int         appliedOffsetSamples;   // baked Δt
        juce::int32 nominalSampleInBar, appliedSampleInBar;  // EXACT values pushed to VizEvent
    };
    const std::vector<TestEmittedHit>& getTestEmittedHits() const noexcept { return testEmittedHits; }
    const juce::MidiBuffer&            getLastSequencerMidi() const noexcept { return sequencerMidi; }
#endif

    //==========================================================================
    // Step-grid API. Cells hold 0 (off) or 1-127 (on at that velocity). The UI
    // (Stage 3) writes on the message thread via native functions; the audio
    // thread (Stage 2) reads via atomic load. Lock-free, allocation-free.
    static constexpr int kDefaultStepVelocity = 100;

    void setStep         (int voice, int step, int velocity) noexcept;
    void setStepVelocity (int voice, int step, int velocity) noexcept; // only if cell is on
    void toggleStep      (int voice, int step) noexcept;               // off <-> default vel
    int  getStep         (int voice, int step) const noexcept;
    void clearGrid       () noexcept;

private:
    //==========================================================================
    // Class-scoped aliases so in-class and member-function bodies can use the
    // grid dimensions unqualified (they live in namespace OSimpleBeatmaker).
    static constexpr int kNumVoices = OSimpleBeatmaker::kNumVoices;
    static constexpr int kMaxSteps  = OSimpleBeatmaker::kMaxSteps;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Custom step-grid state (NOT APVTS). Flat row-major: index = voice*kMaxSteps + step.
    std::array<std::atomic<uint8_t>, (size_t) (kNumVoices * kMaxSteps)> grid;

    static constexpr int cellIndex (int voice, int step) noexcept { return voice * kMaxSteps + step; }
    static bool inRange (int voice, int step) noexcept
    {
        return voice >= 0 && voice < kNumVoices && step >= 0 && step < kMaxSteps;
    }

    // PATTERN <-> ValueTree (de)serialization helpers.
    juce::ValueTree buildPatternTree() const;
    void            restorePatternTree (const juce::ValueTree& pattern); // message thread; may allocate

    double currentSampleRate = 44100.0;

    //==========================================================================
    // Stage-2 DSP spine (all audio-thread-owned).
    OSimpleBeatmaker::DrumVoiceEngine     voices;
    OSimpleBeatmaker::UnifiedTriggerRouter router;
    OSimpleBeatmaker::SequencerClock       clock;
    OSimpleBeatmaker::TimingFeelEngine     feel;
    OSimpleBeatmaker::VizAnalyzer          viz;

    juce::SmoothedValue<float> outputGain { 1.0f };
    juce::MidiBuffer           sequencerMidi;          // emitted + merged stream

    // Firing-column scratch (fixed; enumerated once per block, alloc-free).
    static constexpr int kMaxFiringColumns = 128;
    std::array<OSimpleBeatmaker::FiringColumn, (size_t) kMaxFiringColumns> firingColumns;

    // Carry-over queue for late (Fallback A) hits that cross the block end.
    struct PendingHit
    {
        juce::int64 absTarget = 0;
        int         voiceIndex = 0, stepIndex = 0, velocity = 0;
        juce::int32 nominalSampleInBar = 0, appliedSampleInBar = 0;
        int         appliedOffsetSamples = 0;
    };
    static constexpr int kMaxPending = 64;
    std::array<PendingHit, (size_t) kMaxPending> pending;
    int         pendingCount  = 0;
    juce::int64 absSamplePos  = 0;                     // running absolute sample counter

    // Cached APVTS atomic pointers (read once/block, no String lookup in audio).
    std::atomic<float>* pSwing = nullptr;
    std::atomic<float>* pHumanize = nullptr;
    std::atomic<float>* pQuant = nullptr;
    std::atomic<float>* pPatternLen = nullptr;
    std::atomic<float>* pTempo = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::array<std::atomic<float>*, (size_t) kNumVoices> pTune {}, pDecay {}, pTone {}, pLevel {}, pMute {}, pSolo {};

    std::array<bool, (size_t) kNumVoices> muteArr {}, soloArr {};

    void cacheParamPointers();
    int  patternLengthSteps() const noexcept;          // choice idx -> 8/16/32

    void emitSequencerHit (int voiceIndex, int stepIndex, int offsetInBlock, int finalVel,
                           juce::int32 nominalSampleInBar, juce::int32 appliedSampleInBar,
                           int appliedOffsetSamples, juce::int64 blockStartAbs) noexcept;
    void drainCarryOver (juce::int64 blockStartAbs, int numSamples) noexcept;

#if OUARICON_BUILD_TESTS
    std::vector<TestEmittedHit> testEmittedHits;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleBeatmakerAudioProcessor)
};
