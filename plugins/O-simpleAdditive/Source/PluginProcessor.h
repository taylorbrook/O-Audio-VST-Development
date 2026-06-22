/*
  ==============================================================================

    O-simpleAdditive - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical 16-voice additive / wavetable-scan synthesizer.

    Stage 2 — Phase 2.1 (Core Additive Voice): a polyphonic, MIDI-playable
    additive voice. 16 drawbars (Frame A) are summed into a per-note
    band-limited single-cycle table, read by phase, shaped by the amp ADSR.
    Zero latency (additive band-limits exactly → setLatencySamples(0)).

    Not yet wired (later Stage 2 phases): Frame A→B scan/morph + scan LFO +
    mod-env (2.2); spectral-decay + bit-depth + viz tap (2.3). The APVTS holds
    all 33 params; the voice currently consumes only Frame A + amp ADSR.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "AdditiveVoice.h"

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs.
// Referenced by the processor now; by the additive voices (Stage 2) and the
// WebView relays/attachments (Stage 3) later.
namespace OSimpleAdditive::ParamIDs
{
    // Additive spectrum — Frame A: the 16 harmonic drawbars (stored 0–1 normalized).
    inline constexpr auto partial1  = "partial1";
    inline constexpr auto partial2  = "partial2";
    inline constexpr auto partial3  = "partial3";
    inline constexpr auto partial4  = "partial4";
    inline constexpr auto partial5  = "partial5";
    inline constexpr auto partial6  = "partial6";
    inline constexpr auto partial7  = "partial7";
    inline constexpr auto partial8  = "partial8";
    inline constexpr auto partial9  = "partial9";
    inline constexpr auto partial10 = "partial10";
    inline constexpr auto partial11 = "partial11";
    inline constexpr auto partial12 = "partial12";
    inline constexpr auto partial13 = "partial13";
    inline constexpr auto partial14 = "partial14";
    inline constexpr auto partial15 = "partial15";
    inline constexpr auto partial16 = "partial16";

    // Wavetable dimension — scan / morph (Frame A → Frame B).
    inline constexpr auto frameBSource  = "frameBSource";   // choice: Sine/Saw/Square/Odd
    inline constexpr auto scanPosition  = "scanPosition";   // morph pointer A↔B (0–1)
    inline constexpr auto scanLfoRate   = "scanLfoRate";    // LFO speed (Hz, log skew)
    inline constexpr auto scanLfoDepth  = "scanLfoDepth";   // LFO sweep depth (0–1)
    inline constexpr auto scanEnvAmount = "scanEnvAmount";  // mod-env → scan (bipolar -1..1)

    // Spectral shaping.
    inline constexpr auto spectralDecay = "spectralDecay";  // higher-partial decay macro (0–1)
    inline constexpr auto bitDepth      = "bitDepth";       // choice: Off/12/10/8/6/4/2
    inline constexpr auto velToDecay    = "velToDecay";     // velocity → spectral decay (0–1)

    // Amplitude envelope (ADSR → per-voice output).
    inline constexpr auto ampAttack  = "ampAttack";
    inline constexpr auto ampDecay   = "ampDecay";
    inline constexpr auto ampSustain = "ampSustain";
    inline constexpr auto ampRelease = "ampRelease";

    // Modulation envelope (ADSR → scan position).
    inline constexpr auto modAttack  = "modAttack";
    inline constexpr auto modDecay   = "modDecay";
    inline constexpr auto modSustain = "modSustain";
    inline constexpr auto modRelease = "modRelease";

    // Output.
    inline constexpr auto outputLevel = "outputLevel";      // master trim (dB)
}

//==============================================================================
class OSimpleAdditiveAudioProcessor : public juce::AudioProcessor
{
public:
    OSimpleAdditiveAudioProcessor();
    ~OSimpleAdditiveAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "O-simpleAdditive"; }
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

private:
    //==========================================================================
    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // DSP — Stage 2. 16-voice additive Synthesiser; APVTS is read once per block
    // and pushed to the voices (voices never touch APVTS). No oversampling.
    static constexpr int kNumVoices = 16;

    juce::Synthesiser synth;
    void pushParamsToVoices (int numSamples);

    // Global scan LFO phase [0,1) — one sine shared by all voices (advanced once
    // per block in pushParamsToVoices so all notes morph in phase).
    float lfoPhase = 0.0f;

    juce::SmoothedValue<float> outputGain { 1.0f };   // dB->lin, 20 ms
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAdditiveAudioProcessor)
};
