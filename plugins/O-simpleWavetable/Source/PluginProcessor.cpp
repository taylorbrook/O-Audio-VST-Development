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
#include <cstring>
#include <limits>

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
    constexpr int kNumLfoSyncModes    = 2;    // Free, Tempo
    constexpr int kMidiChunkBytes     = 32768;
    constexpr int kWheelMidiBytes     = kMidiChunkBytes;   // note 3: wheelMidi is a subset of chunkMidi
    constexpr int kMidiEventHeader    = (int) (sizeof (juce::int32) + sizeof (juce::uint16));   // MidiBuffer per-event header

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
    for (size_t i = 0; i < ids::all.size(); ++i)
    {
        raw[i] = parameters.getRawParameterValue (ids::all[i]);
        jassert (raw[i] != nullptr);
    }

    for (int i = 0; i < kNumVoices; ++i)
    {
        auto* v = new WtVoice();                   // all allocation here, never on the audio thread
        v->setBlockContext (&blockCtx);            // stable for the processor's lifetime
        wtVoices[(size_t) i] = v;
        synth.addVoice (v);                        // synth owns it
    }
    synth.addSound (new WtSound());
    synth.setNoteStealingEnabled (true);
    // D-L: events split the render at their exact sample (JUCE's default 32
    // non-strict handles events < 32 samples into a sub-block EARLY, which
    // depends on the host partition). Constructor = no audio thread yet.
    synth.setMinimumRenderingSubdivisionSize (1, true);
    midiCollector.reset (44100.0);                 // valid base before the first prepareToPlay (sibling)

    // Stage 4: a fresh instance IS the Init preset (kInit == the APVTS
    // defaults), unmodified (D-AD). No file I/O here (D-AH).
    setCurrentPreset (wtpresets::kFactory[0].name, resolveTargets (recipeTargets (wtpresets::kFactory[0])), true, false);
    lastStatusBank.store (getSelectedBankIndex());  // W4 seed (D-AL)

    startTimer (250);                              // reaper sweep (no callbacks without a message loop: harness sweeps itself)
}

OSimpleWavetableAudioProcessor::~OSimpleWavetableAudioProcessor()
{
    importGen.fetch_add (1);                       // supersede: an in-flight decode exits at its next chunk
    importPool.removeAllJobs (true, 10000);        // jobs capture this
    stopTimer();
    cancelPendingUpdate();
}

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
    const float a = juce::jlimit (0.001f, 5.0f, finiteOr (raw[Slot::sAmpAttack]->load(),  0.005f));
    const float d = juce::jlimit (0.001f, 5.0f, finiteOr (raw[Slot::sAmpDecay]->load(),   0.3f));
    const float s = juce::jlimit (0.0f,   1.0f, finiteOr (raw[Slot::sAmpSustain]->load(), 0.8f));
    const float r = juce::jlimit (0.001f, 5.0f, finiteOr (raw[Slot::sAmpRelease]->load(), 0.2f));
    return { a, d, s, r };
}

juce::ADSR::Parameters OSimpleWavetableAudioProcessor::currentModEnvParams() const noexcept
{
    const float a = juce::jlimit (0.001f, 10.0f, finiteOr (raw[Slot::sMenvAttack]->load(),  0.5f));
    const float d = juce::jlimit (0.001f, 10.0f, finiteOr (raw[Slot::sMenvDecay]->load(),   1.0f));
    const float s = juce::jlimit (0.0f,   1.0f,  finiteOr (raw[Slot::sMenvSustain]->load(), 0.0f));
    const float r = juce::jlimit (0.001f, 10.0f, finiteOr (raw[Slot::sMenvRelease]->load(), 0.5f));
    return { a, d, s, r };
}

float OSimpleWavetableAudioProcessor::currentKnob() const noexcept
{
    return wt::clamp01 (finiteOr (raw[Slot::sPosition]->load(), 0.0f));
}

float OSimpleWavetableAudioProcessor::currentLfoDepth() const noexcept
{
    return wt::clamp01 (finiteOr (raw[Slot::sLfoDepth]->load(), 0.0f));
}

float OSimpleWavetableAudioProcessor::currentEnvAmount() const noexcept
{
    return juce::jlimit (-1.0f, 1.0f, finiteOr (raw[Slot::sEnvAmount]->load(), 0.0f));   // finite before jlimit (NaN)
}

float OSimpleWavetableAudioProcessor::currentOutputGain() const noexcept
{
    const float db = juce::jlimit (-60.0f, 6.0f, finiteOr (raw[Slot::sOutputLevel]->load(), -6.0f));
    return juce::Decibels::decibelsToGain (db, -60.0f);   // -60 dB -> exactly 0
}

//==============================================================================
void OSimpleWavetableAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedBlock = juce::jmax (1, samplesPerBlock);

    midiCollector.reset (sampleRate);
    synth.setCurrentPlaybackSampleRate (sampleRate);

    // Per-chunk buffers (the chunk loop never exceeds preparedBlock). The
    // LFO is rendered even at depth 0 (the display needs it), so its buffer
    // is always valid.
    knobBuf.assign ((size_t) preparedBlock, 0.0f);
    depthBuf.assign ((size_t) preparedBlock, 0.0f);
    amtBuf.assign ((size_t) preparedBlock, 0.0f);
    lfo.prepare (sampleRate, preparedBlock);           // phase 0, cycle 0: deterministic
    blockCtx.knobPos   = knobBuf.data();
    blockCtx.lfo       = lfo.data();
    blockCtx.lfoDepth  = depthBuf.data();
    blockCtx.envAmount = amtBuf.data();
    // bank / interp / bandlimit / bitDepthIndex are set by renderBlock before
    // any voice renders (no voice renders outside it).

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
    depthSmooth.reset (sampleRate, 0.02);
    depthSmooth.setCurrentAndTargetValue (currentLfoDepth());
    amtSmooth.reset (sampleRate, 0.02);
    amtSmooth.setCurrentAndTargetValue (currentEnvAmount());

    const auto amp  = currentAmpParams();
    const auto menv = currentModEnvParams();
    for (auto* v : wtVoices)                                    // SynthesiserVoice has no virtual prepare
        v->prepareToPlay (sampleRate, preparedBlock, amp, menv);

    monoStack.clear();
    lastVoiceMode = choiceIndex (raw[Slot::sVoiceMode]->load(), kNumVoiceModes);
    lastBankIndex = -1;
    // Voices were just reset (no cfg survives), and prepareToPlay is never
    // concurrent with processBlock: nothing is audio-held any more.
    audioHeldBank.store (nullptr);
    resetDisplayState (sampleRate);

    setLatencySamples (0);                                      // getLatencySamples() is non-virtual (JUCE 8)
    prepared.store (true);                                      // note 4: LAST (buffers sized above)
}

void OSimpleWavetableAudioProcessor::resetDisplayState (double fs) noexcept
{
    dispSounding.store (false, std::memory_order_relaxed);
    dispLfo.store (0.0f, std::memory_order_relaxed);
    dispMenv.store (0.0f, std::memory_order_relaxed);
    dispAmp.store (0.0f, std::memory_order_relaxed);
    dispNote.store (-1, std::memory_order_relaxed);                    // D-X
    dispHz.store (0.0f, std::memory_order_relaxed);
    displayFs.store (fs, std::memory_order_relaxed);
}

