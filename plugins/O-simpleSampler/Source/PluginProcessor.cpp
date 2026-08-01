/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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

    O-simpleSampler - Audio Processor (implementation)

    Stage 2.1 (Core Playable Sampler): a 16-voice juce::Synthesiser of custom
    SampleVoice Repitch read heads playing the embedded piano.wav. Builds the full
    21-parameter APVTS, decodes/resamples/atomic-publishes the source OFF the audio
    thread, and pushes the per-block param bundle (root/tune/fine, region, velToAmp,
    amp ADSR) to every voice before rendering. Allocation-free in processBlock.

  ==============================================================================
*/

#include "PluginProcessor.h"
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"   // pulls in WebView types — only valid under JUCE_WEB_BROWSER=1
#endif
#include "SampleSound.h"
#include "SampleVoice.h"
#include "BinaryData.h"      // embedded piano.wav (Source/samples/piano.wav)

namespace
{
    // Shared ADSR-time skew (match parameter-spec.md → research-locked ARCHITECTURE.md).
    constexpr float kAdsrTimeSkew = 0.35f; // perceptual taper for 0–5 s envelope times

    juce::NormalisableRange<float> adsrTimeRange()
    {
        return { 0.0f, 5.0f, 0.0001f, kAdsrTimeSkew };
    }

    // 0–100 "percent" range stored as raw percent (UI uses it directly).
    juce::NormalisableRange<float> percentRange()
    {
        return { 0.0f, 100.0f, 0.01f };
    }

    // 0–1 normalized range for the ADSR sustain (feeds juce::ADSR directly; UI ×100).
    juce::NormalisableRange<float> unitRange()
    {
        return { 0.0f, 1.0f, 0.0001f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleSamplerAudioProcessor::createParameterLayout()
{
    using namespace OSimpleSampler::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Source ------------------------------------------------------------
    // Which built-in recording plays. The "(loaded)" user-file state is reflected
    // in custom (non-APVTS) state, NOT as a 5th choice. Default piano (index 0).
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { sourceSample, 1 }, "Source",
        juce::StringArray { "piano", "cello", "pizz", "hit" }, 0));

    //--- Region: Start / End ----------------------------------------------
    // Played region of the source. Start/End together are the "isolate the useful
    // part" lesson. Stored as raw percent; zero-crossing snap lands Stage 2.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { regionStart, 1 }, "Start", percentRange(), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { regionEnd, 1 }, "End", percentRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Region: Loop ------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { loopMode, 1 }, "Loop Mode",
        juce::StringArray { "Off", "Forward", "Ping-Pong" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { loopStart, 1 }, "Loop Start", percentRange(), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { loopEnd, 1 }, "Loop End", percentRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    // Equal-power crossfade across the loop seam. Skew ~0.4 biases control toward
    // the short low end where the per-repeat click lives.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { loopCrossfade, 1 }, "Loop Crossfade",
        juce::NormalisableRange<float> { 0.0f, 500.0f, 0.01f, 0.4f }, 10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    //--- Region: Reverse ---------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { reverse, 1 }, "Reverse", false));

    //--- Pitch: Root / Mode / Tune / Fine ----------------------------------
    // Key at which the sample plays at original pitch. Seeded per built-in
    // (Stage 2); user-overridable. Default C3 = 60.
    params.push_back (std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { rootKey, 1 }, "Root Key", 0, 127, kRootNote));
    // HEADLINE A/B: Repitch = varispeed (pitch+time coupled); Stretch =
    // synchronous-granular pitch shift (pitch/time independent). Default Repitch.
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { pitchMode, 1 }, "Pitch Mode",
        juce::StringArray { "Repitch", "Stretch" }, 0));
    // Coarse transpose, kept separate from Fine (do not consolidate — FUNC-08).
    params.push_back (std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { tune, 1 }, "Tune", -24, 24, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { fine, 1 }, "Fine",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("cents")));

    //--- Vintage -----------------------------------------------------------
    // S&H decimation + bit-crush macro (SP-1200 grit). Full bypass at 0 (Stage 2).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { vintage, 1 }, "Vintage", percentRange(), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Filter (resonant low-pass) ----------------------------------------
    // 20–20000 Hz, logarithmic feel via setSkewForCentre. Open at default.
    {
        juce::NormalisableRange<float> cutoffRange { 20.0f, 20000.0f, 1.0f };
        cutoffRange.setSkewForCentre (1000.0f); // musical centre ~1 kHz (log skew ≈0.25)
        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { filterCutoff, 1 }, "Filter Cutoff", cutoffRange, 20000.0f,
            juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    }
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterResonance, 1 }, "Filter Resonance", percentRange(), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Amplitude envelope (per-voice ADSR) -------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.005f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    // Sustain stored 0–1 (UI scales ×100). Default 1.0 (100 %).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 1.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.2f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Voice / Output ----------------------------------------------------
    // How much note velocity scales loudness. Default 50 %.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToAmp, 1 }, "Vel -> Amp", percentRange(), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    // −inf–0 dB master trim. −60 dB floor maps to "−inf" perceptually.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleSamplerAudioProcessor::OSimpleSamplerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    using namespace OSimpleSampler::ParamIDs;

    // Cache raw-param atomic pointers. Established now (read once per block by the
    // sampler engine in Stage 2; unused while silent).
    sourceSampleParam    = apvts.getRawParameterValue (sourceSample);
    startParam           = apvts.getRawParameterValue (regionStart);
    endParam             = apvts.getRawParameterValue (regionEnd);
    loopModeParam        = apvts.getRawParameterValue (loopMode);
    loopStartParam       = apvts.getRawParameterValue (loopStart);
    loopEndParam         = apvts.getRawParameterValue (loopEnd);
    loopCrossfadeParam   = apvts.getRawParameterValue (loopCrossfade);
    reverseParam         = apvts.getRawParameterValue (reverse);
    rootKeyParam         = apvts.getRawParameterValue (rootKey);
    pitchModeParam       = apvts.getRawParameterValue (pitchMode);
    tuneParam            = apvts.getRawParameterValue (tune);
    fineParam            = apvts.getRawParameterValue (fine);
    vintageParam         = apvts.getRawParameterValue (vintage);
    filterCutoffParam    = apvts.getRawParameterValue (filterCutoff);
    filterResonanceParam = apvts.getRawParameterValue (filterResonance);
    ampAttackParam       = apvts.getRawParameterValue (ampAttack);
    ampDecayParam        = apvts.getRawParameterValue (ampDecay);
    ampSustainParam      = apvts.getRawParameterValue (ampSustain);
    ampReleaseParam      = apvts.getRawParameterValue (ampRelease);
    velToAmpParam        = apvts.getRawParameterValue (velToAmp);
    outputLevelParam     = apvts.getRawParameterValue (outputLevel);

    // Build the sampler synth: 16 custom Repitch voices + one shared sound.
    // Preallocated up front (no audio-thread allocation later). The synth takes
    // ownership of each voice/sound pointer.
    for (int i = 0; i < kMaxVoices; ++i)
        synth.addVoice (new SampleVoice());

    synth.addSound (new SampleSound());          // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);         // steal quietest/oldest voice on overflow

    // Listen for sourceSample selection changes. The decode/resample is dispatched
    // to the message thread via AsyncUpdater (never the audio thread) — see
    // parameterChanged / handleAsyncUpdate.
    apvts.addParameterListener (sourceSample, this);

    // Phase 2.2a: the region/loop markers drive the off-thread zero-cross snap. A
    // change flags pendingSnap + triggers the AsyncUpdater (the scan runs on the
    // message thread — never the audio thread).
    apvts.addParameterListener (regionStart, this);
    apvts.addParameterListener (regionEnd,   this);
    apvts.addParameterListener (loopStart,   this);
    apvts.addParameterListener (loopEnd,     this);
}

