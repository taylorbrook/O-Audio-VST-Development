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
    // NaN / inf guard for every APVTS read on the audio thread (a NaN that
    // reaches a SmoothedValue is sticky across reset()).
    float finiteOr (float v, float fallback) noexcept
    {
        return std::isfinite (v) ? v : fallback;
    }

    // Choice / bool resolve: round + clamp, never a truncating cast.
    int choiceIndex (float v, int numChoices) noexcept
    {
        if (! std::isfinite (v) || numChoices <= 1)
            return 0;
        const float c = juce::jlimit (0.0f, (float) (numChoices - 1), v);
        return juce::jlimit (0, numChoices - 1, (int) std::round (c));
    }

    constexpr int kNumBitDepthChoices = 15;   // Full, 16..3
    constexpr int kNumVoiceModes      = 2;    // Poly, Mono
    constexpr int kMidiChunkBytes     = 32768;
    constexpr int kWheelMidiBytes     = 4096;

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
    namespace ids = OSimpleWavetable::ParamIDs;
    pBank       = parameters.getRawParameterValue (ids::bank);
    pPosition   = parameters.getRawParameterValue (ids::position);
    pInterp     = parameters.getRawParameterValue (ids::interp);
    pBandlimit  = parameters.getRawParameterValue (ids::bandlimit);
    pBitDepth   = parameters.getRawParameterValue (ids::bitDepth);
    pAmpAttack  = parameters.getRawParameterValue (ids::ampAttack);
    pAmpDecay   = parameters.getRawParameterValue (ids::ampDecay);
    pAmpSustain = parameters.getRawParameterValue (ids::ampSustain);
    pAmpRelease = parameters.getRawParameterValue (ids::ampRelease);
    pVoiceMode  = parameters.getRawParameterValue (ids::voiceMode);
    pOutput     = parameters.getRawParameterValue (ids::outputLevel);
    jassert (pBank != nullptr && pPosition != nullptr && pInterp != nullptr && pBandlimit != nullptr
             && pBitDepth != nullptr && pAmpAttack != nullptr && pAmpDecay != nullptr
             && pAmpSustain != nullptr && pAmpRelease != nullptr && pVoiceMode != nullptr
             && pOutput != nullptr);

    for (int i = 0; i < kNumVoices; ++i)
    {
        auto* v = new WtVoice();                   // all allocation here, never on the audio thread
        v->setBlockContext (&blockCtx);            // stable for the processor's lifetime
        wtVoices[(size_t) i] = v;
        synth.addVoice (v);                        // synth owns it
    }
    synth.addSound (new WtSound());
    synth.setNoteStealingEnabled (true);
    midiCollector.reset (44100.0);                 // valid base before the first prepareToPlay (sibling)
}

OSimpleWavetableAudioProcessor::~OSimpleWavetableAudioProcessor() = default;

//==============================================================================
const WavetableBank* OSimpleWavetableAudioProcessor::resolveBank (int bankIndex) const noexcept
{
    // 0..4 built-in (never freed while this instance lives: builtIns holds a
    // ref); 5 Imported (nullptr = empty = silence until 2.4 publishes).
    return bankIndex < BuiltInBanks::kCount ? builtIns->get (bankIndex)
                                            : importedForAudio.load();
}

juce::ADSR::Parameters OSimpleWavetableAudioProcessor::currentAmpParams() const noexcept
{
    const float a = juce::jlimit (0.001f, 5.0f, finiteOr (pAmpAttack->load(),  0.005f));
    const float d = juce::jlimit (0.001f, 5.0f, finiteOr (pAmpDecay->load(),   0.3f));
    const float s = juce::jlimit (0.0f,   1.0f, finiteOr (pAmpSustain->load(), 0.8f));
    const float r = juce::jlimit (0.001f, 5.0f, finiteOr (pAmpRelease->load(), 0.2f));
    return { a, d, s, r };
}

float OSimpleWavetableAudioProcessor::currentKnob() const noexcept
{
    return wt::clamp01 (finiteOr (pPosition->load(), 0.0f));
}

float OSimpleWavetableAudioProcessor::currentOutputGain() const noexcept
{
    const float db = juce::jlimit (-60.0f, 6.0f, finiteOr (pOutput->load(), -6.0f));
    return juce::Decibels::decibelsToGain (db, -60.0f);   // -60 dB -> exactly 0
}