void OSimpleWavetableAudioProcessor::releaseResources()
{
    // Note 4: prepared is NOT cleared here (D-AJ(1)): the buffers stay valid,
    // and a host that processes after release must not go silent.
    synth.allNotesOff (0, false);                  // hard stop: no voice keeps a cfg.bank
    monoStack.clear();
    audioHeldBank.store (nullptr);                 // not concurrent with processBlock
    bool clearDisp = true;
   #if OSIW_TEST_HOOKS
    clearDisp = testReleaseClears;
   #endif
    if (clearDisp)
        resetDisplayState (displayFs.load (std::memory_order_relaxed));   // N4: no frozen "sounding" lamp
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
    blockEntries.fetch_add (1);                                // REG-01: FIRST statement, seq_cst

    const int numSamples = buffer.getNumSamples();
    buffer.clear();                                            // voices ADD into a cleared buffer
    bool ready = prepared.load();                              // note 4: unprepared -> silence
   #if OSIW_TEST_HOOKS
    if (! testPreparedGuard)
        ready = true;
   #endif
    if (ready && numSamples > 0 && buffer.getNumChannels() > 0)   // collector jasserts numSamples > 0
    {
        const WavetableBank* resolved = renderBlock (buffer, midi, numSamples);
        audioHeldBank.store (resolved);                        // D-C: BEFORE the exit increment, seq_cst
    }
    // 0-sample / 0-channel block: no voice ran, so audioHeldBank is unchanged.

   #if OSIW_TEST_HOOKS
    if (midBlockFn != nullptr)
        midBlockFn (midBlockCtx);                              // block still in flight (entries != exits)
   #endif

    // This is the ONLY exit of processBlock (the 0-sample / 0-channel case
    // falls through to it): the entry/exit pair stays consistent (REG-01).
    blockGeneration.fetch_add (1);                             // LAST, seq_cst
}

const WavetableBank* OSimpleWavetableAudioProcessor::renderBlock (juce::AudioBuffer<float>& buffer,
                                                                  juce::MidiBuffer& midi, int numSamples)
{
    const int numCh = buffer.getNumChannels();
    midiCollector.removeNextBlockOfMessages (midi, numSamples);

    // Poly <-> Mono switch: detected HERE (never call synth.* from the
    // message thread; the Synthesiser locks inside processNextBlock).
    const int mode = choiceIndex (raw[Slot::sVoiceMode]->load(), kNumVoiceModes);
    if (mode != lastVoiceMode)
    {
        synth.allNotesOff (0, false);
        monoStack.clear();
        lastVoiceMode = mode;
        bool seed = mode == 1;
       #if OSIW_TEST_HOOKS
        seed = seed && testMonoWheelSeed;
       #endif
        if (seed)
            wtVoices[0]->pitchWheelMoved (wheelNow);           // W3: voice 0 missed the wheel while idle in Poly
    }

    // Resolve the bank ONCE per block (Processing Order 2). Every voice
    // renders this block against this pointer; it becomes audioHeldBank.
    const int bankIndex = choiceIndex (raw[Slot::sBank]->load(), kNumBanks);
    if (bankIndex != lastBankIndex)
    {
        bankDisplayGen.fetch_add (1, std::memory_order_relaxed);   // Stage 3 bankUpdate seam
        lastBankIndex = bankIndex;
    }
    const WavetableBank* resolved = resolveBank (bankIndex);
   #if OSIW_TEST_HOOKS
    WavetableBank::testNoteDeref (resolved);
   #endif
    blockCtx.bank          = resolved;
    blockCtx.interp        = finiteOr (raw[Slot::sInterp]->load(), 1.0f) >= 0.5f;
    blockCtx.bandlimit     = finiteOr (raw[Slot::sBandlimit]->load(), 1.0f) >= 0.5f;
    blockCtx.bitDepthIndex = choiceIndex (raw[Slot::sBitDepth]->load(), kNumBitDepthChoices);

    const auto amp  = currentAmpParams();
    const auto menv = currentModEnvParams();
    for (auto* v : wtVoices)
        v->setBlockParams (amp, menv);                         // dirty-checked inside (both envs)

    knobSmooth.setTargetValue (currentKnob());
    depthSmooth.setTargetValue (currentLfoDepth());
    amtSmooth.setTargetValue (currentEnvAmount());
   #if OSIW_TEST_HOOKS
    if (testKnobRampOff)
        knobSmooth.setCurrentAndTargetValue (currentKnob());
   #endif

    // LFO block settings + host transport, read ONCE per block.
    const auto lfoShape = (PositionLfo::Shape) choiceIndex (raw[Slot::sLfoShape]->load(), PositionLfo::kNumShapes);
    const bool lfoTempo = choiceIndex (raw[Slot::sLfoSync]->load(), kNumLfoSyncModes) == 1;
    const int  lfoDiv   = choiceIndex (raw[Slot::sLfoDiv]->load(), PositionLfo::kNumDivisions);
    const double lfoRate = (double) juce::jlimit (0.01f, 20.0f, finiteOr (raw[Slot::sLfoRate]->load(), 0.5f));
    const auto transport = PositionLfo::readTransport (getPlayHead());
    int lastChunkLen = 0;

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
    bool guardMidi = true;
   #if OSIW_TEST_HOOKS
    guardMidi = testMidiGuard;
   #endif
    auto evIt = midi.cbegin();
    const auto evEnd = midi.cend();
    for (int start = 0; start < numSamples;)
    {
        int n = juce::jmin (preparedBlock, numSamples - start);

        // note 3: fill the chunk's MIDI only up to the capacity reserved in
        // prepareToPlay. A chunk that would overflow ends at the first event
        // that does not fit (n may be 0: the events are then handled with no
        // render), so the copy never grows chunkMidi on the audio thread.
        chunkMidi.clear();                                    // keeps capacity: no allocation
        int chunkBytes = 0;
        for (; evIt != evEnd; ++evIt)
        {
            const auto meta = *evIt;
            const int p = juce::jlimit (0, numSamples - 1, meta.samplePosition);
            if (p >= start + n)
                break;
            if (guardMidi && meta.numBytes > 3)
                continue;                                     // sysex / meta: neither path reads them
            const int need = kMidiEventHeader + meta.numBytes;
            if (guardMidi && chunkBytes + need > kMidiChunkBytes)
            {
                n = p - start;                                // end the chunk at this event
                break;
            }
            chunkMidi.addEvent (meta.data, meta.numBytes, p - start);
            chunkBytes += need;
            if (meta.numBytes >= 3 && (meta.data[0] & 0xf0) == 0xe0)
                wheelNow = (meta.data[1] & 0x7f) | ((meta.data[2] & 0x7f) << 7);   // W3
        }

        for (int i = 0; i < n; ++i)
        {
            knobBuf[(size_t) i]  = knobSmooth.getNextValue();
            depthBuf[(size_t) i] = depthSmooth.getNextValue();
            amtBuf[(size_t) i]   = amtSmooth.getNextValue();
        }

        // Rendered even at depth 0 (display). Writes into lfo's own buffer,
        // which blockCtx.lfo already points at. PPQ advanced to this chunk.
        lfo.render (n, lfoShape, lfoTempo, lfoRate, lfoDiv, lfo.offsetBy (transport, start));
        if (n > 0)
            lastChunkLen = n;

        float* chunkChannels[1] = { ch0 + start };
        juce::AudioBuffer<float> view (chunkChannels, 1, n);   // refers to ch0; no allocation (<= 32 ch)

        if (mode == 0)
            synth.renderNextBlock (view, chunkMidi, 0, n);
        else
            renderMono (view, chunkMidi, n);
        start += n;
    }

    updateDisplayFromLeadVoice();
    dispLfo.store (lfo.lastValue (lastChunkLen), std::memory_order_relaxed);

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

    return resolved;
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
            monoStack.push (m.getNoteNumber());
            v0->noteOnDirect (m.getNoteNumber(), m.getFloatVelocity(), ! wasHeld);   // legato while held
        }
        else if (m.isNoteOff())
        {
            const bool wasHeld = ! monoStack.empty();
            monoStack.remove (m.getNoteNumber());
            if (! monoStack.empty())
                v0->setPitchNote (monoStack.topNote());        // back to the held note, no retrigger
            else if (wasHeld)
                v0->noteOffDirect (true);                      // held -> empty: ONE release (v1.0.1)
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            // Same as Poly (juce::Synthesiser::handleMidiEvent maps both to a
            // release), so a transport stop rings out in both modes (v1.0.1).
            monoStack.clear();
            v0->noteOffDirect (true);
        }
        else if (m.isPitchWheel())
        {
            v0->pitchWheelMoved (m.getPitchWheelValue());
        }
    }

    if (pos < numSamples)
        v0->renderNextBlock (view, pos, numSamples - pos);

    // Voices 1.. never get a Mono note, but a Poly -> Mono switch hard-stops
    // them: render their 2 ms hard-stop tails (W1). Idle voices return at once.
    for (size_t i = 1; i < wtVoices.size(); ++i)
        wtVoices[i]->renderNextBlock (view, 0, numSamples);
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
        dispMenv.store  (lead->getLastModEnv(), std::memory_order_relaxed);
        dispAmp.store   (lead->getLastAmpEnv(), std::memory_order_relaxed);
        dispNote.store  (lead->getLastNote(), std::memory_order_relaxed);         // D-X
        dispHz.store    ((float) lead->getCurrentHz(), std::memory_order_relaxed);
        dispSounding.store (true, std::memory_order_relaxed);
    }
    else
    {
        dispMenv.store (0.0f, std::memory_order_relaxed);
        dispAmp.store (0.0f, std::memory_order_relaxed);
        dispNote.store (-1, std::memory_order_relaxed);                // D-X
        dispHz.store (0.0f, std::memory_order_relaxed);
        dispSounding.store (false, std::memory_order_relaxed);
    }
}