OSimpleSamplerAudioProcessor::~OSimpleSamplerAudioProcessor()
{
    using namespace OSimpleSampler::ParamIDs;
    apvts.removeParameterListener (sourceSample, this);
    apvts.removeParameterListener (regionStart,  this);
    apvts.removeParameterListener (regionEnd,    this);
    apvts.removeParameterListener (loopStart,    this);
    apvts.removeParameterListener (loopEnd,      this);
    cancelPendingUpdate();
}

//==============================================================================
void OSimpleSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // The sampler design adds no inherent latency (no oversampling / lookahead).
    // NB: getLatencySamples() is non-virtual in JUCE 8 — never override it.
    setLatencySamples (0);

    // On-screen-keyboard MIDI queue (Stage 3.1). Reset to the engine rate so the
    // collector timestamps line up with processBlock's sample clock.
    midiCollector.reset (sampleRate);

    // Synthesiser + per-voice prepare. juce::SynthesiserVoice has no virtual
    // prepareToPlay in JUCE 8 — dispatch the custom one via dynamic_cast.
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SampleVoice*> (synth.getVoice (v)))
        {
            sv->prepareToPlay (sampleRate, samplesPerBlock);
            // Wire the shared Hann LUT once (the table never changes; building it
            // off the audio thread, all voices reference the same const pointer —
            // RT-CRITICAL, never per-voice / never in render). Phase 2.2b SOLA.
            sv->setWindowLuts (&windowLuts);
        }

    // Output trim (dB->lin, 20 ms smoothing).
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));

    // Filter cutoff/Q smoothers (Phase 2.2a). 20 ms ramp, seeded at the current param
    // so a fresh instance does not sweep on the first block. Q floor 0.707 (Butterworth
    // — setResonance asserts >0).
    filterCutoffSm.reset (sampleRate, 0.02);
    filterCutoffSm.setCurrentAndTargetValue (
        juce::jlimit (20.0f, 0.45f * (float) sampleRate, filterCutoffParam->load()));
    filterQSm.reset (sampleRate, 0.02);
    filterQSm.setCurrentAndTargetValue (
        juce::jmap (juce::jlimit (0.0f, 100.0f, filterResonanceParam->load()),
                    0.0f, 100.0f, 0.707f, 12.0f));

    // Decode + resample the active source to the engine rate, OFF the audio thread,
    // and publish it via the atomic shared_ptr swap (resample on every prepareToPlay
    // since the engine rate may change). The active source is either the restored
    // built-in (identity "embedded:<name>") or — for a user-file path, a Stage 2.3
    // feature — falls back to the embedded piano for Phase 2.1.
    if (currentSourceIdentity.startsWith ("embedded:"))
    {
        loadBuiltInSource (builtInIndexForIdentity (currentSourceIdentity), sampleRate);
    }
    else if (juce::File::isAbsolutePath (currentSourceIdentity)
             && juce::File (currentSourceIdentity).existsAsFile())
    {
        // Picker-loaded user file (identity = absolute path) — re-decode so the
        // session restores the same source. A "dropped:<name>" identity has no
        // path (macOS strips it), so it is NOT absolute and falls through to the
        // piano fallback below (the dropped bytes are not persisted).
        juce::MemoryBlock mb;
        const juce::File f (currentSourceIdentity);
        if (! (f.loadFileAsData (mb)
               && decodeAndPublish (mb.getData(), mb.getSize(), sampleRate, currentSourceIdentity)))
        {
            currentSourceIdentity = "embedded:piano";
            loadBuiltInSource (0, sampleRate);
        }
    }
    else
    {
        currentSourceIdentity = "embedded:piano";
        loadBuiltInSource (0, sampleRate);
    }

    // Prepare-time guarded root seed (RESEARCH §6). A FRESH instance (state NOT
    // restored) seeds the per-source root (piano = 48) ONCE so the keyboard plays
    // in standard tune; a restored session keeps its saved rootKey (stateWasRestored
    // gates this off). rootSeeded ensures it runs only on the first prepare.
    //
    // RT-safety item 3 (RESEARCH §3): do NOT call seedRootForSource (→
    // setValueNotifyingHost) directly here — prepareToPlay can run off the message
    // thread / during plugin scans / before the editor exists, where notifying the
    // host of a parameter change is discouraged (re-entrancy / dropped / dirties the
    // project). Defer it to the existing AsyncUpdater (guaranteed message thread).
    if (! stateWasRestored && ! rootSeeded)
    {
        pendingRootSeed.store (true, std::memory_order_relaxed);
        rootSeeded = true;
        triggerAsyncUpdate();
    }

    // Snap the region/loop markers to source zero crossings now the source is
    // published (Phase 2.2a). prepareToPlay is NOT the audio thread, so this direct
    // call is RT-safe; live param changes re-snap via the AsyncUpdater.
    computeZeroCrossSnaps();
}

void OSimpleSamplerAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
}

bool OSimpleSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

//==============================================================================
// Source decode/resample/publish (Phase 2.1). ALL of this runs OFF the audio
// thread (prepareToPlay, the AsyncUpdater for sourceSample changes, and
// setStateInformation). The audio thread only ever sees a fully-built buffer
// published via the atomic shared_ptr swap.

// Map embedded BinaryData symbols by built-in index. Stage 4 — the curated set is
// delivered: 0=piano, 1=cello, 2=pizz, 3=hit. JUCE mangles each filename to its
// BinaryData symbol (piano.wav -> piano_wav, cello.aif -> cello_aif, etc.). The
// `default` keeps piano as the never-silent fallback for any out-of-range index.
namespace
{
    struct BuiltInBlob { const char* data; int size; };

    BuiltInBlob builtInBlob (int idx) noexcept
    {
        switch (idx)
        {
            case 0:  return { BinaryData::piano_wav, BinaryData::piano_wavSize };
            case 1:  return { BinaryData::cello_aif, BinaryData::cello_aifSize };
            case 2:  return { BinaryData::pizz_aif,  BinaryData::pizz_aifSize };
            case 3:  return { BinaryData::hit_wav,   BinaryData::hit_wavSize };
            default: return { BinaryData::piano_wav, BinaryData::piano_wavSize };
        }
    }
}

int OSimpleSamplerAudioProcessor::builtInIndexForIdentity (const juce::String& identity) const
{
    if (identity.startsWith ("embedded:"))
    {
        const auto name = identity.fromFirstOccurrenceOf ("embedded:", false, false);
        for (int i = 0; i < kNumBuiltIns; ++i)
            if (name == kBuiltInNames[i])
                return i;
    }
    // Fall back to the live choice param, then piano.
    if (sourceSampleParam != nullptr)
        return juce::jlimit (0, kNumBuiltIns - 1, (int) sourceSampleParam->load());
    return 0;
}

// Resample a decoded buffer (srcRate) to engineRate, capped at the source-length
// cap. Returns a fully-built shared_ptr ready to publish. `truncated` is set when
// the source exceeded the cap.
std::shared_ptr<juce::AudioBuffer<float>>
OSimpleSamplerAudioProcessor::resampleToEngineRate (const juce::AudioBuffer<float>& src,
                                                    double srcRate, double engineRate,
                                                    bool& truncated) const
{
    const int    nCh    = juce::jmax (1, src.getNumChannels());
    const int    nSmp   = src.getNumSamples();
    const double ratio  = (srcRate > 0.0 ? srcRate : engineRate) / engineRate; // speedRatio
    const int    maxOut = (int) (kMaxSourceSeconds * engineRate);

    int numOut = (int) std::floor ((double) nSmp / ratio);
    truncated  = (numOut > maxOut);
    numOut     = juce::jlimit (1, maxOut, numOut);

    auto out = std::make_shared<juce::AudioBuffer<float>> (nCh, numOut);
    out->clear();
    for (int ch = 0; ch < nCh; ++ch)
    {
        juce::LagrangeInterpolator interp;       // streaming one-shot — correct for a whole-buffer resample
        interp.reset();
        interp.process (ratio, src.getReadPointer (ch), out->getWritePointer (ch), numOut);
    }
    return out;
}

// Decode a raw byte block (a complete .wav/.aiff/.flac file in memory) through the
// format manager, resample to the engine rate, cap, and atomic-publish. An invalid
// reader keeps the previous source (returns false).
bool OSimpleSamplerAudioProcessor::decodeAndPublish (const void* data, size_t numBytes,
                                                     double engineRate, const juce::String& identity)
{
    if (data == nullptr || numBytes == 0)
        return false;

    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();                  // WAV / AIFF / FLAC

    std::unique_ptr<juce::AudioFormatReader> reader (
        fmt.createReaderFor (std::make_unique<juce::MemoryInputStream> (data, numBytes, false)));
    if (reader == nullptr)
        return false;                            // unsupported/invalid — keep the previous source

    const int    nCh     = juce::jmax (1, (int) reader->numChannels);
    const int    nSmp    = (int) reader->lengthInSamples;
    const double srcRate = reader->sampleRate > 0.0 ? reader->sampleRate : engineRate;
    if (nSmp <= 0)
        return false;

    // Decode at the source rate (1–2 channels; the engine reads channel 0 as mono
    // and duplicates across the output — see processBlock's snapshot).
    juce::AudioBuffer<float> tmp (nCh, nSmp);
    reader->read (&tmp, 0, nSmp, 0, true, true);

    bool truncated = false;
    auto resampled = resampleToEngineRate (tmp, srcRate, engineRate, truncated);
    lastLoadTruncated.store (truncated, std::memory_order_relaxed);   // Stage-3 UI notice (>30 s)

    // RT-safe publish (RESEARCH §3). Keep the previous buffer alive ONE generation
    // (retiredSource) so any in-flight audio block finishes reading it, install the
    // new buffer, then hand the audio thread a raw pointer (release). The lock guards
    // the shared_ptr swap against the message-thread readers (getSourceThumbnail /
    // computeZeroCrossSnaps) and the prepareToPlay writer; the audio thread NEVER
    // locks (it reads sourceForAudio). The old buffer is freed on THIS off-audio
    // thread at the NEXT publish (retiredSource overwrite) — never in processBlock.
    {
        const juce::ScopedLock sl (sourceMutex);
        retiredSource = std::move (currentSource);
        currentSource = std::move (resampled);
        sourceForAudio.store (currentSource.get(), std::memory_order_release);
    }
    currentSourceIdentity = identity;
    return true;
}

// Decode one embedded built-in by index and publish. OFF the audio thread.
bool OSimpleSamplerAudioProcessor::loadBuiltInSource (int builtInIndex, double engineRate)
{
    builtInIndex = juce::jlimit (0, kNumBuiltIns - 1, builtInIndex);
    const auto blob = builtInBlob (builtInIndex);
    const juce::String identity = juce::String ("embedded:") + kBuiltInNames[builtInIndex];
    return decodeAndPublish (blob.data, (size_t) blob.size, engineRate, identity);
}

