/*
  ==============================================================================

    O-simpleSubtractive - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical subtractive synthesizer (osc -> filter -> amp, dual ADSR).

    Stage 2 (DSP): the silent shell is now an audible polyphonic instrument.
    OscillatorBank -> multimode ZDF SVF -> VCA, two independent ADSRs, Poly /
    Mono / Legato voicing + glide, and a real-time-safe visualization tap. No
    oversampling (PolyBLEP band-limits at source) -> zero added latency. WebView
    UI is still deferred to Stage 3 (GenericAudioProcessorEditor host for now).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SubVoice.h"
#include "SubVizAnalyzer.h"
#include <array>

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// IDs/ranges/defaults track ARCHITECTURE.md -> Parameter Mapping exactly.
namespace OSimpleSubtractive::ParamIDs
{
    // Oscillator / sources
    inline constexpr auto oscWave         = "oscWave";          // Saw / Square / Triangle / Sine
    inline constexpr auto subLevel        = "subLevel";         // sub-osc (-1 oct square) mix
    inline constexpr auto noiseLevel      = "noiseLevel";       // white-noise mix

    // Filter (state-variable)
    inline constexpr auto filterType      = "filterType";       // LP / HP / BP / Notch
    inline constexpr auto filterSlope     = "filterSlope";      // 6 / 12 / 24 dB/oct
    inline constexpr auto cutoff          = "cutoff";           // base cutoff (Hz, log)
    inline constexpr auto resonance       = "resonance";        // resonance / self-osc
    inline constexpr auto filterEnvAmount = "filterEnvAmount";  // filter ADSR -> cutoff (bipolar)
    inline constexpr auto keyTrack        = "keyTrack";         // cutoff key-tracking

    // Filter envelope (ADSR -> cutoff)
    inline constexpr auto filterAttack    = "filterAttack";
    inline constexpr auto filterDecay     = "filterDecay";
    inline constexpr auto filterSustain   = "filterSustain";
    inline constexpr auto filterRelease   = "filterRelease";

    // Amplitude envelope (ADSR -> output + voice lifetime)
    inline constexpr auto ampAttack       = "ampAttack";
    inline constexpr auto ampDecay        = "ampDecay";
    inline constexpr auto ampSustain      = "ampSustain";
    inline constexpr auto ampRelease      = "ampRelease";

    // Voicing
    inline constexpr auto voiceMode       = "voiceMode";        // Poly / Mono / Legato
    inline constexpr auto glide           = "glide";            // portamento time (s)

    // Output
    inline constexpr auto outputLevel     = "outputLevel";      // master trim (dB)
}

//==============================================================================
// RT-safe held-note stack (last-note priority) for Mono / Legato. Fixed
// capacity, no allocation on the audio thread.
class MonoStack
{
public:
    void clear() noexcept { count = 0; }
    bool empty() const noexcept { return count == 0; }

    // Push a note (most recent = top). If already present, move it to the top.
    void push (int note, float vel) noexcept
    {
        remove (note);
        if (count < kCap)
        {
            notes[(size_t) count] = note;
            vels [(size_t) count] = vel;
            ++count;
        }
        else
        {
            // Full: drop the oldest, append newest (extremely rare — 32 held notes).
            for (int i = 1; i < kCap; ++i) { notes[(size_t)(i-1)] = notes[(size_t)i]; vels[(size_t)(i-1)] = vels[(size_t)i]; }
            notes[(size_t)(kCap-1)] = note; vels[(size_t)(kCap-1)] = vel;
        }
    }

    void remove (int note) noexcept
    {
        int w = 0;
        for (int r = 0; r < count; ++r)
            if (notes[(size_t) r] != note)
            {
                notes[(size_t) w] = notes[(size_t) r];
                vels [(size_t) w] = vels [(size_t) r];
                ++w;
            }
        count = w;
    }

    int   topNote() const noexcept { return count > 0 ? notes[(size_t)(count-1)] : -1; }
    float topVel()  const noexcept { return count > 0 ? vels [(size_t)(count-1)] : 1.0f; }
    int   size()    const noexcept { return count; }

private:
    static constexpr int kCap = 32;
    std::array<int,   kCap> notes {};
    std::array<float, kCap> vels  {};
    int count = 0;
};

//==============================================================================
class OSimpleSubtractiveAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleSubtractiveAudioProcessor();
    ~OSimpleSubtractiveAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleSubtractive"; }
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
    // Public access to APVTS for the editor (and DSP param push).
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

    // Visualization tap — editor reads these on its message-thread Timer (Stage 3).
    VizRing& getVizRing() noexcept { return vizRing; }
    double   getCurrentSampleRate() const noexcept { return currentSampleRate; }

    // Lead-voice filter state for the headline magnitude curve (Stage 3 / harness).
    double getDisplayCutoffHz() const noexcept { return displayCutoffHz.load (std::memory_order_relaxed); }
    double getDisplayK()        const noexcept { return displayK.load (std::memory_order_relaxed); }
    int    getDisplayType()     const noexcept { return displayType.load (std::memory_order_relaxed); }
    int    getDisplaySlope()    const noexcept { return displaySlope.load (std::memory_order_relaxed); }
    float  getFilterEnvValue()  const noexcept { return filterEnvValue.load (std::memory_order_relaxed); }
    float  getAmpEnvValue()     const noexcept { return ampEnvValue.load (std::memory_order_relaxed); }

    // On-screen keyboard: editor injects note on/off from the WebView (Stage 3).
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // DSP (Stage 2)
    static constexpr int kNumVoices   = 16;
    static constexpr int kMaxChannels = 2;

    juce::Synthesiser synth;
    void pushParamsToVoices();

    // Mono/Legato note management (processor-side, drives synth voice 0).
    MonoStack monoStack;
    int  lastVoiceMode = 0;     // detect Poly<->Mono/Legato switches (hard reset on change)
    void renderMonoLegato (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                           int numSamples, int mode);
    void updateDisplayFromLeadVoice();

    // On-screen-keyboard MIDI: thread-safe queue drained into processBlock.
    juce::MidiMessageCollector midiCollector;

    juce::SmoothedValue<float> outputGain { 1.0f };   // dB->lin, 20 ms
    double currentSampleRate = 44100.0;

    // Real-time-safe visualization tap (audio-thread copy-only writer).
    VizRing vizRing;

    // Lead-voice display state (audio thread writes, message thread reads).
    std::atomic<double> displayCutoffHz { 2000.0 };
    std::atomic<double> displayK        { 1.4 };
    std::atomic<int>    displayType     { 0 };
    std::atomic<int>    displaySlope    { 2 };
    std::atomic<float>  filterEnvValue  { 0.0f };
    std::atomic<float>  ampEnvValue     { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSubtractiveAudioProcessor)
};