//==============================================================================
// Stage 3 visualization (message thread only; see PluginProcessor.h).
int OSimpleWavetableAudioProcessor::getSelectedBankIndex() const noexcept
{
    return choiceIndex (raw[Slot::sBank]->load(), kNumBanks);
}

void OSimpleWavetableAudioProcessor::buildCycleView (CycleView& v)
{
    const int bankIdx = getSelectedBankIndex();

    // Imported: a snapshot held for THIS call only (Amendment 12, P6). The
    // raw audio-thread bank pointer is never read here.
    std::shared_ptr<const WavetableBank> hold;
    const WavetableBank* b = nullptr;
    if (bankIdx < BuiltInBanks::kCount)
    {
        b = builtIns->get (bankIdx);
    }
    else
    {
        hold = getImportedBankSnapshot();
        b = hold.get();
    }
    if (b != nullptr && b->numFrames <= 0)
        b = nullptr;

    // Same reads as renderBlock (interp / bit_depth).
    const bool interp   = finiteOr (raw[Slot::sInterp]->load(), 1.0f) >= 0.5f;
    const int  bitIdx   = choiceIndex (raw[Slot::sBitDepth]->load(), kNumBitDepthChoices);
    const bool sounding = dispSounding.load (std::memory_order_relaxed);
    const int  nF       = b != nullptr ? b->numFrames : 0;

    // Sounding: the lead voice's effective position, mip level and latched
    // frame. Silent (D-R, P4): the knob, level 0, the knob's latched frame
    // (the stale disp* values are ignored).
    const float pos     = sounding ? dispPos.load (std::memory_order_relaxed) : currentKnob();
    const int   level   = sounding ? juce::jlimit (0, WavetableBank::kLevels - 1,
                                                   dispLevel.load (std::memory_order_relaxed))
                                   : 0;
    const int   latched = sounding ? dispFrame.load (std::memory_order_relaxed) : wt::latchFrame (pos, nF);

    vizRenderer.render (b, level, interp, pos, latched, bitIdx, v);

    v.pos      = wt::clamp01 (pos);
    v.frame    = interp ? -1 : juce::jlimit (0, juce::jmax (0, nF - 1), latched);   // P3: dispFrame is rounded with Interp On
    v.level    = level;
    v.kmax     = WavetableBank::kmax (level);
    v.sounding = sounding;

    if (sounding)
    {
        const float hz    = dispHz.load (std::memory_order_relaxed);
        const double fs   = displayFs.load (std::memory_order_relaxed);
        v.note     = dispNote.load (std::memory_order_relaxed);
        v.f0       = std::isfinite (hz) && hz > 0.0f ? hz : 0.0f;
        v.nyquistH = v.f0 > 0.0f && fs > 0.0 ? (float) (0.5 * fs / (double) v.f0) : 0.0f;
    }
    else
    {
        v.note     = -1;
        v.f0       = 0.0f;
        v.nyquistH = 0.0f;
    }

    // D-P / P1: the free LFO runs at depth 0 (display); the lamp only shows it
    // when the depth is non-zero, so an idle editor stays quiet.
    v.lfo  = currentLfoDepth() > 0.0f ? dispLfo.load (std::memory_order_relaxed) : 0.0f;
    v.menv = dispMenv.load (std::memory_order_relaxed);
    v.amp  = dispAmp.load (std::memory_order_relaxed);
}

void OSimpleWavetableAudioProcessor::getBankThumbnails (BankThumbs& t) const
{
    const int bankIdx = getSelectedBankIndex();
    t.bank     = bankIdx;
    t.imported = bankIdx >= BuiltInBanks::kCount;

    std::shared_ptr<const WavetableBank> hold;
    const WavetableBank* b = nullptr;
    juce::String name;

    if (! t.imported)
    {
        b = builtIns->get (bankIdx);
    }
    else
    {
        // D-S / P8: the bank and ITS filename in one scope. importStatus.filename
        // may already name a later, failed import.
        const juce::ScopedLock sl (bankStateLock);
        hold = importedOwner;
        name = cachedBlob.filename;
    }
    if (hold != nullptr)
        b = hold.get();

    if (b == nullptr || b->numFrames <= 0)
    {
        t.numFrames = 0;
        t.filename  = juce::String();
        t.points.clear();                          // keeps capacity
        return;
    }

    t.numFrames = b->numFrames;
    t.filename  = t.imported ? name : juce::String();
    t.points.assign (b->thumbs.begin(), b->thumbs.end());   // reuses capacity (editor reserves 256 * 128)
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

    // W5 (D-AM): what the on-screen keyboard holds (a velocity-0 note-on is a
    // note-off). Message thread (the uiMidi native).
    uiHeld[(size_t) noteNumber] = noteOn && velocity > 0.0f;
}