// Min/max envelope of the currently-published source (Stage-3 waveform-editor
// background). Ported VERBATIM from O-simpleGrain (getSourceThumbnail). Message-
// thread read: snapshots the atomic shared_ptr (held for the call), then reduces
// channel 0 to `numPairs` min/max pairs in [-1,1]. The audio thread is untouched.
// Returns a flat [min,max,…] vector (empty if no source).
std::vector<float> OSimpleSamplerAudioProcessor::getSourceThumbnail (int numPairs) const
{
    numPairs = juce::jlimit (1, 4096, numPairs);

    // Message-thread read: copy the owning shared_ptr under the lock (held for this
    // call) so a concurrent prepareToPlay/AsyncUpdater publish can't free it mid-scan.
    // Off the audio thread, so locking is fine.
    std::shared_ptr<juce::AudioBuffer<float>> src;
    {
        const juce::ScopedLock sl (sourceMutex);
        src = currentSource;
    }
    if (src == nullptr || src->getNumSamples() <= 0)
        return {};

    const float* data = src->getReadPointer (0);    // mono read (channel 0), matches the engine
    const int    len  = src->getNumSamples();

    std::vector<float> out;
    out.resize ((size_t) numPairs * 2, 0.0f);

    for (int p = 0; p < numPairs; ++p)
    {
        const int a = (int) ((juce::int64) p       * len / numPairs);
        int       b = (int) ((juce::int64) (p + 1) * len / numPairs);
        if (b <= a) b = juce::jmin (a + 1, len);

        float mn = 0.0f, mx = 0.0f;
        for (int i = a; i < b; ++i)
        {
            const float s = data[i];
            if (s < mn) mn = s;
            if (s > mx) mx = s;
        }
        out[(size_t) (p * 2)]     = juce::jlimit (-1.0f, 1.0f, mn);
        out[(size_t) (p * 2 + 1)] = juce::jlimit (-1.0f, 1.0f, mx);
    }
    return out;
}

//==============================================================================
// Concept-preset tour (Stage 3.3 / FUNC-07). Seven factory snapshots, each
// isolating ONE sampler concept. Every preset resets to defaults first (so it
// reproduces regardless of prior state), then — once Stage 4 authors the values —
// applies its named snapshot via setValueNotifyingHost: real (scaled) floats
// through convertTo0to1, choices / bool through their index. The WebView relays/
// attachments propagate every change back to the page (no DOM poking). Mirrors
// O-simpleGrain::applyFactoryPreset.
//
// STAGE 4: all 7 branch bodies authored — each resets to defaults, re-seeds the
// active source's recorded root (so nothing plays octave-flat), then sets only the
// params that isolate its concept. The WebView relays/attachments resync every
// knob/combo/toggle back to the page (JS→native-fn→APVTS→relay round-trip).
//
// Stored-range reminders (parameter-spec.md): start/end/loopStart/loopEnd/vintage/
// filterResonance/velToAmp are 0–100; loopCrossfade 0–500 ms; rootKey/tune are INT
// params; fine ±100 c; filterCutoff 20–20000 Hz (log); ADSR times in seconds;
// ampSustain stored 0–1 (NOT %). Choice indices — sourceSample: 0=piano 1=cello
// 2=pizz 3=hit; loopMode: 0=Off 1=Forward 2=Ping-Pong; pitchMode: 0=Repitch
// 1=Stretch. setReal/setChoice take the STRING id (apvts.getParameter("start")),
// so the regionStart/regionEnd identifier rename does not affect these helpers.
void OSimpleSamplerAudioProcessor::applyFactoryPreset (const juce::String& name)
{
    // 1. Reset every parameter to its default — a clean slate per concept.
    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    // 2. Snapshot helpers (scaled → normalised; choices/bool by index). Wired now;
    //    each branch below uses them once Stage 4 authors the per-preset values.
    auto setReal = [this] (const char* id, float real) {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };
    auto setChoice = [this] (const char* id, int index) {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    };
    auto setBool = [this] (const char* id, bool on) {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
    };
    // 3. CRITICAL central fix — re-apply the active source's recorded root AFTER the
    //    default-reset (which forced rootKey → 60). Without this every preset would
    //    play the source an octave-or-more flat (piano's recorded root is 48). Doing it
    //    here (not in each branch) keeps the presets SOURCE-AGNOSTIC: no branch touches
    //    rootKey, so each preset reads correctly on whatever source is loaded.
    using namespace OSimpleSampler::ParamIDs;
    seedRootForSource (builtInIndexForIdentity (currentSourceIdentity));

    // 4. Per-preset values. Each branch sets only the params that isolate its ONE
    //    concept (the rest stay at the defaults reset in step 1). Storage ranges
    //    (parameter-spec.md): start/end/loop*/vintage/filterResonance/velToAmp 0–100;
    //    loopCrossfade ms (0–500); filterCutoff Hz (log); ADSR seconds (0–5);
    //    ampSustain 0–1; rootKey/tune INT; Choice indices per parameter-spec. Values
    //    track the PRESET_LESSONS prose (app.js) so the lesson text matches the sound.

    // ── Raw One-Shot — the whole sample, once, no loop. The simplest sampler case.
    if (name == "Raw One-Shot")
    {
        setChoice (loopMode, 0);                 // no loop
        setReal   (regionStart, 0.0f);
        setReal   (regionEnd, 100.0f);           // whole sample
        setChoice (pitchMode, 0);                // Repitch
        setReal   (ampAttack, 0.002f);
        setReal   (ampDecay, 0.0f);
        setReal   (ampSustain, 1.0f);
        setReal   (ampRelease, 0.08f);
    }
    // ── Tuned Across the Keyboard — one recording becomes a playable instrument.
    else if (name == "Tuned Across the Keyboard")
    {
        setChoice (pitchMode, 0);                // Repitch — pitch follows the key
        setChoice (loopMode, 0);
        setReal   (regionStart, 0.0f);
        setReal   (regionEnd, 100.0f);
        // Root Key demo: handled by the central re-seed above (source-agnostic).
    }
    // ── Looped Pad — a crossfaded loop turns a short sound into an endless one.
    else if (name == "Looped Pad")
    {
        setChoice (loopMode, 1);                 // Forward loop
        setReal   (loopStart, 20.0f);
        setReal   (loopEnd, 80.0f);
        setReal   (loopCrossfade, 120.0f);       // ms — crossfaded seam
        setReal   (ampAttack, 1.5f);
        setReal   (ampSustain, 1.0f);
        setReal   (ampRelease, 2.0f);
    }
    // ── Reversed Swell — Reverse + slow attack = a backwards swell into the beat.
    else if (name == "Reversed Swell")
    {
        setBool   (reverse, true);
        setChoice (loopMode, 0);
        setReal   (ampAttack, 1.5f);             // slow swell in
        setReal   (ampDecay, 0.0f);
        setReal   (ampSustain, 1.0f);
        setReal   (ampRelease, 1.5f);
    }
    // ── Repitch vs Stretch A/B — the headline pitch-mode comparison.
    else if (name == "Repitch vs Stretch A/B")
    {
        setChoice (pitchMode, 1);                // Stretch — toggle vs Repitch to hear it
        setChoice (loopMode, 1);                 // Forward loop = indefinite hold for the A/B
        setReal   (loopCrossfade, 80.0f);
        setReal   (ampAttack, 0.01f);
        setReal   (ampSustain, 1.0f);
        setReal   (ampRelease, 0.3f);
    }
    // ── SP-1200 Crunch — Vintage adds the gritty low-rate / low-bit character.
    else if (name == "SP-1200 Crunch")
    {
        setReal   (vintage, 80.0f);              // decimation + bit-crush grit
        setReal   (filterCutoff, 12000.0f);      // Hz — slight top roll-off
        setReal   (ampAttack, 0.002f);
        setReal   (ampDecay, 0.15f);
        setReal   (ampSustain, 0.8f);
        setReal   (ampRelease, 0.15f);
    }
    // ── Filtered & Enveloped — the filter + amp envelope shape a raw sample.
    else if (name == "Filtered & Enveloped")
    {
        setReal   (filterCutoff, 1000.0f);       // Hz — closed-down
        setReal   (filterResonance, 40.0f);      // resonant peak
        setReal   (ampAttack, 0.3f);
        setReal   (ampDecay, 0.6f);
        setReal   (ampSustain, 0.5f);
        setReal   (ampRelease, 0.6f);
    }
    // Unknown name → leave at defaults (already reset above), root re-seeded.
}

