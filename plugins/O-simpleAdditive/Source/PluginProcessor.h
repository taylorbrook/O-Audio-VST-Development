/*
  ==============================================================================

    O-simpleAdditive - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

    Pedagogical 16-voice additive / wavetable-scan synthesizer.

    16 drawbars (Frame A) morph toward a preset (Frame B) via scan/LFO/mod-env,
    tilt darker over the note via the spectral-decay macro, and quantize via the
    bit-depth choice. The summed single-cycle table is read by phase and shaped by
    the amp ADSR. Zero latency (additive band-limits exactly → setLatencySamples(0)).

    A real-time-safe visualization tap feeds the editor: a lock-free VizRing
    (post-gain mono sum, copy-only on the audio thread — the FFT runs on the editor
    Timer) and an atomic 16-element active-spectrum snapshot (the morphed + decayed
    amplitudes of the newest sounding voice) driving the drawbar display. The APVTS
    holds all 33 params; the voices consume every DSP parameter.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <vector>
#include "AdditiveVoice.h"
#include "AdditiveVizAnalyzer.h"   // VizRing (audio-thread tap) + AdditiveVizAnalyzer (Stage 3 FFT)

//==============================================================================
// Parameter identifiers — single source of truth for APVTS IDs. Referenced by the
// processor, the additive voices, and the WebView relays/attachments.
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

    // The 16 partial IDs as one array — single source of truth for every loop that
    // builds, pushes, or presets the additive spectrum (processor + editor).
    inline constexpr std::array<const char*, 16> partialIds {
        partial1,  partial2,  partial3,  partial4,
        partial5,  partial6,  partial7,  partial8,
        partial9,  partial10, partial11, partial12,
        partial13, partial14, partial15, partial16
    };

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

    // Host sample rate (Stage 3: the editor's AdditiveVizAnalyzer needs it for the
    // scope/FFT; also reset on the on-screen-keyboard MIDI collector).
    double getCurrentSampleRate() const noexcept { return currentSampleRate; }

    // True while any voice is sounding. The editor uses this so the drawbar
    // live-glow snaps back to each drawbar's set level when nothing plays,
    // instead of holding the last note's (now stale) active-spectrum snapshot.
    bool isSounding() const noexcept { return anyVoiceSounding.load (std::memory_order_relaxed); }

    // On-screen keyboard: the editor injects note on/off from the WebView (any
    // thread). Queued via a MidiMessageCollector and merged into processBlock.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    // Lesson preset tour (Stage 3): apply a full APVTS snapshot by name. Sets every
    // parameter via setValueNotifyingHost, so the relays/attachments propagate the
    // change back to the WebView controls automatically. Persistent user-preset
    // save/load (OuariconPresetManager) is a Stage-4 concern.
    void applyFactoryPreset (const juce::String& name);

    //==========================================================================
    // Visualization access for the Stage-3 editor.
    //  - getVizRing: the lock-free audio→message ring (the editor's Timer feeds
    //    it to an AdditiveVizAnalyzer for the scope + FFT overlay).
    //  - readActiveSpectrum: copies the 16 atomic active-spectrum amplitudes (the
    //    morphed + decayed bars of the newest sounding voice) into dest16 for the
    //    exact drawbar display. Idle holds the last sounded spectrum (Stage 3
    //    fades/handles idle). Safe from the message thread (relaxed atomics).
    const VizRing& getVizRing() const noexcept { return viz; }

    void readActiveSpectrum (float* dest16) const noexcept
    {
        for (int k = 0; k < AdditiveVoice::kNumPartials; ++k)
            dest16[k] = activeSpectrumSnapshot[(size_t) k].load (std::memory_order_relaxed);
    }

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

    // On-screen-keyboard MIDI: thread-safe queue drained into processBlock so the
    // WebView keyboard plays the synth alongside (and merged with) host MIDI.
    juce::MidiMessageCollector midiCollector;

    // Global scan LFO phase [0,1) — one sine shared by all voices (advanced once
    // per block in pushParamsToVoices so all notes morph in phase).
    float lfoPhase = 0.0f;

    juce::SmoothedValue<float> outputGain { 1.0f };   // dB->lin, 20 ms
    double currentSampleRate = 44100.0;

    //==========================================================================
    // Visualization tap (Phase 2.3). PERF-01: audio thread is COPY-ONLY.
    //  - viz: lock-free overwrite ring; the post-gain mono sum is written here.
    //  - monoScratch: pre-allocated (prepareToPlay) mono down-mix buffer so the
    //    block sum needs NO audio-thread allocation.
    //  - activeSpectrumSnapshot: 16 atomic amplitudes of the primary (newest
    //    sounding) voice's morphed + decayed spectrum, published post-render for
    //    the message-thread drawbar display. Producer = audio thread (same thread
    //    that renders the voices), consumer = editor Timer via readActiveSpectrum.
    VizRing             viz;
    std::vector<float>  monoScratch;
    std::array<std::atomic<float>, (size_t) AdditiveVoice::kNumPartials> activeSpectrumSnapshot {};

    // True when any voice was sounding in the last block (drives the editor's
    // idle handling of the drawbar live-glow). Written on the audio thread.
    std::atomic<bool> anyVoiceSounding { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleAdditiveAudioProcessor)
};