int OSimpleWavetableAudioProcessor::releaseUiHeldNotes()
{
    int n = 0;
    for (int note = 0; note < (int) uiHeld.size(); ++note)
        if (uiHeld[(size_t) note])
        {
            handleUiMidi (note, false, 0.0f);      // queues the note-off and clears the entry
            ++n;
        }
    return n;
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
    state.setProperty (kCurrentPresetProp, getPresetName(), nullptr);   // D-AF ("" = unnamed)
    writeImportedBank (state);                    // exactly one child from the cache (or none)
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

    // D-AF: the preset name. Same isVoid gate; ABSENT -> "" (an older session
    // is unnamed, never the ctor's Init name). Untrusted: sanitised.
    const juce::var cp = state.getProperty (kCurrentPresetProp);
    const juce::String restoredPreset = cp.isVoid() ? juce::String() : sanitisePresetName (cp.toString());

    // Detach BEFORE replaceState so the live APVTS tree never carries the blob
    // (stale-child trap, critical_preset_manager_stale_customstate_child).
    const auto bankChild = state.getChildWithName (kImportedBankTag).createCopy();   // invalid if absent
    stripImportedBank (state);                    // ALL copies

    // Forward compat (v1.0.1): a parameter ABSENT from the incoming state (a
    // session saved before that parameter existed) restores to its DEFAULT.
    // Without this, APVTS::replaceState creates the missing child from the
    // instance's LIVE value (updateParameterConnectionsToChildTrees flushes
    // the current value), i.e. whatever the last preset left behind.
    for (const auto* id : OSimpleWavetable::ParamIDs::all)
    {
        if (state.getChildWithProperty ("id", id).isValid())
            continue;
        if (auto* p = parameters.getParameter (id))
        {
            juce::ValueTree c ("PARAM");
            c.setProperty ("id", id, nullptr);
            c.setProperty ("value", p->convertFrom0to1 (p->getDefaultValue()), nullptr);
            state.appendChild (c, nullptr);
        }
    }

    parameters.replaceState (state);
    restoreImportedBank (bankChild);

    // A factory name rebuilds its targets from the table; a user name is
    // stale until refreshPresetTargetsIfNeeded re-reads its file (D-AF).
    if (const auto* recipe = wtpresets::findByName (restoredPreset))
        setCurrentPreset (recipe->name, resolveTargets (recipeTargets (*recipe)), true, false);
    else
        setCurrentPreset (restoredPreset, Targets {}, false, restoredPreset.isNotEmpty());

    lastStatusBank.store (getSelectedBankIndex());   // W4 seed: LAST (a restored notice survives the first poll)
}

//==============================================================================
// IMPORTED_BANK restore (RESEARCH 6.2 / 6.3). Synchronous on the calling
// (non-audio) thread: a host that calls getStateInformation right after
// setStateInformation gets the same blob, and the harness needs no message
// loop. Untrusted input (ASVS V5): every attribute is a STRING var after an
// XML round trip -> isVoid() gate + getIntValue(); numFrames 1..256; data
// <= 4 M chars; decoded length must equal numFrames * 2048.
void OSimpleWavetableAudioProcessor::restoreImportedBank (const juce::ValueTree& child)
{
    importGen.fetch_add (1);                      // supersede any in-flight import: restore wins

    // Empty Imported bank: RETIRE the old one (never free here), clear the
    // cache, and set the status. `passthrough` keeps a newer-format blob
    // verbatim so it is re-emitted on save.
    auto finishEmpty = [this] (const juce::String& err, const juce::String& filename = {},
                               juce::ValueTree passthrough = {})
    {
        const juce::ScopedLock sl (bankStateLock);
        publishImportedBank (nullptr);
        cachedBlob = {};
        cachedBlob.passthrough = std::move (passthrough);
        pendingAutoSelect = false;
        importStatus = err.isEmpty() ? ImportStatus {}
                                     : ImportStatus { ImportStatus::State::error, filename, 0, err };
        importStatusVersion.fetch_add (1);
    };

    if (! child.isValid())                        // absent -> empty Imported (pre-import sessions)
    {
        finishEmpty ({});
        return;
    }

    auto str = [&child] (const char* key)
    {
        const juce::var v = child.getProperty (key);
        return v.isVoid() ? juce::String() : v.toString();
    };

    const auto versionText = str ("version");
    const int  version     = versionText.getIntValue();
    const auto encoding    = str ("encoding");

    // FORWARD COMPAT: a newer save is kept verbatim, never destroyed.
    if (version > 1 || (version == 1 && encoding != "flac16" && encoding != "pcm16gz"))
    {
        const auto shownName = WavetableImporter::sanitiseName (str ("filename"));
        finishEmpty ("unsupported", shownName, child.createCopy());
        return;
    }

    const int  frames = str ("numFrames").getIntValue();
    const auto data   = str ("data");
    if (version != 1 || frames < 1 || frames > WavetableImporter::kMaxFrames
        || data.isEmpty() || data.length() > WavetableImporter::kMaxDataChars)
    {
        finishEmpty ("unreadable");               // malformed v1 is dropped (Stage 1 P3/P4 fixture)
        return;
    }

    juce::MemoryBlock bytes;
    {
        juce::MemoryOutputStream mos (bytes, false);
        if (! juce::Base64::convertFromBase64 (mos, data))
        {
            finishEmpty ("unreadable");
            return;
        }
    }

    ImportedPcm pcm;
    pcm.filename  = WavetableImporter::sanitiseName (str ("filename"));
    pcm.numFrames = frames;
    const bool decoded = encoding == "flac16" ? WavetableImporter::decodeFlac16 (bytes, frames, pcm.pcm)
                                              : WavetableImporter::decodePcm16Gz (bytes, frames, pcm.pcm);
    auto bank = decoded ? WavetableImporter::buildImportedBank (pcm) : nullptr;   // SAME builder as import (FUNC-04)
    if (bank == nullptr)
    {
        finishEmpty ("unreadable");
        return;
    }

    const juce::ScopedLock sl (bankStateLock);
    publishImportedBank (std::move (bank));
    cachedBlob = { pcm.filename, encoding, data, frames, {} };   // VERBATIM incoming string: save-after-load is identical
    pendingAutoSelect = false;                    // the restored bank param stands
    importStatus = { ImportStatus::State::done, pcm.filename, frames, {} };
    importStatusVersion.fetch_add (1);
}

void OSimpleWavetableAudioProcessor::writeImportedBank (juce::ValueTree& state) const
{
    const juce::ScopedLock sl (bankStateLock);    // never taken on the audio thread

    if (cachedBlob.passthrough.isValid())
    {
        state.appendChild (cachedBlob.passthrough.createCopy(), nullptr);
        return;
    }

    if (cachedBlob.data.isEmpty())
        return;                                   // no import -> no child

    juce::ValueTree c (kImportedBankTag);
    c.setProperty ("version",   1, nullptr);
    c.setProperty ("filename",  cachedBlob.filename, nullptr);
    c.setProperty ("numFrames", cachedBlob.numFrames, nullptr);
    c.setProperty ("encoding",  cachedBlob.encoding, nullptr);
    c.setProperty ("data",      cachedBlob.data, nullptr);   // EXACTLY one child, cached string, no work here
    state.appendChild (c, nullptr);
}