// Seed the LIVE rootKey param to the per-source recorded-pitch root (RESEARCH §6).
// The APVTS rootKey DEFAULT stays 60 (frozen); this overwrites the live value via
// the parameter object so the host records it + the UI (Stage 3) syncs. OFF the
// audio thread.
void OSimpleSamplerAudioProcessor::seedRootForSource (int builtInIndex)
{
    builtInIndex = juce::jlimit (0, kNumBuiltIns - 1, builtInIndex);
    if (auto* p = apvts.getParameter (OSimpleSampler::ParamIDs::rootKey))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) kBuiltInRoot[builtInIndex]));
}

//==============================================================================
// Drag-drop streaming bridge (Stage 3.1 / FUNC-03). The C++ side of the WebView
// content-streaming drop: Start -> Chunk -> Commit. The editor registers JS that
// CALLS these via withNativeFunction. Single sampler source -> the single-file
// path is the live path. All on the message thread; decode via
// juce::Base64::convertFromBase64 (standard btoa() — NOT the JUCE proprietary
// MemoryBlock::fromBase64Encoding, which silently rejects btoa output).
bool OSimpleSamplerAudioProcessor::dropSampleStart (const juce::String& sessionId,
                                                    const juce::String& name)
{
    if (sessionId.isEmpty())
        return false;

    dropSessionId     = sessionId;
    dropAccumFilename = name;
    dropAccumBase64.clear();
    return true;
}

bool OSimpleSamplerAudioProcessor::dropSampleChunk (const juce::String& sessionId,
                                                    const juce::String& name,
                                                    const juce::String& base64)
{
    if (sessionId != dropSessionId || base64.isEmpty())
        return false;

    // Single source: keep only the most recent chunk's bytes (a later Chunk for the
    // same session replaces it). Commit decodes/publishes the accumulated payload.
    dropAccumFilename = name.isNotEmpty() ? name : dropAccumFilename;
    dropAccumBase64   = base64;
    return true;
}

bool OSimpleSamplerAudioProcessor::dropSampleCommit (const juce::String& sessionId,
                                                     const juce::String& name,
                                                     const juce::String& base64)
{
    if (sessionId != dropSessionId)
        return false;

    // Prefer the commit's own base64 (if any); fall back to the accumulated Chunk.
    const juce::String& b64  = base64.isNotEmpty()  ? base64  : dropAccumBase64;
    const juce::String  fnm  = name.isNotEmpty()    ? name    : dropAccumFilename;
    if (b64.isEmpty())
    {
        endDropSession();
        return false;
    }

    // Decode standard btoa() base64 with convertFromBase64 ONLY (Pitfall 4).
    juce::MemoryBlock raw;
    {
        juce::MemoryOutputStream mos (raw, false);
        if (! juce::Base64::convertFromBase64 (mos, b64))
        {
            endDropSession();
            return false;
        }
    }

    // Identity = the dropped filename (macOS WKWebView strips the disk path, so we
    // persist the name; a name-only identity falls back to the default built-in on
    // restore — handled in setStateInformation/prepareToPlay).
    const juce::String identity = juce::String ("dropped:") + fnm;
    const bool ok = decodeAndPublish (raw.getData(), raw.getSize(), currentSampleRate, identity);
    endDropSession();

    if (ok)
    {
        // New source → re-snap the region/loop markers off the audio thread.
        pendingSnap.store (true, std::memory_order_relaxed);
        triggerAsyncUpdate();
    }
    return ok;
}

void OSimpleSamplerAudioProcessor::endDropSession() noexcept
{
    dropSessionId.clear();
    dropAccumBase64.clear();
    dropAccumFilename.clear();
}

//==============================================================================
// File-picker fallback (async, message thread). The Stage-3 "Load…" button calls
// this. Same decode/resample/publish path as drag-drop; identity = the file path
// (re-readable on session restore).
void OSimpleSamplerAudioProcessor::loadSourceFromFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load a source sound", juce::File{}, "*.wav;*.aif;*.aiff;*.flac");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        const juce::File f = fc.getResult();
        if (f == juce::File{} || ! f.existsAsFile())
            return;                              // cancelled — keep the current source

        juce::MemoryBlock mb;
        if (f.loadFileAsData (mb)
            && decodeAndPublish (mb.getData(), mb.getSize(), currentSampleRate,
                                 f.getFullPathName()))
        {
            pendingSnap.store (true, std::memory_order_relaxed);   // re-snap markers
            triggerAsyncUpdate();
        }
    });
}

