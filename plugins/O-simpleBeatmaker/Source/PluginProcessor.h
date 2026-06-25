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

//==============================================================================
namespace OSimpleBeatmaker
{
    // Voice roster — row order is the grid's row order AND the GM-map order.
    inline constexpr int kNumVoices = 6;   // Kick Snare Clap ClosedHat OpenHat Tom
    inline constexpr int kMaxSteps  = 32;  // max columns; patternLength picks 8/16/32

    enum Voice { Kick = 0, Snare, Clap, ClosedHat, OpenHat, Tom };

    // General-MIDI drum note per voice (consumed by the Stage-2 trigger router).
    inline constexpr std::array<int, (size_t) kNumVoices> kGmNotes { 36, 38, 39, 42, 46, 45 };

    // lowerCamel prefixes used to compose the 36 per-voice parameter IDs.
    inline constexpr std::array<const char*, (size_t) kNumVoices> kVoicePrefix
        { "kick", "snare", "clap", "closedHat", "openHat", "tom" };

    // Human-readable names (generic editor / Stage-3 labels).
    inline constexpr std::array<const char*, (size_t) kNumVoices> kVoiceName
        { "Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Tom" };

    //==========================================================================
    // APVTS identifiers — single source of truth. IDs/ranges/defaults track
    // parameter-spec.md (which mirrors ARCHITECTURE.md -> Parameter Mapping).
    namespace ParamIDs
    {
        // Sequencer / timing-feel (5)
        inline constexpr auto swing            = "swing";            // 0-1 (display 0-75%)
        inline constexpr auto humanize         = "humanize";         // 0-1 (display 0-100%)
        inline constexpr auto quantizeStrength = "quantizeStrength"; // 0-1 (display 0-100%)
        inline constexpr auto patternLength    = "patternLength";    // choice 8/16/32
        inline constexpr auto tempo            = "tempo";            // 40-240 BPM (free-run)

        // Master (1)
        inline constexpr auto outputLevel      = "outputLevel";      // -60..0 dB

        // Per-voice suffixes — combine with kVoicePrefix for the 36 voice IDs.
        inline constexpr auto sufTune  = "Tune";   // -12..+12 st
        inline constexpr auto sufDecay = "Decay";  // 0-1 (per-voice ms mapped in DSP)
        inline constexpr auto sufTone  = "Tone";   // 0-1 snap/body-noise/brightness
        inline constexpr auto sufLevel = "Level";  // -60..0 dB
        inline constexpr auto sufMute  = "Mute";   // bool
        inline constexpr auto sufSolo  = "Solo";   // bool
    }

    // Compose a per-voice parameter ID, e.g. voiceParamID (Kick, "Tune") -> "kickTune".
    inline juce::String voiceParamID (int voice, const char* suffix)
    {
        return juce::String (kVoicePrefix[(size_t) voice]) + suffix;
    }
}

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleBeatmakerAudioProcessor)
};