//==============================================================================
// Publish / retire / sweep: REG-01 (O-Prism 0f7d65ce) + the D-C amendment.
// Caller holds bankStateLock; any thread except the audio thread.
void OSimpleWavetableAudioProcessor::publishImportedBank (std::shared_ptr<const WavetableBank> nb)
{
    auto old = std::move (importedOwner);
    importedOwner = std::move (nb);
    importedForAudio.store (importedOwner.get());             // seq_cst publish
    retireBank (std::move (old));
    bankDisplayGen.fetch_add (1, std::memory_order_relaxed);  // Stage 3 bankUpdate seam
}

void OSimpleWavetableAudioProcessor::retireBank (std::shared_ptr<const WavetableBank> b)
{
    sweepRetiredBanks();                                       // producer sweep (REG-01)
    if (b == nullptr)
        return;
    retiredBanks.push_back ({ std::move (b), blockGeneration.load() });   // stamp AFTER the publish store
}

void OSimpleWavetableAudioProcessor::sweepRetiredBanks()
{
    const auto exits     = blockGeneration.load();             // exits FIRST (REG-01)
    const auto entries   = blockEntries.load();
    const bool quiescent = entries == exits;
    const WavetableBank* held = audioHeldBank.load();          // AFTER entries (D-C)

    bool excludeHeld = true;
   #if OSIW_TEST_HOOKS
    excludeHeld = ! testDisableHeldExclusion.load();
   #endif

    size_t keep = 0;
    for (size_t i = 0; i < retiredBanks.size(); ++i)
    {
        auto& r = retiredBanks[i];
        const bool unreachable = ! (excludeHeld && r.bank.get() == held)   // AMENDMENT: the next block may capture from it
                              && (quiescent || exits >= r.retiredAt + 2);  // REG-01 verbatim
        if (! unreachable)
        {
            if (keep != i)
                retiredBanks[keep] = std::move (r);
            ++keep;
            continue;
        }

       #if OSIW_TEST_HOOKS
        if (testGraveyard.load())
        {
            r.bank->testReaped.store (true);                   // parked, not freed: a later read is counted
            graveyard.push_back (std::move (r.bank));
            continue;
        }
       #endif
        r.bank.reset();                                        // freed HERE (never on the audio thread)
    }
    retiredBanks.erase (retiredBanks.begin() + (std::ptrdiff_t) keep, retiredBanks.end());
}

void OSimpleWavetableAudioProcessor::timerCallback()
{
    pollBankForImportStatus();                     // W4: takes bankStateLock itself
    const juce::ScopedLock sl (bankStateLock);
    sweepRetiredBanks();
}

// W4 (D-AL): mirrors the page rule "an error lives until the next bank
// change", with the editor open or closed. The passthrough blob is untouched.
void OSimpleWavetableAudioProcessor::pollBankForImportStatus()
{
    const int idx = getSelectedBankIndex();
    if (idx == lastStatusBank.load())
        return;
    lastStatusBank.store (idx);

    const juce::ScopedLock sl (bankStateLock);
    if (importStatus.state == ImportStatus::State::error)
    {
        importStatus = {};
        importStatusVersion.fetch_add (1);
    }
}

//==============================================================================
// Import API (D-E). Any non-audio thread.
void OSimpleWavetableAudioProcessor::setImportStatus (ImportStatus::State state, const juce::String& filename,
                                                      int frames, const juce::String& error)
{
    const juce::ScopedLock sl (bankStateLock);
    importStatus = { state, filename, frames, error };
    importStatusVersion.fetch_add (1);
}

void OSimpleWavetableAudioProcessor::setImportStatusForJob (juce::uint32 gen, ImportStatus::State state,
                                                            const juce::String& filename, int frames,
                                                            const juce::String& error)
{
    const juce::ScopedLock sl (bankStateLock);
    if (importGen.load() != gen)
        return;                                   // superseded: the newer job owns the status
    importStatus = { state, filename, frames, error };
    importStatusVersion.fetch_add (1);
}

OSimpleWavetableAudioProcessor::ImportStatus OSimpleWavetableAudioProcessor::getImportStatus() const
{
    const juce::ScopedLock sl (bankStateLock);
    return importStatus;
}

std::shared_ptr<const WavetableBank> OSimpleWavetableAudioProcessor::getImportedBankSnapshot() const
{
    const juce::ScopedLock sl (bankStateLock);
    return importedOwner;
}

bool OSimpleWavetableAudioProcessor::importFromFile (const juce::File& file)
{
    const auto name = WavetableImporter::sanitiseName (file.getFileName());
    if (! file.existsAsFile())
    {
        setImportStatus (ImportStatus::State::error, name, 0, "unreadable");
        return false;
    }

    const auto gen = importGen.fetch_add (1) + 1;             // supersedes any in-flight job
    setImportStatus (ImportStatus::State::busy, name, 0, {});

    importPool.addJob ([this, file, name, gen]
    {
        if (importGen.load() != gen)
            return;                                            // superseded while queued
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
        runImportJob (std::move (r), name, gen);
    });
    return true;
}

bool OSimpleWavetableAudioProcessor::importFromMemory (const juce::String& rawName, juce::MemoryBlock&& bytes)
{
    const auto name = WavetableImporter::sanitiseName (rawName);   // JS-supplied: persisted + displayed
    if (bytes.getSize() > WavetableImporter::kMaxMemoryBytes)
    {
        setImportStatus (ImportStatus::State::error, name, 0, "tooLarge");
        return false;
    }
    if (bytes.getSize() == 0)
    {
        setImportStatus (ImportStatus::State::error, name, 0, "unreadable");
        return false;
    }

    const auto gen = importGen.fetch_add (1) + 1;
    setImportStatus (ImportStatus::State::busy, name, 0, {});

    auto block = std::make_shared<juce::MemoryBlock> (std::move (bytes));   // std::function needs a copyable capture
    importPool.addJob ([this, block, name, gen]
    {
        if (importGen.load() != gen)
            return;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (std::make_unique<juce::MemoryInputStream> (*block, false)));
        runImportJob (std::move (r), name, gen);              // the reader dies inside, before the block
    });
    return true;
}

// Worker thread. Decode -> canonical int16 -> blob -> bank, then re-check the
// generation UNDER the lock and publish. Publish BEFORE auto-select.
void OSimpleWavetableAudioProcessor::runImportJob (std::unique_ptr<juce::AudioFormatReader> reader,
                                                   const juce::String& name, juce::uint32 gen)
{
    auto stale = [this, gen] { return importGen.load() != gen; };

    if (reader == nullptr)
    {
        setImportStatusForJob (gen, ImportStatus::State::error, name, 0, "unreadable");
        return;
    }

    auto res = WavetableImporter::decode (*reader, name, stale);
    reader.reset();

    if (res.error == ImportError::cancelled || stale())
        return;                                               // superseded: never surfaced
    if (res.error != ImportError::none)
    {
        setImportStatusForJob (gen, ImportStatus::State::error, name, 0, WavetableImporter::errorCode (res.error));
        return;                                               // bank untouched
    }

    juce::String encoding = "flac16";
    auto data = WavetableImporter::encodeFlac16 (res.pcm.pcm);
    if (data.isEmpty())
    {
        encoding = "pcm16gz";                                 // FLAC writer refused: lossless fallback
        data = WavetableImporter::encodePcm16Gz (res.pcm.pcm);
    }

    auto bank = WavetableImporter::buildImportedBank (res.pcm);   // ~23 MB worst case, on the worker
    if (bank == nullptr || data.isEmpty())
    {
        setImportStatusForJob (gen, ImportStatus::State::error, name, 0, "unreadable");
        return;
    }

    {
        const juce::ScopedLock sl (bankStateLock);
        if (importGen.load() != gen)
            return;                                           // superseded at the last moment: the bank dies here (worker)
        publishImportedBank (std::move (bank));               // atomic store + retire + producer sweep
        cachedBlob = { name, encoding, data, res.pcm.numFrames, {} };
        importStatus = { ImportStatus::State::done, name, res.pcm.numFrames, {} };
        importStatusVersion.fetch_add (1);
        pendingAutoSelect = true;                             // set with the publish (a later restore clears it)
    }
    triggerAsyncUpdate();                                     // handleAsyncUpdate (message thread): bank = Imported
}