//==============================================================================
// On-screen keyboard → synth (Stage 3.1). The editor's uiMidi native fn forwards
// here; queued via the MidiMessageCollector and merged into processBlock's MIDI
// stream so UI notes drive the sampler identically to host MIDI.
void OSimpleSamplerAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);   // collector wants seconds
    midiCollector.addMessageToQueue (msg);
}

//==============================================================================
// APVTS listener (message thread, possibly off it depending on host). Never
// decodes here — defers to the AsyncUpdater so the decode always runs on the
// message thread (RT-safety: no decode on the audio thread).
void OSimpleSamplerAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    using namespace OSimpleSampler::ParamIDs;

    if (parameterID == sourceSample)
    {
        pendingBuiltInIndex.store (juce::jlimit (0, kNumBuiltIns - 1, (int) newValue),
                                   std::memory_order_relaxed);
        triggerAsyncUpdate();
    }
    else if (parameterID == regionStart || parameterID == regionEnd
             || parameterID == loopStart || parameterID == loopEnd)
    {
        // Phase 2.2a: defer the zero-cross snap to the message thread (no audio-thread
        // source scan). Coalesces — multiple marker drags collapse into one re-scan.
        pendingSnap.store (true, std::memory_order_relaxed);
        triggerAsyncUpdate();
    }
}

void OSimpleSamplerAudioProcessor::handleAsyncUpdate()
{
    // RT-safety item 3: consume the deferred fresh-instance root seed set by
    // prepareToPlay. This runs on the guaranteed message thread, so
    // setValueNotifyingHost is safe here. Seeds the CURRENT source's recorded root.
    if (pendingRootSeed.exchange (false, std::memory_order_relaxed))
        seedRootForSource (builtInIndexForIdentity (currentSourceIdentity));

    const int idx = pendingBuiltInIndex.exchange (-1, std::memory_order_relaxed);
    if (idx >= 0)
    {
        // A state restore that lands on a source cancels this pending update
        // (setStateInformation publishes the restored source then cancelPendingUpdate()s),
        // so reaching here always means a genuine user sourceSample-choice change — seed
        // the per-source root so an explicit pick retunes the keyboard.
        loadBuiltInSource (idx, currentSampleRate);
        seedRootForSource (idx);
        pendingSnap.store (true, std::memory_order_relaxed);   // new source → re-snap markers
    }

    // Phase 2.2a: recompute zero-cross-snapped markers off the audio thread.
    if (pendingSnap.exchange (false, std::memory_order_relaxed))
        computeZeroCrossSnaps();
}

//==============================================================================
// Zero-cross marker snap (Phase 2.2a, RESEARCH P3.4). OFF the audio thread.
namespace
{
    // Nearest sign change to `nominal` within ±W, minimizing |src[i]| at the flip.
    // Returns `nominal` (clamped) when no sign change is found in the window.
    int snapToZeroCross (const float* d, int len, int nominal, int W = 256) noexcept
    {
        if (d == nullptr || len <= 1)
            return juce::jlimit (0, juce::jmax (0, len - 1), nominal);

        nominal = juce::jlimit (0, len - 1, nominal);
        int   best    = nominal;
        float bestAbs = std::abs (d[nominal]);
        const int lo = juce::jmax (1, nominal - W);
        const int hi = juce::jmin (len - 1, nominal + W);
        for (int i = lo; i <= hi; ++i)
        {
            const bool flip = (d[i - 1] < 0.0f) != (d[i] < 0.0f);
            if (flip)
            {
                const float a   = std::abs (d[i]);
                const float b   = std::abs (d[i - 1]);
                const int   idx = (a <= b) ? i : i - 1;
                const float v   = juce::jmin (a, b);
                if (v < bestAbs) { bestAbs = v; best = idx; }
            }
        }
        return best;
    }
}

void OSimpleSamplerAudioProcessor::computeZeroCrossSnaps()
{
    // Off-audio reader (prepareToPlay / handleAsyncUpdate / setStateInformation):
    // copy the owning shared_ptr under the lock and hold it for the scan.
    std::shared_ptr<juce::AudioBuffer<float>> snapSrc;
    {
        const juce::ScopedLock sl (sourceMutex);
        snapSrc = currentSource;
    }
    if (snapSrc == nullptr || snapSrc->getNumSamples() <= 1)
    {
        // Invalidate → audio thread falls back to the live %-computed bounds.
        snapRegionStart.store (-1, std::memory_order_relaxed);
        snapRegionEnd.store   (-1, std::memory_order_relaxed);
        snapLoopStart.store   (-1, std::memory_order_relaxed);
        snapLoopEnd.store     (-1, std::memory_order_relaxed);
        return;
    }

    const float* d   = snapSrc->getReadPointer (0);
    const int    len = snapSrc->getNumSamples();

    // Nominal region bounds (% of source) — mirror the processBlock math.
    const float startPct = juce::jlimit (0.0f, 100.0f, startParam->load());
    const float endPct   = juce::jlimit (0.0f, 100.0f, endParam->load());
    const int rsNom = juce::jlimit (0, len - 1, (int) (startPct * 0.01f * (float) len));
    const int reNom = juce::jlimit (rsNom + 1, len, (int) (endPct * 0.01f * (float) len));

    // Nominal loop bounds (% of region).
    const int   regionLen = juce::jmax (1, reNom - rsNom);
    const float lsPct = juce::jlimit (0.0f, 100.0f, loopStartParam->load());
    const float lePct = juce::jlimit (0.0f, 100.0f, loopEndParam->load());
    const int lsNom = rsNom + (int) (lsPct * 0.01f * (float) regionLen);
    const int leNom = rsNom + (int) (lePct * 0.01f * (float) regionLen);

    snapRegionStart.store (snapToZeroCross (d, len, rsNom), std::memory_order_relaxed);
    snapRegionEnd.store   (snapToZeroCross (d, len, reNom), std::memory_order_relaxed);
    snapLoopStart.store   (snapToZeroCross (d, len, lsNom), std::memory_order_relaxed);
    snapLoopEnd.store     (snapToZeroCross (d, len, leNom), std::memory_order_relaxed);
}