//==============================================================================
void OSimpleWavetableAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedBlock = juce::jmax (1, samplesPerBlock);

    midiCollector.reset (sampleRate);
    synth.setCurrentPlaybackSampleRate (sampleRate);

    // Per-chunk buffers (the chunk loop never exceeds preparedBlock).
    knobBuf.assign ((size_t) preparedBlock, 0.0f);
    zeroBuf.assign ((size_t) preparedBlock, 0.0f);
    blockCtx.knobPos   = knobBuf.data();
    blockCtx.lfo       = zeroBuf.data();   // 2.3 replaces these three
    blockCtx.lfoDepth  = zeroBuf.data();
    blockCtx.envAmount = zeroBuf.data();
    blockCtx.bank          = resolveBank (choiceIndex (pBank->load(), kNumBanks));
    blockCtx.interp        = pInterp->load() >= 0.5f;
    blockCtx.bandlimit     = pBandlimit->load() >= 0.5f;
    blockCtx.bitDepthIndex = choiceIndex (pBitDepth->load(), kNumBitDepthChoices);

    chunkMidi.clear();
    chunkMidi.ensureSize ((size_t) kMidiChunkBytes);
    wheelMidi.clear();
    wheelMidi.ensureSize ((size_t) kWheelMidiBytes);

    // SEED the smoothers after reset() so prepareToPlay never fades in from
    // silence (pattern_gain_ramp_without_seeding_fades_in_from_silence).
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (currentOutputGain());
    knobSmooth.reset (sampleRate, 0.02);
    knobSmooth.setCurrentAndTargetValue (currentKnob());

    const auto amp = currentAmpParams();
    for (auto* v : wtVoices)                                    // SynthesiserVoice has no virtual prepare
        v->prepareToPlay (sampleRate, preparedBlock, amp);

    monoStack.clear();
    lastVoiceMode = choiceIndex (pVoiceMode->load(), kNumVoiceModes);
    dispSounding.store (false, std::memory_order_relaxed);

    setLatencySamples (0);                                      // getLatencySamples() is non-virtual (JUCE 8)
}

void OSimpleWavetableAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
    monoStack.clear();
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

    // ---- 2.4 seam (REG-01): blockEntries.fetch_add (1) goes HERE, first. ----

    const int numSamples = buffer.getNumSamples();
    buffer.clear();                                            // voices ADD into a cleared buffer
    if (numSamples > 0 && buffer.getNumChannels() > 0)         // collector jasserts numSamples > 0
        renderBlock (buffer, midi, numSamples);

    // ---- 2.4 seam: audioHeldBank.store (this block's resolved bank) and then
    //      blockGeneration.fetch_add (1) go HERE. This is the ONLY exit of
    //      processBlock (the 0-sample / 0-channel case falls through to it):
    //      keep it that way so the entry/exit pair stays consistent. ----
}