void OSimpleWavetableAudioProcessor::handleAsyncUpdate()
{
    bool select = false;
    {
        const juce::ScopedLock sl (bankStateLock);
        select = pendingAutoSelect;
        pendingAutoSelect = false;
    }

    if (select)
        if (auto* p = parameters.getParameter (OSimpleWavetable::ParamIDs::bank))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) kImportedIdx));
            p->endChangeGesture();
        }
}

// Drop path (D-Z). Any non-audio thread (the WebView native runs on the
// message thread). Cap first: never decode a payload that could not pass
// importFromMemory's byte cap anyway.
bool OSimpleWavetableAudioProcessor::importFromBase64 (const juce::String& name, const juce::String& base64)
{
    constexpr std::size_t kMaxBase64Chars = WavetableImporter::kMaxMemoryBytes / 3 * 4 + 4;

    if (base64.getNumBytesAsUTF8() > kMaxBase64Chars)
    {
        setImportStatus (ImportStatus::State::error, WavetableImporter::sanitiseName (name), 0, "tooLarge");
        return false;
    }

    // STANDARD base64 (btoa alphabet) via juce::Base64. JUCE's MemoryBlock
    // base64 is its own non-standard format and rejects the page's payload.
    juce::MemoryBlock bytes;
    bool decodedOk = false;
    {
        juce::MemoryOutputStream out (bytes, false);   // trims `bytes` to the written size on destruction
        decodedOk = juce::Base64::convertFromBase64 (out, base64);
    }
    if (! decodedOk)
    {
        setImportStatus (ImportStatus::State::error, WavetableImporter::sanitiseName (name), 0, "unreadable");
        return false;
    }

    return importFromMemory (name, std::move (bytes));   // sanitises the name and re-caps the bytes
}

//==============================================================================
// Stage 4 presets (FUNC-08; D-AA .. D-AH). The factory recipes live in ONE
// table, PresetRecipes.h (D-AD). Message thread unless noted. presetLock
// guards the identity (name / targets / validity); it is never taken on the
// audio thread and never held across setValueNotifyingHost.
namespace
{
    // output_level is user-owned: never in a preset, never "modified".
    constexpr bool isPresetOutputSlot (size_t i) noexcept
    {
        return i == OSimpleWavetable::ParamIDs::sOutputLevel;
    }

    // D-AE strip set for preset names: C0, DEL, C1, bidi controls.
    bool isStrippedPresetChar (juce::juce_wchar c) noexcept
    {
        return c < 0x20 || c == 0x7f || (c >= 0x80 && c <= 0x9f)
            || c == 0x200e || c == 0x200f
            || (c >= 0x202a && c <= 0x202e)
            || (c >= 0x2066 && c <= 0x2069);
    }

    bool isJsonNumberOrBool (const juce::var& v) noexcept
    {
        return v.isDouble() || v.isInt() || v.isInt64() || v.isBool();
    }

    constexpr int   kMaxPresetNameChars = 64;
    constexpr float kApplySkipTolerance = 1.0e-6f;   // D-AB (4)
    constexpr float kModifiedTolerance  = 1.0e-4f;   // D-AG
}

//------------------------------------------------------------------------------
OSimpleWavetableAudioProcessor::Targets
OSimpleWavetableAudioProcessor::recipeTargets (const wtpresets::Recipe& recipe) const
{
    namespace ids = OSimpleWavetable::ParamIDs;
    Targets t;
    t.fill (std::numeric_limits<float>::quiet_NaN());

    int matched = 0;
    for (int e = 0; e < recipe.count; ++e)
    {
        const auto& entry = recipe.entries[e];
        for (size_t i = 0; i < ids::all.size(); ++i)
        {
            if (isPresetOutputSlot (i) || std::strcmp (entry.paramId, ids::all[i]) != 0)
                continue;
            if (auto* p = parameters.getParameter (ids::all[i]))
            {
                t[i] = p->convertTo0to1 (entry.raw);
                ++matched;
            }
        }
    }
    jassert (matched == recipe.count);            // a typo in a recipe id (G-FACTORY)
    juce::ignoreUnused (matched);
    return t;
}

// NaN -> the parameter's default; finite -> clamped to 0..1; then quantised to
// the value the parameter will actually hold (choice / bool snap), so the
// cached targets equal what getValue() reads back. output_level -> NaN.
OSimpleWavetableAudioProcessor::Targets
OSimpleWavetableAudioProcessor::resolveTargets (const Targets& targets) const
{
    namespace ids = OSimpleWavetable::ParamIDs;
    Targets r;
    r.fill (std::numeric_limits<float>::quiet_NaN());
    for (size_t i = 0; i < ids::all.size(); ++i)
    {
        if (isPresetOutputSlot (i))
            continue;                              // user-owned: never reset, never set
        auto* p = parameters.getParameter (ids::all[i]);
        if (p == nullptr)
            continue;
        const float t = std::isfinite (targets[i]) ? juce::jlimit (0.0f, 1.0f, targets[i])
                                                   : p->getDefaultValue();   // reset-to-defaults first
        r[i] = p->convertTo0to1 (p->convertFrom0to1 (t));
    }
    return r;
}

// D-AB: the ONE apply core.
bool OSimpleWavetableAudioProcessor::applyNormalisedTargets (const Targets& targets)
{
    {
        // Stage 3 N5: a queued Imported auto-select must not override the
        // preset's bank. handleAsyncUpdate reads and clears this flag under the
        // same lock, so a queued callback becomes a no-op (the flag, not the
        // queue, is the authority). A later import publish re-arms it.
        const juce::ScopedLock sl (bankStateLock);
        pendingAutoSelect = false;
    }

    namespace ids = OSimpleWavetable::ParamIDs;
    const auto resolved = resolveTargets (targets);
    for (size_t i = 0; i < ids::all.size(); ++i)
    {
        if (isPresetOutputSlot (i))
            continue;
        auto* p = parameters.getParameter (ids::all[i]);
        if (p == nullptr || ! std::isfinite (resolved[i]))
            continue;
        if (std::abs (p->getValue() - resolved[i]) < kApplySkipTolerance)
            continue;                              // already there: no host edit, no gesture
        p->beginChangeGesture();
        p->setValueNotifyingHost (resolved[i]);
        p->endChangeGesture();
    }
    return true;
}