//==============================================================================
void OSimpleSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // sampler voices ADD into a cleared buffer

    // Merge any on-screen-keyboard notes into the host MIDI stream before the
    // sampler voices render below (the WebView keyboard injects via handleUiMidi).
    // Always called — even with no host MIDI — so a UI note is never dropped
    // (bridge-gap guard: a dead uiMidi would otherwise pass build/auval silently).
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    // --- Read the source buffer pointer ONCE (RT-safe raw-ptr handoff) --------
    // The audio thread reads ONLY this raw atomic pointer (acquire) — never a
    // shared_ptr — so it can never own the last ref and free a buffer in the render
    // path. retiredSource (message side) keeps the previous generation alive until the
    // next user-paced swap, so this pointer stays valid for the whole block.
    auto* src = sourceForAudio.load (std::memory_order_acquire);
    const float* srcPtr = (src != nullptr && src->getNumSamples() > 0) ? src->getReadPointer (0) : nullptr;
    const int    srcLen = (src != nullptr) ? src->getNumSamples() : 0;

    // --- Read APVTS atomics once and build the per-voice param push ----------
    // Voices never touch the APVTS — the processor reads it here and calls
    // setParams(...). keyRatio uses the LIVE root/tune/fine (not kRootNote).
    SamplerVoiceParams p;
    p.rootKey  = (int) rootKeyParam->load();
    p.tune     = (int) tuneParam->load();
    p.fine     = fineParam->load();
    p.velToAmp = velToAmpParam->load();
    p.amp = juce::ADSR::Parameters {
        ampAttackParam->load(), ampDecayParam->load(),
        ampSustainParam->load(), ampReleaseParam->load() };

    // --- Phase 2.2a: loop / reverse / pitch-mode / Vintage (live per block) ----
    p.loopMode  = juce::jlimit (0, 2, (int) loopModeParam->load());
    p.reverse   = reverseParam->load() > 0.5f;
    p.pitchMode = juce::jlimit (0, 1, (int) pitchModeParam->load());   // latched at note-on by the voice
    p.vintage   = juce::jlimit (0.0f, 100.0f, vintageParam->load());

    // --- Filter: processor-side smoothing (NOT 16× per-voice — the filter is
    //     global). One SmoothedValue each (20 ms), advanced per block; the resulting
    //     scalar is pushed to every voice (one tan recompute) + drives the display
    //     atomics. res%→Q is a NET-NEW map (do NOT reuse resonanceToK); floor 0.707
    //     so setResonance is never 0 (it asserts >0). RESEARCH P1.1 / P5.2.
    const float cutoffTarget = juce::jlimit (20.0f, 0.45f * (float) currentSampleRate,
                                             filterCutoffParam->load());
    const float qTarget      = juce::jmap (juce::jlimit (0.0f, 100.0f, filterResonanceParam->load()),
                                           0.0f, 100.0f, 0.707f, 12.0f);
    filterCutoffSm.setTargetValue (cutoffTarget);
    filterQSm.setTargetValue      (qTarget);
    p.filterCutoff = filterCutoffSm.skip (numSamples);   // advance + take the block scalar
    p.filterQ      = filterQSm.skip (numSamples);

    // --- Region: start/end as % of the source length, in the SOURCE frame; loop
    //     bounds as % of the region. Both are then OVERWRITTEN by the off-thread
    //     zero-cross-snapped indices when available (RESEARCH P3.4 / Task 1).
    //     startSamp clamped to [0, srcLen-1] so the endSamp lower bound (startSamp+1)
    //     never exceeds srcLen even at start = 100 %.
    if (srcLen > 0)
    {
        const float startPct = juce::jlimit (0.0f, 100.0f, startParam->load());
        const float endPct   = juce::jlimit (0.0f, 100.0f, endParam->load());
        p.startSamp = juce::jlimit (0, srcLen - 1, (int) (startPct * 0.01f * (float) srcLen));
        p.endSamp   = juce::jlimit (p.startSamp + 1, srcLen, (int) (endPct * 0.01f * (float) srcLen));

        // Override region bounds with the snapped markers (secondary seam defense).
        const int ss = snapRegionStart.load (std::memory_order_relaxed);
        if (ss >= 0 && ss < srcLen)               p.startSamp = ss;
        const int se = snapRegionEnd.load (std::memory_order_relaxed);
        if (se > p.startSamp && se <= srcLen)     p.endSamp = se;

        // Loop bounds = % of REGION, then snapped override, then clamp
        // startSamp ≤ loopStart < loopEnd ≤ endSamp.
        const int   regionLen = juce::jmax (1, p.endSamp - p.startSamp);
        const float lsPct = juce::jlimit (0.0f, 100.0f, loopStartParam->load());
        const float lePct = juce::jlimit (0.0f, 100.0f, loopEndParam->load());
        int loopAbsStart = p.startSamp + (int) (lsPct * 0.01f * (float) regionLen);
        int loopAbsEnd   = p.startSamp + (int) (lePct * 0.01f * (float) regionLen);

        const int sls = snapLoopStart.load (std::memory_order_relaxed);
        if (sls >= 0) loopAbsStart = sls;
        const int sle = snapLoopEnd.load (std::memory_order_relaxed);
        if (sle >= 0) loopAbsEnd = sle;

        loopAbsStart = juce::jlimit (p.startSamp, p.endSamp - 1, loopAbsStart);
        loopAbsEnd   = juce::jlimit (loopAbsStart + 1, p.endSamp, loopAbsEnd);
        p.loopStartSamp = loopAbsStart;
        p.loopEndSamp   = loopAbsEnd;

        // loopCrossfade ms → samples, capped at half the loop length.
        const float xfMs = juce::jlimit (0.0f, 500.0f, loopCrossfadeParam->load());
        p.xfadeSamp = juce::jlimit (0, (loopAbsEnd - loopAbsStart) / 2,
                                    (int) (xfMs * 0.001f * (float) currentSampleRate));
    }
    else
    {
        p.startSamp = 0; p.endSamp = 0;
        p.loopStartSamp = 0; p.loopEndSamp = 0; p.xfadeSamp = 0;
    }

    // Push params + the source snapshot to every voice.
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SampleVoice*> (synth.getVoice (v)))
        {
            sv->setParams (p);
            sv->setSource (srcPtr, srcLen);
        }

    // --- Render the sampler voices -------------------------------------------
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // --- Lead-voice (loudest-active) display atomics (Phase 2.2a, RESEARCH P5.3) --
    // The filter is GLOBAL in v1, so the published cutoff/k are the processor-smoothed
    // scalars every voice runs; the loudest-active scan stages the Stage-3 lead-voice
    // pattern (Stage 3 extends it to a per-voice playhead without a structural change).
    // Published every block so the Stage-3 curve tracks the knob even with no note;
    // k = 1/Q = JUCE's R2 → SubFilterCurve matches the running filter (QUAL-02).
    // Phase 2.3: the loudest-active scan now also captures that voice's playhead
    // (normalized [0,1] over the live region) and publishes it to displayPlayhead
    // for the Stage-3 waveform overlay (RESEARCH §12). 0 when nothing sounds.
    float        leadAmp   = -1.0f;
    SampleVoice* leadVoice = nullptr;
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SampleVoice*> (synth.getVoice (v)))
            if (sv->isSounding())
            {
                const float a = sv->getAmpEnvVal();
                if (a > leadAmp) { leadAmp = a; leadVoice = sv; }
            }
    displayPlayhead.store (leadVoice != nullptr ? leadVoice->getPlayheadPos() : 0.0f,
                           std::memory_order_relaxed);
    displayCutoffHz.store (p.filterCutoff, std::memory_order_relaxed);
    displayK.store        (1.0f / juce::jmax (1.0e-6f, p.filterQ), std::memory_order_relaxed);

    // --- Master output trim (dB->lin, smoothed) ------------------------------
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));
    const float g0 = outputGain.getCurrentValue();
    const float g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, numSamples, g0, g1);

    // --- Suite-wide NaN/Inf insurance ----------------------------------------
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }

    // --- Output scope/spectrum tap (Phase 2.3, RESEARCH §12): post-gain mono sum
    // into the lock-free ring (COPY-ONLY, NO alloc, NO FFT — the FFT is the Stage-3
    // editor Timer's job). Mirror O-simpleGrain PluginProcessor.cpp:771-797: a stack
    // mono[] in <= 4096-sample chunks so the audio thread never allocates. The ring
    // is the ONLY audio-thread viz work; everything downstream (FFT, scope draw,
    // playhead overlay) is Stage-3 editor-side off the audio thread.
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
juce::AudioProcessorEditor* OSimpleSamplerAudioProcessor::createEditor()
{
   #if JUCE_WEB_BROWSER
    return new OSimpleSamplerAudioProcessorEditor (*this);
   #else
    // Render-harness / WebView-disabled builds (JUCE_WEB_BROWSER=0): the WebView
    // editor types are unavailable, so fall back to the generic editor. The Stage-4
    // harness re-run also drops PluginEditor.cpp from its sources (Pitfall 9).
    return new juce::GenericAudioProcessorEditor (*this);
   #endif
}

