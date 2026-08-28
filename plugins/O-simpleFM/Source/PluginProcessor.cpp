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

    O-simpleFM - Audio Processor (implementation)

    Stage 1 (Foundation): silent synth shell. Builds the full 17-parameter APVTS
    and persists it. processBlock clears the buffer (no audio until Stage 2).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"

namespace
{
    // Shared skews (match ARCHITECTURE.md → Parameter Mapping).
    constexpr float kAdsrTimeSkew = 0.35f; // perceptual skew for 0.001–5 s envelope times
    constexpr float kMinAdsrTime  = 0.001f;
    constexpr float kMaxAdsrTime  = 5.0f;

    juce::NormalisableRange<float> adsrTimeRange()
    {
        return { kMinAdsrTime, kMaxAdsrTime, 0.0001f, kAdsrTimeSkew };
    }

    // 0–1 normalized "percent" range (stored 0–1; UI scales ×100 in Stage 3).
    juce::NormalisableRange<float> unitRange()
    {
        return { 0.0f, 1.0f, 0.0001f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleFMAudioProcessor::createParameterLayout()
{
    using namespace OSimpleFM::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Core FM controls --------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ratio, 1 }, "Ratio (C:M)",
        juce::NormalisableRange<float> { 0.5f, 16.0f, 0.01f }, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ratioSnap, 1 }, "Ratio Snap", false));

    // Raw radian modulation index I (0–kIndexMax), skew biases control toward the
    // musically dense low end (perceptual taper applied in DSP at Stage 2).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modIndex, 1 }, "Modulation Index",
        juce::NormalisableRange<float> { 0.0f, OSimpleFM::kIndexMax, 0.001f, 0.3f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { feedback, 1 }, "Feedback",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f, 0.5f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { modFixedMode, 1 }, "Modulator Fixed Mode", false));

    // Fixed modulator frequency (active only in fixed mode); log skew.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modFixedHz, 1 }, "Modulator Fixed Hz",
        juce::NormalisableRange<float> { 1.0f, 8000.0f, 0.01f, 0.25f }, 220.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modEnvToIndex, 1 }, "Mod Env -> Index",
        unitRange(), 1.0f)); // headline feature ON by default (depth 1.0)

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToIndex, 1 }, "Velocity -> Index",
        unitRange(), 0.0f));

    //--- Modulator envelope (ADSR -> index) --------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modAttack, 1 }, "Mod Attack", adsrTimeRange(), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modDecay, 1 }, "Mod Decay", adsrTimeRange(), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modSustain, 1 }, "Mod Sustain", unitRange(), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modRelease, 1 }, "Mod Release", adsrTimeRange(), 0.3f));

    //--- Amplitude envelope (ADSR -> carrier + voice lifetime) -------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.3f));

    //--- Output ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleFMAudioProcessor::OSimpleFMAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager (parameters, "O-simpleFM")
{
    // Pre-allocate all voices up front (no audio-thread allocation later).
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new FMVoice());

    synth.addSound (new FMSound());          // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);

    // Materialize the 6 factory presets to ~/Library/O-simpleFM/Presets/Factory/ on first
    // construction. The shared module writes idempotently (replaceWithText); unconditional
    // init matches the suite norm (O-AnalogEQ) and is safe under AU validation.
    presetManager.initializeFactoryPresets (FactoryPresets::build (parameters));
}

OSimpleFMAudioProcessor::~OSimpleFMAudioProcessor() = default;

//==============================================================================
void OSimpleFMAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize      = juce::jmax (1, samplesPerBlock);

    // On-screen-keyboard MIDI runs in HOST sample space (collected before
    // oversampling), so reset the collector at the host rate.
    midiCollector.reset (sampleRate);

    const int    osFactor = 1 << kOsFactorLog2;            // 2
    const double osRate   = sampleRate * osFactor;

    // 2x polyphase-IIR oversampling. Built for the FIXED max channel count
    // (mono/stereo) so a runtime layout change can never exceed the internal
    // buffer width. The synth renders at osRate; processSamplesDown applies the
    // half-band decimation filter that suppresses FM aliasing images.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) kMaxChannels, (size_t) kOsFactorLog2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true /*max quality*/, false /*no gain normalise*/);
    oversampler->initProcessing ((size_t) maxBlockSize);   // chunked if a host exceeds this
    oversampler->reset();
    setLatencySamples ((int) std::round (oversampler->getLatencyInSamples()));

    // Sized so processBlock's per-chunk copy can never reallocate on the audio
    // thread, even under a pathological sysex/CC flood (~16 KB of headroom).
    scaledMidi.ensureSize (16384);

    // Synthesiser + per-voice prepare AT THE OVERSAMPLED RATE. juce::SynthesiserVoice
    // has no virtual prepareToPlay in JUCE 8 — dispatch the custom one via dynamic_cast.
    synth.setCurrentPlaybackSampleRate (osRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* fv = dynamic_cast<FMVoice*> (synth.getVoice (v)))
            fv->prepareToPlay (osRate, samplesPerBlock * osFactor);

    outputGain.reset (sampleRate, 0.02);
    const float outDb = parameters.getRawParameterValue (OSimpleFM::ParamIDs::outputLevel)->load();
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));
}