OSimpleWavetableAudioProcessor::Targets OSimpleWavetableAudioProcessor::currentNormalisedValues() const
{
    namespace ids = OSimpleWavetable::ParamIDs;
    Targets t;
    t.fill (std::numeric_limits<float>::quiet_NaN());
    for (size_t i = 0; i < ids::all.size(); ++i)
        if (! isPresetOutputSlot (i))
            if (auto* p = parameters.getParameter (ids::all[i]))
                t[i] = p->getValue();
    return t;
}

void OSimpleWavetableAudioProcessor::setCurrentPreset (const juce::String& newName, const Targets& resolved,
                                                       bool valid, bool stale)
{
    {
        const juce::ScopedLock sl (presetLock);
        curPresetName      = newName;
        presetTargets      = resolved;
        presetTargetsValid = valid;
        presetTargetsStale = stale;
    }
    presetRevision.fetch_add (1);
}

juce::String OSimpleWavetableAudioProcessor::getPresetName() const
{
    const juce::ScopedLock sl (presetLock);
    return curPresetName;
}

juce::String OSimpleWavetableAudioProcessor::getPresetId() const
{
    const auto* recipe = wtpresets::findByName (getPresetName());
    return recipe != nullptr ? juce::String (recipe->id) : juce::String();
}

bool OSimpleWavetableAudioProcessor::isPresetFactory() const
{
    return isFactoryPresetName (getPresetName());
}

