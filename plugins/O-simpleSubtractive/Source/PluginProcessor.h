/*
  ==============================================================================

    O-simpleSubtractive - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical subtractive synthesizer (osc -> filter -> amp, dual ADSR).

    Stage 1 (Foundation): silent synth shell. Full 20-parameter APVTS +
    state persistence. No audio rendering yet (first sound: Stage 2), no
    WebView UI yet (Stage 3 — a GenericAudioProcessorEditor host for now).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by voice param-push (Stage 2) and WebView
// relays/attachments (Stage 3) later. IDs/ranges/defaults track
// ARCHITECTURE.md -> Parameter Mapping exactly.
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
    // Public access to APVTS for the editor (and Stage 2 DSP param push).
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached host rate (Stage 2 uses it to seed smoothing / DSP prepare).
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleSubtractiveAudioProcessor)
};
