/*
  ==============================================================================

    O-simplePhysicalModelSynth - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical physical-modeling synthesizer (Karplus-Strong string + modal
    resonator bank, swappable exciters).

    Stage 1 (Foundation): silent synth shell. Full 17-parameter APVTS +
    plain-APVTS state persistence + 16-voice juce::Synthesiser with a header-only
    SILENT PhysicalModelVoice. No audio rendering yet (first sound: Stage 2 /
    Phase 2.1), no WebView UI yet (Stage 3 — GenericAudioProcessorEditor for now,
    behind the #if JUCE_WEB_BROWSER seam in createEditor()).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PhysicalModelVoice.h"
#include "OuariconPresetManager.h"   // suite preset-manager module (Stage 3 / D4)
#include <map>
#include <atomic>

//==============================================================================
// Parameter identifiers — single source of truth for the 17 APVTS IDs (verbatim
// from parameter-spec.md, the locked zero-drift contract). Referenced by the
// processor now; by voice param-push (Stage 2) and WebView relays/attachments
// (Stage 3) later. None of these IDs are bare `end`/`begin` (the param-ID shadows
// juce-free-fn gotcha) — clear.
namespace ParamIDs
{
    // Excitation (how energy is injected)
    inline constexpr auto excitationType     = "excitationType";     // choice: Pluck/Strike/Bow
    inline constexpr auto excitationPosition = "excitationPosition"; // % 0–100
    inline constexpr auto excitationColor    = "excitationColor";    // % 0–100
    inline constexpr auto bowForce           = "bowForce";           // % 0–100

    // Resonator (what carries the energy)
    inline constexpr auto resonatorType      = "resonatorType";      // choice: String/Modal
    inline constexpr auto stringModel        = "stringModel";        // choice: Karplus-Strong/Waveguide
    inline constexpr auto inharmonicity      = "inharmonicity";      // % 0–100 (modal)
    inline constexpr auto modeBrightness     = "modeBrightness";     // % 0–100 (modal)

    // Material / Damping (how energy is lost)
    inline constexpr auto damping            = "damping";            // % 0–100
    inline constexpr auto decay              = "decay";              // % 0–100
    inline constexpr auto material           = "material";           // % 0–100 (macro)

    // Tuning
    inline constexpr auto coarseTune         = "coarseTune";         // int semitones −24…+24
    inline constexpr auto fineTune           = "fineTune";           // cents −100…+100

    // Amp + dynamics
    inline constexpr auto ampAttack          = "ampAttack";          // seconds 0–2
    inline constexpr auto ampRelease         = "ampRelease";         // seconds 0–5
    inline constexpr auto velToBrightness    = "velToBrightness";    // % 0–100

    // Output
    inline constexpr auto outputLevel        = "outputLevel";        // dB −60…0
}

//==============================================================================
class OSimplePhysicalModelSynthAudioProcessor : public juce::AudioProcessor,
                                                private juce::AudioProcessorValueTreeState::Listener
{
public:
    OSimplePhysicalModelSynthAudioProcessor();
    ~OSimplePhysicalModelSynthAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    // createEditor is DECLARED here and DEFINED in PluginProcessor.cpp behind the
    // #if JUCE_WEB_BROWSER seam (returns the WebView editor under =1, the generic
    // editor under =0). Declaration-only here — plus the guarded #include of
    // PluginEditor.h in the .cpp — keeps the render-harness translation unit
    // (compiled at JUCE_WEB_BROWSER=0, without PluginEditor.cpp) free of WebView
    // types. Do NOT include PluginEditor.h from this header.
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simplePhysicalModelSynth"; }
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
    // Public access to APVTS for the editor + render-harness.
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Preset manager (factory/user JSON presets) — editor wires the WebView native
    // fns to this (D4). Factory set is seeded in Stage 4; the shell is wired now.
    OuariconPresetManager& getPresetManager() noexcept { return presetManager; }

    // On-screen keyboard: editor injects note on/off from the WebView (any thread).
    // Queued via a MidiMessageCollector and merged into processBlock's MIDI stream.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    // Stage-3 editor reads the visualization taps (waveform ring + loop energy +
    // modal stems) from here. Written copy-only on the audio thread (PERF-01).
    VizTap& getVizTap() noexcept { return viz; }

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;

    // Preset persistence (factory/user JSON). Declared AFTER parameters so it is
    // constructed second (it holds an APVTS reference). Stage 3 / D4.
    OuariconPresetManager presetManager;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Material macro (DSP-07): when `material` moves it writes BOTH `damping` and
    // `decay` (so the two knobs visibly co-move). Runs on the message thread via
    // this listener — never on the audio thread (RESEARCH §2.4).
    void parameterChanged (const juce::String& paramID, float newValue) override;

    // Cached raw-atomic param pointers — looked up once (constructor), read with
    // a relaxed load() per block on the audio thread (no string hashing in
    // processBlock, no per-sample APVTS access).
    void cacheParamPointers();
    std::atomic<float>* p (const char* id) const { return rawParams.count (id) ? rawParams.at (id) : nullptr; }
    std::map<juce::String, std::atomic<float>*> rawParams;

    PhysicalModelParams readParams() const;

    // Master output level (dB→gain), smoothed to avoid zipper on fast moves.
    juce::SmoothedValue<float> outputGain { 1.0f };

    // Visualization taps (Stage 3 consumer) + monotonic trigger counter so the
    // lead voice (most-recently-triggered) can be identified for the per-block publish.
    VizTap viz;
    std::atomic<juce::uint64> triggerSeqCounter { 0 };
    void publishViz (int numSamples, const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // 16-voice polyphonic synth.
    static constexpr int kNumVoices = 16;
    juce::Synthesiser synth;

    // On-screen-keyboard MIDI: thread-safe queue drained at the top of processBlock
    // (Stage 3 / D4 — absent in Stage 1). Not a WebView type → harness stays clean.
    juce::MidiMessageCollector midiCollector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimplePhysicalModelSynthAudioProcessor)
};