bool OSimpleWavetableAudioProcessor::isPresetModified() const
{
    Targets t;
    {
        const juce::ScopedLock sl (presetLock);
        if (curPresetName.isEmpty() || ! presetTargetsValid)
            return false;
        t = presetTargets;
    }

    namespace ids = OSimpleWavetable::ParamIDs;
    for (size_t i = 0; i < ids::all.size(); ++i)
    {
        if (isPresetOutputSlot (i) || ! std::isfinite (t[i]))
            continue;                              // output_level never makes a preset "modified"
        if (auto* p = parameters.getParameter (ids::all[i]))
            if (std::abs (p->getValue() - t[i]) > kModifiedTolerance)
                return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// Factory presets by stable id: the table, never the disk (D-AD).
bool OSimpleWavetableAudioProcessor::applyFactoryPreset (const juce::String& id)
{
    const auto* recipe = wtpresets::findById (id);
    if (recipe == nullptr)
        return false;                              // unknown id: no change

    const auto targets = recipeTargets (*recipe);
    applyNormalisedTargets (targets);
    setCurrentPreset (recipe->name, resolveTargets (targets), true, false);
    return true;
}

//------------------------------------------------------------------------------
juce::String OSimpleWavetableAudioProcessor::sanitisePresetName (const juce::String& raw)
{
    juce::String clean;
    const auto trimmed = raw.trim();
    for (auto cp = trimmed.getCharPointer(); ! cp.isEmpty();)
    {
        const juce::juce_wchar c = cp.getAndAdvance();
        if (! isStrippedPresetChar (c))
            clean += c;
    }

    clean = juce::File::createLegalFileName (clean);   // removes "#@,;:<>*^|?\/

    // Leading dots (hidden files, "..") and any whitespace the strip exposed.
    for (;;)
    {
        auto next = clean.trimStart();
        while (next.startsWithChar ('.'))
            next = next.substring (1);
        if (next == clean)
            break;
        clean = next;
    }

    return clean.substring (0, kMaxPresetNameChars).trimEnd();   // "" = refused
}

std::vector<OuariconPresetManager::FactoryPresetDef> OSimpleWavetableAudioProcessor::buildFactoryPresetDefs() const
{
    namespace ids = OSimpleWavetable::ParamIDs;
    std::vector<OuariconPresetManager::FactoryPresetDef> defs;
    defs.reserve ((size_t) wtpresets::kNumFactory);

    for (const auto& recipe : wtpresets::kFactory)
    {
        OuariconPresetManager::FactoryPresetDef def;
        def.name = recipe.name;
        const auto t = resolveTargets (recipeTargets (recipe));
        for (size_t i = 0; i < ids::all.size(); ++i)
            if (! isPresetOutputSlot (i) && std::isfinite (t[i]))
                def.parameters[juce::String (ids::all[i])] = t[i];
        defs.push_back (std::move (def));
    }
    return defs;
}

void OSimpleWavetableAudioProcessor::ensureFactoryBankOnDisk()
{
    if (factoryBankEnsured)
        return;
    factoryBankEnsured = true;
    presetManager.initializeFactoryPresets (buildFactoryPresetDefs());   // sentinel-gated inside the module
}

//------------------------------------------------------------------------------
juce::StringArray OSimpleWavetableAudioProcessor::buildWalkOrder (const juce::StringArray& userStems)
{
    juce::StringArray order;
    for (const auto& recipe : wtpresets::kFactory)
        order.add (recipe.name);

    auto users = userStems;
    users.sort (true);                             // case-insensitive
    for (const auto& u : users)
    {
        if (u.isEmpty() || isFactoryPresetName (u) || order.contains (u, true))
            continue;
        order.add (u);
    }
    return order;
}

juce::StringArray OSimpleWavetableAudioProcessor::getPresetWalkOrder() const
{
    juce::StringArray stems;
    const auto dir = presetManager.getUserPresetsDirectory();   // a path only: nothing is created
    if (dir.isDirectory())
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
            stems.add (f.getFileNameWithoutExtension());
    return buildWalkOrder (stems);
}

juce::String OSimpleWavetableAudioProcessor::neighbourInOrder (const juce::StringArray& order,
                                                                const juce::String& current, int dir)
{
    const int n = order.size();
    if (n == 0)
        return {};

    int idx = current.isEmpty() ? -1 : order.indexOf (current);
    if (idx < 0 && current.isNotEmpty())
        idx = order.indexOf (current, true);
    if (idx < 0)
        return dir >= 0 ? order[0] : order[n - 1];   // unnamed / unknown: top going forward, bottom going back

    const int step = dir >= 0 ? 1 : -1;
    return order[(idx + step + n) % n];
}

juce::String OSimpleWavetableAudioProcessor::getNeighbourPreset (int dir) const
{
    return neighbourInOrder (getPresetWalkOrder(), getPresetName(), dir);
}

//------------------------------------------------------------------------------
bool OSimpleWavetableAudioProcessor::userJsonTargets (const juce::var& presetJson, Targets& out) const
{
    namespace ids = OSimpleWavetable::ParamIDs;
    out.fill (std::numeric_limits<float>::quiet_NaN());

    const auto* root = presetJson.getDynamicObject();
    if (! presetJson.isObject() || root == nullptr)
        return false;

    const juce::var paramsVar = root->getProperty ("parameters");
    const auto* paramsObj = paramsVar.getDynamicObject();
    if (! paramsVar.isObject() || paramsObj == nullptr)
        return false;

    for (size_t i = 0; i < ids::all.size(); ++i)
    {
        if (isPresetOutputSlot (i))
            continue;                              // stored by the module's save, never applied
        const juce::var v = paramsObj->getProperty (juce::Identifier (ids::all[i]));
        if (isJsonNumberOrBool (v))
            out[i] = (float) (double) v;           // anything else stays NaN -> default
    }
    return true;
}

bool OSimpleWavetableAudioProcessor::applyUserPresetJson (const juce::var& presetJson)
{
    Targets t;
    if (! userJsonTargets (presetJson, t))
        return false;                              // nothing changed
    return applyNormalisedTargets (t);
}

bool OSimpleWavetableAudioProcessor::loadPresetByName (const juce::String& presetName)
{
    if (const auto* recipe = wtpresets::findByName (presetName))
        return applyFactoryPreset (recipe->id);    // the table, never the disk copy

    if (presetName.isEmpty())
        return false;

    // Only a listed user preset loads: the name must BE a stem of the User
    // folder listing, which also blocks any path traversal.
    const auto order = getPresetWalkOrder();
    const int idx = order.indexOf (presetName);
    if (idx < wtpresets::kNumFactory)
        return false;

    const auto stem = order[idx];
    const auto file = presetManager.getUserPresetsDirectory().getChildFile (stem + ".json");
    if (! file.existsAsFile())
        return false;

    Targets t;
    if (! userJsonTargets (juce::JSON::parse (file.loadFileAsString()), t))
        return false;

    applyNormalisedTargets (t);
    setCurrentPreset (stem, resolveTargets (t), true, false);
    return true;
}

bool OSimpleWavetableAudioProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto clean = sanitisePresetName (rawName);
    if (clean.isEmpty() || isFactoryPresetName (clean))
        return false;                              // refused BEFORE the module

    ensureFactoryBankOnDisk();
    if (! presetManager.savePreset (clean))
        return false;

    // The module tracks the on-disk (sanitised) name.
    setCurrentPreset (presetManager.getCurrentPresetName(), resolveTargets (currentNormalisedValues()), true, false);
    return true;
}

bool OSimpleWavetableAudioProcessor::deleteUserPreset (const juce::String& presetName)
{
    if (presetName.isEmpty() || isFactoryPresetName (presetName))
        return false;

    const auto order = getPresetWalkOrder();
    const int idx = order.indexOf (presetName);
    if (idx < wtpresets::kNumFactory)
        return false;                              // not a listed user preset

    ensureFactoryBankOnDisk();
    if (! presetManager.deletePreset (order[idx]))
        return false;

    bool wasCurrent = false;
    {
        const juce::ScopedLock sl (presetLock);
        wasCurrent = curPresetName == order[idx];
    }
    if (wasCurrent)
        setCurrentPreset ({}, Targets {}, false, false);   // unnamed (never the module's "Default")
    else
        presetRevision.fetch_add (1);
    return true;
}

juce::var OSimpleWavetableAudioProcessor::getPresetCatalog() const
{
    juce::Array<juce::var> factory, user;
    for (const auto& recipe : wtpresets::kFactory)
    {
        auto* entry = new juce::DynamicObject();
        entry->setProperty ("name", juce::String (recipe.name));
        entry->setProperty ("id",   juce::String (recipe.id));
        factory.add (juce::var (entry));
    }

    const auto order = getPresetWalkOrder();
    for (int i = wtpresets::kNumFactory; i < order.size(); ++i)
        user.add (order[i]);

    auto* root = new juce::DynamicObject();
    root->setProperty ("factory", juce::var (std::move (factory)));
    root->setProperty ("user",    juce::var (std::move (user)));
    return juce::var (root);
}

void OSimpleWavetableAudioProcessor::refreshPresetTargetsIfNeeded()
{
    juce::String stale;
    {
        const juce::ScopedLock sl (presetLock);
        if (! presetTargetsStale)
            return;
        presetTargetsStale = false;                // ONE attempt per restore
        stale = curPresetName;
    }
    if (stale.isEmpty())
        return;

    // The name was sanitised on restore (no separators, no leading dots).
    const auto file = presetManager.getUserPresetsDirectory().getChildFile (stale + ".json");
    if (! file.existsAsFile())
        return;                                    // missing: targets stay unknown -> unmodified

    Targets t;
    if (! userJsonTargets (juce::JSON::parse (file.loadFileAsString()), t))
        return;
    const auto resolved = resolveTargets (t);

    {
        const juce::ScopedLock sl (presetLock);
        if (curPresetName != stale || presetTargetsValid)
            return;                                // superseded meanwhile
        presetTargets      = resolved;
        presetTargetsValid = true;
    }
    presetRevision.fetch_add (1);
}

//------------------------------------------------------------------------------
// N13 (D-AP): one host gesture per stepped-knob DRAG (a ComboBoxState change
// is a complete gesture per detent).
bool OSimpleWavetableAudioProcessor::stepKnobGesture (const juce::String& paramId, int phase, int index)
{
    namespace ids = OSimpleWavetable::ParamIDs;
    const int slot = paramId == ids::bitDepth ? 0 : paramId == ids::lfoDiv ? 1 : -1;
    if (slot < 0)
        return false;                              // allow-list

    auto* choice = dynamic_cast<juce::AudioParameterChoice*> (parameters.getParameter (paramId));
    if (choice == nullptr || choice->choices.size() < 1)
        return false;

    bool& isOpen = stepGestureOpen[(size_t) slot];
    if (phase == 0)
    {
        if (! isOpen)
        {
            choice->beginChangeGesture();
            isOpen = true;
        }
        return true;
    }
    if (phase == 1)
    {
        if (! isOpen)
            return false;                          // a move outside a drag changes nothing
        const int n = choice->choices.size();
        const int target = juce::jlimit (0, n - 1, index);
        if (choice->getIndex() != target)
            choice->setValueNotifyingHost (choice->convertTo0to1 ((float) target));
        return true;
    }
    if (phase == 2)
    {
        if (! isOpen)
            return false;
        choice->endChangeGesture();
        isOpen = false;
        return true;
    }
    return false;
}

void OSimpleWavetableAudioProcessor::closeStepKnobGestures()
{
    namespace ids = OSimpleWavetable::ParamIDs;
    const std::array<const char*, 2> slotIds { ids::bitDepth, ids::lfoDiv };
    for (size_t s = 0; s < slotIds.size(); ++s)
    {
        if (! stepGestureOpen[s])
            continue;
        if (auto* p = parameters.getParameter (slotIds[s]))
            p->endChangeGesture();
        stepGestureOpen[s] = false;
    }
}

//==============================================================================
#if OSIW_TEST_HOOKS
int OSimpleWavetableAudioProcessor::getHeldBankCountForTesting() const
{
    const juce::ScopedLock sl (bankStateLock);
    return (importedOwner != nullptr ? 1 : 0) + (int) retiredBanks.size();
}

int OSimpleWavetableAudioProcessor::getRetiredBankCountForTesting() const
{
    const juce::ScopedLock sl (bankStateLock);
    return (int) retiredBanks.size();
}

int OSimpleWavetableAudioProcessor::getGraveyardCountForTesting() const
{
    const juce::ScopedLock sl (bankStateLock);
    return (int) graveyard.size();
}

void OSimpleWavetableAudioProcessor::clearGraveyardForTesting()
{
    const juce::ScopedLock sl (bankStateLock);
    graveyard.clear();
}

void OSimpleWavetableAudioProcessor::sweepNowForTesting()
{
    const juce::ScopedLock sl (bankStateLock);
    sweepRetiredBanks();
}

void OSimpleWavetableAudioProcessor::publishImportedBankForTesting (std::shared_ptr<const WavetableBank> b)
{
    const juce::ScopedLock sl (bankStateLock);
    importGen.fetch_add (1);
    publishImportedBank (std::move (b));
}
#endif

//==============================================================================
// Factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleWavetableAudioProcessor();
}