void OSimpleFMAudioProcessor::releaseResources()
{
    if (oversampler != nullptr)
        oversampler->reset();
    synth.allNotesOff (0, false);
}

bool OSimpleFMAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: output-only. Accept mono or stereo output, no input bus.
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono()
        && out != juce::AudioChannelSet::stereo())
        return false;

    // No input bus on an instrument.
    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return true;
}

void OSimpleFMAudioProcessor::pushParamsToVoices()
{
    using namespace OSimpleFM::ParamIDs;
    auto get = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };

    const float ratioV     = get (ratio);
    const bool  snapV      = get (ratioSnap)    > 0.5f;
    const float indexV     = get (modIndex) / OSimpleFM::kIndexMax; // stored -> 0..1 norm for taper
    const float fbV        = get (feedback);
    const bool  fixedV     = get (modFixedMode) > 0.5f;
    const float fixedHzV   = get (modFixedHz);
    const float depthV     = get (modEnvToIndex);
    const float velIdxV    = get (velToIndex);

    const juce::ADSR::Parameters ampP { get (ampAttack), get (ampDecay), get (ampSustain), get (ampRelease) };
    const juce::ADSR::Parameters modP { get (modAttack), get (modDecay), get (modSustain), get (modRelease) };

    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* fv = dynamic_cast<FMVoice*> (synth.getVoice (v)))
            fv->setParams (ratioV, snapV, indexV, fbV, fixedV, fixedHzV, depthV, velIdxV, ampP, modP);
}

void OSimpleFMAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    // The note number arrives from the WebView — never trust the page at the
    // native boundary (out-of-range values jassert in debug, build malformed
    // MIDI in release).
    noteNumber = juce::jlimit (0, 127, noteNumber);
    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);   // MidiMessageCollector wants seconds
    midiCollector.addMessageToQueue (msg);
}

void OSimpleFMAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // synth voices ADD into a cleared buffer

    // Merge any on-screen-keyboard notes into the host MIDI stream (before the
    // per-chunk oversampling rebase below picks them up).
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    // Carrier tracking for the spectrum sideband markers (viz only — no audio
    // effect). Capture the most-recently-started note's pitch from the merged
    // stream (covers both host MIDI and the on-screen keyboard). "Sounding" is
    // decided post-render below from the live voices.
    for (const auto meta : midiMessages)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
            lastNoteHz.store (juce::MidiMessage::getMidiNoteInHertz (m.getNoteNumber()),
                              std::memory_order_relaxed);
    }

    pushParamsToVoices();

    const int osFactor = 1 << kOsFactorLog2;

    // Render the synth at the oversampled rate, then decimate. Chunked into
    // <= maxBlockSize slices so a host sending a larger-than-prepared block can
    // never overrun the oversampler's internal buffer. processSamplesUp returns
    // the internal 2N block (upsampled silence); we render the voices INTO it,
    // then processSamplesDown applies the half-band decimation (the AA step for
    // the generated FM sidebands). Filter state carries across chunks correctly.
    if (oversampler != nullptr)
    {
        auto* const* writePtrs = buffer.getArrayOfWritePointers();
        int pos = 0;
        while (pos < numSamples)
        {
            const int n = juce::jmin (maxBlockSize, numSamples - pos);

            // Per-chunk MIDI: events in [pos, pos+n), rebased to 0, scaled x2.
            scaledMidi.clear();
            for (const auto meta : midiMessages)
            {
                const int sp = meta.samplePosition;
                if (sp >= pos && sp < pos + n)
                    scaledMidi.addEvent (meta.getMessage(), (sp - pos) * osFactor);
            }

            juce::AudioBuffer<float> sub (writePtrs, numCh, pos, n);
            juce::dsp::AudioBlock<float> subBlock (sub);

            auto osBlock = oversampler->processSamplesUp (subBlock);
            const int osNumCh   = juce::jmin ((int) osBlock.getNumChannels(), kMaxChannels);
            const int osNumSamp = (int) osBlock.getNumSamples();
            float* chans[kMaxChannels] = { nullptr, nullptr };
            for (int c = 0; c < osNumCh; ++c)
                chans[c] = osBlock.getChannelPointer ((size_t) c);

            juce::AudioBuffer<float> osBuf (chans, osNumCh, osNumSamp);
            osBuf.clear();
            synth.renderNextBlock (osBuf, scaledMidi, 0, osNumSamp);

            oversampler->processSamplesDown (subBlock);
            pos += n;
        }
    }
    else
    {
        synth.renderNextBlock (buffer, midiMessages, 0, numSamples);   // fallback (unprepared)
    }

    // A note is "sounding" while any voice is still active (covers release tails);
    // the editor skips the sideband markers when nothing rings. Viz only.
    bool anyVoiceActive = false;
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (synth.getVoice (v)->isVoiceActive()) { anyVoiceActive = true; break; }
    carrierSounding.store (anyVoiceActive, std::memory_order_relaxed);

    // Master output trim (dB->lin, smoothed).
    const float outDb = parameters.getRawParameterValue (OSimpleFM::ParamIDs::outputLevel)->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));
    const float g0 = outputGain.getCurrentValue();
    const float g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, numSamples, g0, g1);

    // Suite-wide NaN insurance.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }

    // Visualization tap: post-gain mono sum -> lock-free ring (copy-only, no alloc).
    if (numCh == 1)
    {
        vizRing.write (buffer.getReadPointer (0), numSamples);
    }
    else if (numCh > 1)
    {
        constexpr int kChunk = 4096;
        float mono[kChunk];
        int done = 0;
        while (done < numSamples)
        {
            const int n = juce::jmin (kChunk, numSamples - done);
            for (int i = 0; i < n; ++i)
            {
                float s = 0.0f;
                for (int ch = 0; ch < numCh; ++ch)
                    s += buffer.getReadPointer (ch)[done + i];
                mono[i] = s / (float) numCh;
            }
            vizRing.write (mono, n);
            done += n;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleFMAudioProcessor::createEditor()
{
    return new OSimpleFMAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleFMAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // v1.3.0: the interface LANGUAGE rides the same tree as a plain property on
    // the ROOT — not a parameter, and not part of any preset. Written as a
    // STRING ("en"/"fr") rather than the atomic's int index, so a hand-inspected
    // session file says what it means, and because the ValueTree -> XML round
    // trip rebuilds every property as a string var anyway
    // (critical_valuetree_xml_roundtrip_loses_type). Storing the code means the
    // value that comes back is the value that went in, with no type predicate to
    // misfire on the way.
    //
    // It is set BEFORE getStateAsXml(), which begins with parameters.copyState():
    // a property written after the copy would never reach the XML.
    parameters.state.setProperty (kUiLanguageProp,
                                  languageCode (uiLanguage.load (std::memory_order_acquire)),
                                  nullptr);

    // Route through the preset manager so the current-preset name is persisted
    // alongside the APVTS tree (getStateAsXml falls back to the plain APVTS XML).
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleFMAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        presetManager.setStateFromXml (xml.get());

        // v1.3.0. Read AFTER setStateFromXml, not before: it ends in
        // AudioProcessorValueTreeState::replaceState(), which assigns the
        // incoming tree wholesale (state = newState), so parameters.state is the
        // OLD tree until it returns and the INCOMING one afterwards.
        //
        // A pre-1.3.0 session has no property, getProperty returns a VOID var,
        // and the default (English) stands. isVoid() is the only correct gate —
        // the value comes back as a STRING var, so a type predicate like isInt()
        // would never fire. languageIndex() clamps anything that is not "fr" to
        // 0, so a hand-edited value degrades to English rather than to a bad
        // index.
        const juce::var lang = parameters.state.getProperty (kUiLanguageProp);
        if (! lang.isVoid())
            uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleFMAudioProcessor();
}
