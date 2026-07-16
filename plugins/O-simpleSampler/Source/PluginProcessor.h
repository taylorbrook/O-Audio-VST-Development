/*
  ==============================================================================

    O-simpleSampler - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical keyboard sampler.

    Stage 2.1 (Core Playable Sampler): a polyphonic, MIDI-playable sampler — the
    embedded piano.wav read through a fractional-read varispeed ("Repitch") head,
    isolated by start/end region, anti-aliased, shaped by a per-voice amp ADSR +
    VCA + velocity sensitivity, tuned relative to the live Root Key. Decode /
    resample / atomic-publish of the source happens OFF the audio thread
    (prepareToPlay / AsyncUpdater). Loop / reverse / Stretch / Vintage / filter
    land Phase 2.2; viz taps + render-harness land Phase 2.3. WebView UI Stage 3.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>

// Forward declarations — the custom voice/sound are pulled in only by the .cpp.
// NB: named Sample{Voice,Sound}, NOT Sampler{Voice,Sound} — juce::SamplerVoice /
// juce::SamplerSound exist and JuceHeader.h does `using namespace juce;`, so the
// `Sampler`-prefixed names would be ambiguous (cf. regionStart/regionEnd vs juce::end).
class SampleVoice;
class SampleSound;

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
class OSimpleSamplerAudioProcessor : public juce::AudioProcessor,
                                     private juce::AudioProcessorValueTreeState::Listener,
                                     private juce::AsyncUpdater,
                                     private juce::Timer
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
    // the same source. Returned by VALUE under the publish lock — identity is
    // written from several host threads (CR-02), so a reference would be unsafe.
    juce::String getSourceIdentity() const
    {
        const juce::ScopedLock sl (sourcePublishLock);
        return currentSourceIdentity;
    }
    void setSourceIdentity (const juce::String& id)
    {
        const juce::ScopedLock sl (sourcePublishLock);
        currentSourceIdentity = id;
    }

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
    // Lock-free atomic shared_ptr swap for the source buffer (RESEARCH §4). The
    // audio thread snapshots the pointer ONCE per block and holds the ref for the
    // whole block, so a swap mid-block can never free the buffer WHILE it is being
    // read. The end-of-block release is covered too: the publisher retires the
    // outgoing shared_ptr onto `retiredSources` (below) instead of dropping its
    // reference, so the audio thread's snapshot is never the LAST owner and the
    // multi-MB buffer is only ever freed by the message-thread reaper (CR-01,
    // suite pattern: retired-list freed at use_count()==1). The decode/resample
    // that builds a new buffer always happens off the audio thread (prepareToPlay
    // / AsyncUpdater / setStateInformation — never processBlock).
    // NB: std::atomic_load/store on shared_ptr is deprecated in C++20 and is a
    // spinlock pool on libc++ — bounded (once per block) and accepted suite-wide
    // (IN-02); revisit with std::atomic<std::shared_ptr> on the C++20 migration.
    template <class T>
    static std::shared_ptr<T> atomicLoad (const std::shared_ptr<T>& s) noexcept
    {
        return std::atomic_load (&s);
    }
    template <class T>
    static void atomicStore (std::shared_ptr<T>& s, std::shared_ptr<T> v) noexcept
    {
        std::atomic_store (&s, std::move (v));
    }

    //==========================================================================
    // Source decode/resample/publish (Phase 2.1) — ALL off the audio thread.

    // Decode one embedded BinaryData .wav (by built-in index), resample to the
    // engine rate, cap at kMaxSourceSeconds, and atomic-publish. Returns true on
    // success. OFF the audio thread (prepareToPlay / sourceSample change).
    bool loadBuiltInSource (int builtInIndex, double engineRate);

    // Map an "embedded:<name>" identity to its built-in index (0..kNumBuiltIns-1).
    // Falls back to the live sourceSample choice, then 0 (piano), if unknown.
    int builtInIndexForIdentity (const juce::String& identity) const;

    // Decode a raw byte block (a complete .wav/.aiff/.flac in memory) through the
    // format manager, resample, cap, atomic-publish. Invalid reader → keep the
    // previous source (returns false). OFF the audio thread.
    bool decodeAndPublish (const void* data, size_t numBytes, double engineRate,
                           const juce::String& identity);

    // Resample a decoded buffer (srcRate) to engineRate, capped at kMaxSourceSeconds.
    // Returns a fully-built shared_ptr ready to atomic-publish. Sets `truncated`
    // when the source exceeded the cap.
    std::shared_ptr<juce::AudioBuffer<float>>
        resampleToEngineRate (const juce::AudioBuffer<float>& src, double srcRate,
                              double engineRate, bool& truncated) const;

    // Seed the LIVE rootKey param to the per-source recorded-pitch root (e.g.
    // piano = 48). The APVTS rootKey DEFAULT stays 60 (frozen contract); this
    // overwrites the live value. OFF the audio thread.
    void seedRootForSource (int builtInIndex);

    // APVTS listener: fires on the message thread when `sourceSample` changes. We
    // do NOT decode here (the host may call it from any thread) — we
    // triggerAsyncUpdate() so the decode always runs on the message thread.
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // AsyncUpdater: message-thread callback that performs the actual built-in
    // decode/resample/publish + per-source root seed for the pending selection.
    void handleAsyncUpdate() override;

    // Reaper (CR-01): runs on the message thread; frees retired source buffers
    // once their use_count() drops to 1 (i.e. no in-flight audio block still holds
    // a snapshot). Uses a TRY-lock so the message thread never stalls behind a
    // publisher mid-decode — a skipped tick just retries next period.
    void timerCallback() override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // The sampler synth: kMaxVoices custom SampleVoice + one shared SampleSound
    // (built in the ctor). The resampled source buffer is published to the audio
    // thread via the atomic shared_ptr swap and snapshotted once per block.
    juce::Synthesiser synth;
    std::shared_ptr<juce::AudioBuffer<float>> currentSource;

    //==========================================================================
    // Publish lock (CR-02). Serializes the three off-audio decode/publish paths
    // (prepareToPlay — host audio-setup thread; handleAsyncUpdate — message
    // thread; setStateInformation — arbitrary host thread) and guards the shared
    // non-atomic publish state: currentSourceIdentity, stateWasRestored,
    // rootSeeded, and retiredSources. NEVER taken in processBlock — the audio
    // thread stays on the lock-free atomic shared_ptr snapshot.
    // (juce::CriticalSection is recursive, so decodeAndPublish may be called
    // with the lock already held by prepareToPlay / setStateInformation.)
    juce::CriticalSection sourcePublishLock;

    // Retired source buffers (CR-01) awaiting message-thread free. A publisher
    // pushes the outgoing buffer here (under sourcePublishLock) instead of
    // dropping the last off-audio reference; timerCallback() erases entries at
    // use_count()==1, so the free never happens on the audio thread.
    std::vector<std::shared_ptr<juce::AudioBuffer<float>>> retiredSources;

    // Master output trim (dB->lin, 20 ms smoothing) — final stage in processBlock.
    juce::SmoothedValue<float> outputGain { 1.0f };

    // Pending built-in index for the AsyncUpdater (set by the parameter listener,
    // consumed on the message thread). -1 = nothing pending.
    std::atomic<int> pendingBuiltInIndex { -1 };

    // Root-seed guards (RESEARCH §6). A FRESH instance seeds the per-source root
    // once on the first prepareToPlay (rootSeeded); a RESTORED session keeps its
    // saved rootKey (stateWasRestored gates the prepare-time seed off).
    bool stateWasRestored = false;
    bool rootSeeded       = false;

    //==========================================================================
    // Custom non-APVTS state: which source is loaded (built-in name or file path).
    juce::String currentSourceIdentity { "embedded:piano" };

    // Built-in names, indexed to match the sourceSample choice order. Working
    // placeholder set (ARCHITECTURE: ≈4–6 curated found-sounds); finalized with
    // the embedded .wav assets in Stage 2.3.
    static constexpr const char* kBuiltInNames[kNumBuiltIns] = { "piano", "vocal", "flute", "vinyl" };

    // Per-source recorded-pitch root (engine metadata). piano = 48 (probed f0 ≈
    // 131.25 Hz; root 60 would play an octave flat). vocal/flute/vinyl are the
    // intended roots once their real assets land (Stage 2.3 — all fall back to the
    // piano blob for now, but the root table is already correct per source).
    static constexpr int kBuiltInRoot[kNumBuiltIns] = { 48, 69, 72, 48 };

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
