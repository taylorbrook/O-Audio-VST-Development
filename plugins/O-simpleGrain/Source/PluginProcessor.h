/*
  ==============================================================================

    O-simpleGrain - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical granular synthesizer.

    Stage 1 (Foundation): silent 8-voice synth shell. Full 18-parameter APVTS +
    state persistence (incl. a custom, non-APVTS loaded-source identity). No audio
    rendering yet (grain engine / voices / ADSR / window LUTs land Stage 2), no
    WebView UI yet (minimal placeholder editor for now — Stage 3 brings the UI).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <memory>
#include "dsp/WindowLuts.h"

// Forward declarations — the voice/sound are pulled in only by the .cpp
// (GrainVoice.h itself includes this header for the engine constants).
class GrainVoice;
class GrainSound;

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by the grain engine / voice param-push
// (Stage 2) and WebView relays/attachments (Stage 3) later. IDs/ranges/defaults
// are authoritative per .planning/parameter-spec.md (18 params).
namespace OSimpleGrain::ParamIDs
{
    // Source
    inline constexpr auto sourceSample   = "sourceSample";   // choice: fire/voice/water/piano

    // Grain
    inline constexpr auto grainSize      = "grainSize";      // 2–200 ms
    inline constexpr auto density        = "density";        // 1–200 grains/s (log skew)
    inline constexpr auto position       = "position";       // 0–100 %
    inline constexpr auto scan           = "scan";           // −200–+200 % (bipolar)
    inline constexpr auto freeze         = "freeze";         // bool

    // Window
    inline constexpr auto windowShape    = "windowShape";    // choice: rect/tri/Welch/Gauss/Hann

    // Spray & Scatter
    inline constexpr auto pitchSpray     = "pitchSpray";     // 0–12 st
    inline constexpr auto positionSpray  = "positionSpray";  // 0–100 %
    inline constexpr auto scatter        = "scatter";        // 0–100 %
    inline constexpr auto grainPitch     = "grainPitch";     // −24–+24 st
    inline constexpr auto panSpray       = "panSpray";       // 0–100 %
    inline constexpr auto velToDensity   = "velToDensity";   // 0–100 %

    // Amplitude envelope (per-voice ADSR)
    inline constexpr auto ampAttack      = "ampAttack";      // 0–5 s
    inline constexpr auto ampDecay       = "ampDecay";       // 0–5 s
    inline constexpr auto ampSustain     = "ampSustain";     // 0–1 (0–100 %)
    inline constexpr auto ampRelease     = "ampRelease";     // 0–5 s

    // Output
    inline constexpr auto outputLevel    = "outputLevel";    // master trim (dB)
}

//==============================================================================
class OSimpleGrainAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleGrainAudioProcessor();
    ~OSimpleGrainAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleGrain"; }
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

    // Loaded-source identity (custom, non-APVTS state). "embedded:fire" by
    // default or a user file path once Stage 2.3 wires real loading. Persisted
    // alongside the APVTS tree so a session restores the same source.
    const juce::String& getSourceIdentity() const noexcept { return currentSourceIdentity; }
    void setSourceIdentity (const juce::String& id) { currentSourceIdentity = id; }

    //==========================================================================
    // Engine constants (declared NOW; consumed by the grain engine in Stage 2).
    static constexpr int kMaxVoices        = 8;     // polyphony
    static constexpr int kMaxGrainsPerVoice = 24;   // per-voice active grain cap
    static constexpr int kGlobalGrainCap   = 192;   // global cap (steal-oldest)
    static constexpr int kRootNote         = 60;    // C3 — key-track reference
    static constexpr int kMaxSourceSeconds = 10;    // source-length cap
    static constexpr int kWindowLutSize    = 2048;  // precomputed window LUT length

private:
    //==========================================================================
    // Lock-free atomic shared_ptr swap for the source buffer (RESEARCH §6.3).
    // The audio thread snapshots the pointer ONCE per block and holds the ref
    // for the whole block, so a swap mid-block can never free the buffer it is
    // reading. The decode/resample that builds a new buffer always happens off
    // the audio thread (construction / prepareToPlay in 2.1; selection in 2.3).
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

    // Decode + resample the default source (fire.wav) to the engine rate and
    // publish it via atomicStore. OFF the audio thread (called from prepare).
    void loadDefaultSource (double engineRate);

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Grain engine. Window LUTs built once at construction (declared BEFORE the
    // synth so they exist when the voices receive their LUT pointer in the ctor).
    WindowLuts        windowLuts { kWindowLutSize };
    juce::Synthesiser synth;

    // The resampled source buffer, published to the audio thread via an atomic
    // shared_ptr swap. Snapshotted once per block in processBlock.
    std::shared_ptr<juce::AudioBuffer<float>> currentSource;

    // Master output trim (dB->lin, smoothed) + a fixed headroom factor so dense
    // overlapping clouds don't clip before the trim.
    juce::SmoothedValue<float> outputGain { 1.0f };

    //==========================================================================
    // Global read head (processor-owned, Phase 2.2). One playhead shared by all
    // voices/grains: `position` sets its resting point, `scan` its velocity,
    // `freeze` pins it. Advanced PER SAMPLE in processBlock (wrapped to
    // [0, srcLen) for both directions — negative scan = reverse). Voices read the
    // block-start playhead at spawn (setPlayhead, once per block) so the 2.1 voice
    // spawn signature is untouched (Sequencing Note 3).
    //
    // Click-free (QUAL-01): SmoothedValue ramps on scan / position / playhead
    // velocity. Freeze targets velocity -> 0 (and disengage ramps back to the
    // scan-derived velocity) via the velocity SmoothedValue — the playhead is
    // NEVER hard-jumped (RESEARCH §4.2 "simplest robust approach").
    double playheadPos = 0.0;                        // current global read point (source samples)

    juce::SmoothedValue<float> scanSmoothed     { 0.0f };   // scan % -> velocity ramp
    juce::SmoothedValue<float> positionSmoothed { 0.0f };   // resting point % -> samples ramp
    juce::SmoothedValue<float> playheadVelocity { 0.0f };   // samples/sample, freeze-pinnable

    //==========================================================================
    // Custom non-APVTS state: which source is loaded (built-in name or file path).
    juce::String currentSourceIdentity { "embedded:fire" };

    //==========================================================================
    // Cached raw-param atomic pointers (assigned in the ctor). Established now;
    // read once per block by the grain engine in Stage 2. Unused while silent.
    std::atomic<float>* sourceSampleParam  = nullptr;
    std::atomic<float>* grainSizeParam     = nullptr;
    std::atomic<float>* densityParam       = nullptr;
    std::atomic<float>* positionParam      = nullptr;
    std::atomic<float>* scanParam          = nullptr;
    std::atomic<float>* freezeParam        = nullptr;
    std::atomic<float>* windowShapeParam   = nullptr;
    std::atomic<float>* pitchSprayParam    = nullptr;
    std::atomic<float>* positionSprayParam = nullptr;
    std::atomic<float>* scatterParam       = nullptr;
    std::atomic<float>* grainPitchParam    = nullptr;
    std::atomic<float>* panSprayParam      = nullptr;
    std::atomic<float>* velToDensityParam  = nullptr;
    std::atomic<float>* ampAttackParam     = nullptr;
    std::atomic<float>* ampDecayParam      = nullptr;
    std::atomic<float>* ampSustainParam    = nullptr;
    std::atomic<float>* ampReleaseParam    = nullptr;
    std::atomic<float>* outputLevelParam   = nullptr;

    //==========================================================================
    double currentSampleRate = 44100.0;

    // Custom-state element names for get/setStateInformation.
    static constexpr const char* kSourceStateTag = "SOURCE";
    static constexpr const char* kSourceIdProp   = "identity";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleGrainAudioProcessor)
};