void OSimpleWavetableAudioProcessor::renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                                                  int numSamples)
{
    const int numCh = buffer.getNumChannels();
    midiCollector.removeNextBlockOfMessages (midi, numSamples);

    // Poly <-> Mono switch: detected HERE (never call synth.* from the
    // message thread; the Synthesiser locks inside processNextBlock).
    const int mode = choiceIndex (pVoiceMode->load(), kNumVoiceModes);
    if (mode != lastVoiceMode)
    {
        synth.allNotesOff (0, false);
        monoStack.clear();
        lastVoiceMode = mode;
    }

    // Resolve the bank ONCE per block (Processing Order 2).
    blockCtx.bank          = resolveBank (choiceIndex (pBank->load(), kNumBanks));
    blockCtx.interp        = finiteOr (pInterp->load(), 1.0f) >= 0.5f;
    blockCtx.bandlimit     = finiteOr (pBandlimit->load(), 1.0f) >= 0.5f;
    blockCtx.bitDepthIndex = choiceIndex (pBitDepth->load(), kNumBitDepthChoices);

    const auto amp = currentAmpParams();
    for (auto* v : wtVoices)
        v->setBlockParams (amp);                               // dirty-checked inside

    knobSmooth.setTargetValue (currentKnob());

    // getWritePointer() clears AudioBuffer's isClear flag. The voices write
    // through per-chunk views, which the parent buffer cannot see, so this
    // MUST happen before rendering (else applyGainRamp / copyFrom below would
    // treat channel 0 as silent).
    float* ch0 = buffer.getWritePointer (0);

    // Oversized-host-block guard: chunks of at most preparedBlock. Each chunk
    // is rendered into its own 1-channel VIEW of channel 0 with its MIDI
    // sliced and re-based to the view, so the voices' in-buffer index
    // startSample + i addresses the BlockContext arrays directly (D-H), and
    // a sub-range Synthesiser render never fires later events early.
    for (int start = 0; start < numSamples; start += preparedBlock)
    {
        const int n = juce::jmin (preparedBlock, numSamples - start);

        for (int i = 0; i < n; ++i)
            knobBuf[(size_t) i] = knobSmooth.getNextValue();

        chunkMidi.clear();                                     // keeps capacity: no allocation
        for (const auto meta : midi)
        {
            const int p = juce::jlimit (0, numSamples - 1, meta.samplePosition);
            if (p >= start && p < start + n)
                chunkMidi.addEvent (meta.data, meta.numBytes, p - start);
        }

        float* chunkChannels[1] = { ch0 + start };
        juce::AudioBuffer<float> view (chunkChannels, 1, n);   // refers to ch0; no allocation (<= 32 ch)

        if (mode == 0)
            synth.renderNextBlock (view, chunkMidi, 0, n);
        else
            renderMono (view, chunkMidi, n);
    }

    updateDisplayFromLeadVoice();

    // Output stage: seeded 20 ms gain ramp (-60 dB = exactly 0), isfinite
    // scrub, then mono -> every output channel.
    outputGain.setTargetValue (currentOutputGain());
    const float g0 = outputGain.getCurrentValue();
    const float g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, 0, numSamples, g0, g1);

    for (int i = 0; i < numSamples; ++i)
        if (! std::isfinite (ch0[i]))
            ch0[i] = 0.0f;

    for (int ch = 1; ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

//==============================================================================
// Mono = true legato (CONTEXT): last-note priority on voice 0. An overlapping
// note changes pitch only; a note with nothing held retriggers (noteOn from
// the current level, no reset). Releasing a note returns to the previous
// held one. Adapted from O-simpleSubtractive renderMonoLegato (mode 2).
void OSimpleWavetableAudioProcessor::renderMono (juce::AudioBuffer<float>& view, const juce::MidiBuffer& chunk,
                                                 int numSamples)
{
    auto* v0 = wtVoices[0];

    // Keep the Synthesiser's lastPitchWheelValues current while it is bypassed,
    // so the first Poly note after a Mono -> Poly switch starts with the real
    // bend. numSamples = 0: no voice renders, every event is just handled.
    // (No voice is playing a channel in Mono, so none receives the wheel twice.)
    wheelMidi.clear();
    for (const auto meta : chunk)
        if (meta.numBytes >= 3 && (meta.data[0] & 0xf0) == 0xe0)
            wheelMidi.addEvent (meta.data, meta.numBytes, meta.samplePosition);
    if (! wheelMidi.isEmpty())
        synth.renderNextBlock (view, wheelMidi, 0, 0);

    int pos = 0;
    for (const auto meta : chunk)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos)
        {
            v0->renderNextBlock (view, pos, evPos - pos);
            pos = evPos;
        }

        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const bool wasHeld = ! monoStack.empty();
            monoStack.push (m.getNoteNumber(), m.getFloatVelocity());
            v0->noteOnDirect (m.getNoteNumber(), m.getFloatVelocity(), ! wasHeld);   // legato while held
        }
        else if (m.isNoteOff())
        {
            monoStack.remove (m.getNoteNumber());
            if (monoStack.empty())
                v0->noteOffDirect (true);                      // release tail
            else
                v0->setPitchNote (monoStack.topNote());        // back to the held note, no retrigger
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            monoStack.clear();
            v0->noteOffDirect (false);
        }
        else if (m.isPitchWheel())
        {
            v0->pitchWheelMoved (m.getPitchWheelValue());
        }
    }

    if (pos < numSamples)
        v0->renderNextBlock (view, pos, numSamples - pos);
}

//==============================================================================
void OSimpleWavetableAudioProcessor::updateDisplayFromLeadVoice() noexcept
{
    // Lead voice = the NEWEST note among sounding voices (isSounding() = amp
    // env active; not isVoiceActive(), which is false for the Mono voice).
    const WtVoice* lead = nullptr;
    std::uint64_t newest = 0;
    for (const auto* v : wtVoices)
        if (v->isSounding() && (lead == nullptr || v->getNoteAge() > newest))
        {
            lead = v;
            newest = v->getNoteAge();
        }

    if (lead != nullptr)
    {
        dispPos.store   (lead->getLastPos(),   std::memory_order_relaxed);
        dispLevel.store (lead->getLastLevel(), std::memory_order_relaxed);
        dispFrame.store (lead->getLastFrame(), std::memory_order_relaxed);
        dispSounding.store (true, std::memory_order_relaxed);
    }
    else
    {
        dispSounding.store (false, std::memory_order_relaxed);
    }
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
