/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
    inline constexpr auto ratio          = "ratio";          // M:C ratio — modulator = carrier × ratio, FMVoice.h:210 (harmonic vs inharmonic)
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

    // Carrier frequency of the most-recently-started note, for the spectrum's
    // FM sideband markers (viz only — no effect on audio). Returns 0 when nothing
    // is sounding, so the editor skips the markers. Written audio-thread-side in
    // processBlock (relaxed atomics), read on the message-thread Timer.
    double getCarrierHz() const noexcept
    {
        return carrierSounding.load (std::memory_order_relaxed)
             ? lastNoteHz.load (std::memory_order_relaxed) : 0.0;
    }

    // On-screen keyboard: editor injects note on/off from the WebView (any thread).
    // Queued via a MidiMessageCollector and merged into processBlock's MIDI stream.
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);

    //==========================================================================
    // INTERFACE LANGUAGE (v1.3.0) — the WebView UI's own language preference.
    //
    // Deliberately NOT an AudioParameterChoice: it must not appear in a DAW
    // automation lane, and a lesson preset must not be able to change which
    // language somebody reads their interface in. It rides the APVTS tree as a
    // plain PROPERTY on the ROOT, which is the shape this processor's persistence
    // already has room for — OuariconPresetManager::getStateAsXml() starts from
    // parameters.copyState(), so a root property is carried into the session XML
    // for free, and loading a JSON preset never touches it (applyPresetJson sets
    // parameters only).
    //
    // The RUNTIME form is an index; the PERSISTED form is the language code.
    // The editor PULLS it once at page init; nothing pushes.
    std::atomic<int> uiLanguage { 0 };

    /** The codec. languageIndex() maps anything that is not "fr" to 0, so a
        hand-edited session or an unexpected argument from the page degrades to
        English rather than being stored unvalidated. */
    static juce::String languageCode  (int i)                 { return i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "fr" ? 1 : 0; }

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

    // Carrier tracking for the spectrum sideband markers (viz only). lastNoteHz =
    // pitch of the most-recent note-on; carrierSounding = any voice still audible.
    // Both written in processBlock (audio thread), read via getCarrierHz().
    std::atomic<double> lastNoteHz      { 0.0 };
    std::atomic<bool>   carrierSounding { false };

    // v1.3.0 — the ROOT property name the interface language is persisted under.
    static constexpr const char* kUiLanguageProp = "uiLanguage";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleFMAudioProcessor)
};
