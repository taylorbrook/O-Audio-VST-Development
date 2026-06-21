/*
  ==============================================================================

    O-simpleFM - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical 2-operator FM/PM synthesizer.

    Stage 1 (Foundation): silent synth shell. Full 17-parameter APVTS +
    state persistence. No audio rendering yet (first sound: Stage 2 / Phase 2.1),
    no WebView UI yet (Stage 3 — GenericAudioProcessorEditor for now).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "FMVoice.h"
#include "FmVizAnalyzer.h"
#include "OuariconPresetManager.h"   // suite preset-manager module (added Stage 4)

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by voice param-push (Stage 2) and WebView
// relays/attachments (Stage 3) later.
namespace OSimpleFM::ParamIDs
{
    // Core FM controls
    inline constexpr auto ratio          = "ratio";          // C:M ratio (harmonic vs inharmonic)
    inline constexpr auto ratioSnap      = "ratioSnap";      // integer-snap toggle
    inline constexpr auto modIndex       = "modIndex";       // raw radian index I (0–20)
    inline constexpr auto feedback       = "feedback";       // modulator self-feedback (0–1)
    inline constexpr auto modFixedMode   = "modFixedMode";   // Ratio vs Fixed-Hz modulator
    inline constexpr auto modFixedHz     = "modFixedHz";     // fixed modulator frequency
    inline constexpr auto modEnvToIndex  = "modEnvToIndex";  // mod-env → index depth (default 1.0)
    inline constexpr auto velToIndex     = "velToIndex";     // velocity → index (opt-in)

    // Modulator envelope (ADSR → index)
    inline constexpr auto modAttack      = "modAttack";
    inline constexpr auto modDecay       = "modDecay";
    inline constexpr auto modSustain     = "modSustain";
    inline constexpr auto modRelease     = "modRelease";

    // Amplitude envelope (ADSR → carrier output + voice lifetime)
    inline constexpr auto ampAttack      = "ampAttack";
    inline constexpr auto ampDecay       = "ampDecay";
    inline constexpr auto ampSustain     = "ampSustain";
    inline constexpr auto ampRelease     = "ampRelease";

    // Output
    inline constexpr auto outputLevel    = "outputLevel";    // master trim (dB)
}

//==============================================================================
class OSimpleFMAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleFMAudioProcessor();
    ~OSimpleFMAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleFM"; }
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
    // Public access to APVTS for the editor.
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Preset manager (factory/user JSON presets) — editor wires the WebView native fns to this.
    OuariconPresetManager& getPresetManager() noexcept { return presetManager; }

    // Visualization tap — editor reads these on its message-thread Timer.
    VizRing& getVizRing() noexcept { return vizRing; }
    double   getCurrentSampleRate() const noexcept { return currentSampleRate; }

    // On-screen keyboard: editor injects note on/off from the WebView (any thread).
    // Queued via a MidiMessageCollector and merged into processBlock's MIDI stream.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;

    // Preset persistence (factory/user JSON). Declared AFTER parameters so it is
    // constructed second (it holds an APVTS reference). Stage 4.
    OuariconPresetManager presetManager;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // DSP (Stage 2)
    static constexpr int kNumVoices = 16;

    juce::Synthesiser synth;
    void pushParamsToVoices();

    // On-screen-keyboard MIDI: thread-safe queue drained into processBlock.
    juce::MidiMessageCollector midiCollector;

    // Anti-aliasing: 2x polyphase-IIR oversampling, always-on (v1.0 sine-only).
    // The synth renders at the OVERSAMPLED rate; we decimate via the down filter.
    static constexpr int kOsFactorLog2 = 1;           // 2^1 = 2x
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::MidiBuffer scaledMidi;                       // MIDI offsets scaled x2 (reused, no alloc)

    static constexpr int kMaxChannels = 2;            // synth output is mono/stereo
    juce::SmoothedValue<float> outputGain { 1.0f };   // dB->lin, 20 ms
    double currentSampleRate = 44100.0;
    int    maxBlockSize = 512;                        // OS buffer is sized to this; chunk if exceeded

    // Real-time-safe visualization tap (audio-thread copy-only writer).
    VizRing vizRing;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleFMAudioProcessor)
};
