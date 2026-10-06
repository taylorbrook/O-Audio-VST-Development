/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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

    O-simpleWavetable - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"

#include <cmath>

//==============================================================================
namespace
{
    using Range = juce::NormalisableRange<float>;   // plain {start,end,interval,skew} ONLY

    Range unitRange()     { return { 0.0f,   1.0f,  0.0f, 1.0f  }; }
    Range bipolarRange()  { return { -1.0f,  1.0f,  0.0f, 1.0f  }; }
    Range menvTimeRange() { return { 0.001f, 10.0f, 0.0f, 0.3f  }; }
    Range ampTimeRange()  { return { 0.001f, 5.0f,  0.0f, 0.35f }; }

    // Host/generic-editor text only (JUCE's default prints 7 decimals for an
    // interval-0 range). A TEXT lambda, not a range lambda: the range stays a
    // plain NormalisableRange the WebView slider frontend can read.
    juce::AudioParameterFloatAttributes fixed (int decimals, const juce::String& label = {})
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (label)
            .withStringFromValueFunction ([decimals] (float v, int maxLen)
            {
                juce::String s (v, decimals);
                return maxLen > 0 ? s.substring (0, maxLen) : s;
            });
    }

    // "-inf" at the floor, both directions: the default parser is
    // text.getFloatValue(), which cannot read "-inf" back.
    juce::AudioParameterFloatAttributes outputLevelAttributes()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float db, int maxLen)
            {
                const juce::String s = db <= -59.95f ? juce::String ("-inf") : juce::String (db, 1);
                return maxLen > 0 ? s.substring (0, maxLen) : s;
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                const auto t = text.trim();
                return t.startsWithIgnoreCase ("-inf") ? -60.0f
                                                       : juce::jlimit (-60.0f, 6.0f, t.getFloatValue());
            });
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleWavetableAudioProcessor::createParameterLayout()
{
    namespace ids = OSimpleWavetable::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const char* id, const char* name, Range r, float def,
                               juce::AudioParameterFloatAttributes a)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, r, def, std::move (a)));
    };
    auto addChoice = [&params] (const char* id, const char* name, juce::StringArray c, int def)
    {
        jassert (c.size() >= 2);   // critical_choice_param_needs_two_choices
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id, 1 }, name, std::move (c), def));
    };
    auto addBool = [&params] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id, 1 }, name, def));
    };

    // U+2192 (right arrow) via hex escapes: juce::String(const char*) is
    // ASCII-only (critical_juce_string_char_ctor_is_ascii_only). The source
    // stays ASCII. Keep a non-hex character after every escape.
    // Imported is LAST (index 5); the UI never hard-codes its index.
    juce::StringArray bankChoices;
    bankChoices.add (juce::String (juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw")));
    bankChoices.add (juce::String (juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Square")));
    bankChoices.add ("Pulse Width");
    bankChoices.add ("Formant");
    bankChoices.add ("Drive");
    bankChoices.add ("Imported");

    addChoice (ids::bank,      "Bank", bankChoices, 0);
    addFloat  (ids::position,  "Position", unitRange(), 0.0f, fixed (3));
    addBool   (ids::interp,    "Interpolation", true);
    addBool   (ids::bandlimit, "Band-limiting", true);
    // Mid-rise quantizer (D3): Full = bit-identical bypass, then 16..3 bits.
    addChoice (ids::bitDepth,  "Bit Depth", { "Full", "16", "15", "14", "13", "12", "11", "10",
                                              "9", "8", "7", "6", "5", "4", "3" }, 0);
    addFloat  (ids::lfoRate,   "LFO Rate", { 0.01f, 20.0f, 0.0f, 0.3f }, 0.5f, fixed (2, "Hz"));
    addChoice (ids::lfoSync,   "LFO Sync", { "Free", "Tempo" }, 0);
    // Default index 2 = "1/1" (D5).
    addChoice (ids::lfoDiv,    "LFO Division", { "4 bars", "2 bars", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                                 "1/2.", "1/4.", "1/8.", "1/16.", "1/2T", "1/4T", "1/8T", "1/16T" }, 2);
    addChoice (ids::lfoShape,  "LFO Shape", { "Sine", "Triangle", "Saw", "Square", "S&H" }, 1);
    addFloat  (ids::lfoDepth,  "LFO Depth", unitRange(), 0.0f, fixed (3));
    addFloat  (ids::menvAttack,  "Mod Env Attack",  menvTimeRange(), 0.5f, fixed (3, "s"));
    addFloat  (ids::menvDecay,   "Mod Env Decay",   menvTimeRange(), 1.0f, fixed (3, "s"));
    addFloat  (ids::menvSustain, "Mod Env Sustain", unitRange(),     0.0f, fixed (3));
    addFloat  (ids::menvRelease, "Mod Env Release", menvTimeRange(), 0.5f, fixed (3, "s"));
    addFloat  (ids::envAmount,   "Env Amount",      bipolarRange(),  0.0f, fixed (3));
    addFloat  (ids::ampAttack,   "Amp Attack",  ampTimeRange(), 0.005f, fixed (3, "s"));
    addFloat  (ids::ampDecay,    "Amp Decay",   ampTimeRange(), 0.3f,   fixed (3, "s"));
    addFloat  (ids::ampSustain,  "Amp Sustain", unitRange(),    0.8f,   fixed (3));
    addFloat  (ids::ampRelease,  "Amp Release", ampTimeRange(), 0.2f,   fixed (3, "s"));
    addChoice (ids::voiceMode,   "Voice Mode", { "Poly", "Mono" }, 0);
    addFloat  (ids::outputLevel, "Output Level", { -60.0f, 6.0f, 0.1f, 1.0f }, -6.0f, outputLevelAttributes());

    jassert (params.size() == 21);   // ROADMAP's 22 is the documented slip (parameter-spec.md)
    jassert (params.size() == OSimpleWavetable::ParamIDs::all.size());
    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleWavetableAudioProcessor::OSimpleWavetableAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new WtVoice());            // all allocation here, never on the audio thread
    synth.addSound (new WtSound());
    synth.setNoteStealingEnabled (true);
    midiCollector.reset (44100.0);                 // valid base before the first prepareToPlay (sibling)
}

OSimpleWavetableAudioProcessor::~OSimpleWavetableAudioProcessor() = default;

//==============================================================================
void OSimpleWavetableAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    midiCollector.reset (sampleRate);
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)              // SynthesiserVoice has no virtual prepare
        if (auto* wv = dynamic_cast<WtVoice*> (synth.getVoice (v)))
            wv->prepareToPlay (sampleRate, samplesPerBlock);
    setLatencySamples (0);                                      // getLatencySamples() is non-virtual (JUCE 8)
}

void OSimpleWavetableAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
}

bool OSimpleWavetableAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();      // instrument: no input bus
}

void OSimpleWavetableAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();                                            // silent shell; Stage 2 voices ADD into it
    if (numSamples <= 0)                                       // collector jasserts numSamples > 0
        return;                                                // (Stage 2: bump the reaper counters here too)
    midiCollector.removeNextBlockOfMessages (midi, numSamples);
    synth.renderNextBlock (buffer, midi, 0, numSamples);       // consumes MIDI; WtVoice renders nothing
}

//==============================================================================
void OSimpleWavetableAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    // Defensive native-boundary validation: the JS side clamps today, but the
    // bridge itself accepts any int (out-of-range trips a JUCE jassert /
    // malformed message) and a NaN velocity slips through jlimit (NaN compares
    // false). Never trust the WebView's arguments.
    noteNumber = juce::jlimit (0, 127, noteNumber);
    if (! std::isfinite (velocity))
        velocity = 0.8f;

    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);   // collector wants seconds
    midiCollector.addMessageToQueue (msg);
}

//==============================================================================
// The editor include sits HERE, not at the top of the file: the console test
// targets build with JUCE_WEB_BROWSER=0 and never compile the editor TU
// (pattern_render_harness_breaks_on_webview_editor).
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif
juce::AudioProcessorEditor* OSimpleWavetableAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OSimpleWavetableAudioProcessorEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // console-target build
#endif
}

//==============================================================================
void OSimpleWavetableAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    stripImportedBank (state);                    // never re-emit a child that rode in on replaceState
    state.setProperty (kUiLanguageProp,
                       languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);
    writeImportedBank (state);                    // Stage 1: no bank exists -> nothing appended
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleWavetableAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    // Untrusted input (ASVS V5): wrong root type -> ignore the whole blob.
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType (parameters.state.getType()))
        return;

    // Hand-added property: comes back as a STRING var -> isVoid() is the only
    // honest gate (critical_valuetree_xml_roundtrip_loses_type). Absent leaves
    // the current value standing; languageIndex() allow-lists the code.
    const juce::var lang = state.getProperty (kUiLanguageProp);
    if (! lang.isVoid())
        uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

    // Detach BEFORE replaceState so the live APVTS tree never carries the blob
    // (stale-child trap, critical_preset_manager_stale_customstate_child).
    const auto bankChild = state.getChildWithName (kImportedBankTag).createCopy();   // invalid if absent
    stripImportedBank (state);                    // ALL copies

    parameters.replaceState (state);
    restoreImportedBank (bankChild);
}

//==============================================================================
void OSimpleWavetableAudioProcessor::restoreImportedBank (const juce::ValueTree& childOrInvalid)
{
    // Stage 1: tolerate a present or an absent (invalid) child; publish nothing.
    //
    // Stage 2.4 seams:
    //   - present child: decode -> build mips -> publish (off the audio thread).
    //   - absent child: RETIRE the current imported bank (never free it here;
    //     the reaper frees it once the audio thread has moved on).
    //   - parse numFrames / version via .toString().getIntValue() after an
    //     isVoid() check (XML round trip rebuilds every property as a string).
    //   - never trust numFrames: clamp it to <= 256 and verify it against the
    //     decoded length before allocating.
    juce::ignoreUnused (childOrInvalid);
}

void OSimpleWavetableAudioProcessor::writeImportedBank (juce::ValueTree& state) const
{
    // Stage 1: no bank exists, so nothing is appended.
    // Stage 2.4: append exactly ONE IMPORTED_BANK child (version, filename,
    // numFrames, encoding = flac16 | pcm16gz, data = base64) from the
    // processor-owned cached blob, under its CriticalSection.
    juce::ignoreUnused (state);
}

//==============================================================================
// Factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleWavetableAudioProcessor();
}