//==============================================================================
void OSimpleSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Serialize the APVTS tree PLUS a custom child holding the loaded-source
    // identity, so a session restores both the params and the active source.
    auto state = apvts.copyState();

    auto sourceChild = state.getOrCreateChildWithName (
        juce::Identifier (kSourceStateTag), nullptr);
    sourceChild.setProperty (juce::Identifier (kSourceIdProp),
                             currentSourceIdentity, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || state.getType() != apvts.state.getType())
        return;

    // Restore the custom loaded-source identity (if present) before handing the
    // tree to the APVTS. Default stays "embedded:piano" when absent (legacy state).
    auto sourceChild = state.getChildWithName (juce::Identifier (kSourceStateTag));
    if (sourceChild.isValid())
        currentSourceIdentity = sourceChild.getProperty (
            juce::Identifier (kSourceIdProp), currentSourceIdentity).toString();

    // replaceState() fires the sourceSample listener, which queues an AsyncUpdater
    // to rebuild a built-in source. That update is deferred (it runs AFTER this
    // method returns), so we publish the correct source below, then
    // cancelPendingUpdate() to drop the queued rebuild — otherwise a restored source
    // would be clobbered by the built-in choice (and its root re-seed) a moment later.
    apvts.replaceState (state);

    // Mark the session restored so the prepare-time root seed (Task 5) is skipped —
    // the saved rootKey wins on a restored session (we do NOT seed the root here).
    stateWasRestored = true;

    // Re-decode the active source at the current engine rate so the restored session
    // sounds the same. A host can restore state AFTER prepareToPlay, so reload here
    // too. Phase 2.1 has built-ins only; a user-file identity falls back to piano.
    if (currentSampleRate > 0.0)
    {
        if (currentSourceIdentity.startsWith ("embedded:"))
        {
            loadBuiltInSource (builtInIndexForIdentity (currentSourceIdentity), currentSampleRate);
        }
        else if (juce::File::isAbsolutePath (currentSourceIdentity)
                 && juce::File (currentSourceIdentity).existsAsFile())
        {
            // Restored picker source (identity = absolute path). A "dropped:<name>"
            // identity is not absolute → falls back to piano (bytes not persisted).
            juce::MemoryBlock mb;
            const juce::File f (currentSourceIdentity);
            if (! (f.loadFileAsData (mb)
                   && decodeAndPublish (mb.getData(), mb.getSize(), currentSampleRate,
                                        currentSourceIdentity)))
            {
                currentSourceIdentity = "embedded:piano";
                loadBuiltInSource (0, currentSampleRate);
            }
        }
        else
        {
            currentSourceIdentity = "embedded:piano";
            loadBuiltInSource (0, currentSampleRate);
        }
    }

    // Drop the sourceSample-rebuild that replaceState() queued above — the restored
    // source is already published, so letting the pending built-in load (with its
    // root re-seed) run would only clobber the restored state.
    cancelPendingUpdate();
    pendingBuiltInIndex.store (-1, std::memory_order_relaxed);

    // replaceState() also fired the region/loop listeners (Phase 2.2a), queuing a
    // snap that cancelPendingUpdate() just dropped. Snap the restored markers directly
    // (off the audio thread; self-guards on a null source if state restores before
    // prepareToPlay, which then re-snaps).
    pendingSnap.store (false, std::memory_order_relaxed);
    computeZeroCrossSnaps();
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleSamplerAudioProcessor();
}
