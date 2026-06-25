/*
  ==============================================================================

    O-simpleSampler - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical keyboard sampler.

    Stage 1 (Foundation): silent 16-voice synth shell. Full 21-parameter APVTS +
    state persistence (incl. a custom, non-APVTS loaded-source identity). No audio
    rendering yet (sampler voices / Repitch read head / region / loop / Stretch /
    Vintage / filter / amp ADSR land Stage 2), no WebView UI yet (minimal
    placeholder editor for now — Stage 3 brings the UI).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by the sampler voices / param-push (Stage 2)
// and WebView relays/attachments (Stage 3) later. IDs/ranges/defaults are
// authoritative per .planning/parameter-spec.md (21 params).
namespace OSimpleSampler::ParamIDs
{
    // Source
    inline constexpr auto sourceSample    = "sourceSample";    // choice: piano/vocal/flute/vinyl

    // Region (start/end · loop · reverse)
    // NB: identifiers are regionStart/regionEnd (not start/end) — a bare `end`
    // collides with juce::end (RangedDirectoryIterator) under `using namespace`.
    // The APVTS string IDs stay "start"/"end" (parameter-spec contract unchanged).
    inline constexpr auto regionStart     = "start";           // 0–100 % of source
    inline constexpr auto regionEnd       = "end";             // 0–100 % of source
    inline constexpr auto loopMode        = "loopMode";        // choice: off/forward/ping-pong
    inline constexpr auto loopStart       = "loopStart";       // 0–100 % of region
    inline constexpr auto loopEnd         = "loopEnd";         // 0–100 % of region
    inline constexpr auto loopCrossfade   = "loopCrossfade";   // 0–500 ms
    inline constexpr auto reverse         = "reverse";         // bool

    // Pitch (root · Repitch/Stretch · tune/fine)
    inline constexpr auto rootKey         = "rootKey";         // int MIDI 0–127
    inline constexpr auto pitchMode       = "pitchMode";       // choice: Repitch/Stretch
    inline constexpr auto tune            = "tune";            // int −24–+24 st
    inline constexpr auto fine            = "fine";            // −100–+100 cents

    // Vintage
    inline constexpr auto vintage         = "vintage";         // 0–100 % (bypass at 0)

    // Filter (resonant low-pass)
    inline constexpr auto filterCutoff    = "filterCutoff";    // 20–20000 Hz (log)
    inline constexpr auto filterResonance = "filterResonance"; // 0–100 %

    // Amplitude envelope (per-voice ADSR)
    inline constexpr auto ampAttack       = "ampAttack";       // 0–5 s
    inline constexpr auto ampDecay        = "ampDecay";        // 0–5 s
    inline constexpr auto ampSustain      = "ampSustain";      // 0–1 (0–100 %)
    inline constexpr auto ampRelease      = "ampRelease";      // 0–5 s

    // Voice / Output
    inline constexpr auto velToAmp        = "velToAmp";        // 0–100 %
    inline constexpr auto outputLevel     = "outputLevel";     // master trim (dB)
}

//==============================================================================
class OSimpleSamplerAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleSamplerAudioProcessor();
    ~OSimpleSamplerAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleSampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; } // max amp release

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
    // Public access to APVTS for the editor (Stage 3 relays/attachments).
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Loaded-source identity (custom, non-APVTS state). "embedded:piano" by
    // default, an "embedded:<name>" built-in, or a user file path once Stage 2.3
    // wires real loading. Persisted alongside the APVTS tree so a session restores
    // the same source.
    const juce::String& getSourceIdentity() const noexcept { return currentSourceIdentity; }
    void setSourceIdentity (const juce::String& id) { currentSourceIdentity = id; }

    //==========================================================================
    // Engine constants (declared NOW; consumed by the sampler engine in Stage 2).
    static constexpr int kMaxVoices         = 16;   // polyphony
    static constexpr int kMaxGrainsPerVoice = 4;    // Stretch (synchronous-granular) grain pool
    static constexpr int kRootNote          = 60;   // C3 — key-track reference / default rootKey
    static constexpr int kMaxSourceSeconds  = 30;   // source-length cap
    static constexpr int kStretchGrainMs    = 60;   // Stretch fixed grain size (ms)
    static constexpr int kNumBuiltIns       = 4;    // built-in source count (sourceSample choices)

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Custom non-APVTS state: which source is loaded (built-in name or file path).
    juce::String currentSourceIdentity { "embedded:piano" };

    // Built-in names, indexed to match the sourceSample choice order. Working
    // placeholder set (ARCHITECTURE: ≈4–6 curated found-sounds); finalized with
    // the embedded .wav assets in Stage 2.3.
    static constexpr const char* kBuiltInNames[kNumBuiltIns] = { "piano", "vocal", "flute", "vinyl" };

    //==========================================================================
    // Cached raw-param atomic pointers (assigned in the ctor). Established now;
    // read once per block by the sampler engine in Stage 2. Unused while silent.
    std::atomic<float>* sourceSampleParam    = nullptr;
    std::atomic<float>* startParam           = nullptr;
    std::atomic<float>* endParam             = nullptr;
    std::atomic<float>* loopModeParam        = nullptr;
    std::atomic<float>* loopStartParam       = nullptr;
    std::atomic<float>* loopEndParam         = nullptr;
    std::atomic<float>* loopCrossfadeParam   = nullptr;
    std::atomic<float>* reverseParam         = nullptr;
    std::atomic<float>* rootKeyParam         = nullptr;
    std::atomic<float>* pitchModeParam       = nullptr;
    std::atomic<float>* tuneParam            = nullptr;
    std::atomic<float>* fineParam            = nullptr;
    std::atomic<float>* vintageParam         = nullptr;
    std::atomic<float>* filterCutoffParam    = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* ampAttackParam       = nullptr;
    std::atomic<float>* ampDecayParam        = nullptr;
    std::atomic<float>* ampSustainParam      = nullptr;
    std::atomic<float>* ampReleaseParam      = nullptr;
    std::atomic<float>* velToAmpParam        = nullptr;
    std::atomic<float>* outputLevelParam     = nullptr;

    //==========================================================================
    double currentSampleRate = 44100.0;

    // Custom-state element names for get/setStateInformation.
    static constexpr const char* kSourceStateTag = "SOURCE";
    static constexpr const char* kSourceIdProp   = "identity";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSamplerAudioProcessor)
};
