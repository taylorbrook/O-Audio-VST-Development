/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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

    O-simpleGrain - Audio Processor (implementation)

    Stage 1 (Foundation): silent 8-voice synth shell. Builds the full 19-parameter
    APVTS and persists it alongside a custom loaded-source identity. processBlock
    clears the buffer and consumes MIDI (no audio until Stage 2). Allocation-free.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GrainVoice.h"
#include "GrainSound.h"
#include "BinaryData.h"      // embedded fire/voice/water/piano .wav (Stage 2.3)
#include <algorithm>         // std::remove_if (retired-source reap)

namespace
{
    // Shared skews (match parameter-spec.md → research-locked ARCHITECTURE.md).
    constexpr float kAdsrTimeSkew = 0.35f; // perceptual taper for 0–5 s envelope times
    constexpr float kMinAdsrTime  = 0.0f;
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
OSimpleGrainAudioProcessor::createParameterLayout()
{
    using namespace OSimpleGrain::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Source ------------------------------------------------------------
    // Which built-in short sound is granulated. The "(loaded)" user-file state
    // is reflected in custom (non-APVTS) state, NOT as a 5th choice. Default fire.
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { sourceSample, 1 }, "Source",
        juce::StringArray { "fire", "voice", "water", "piano" }, 0));

    //--- Grain -------------------------------------------------------------
    // Grain size 2–500 ms (v1.4.0: ceiling raised from 200). Skew 0.35 keeps
    // the fine low end (the buzz↔fragments axis lives down there) — the knob's
    // midpoint sits near 70 ms. Stored denormalised (ms), so pre-1.4.0 sessions
    // restore unchanged.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { grainSize, 1 }, "Grain Size",
        juce::NormalisableRange<float> { 2.0f, 500.0f, 0.01f, 0.35f }, 30.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    // Density 1–200 grains/s, logarithmic feel. setSkewForCentre(20) puts the
    // musical centre of travel around 20 g/s (fine control at the sparse low end).
    {
        juce::NormalisableRange<float> densityRange { 1.0f, 200.0f, 0.01f };
        densityRange.setSkewForCentre (20.0f);
        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { density, 1 }, "Density", densityRange, 40.0f,
            juce::AudioParameterFloatAttributes().withLabel ("g/s")));
    }

    // Position 0–100 % — read-head resting point.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { position, 1 }, "Position",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Scan / time-stretch −200–+200 %, bipolar (negative = reverse). Default held.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { scan, 1 }, "Scan",
        juce::NormalisableRange<float> { -200.0f, 200.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Freeze — pin the read head on the current instant. Off by default.
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { freeze, 1 }, "Freeze", false));

    //--- Window Shape ------------------------------------------------------
    // Per-grain amplitude envelope. Default Hann = index 4. Tukey (index 5,
    // v1.4.0) appended LAST so stored indices 0–4 keep their meaning.
    // Rectangular carries a fixed 1 ms guard fade at each edge since v1.4.0
    // (WindowLuts::kRectGuardMs) — the hard step used to click.
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { windowShape, 1 }, "Window",
        juce::StringArray { "Rectangular", "Triangular", "Welch", "Gaussian", "Hann", "Tukey" }, 4));

    // Tukey taper α, 0–100 % of the grain spent fading (half at each edge).
    // 0 % = rectangular (guard-faded), 100 % = Hann. Only read when Window = Tukey.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { windowTaper, 1 }, "Taper",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Spray & Scatter ---------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { pitchSpray, 1 }, "Pitch Spray",
        juce::NormalisableRange<float> { 0.0f, 12.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { positionSpray, 1 }, "Position Spray",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { scatter, 1 }, "Scatter",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { grainPitch, 1 }, "Grain Pitch",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { panSpray, 1 }, "Pan Spray",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToDensity, 1 }, "Vel -> Density",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Amplitude envelope (per-voice ADSR) -------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.01f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    // Sustain stored 0–1 (UI scales ×100). Default 0.8.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 0.8f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.4f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    // ADSR on/off (v1.1.0). Default ON = the v1.0.x always-enveloped behaviour,
    // so existing sessions/presets keep the shipped sound. OFF bypasses A/D/S/R:
    // flat level while held; on release the voice stops launching new grains and
    // the cloud fades out over one grain length through the window envelopes.
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { adsrEnabled, 1 }, "ADSR", true));

    //--- Output ------------------------------------------------------------
    // −inf–0 dB master trim. −60 dB floor maps to "−inf" perceptually.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleGrainAudioProcessor::OSimpleGrainAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    using namespace OSimpleGrain::ParamIDs;

    // Cache raw-param atomic pointers. Established now (read once per block by the
    // grain engine in Stage 2; unused while silent).
    sourceSampleParam  = apvts.getRawParameterValue (sourceSample);
    grainSizeParam     = apvts.getRawParameterValue (grainSize);
    densityParam       = apvts.getRawParameterValue (density);
    positionParam      = apvts.getRawParameterValue (position);
    scanParam          = apvts.getRawParameterValue (scan);
    freezeParam        = apvts.getRawParameterValue (freeze);
    windowShapeParam   = apvts.getRawParameterValue (windowShape);
    windowTaperParam   = apvts.getRawParameterValue (windowTaper);
    pitchSprayParam    = apvts.getRawParameterValue (pitchSpray);
    positionSprayParam = apvts.getRawParameterValue (positionSpray);
    scatterParam       = apvts.getRawParameterValue (scatter);
    grainPitchParam    = apvts.getRawParameterValue (grainPitch);
    panSprayParam      = apvts.getRawParameterValue (panSpray);
    velToDensityParam  = apvts.getRawParameterValue (velToDensity);
    ampAttackParam     = apvts.getRawParameterValue (ampAttack);
    ampDecayParam      = apvts.getRawParameterValue (ampDecay);
    ampSustainParam    = apvts.getRawParameterValue (ampSustain);
    ampReleaseParam    = apvts.getRawParameterValue (ampRelease);
    adsrEnabledParam   = apvts.getRawParameterValue (adsrEnabled);
    outputLevelParam   = apvts.getRawParameterValue (outputLevel);

    // Preallocate all grain voices up front (no audio-thread allocation later).
    // Hand each voice the shared window-LUT table set (built at construction,
    // before the synth member, so the pointer is valid here). The typed pointer
    // is cached (IN-02) — the synth owns the voice for the processor's lifetime,
    // so per-block dynamic_casts are unnecessary RTTI on the audio thread.
    for (int i = 0; i < kMaxVoices; ++i)
    {
        auto* v = new GrainVoice();
        v->setWindowLuts (&windowLuts);
        synth.addVoice (v);
        grainVoices[(size_t) i] = v;
    }

    synth.addSound (new GrainSound());           // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);         // steal quietest/oldest voice on overflow

    // Listen for sourceSample selection changes (Stage 2.3). The decode/resample
    // is dispatched to the message thread via AsyncUpdater (never the audio
    // thread) — see parameterChanged / handleAsyncUpdate.
    apvts.addParameterListener (sourceSample, this);
}

OSimpleGrainAudioProcessor::~OSimpleGrainAudioProcessor()
{
    apvts.removeParameterListener (OSimpleGrain::ParamIDs::sourceSample, this);
    cancelPendingUpdate();
}

//==============================================================================
void OSimpleGrainAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate, std::memory_order_relaxed);

    // Granular processing has no inherent latency in this design.
    // NB: getLatencySamples() is non-virtual in JUCE 8 — never override it.
    setLatencySamples (0);

    // Synthesiser + per-voice prepare. juce::SynthesiserVoice has no virtual
    // prepareToPlay in JUCE 8 — dispatch the custom one via the cached typed
    // pointers (IN-02; the voices are synth-owned for the processor's lifetime).
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (auto* gv : grainVoices)
        gv->prepareToPlay (sampleRate, samplesPerBlock);

    // Reset the shared viz grain count (voices forget their contribution in their
    // own prepareToPlay, so a clean delta is recomputed on the first render).
    activeGrainCount.store (0, std::memory_order_relaxed);

    // On-screen-keyboard MIDI queue: timestamps are seconds; tell it the rate.
    midiCollector.reset (sampleRate);

    // Output trim (dB->lin, 20 ms smoothing).
    outputGain.reset (sampleRate, 0.02);
    const float outDb = outputLevelParam->load();
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));

    // --- Global read-head smoothing (Phase 2.2, ~20 ms ramps) ----------------
    // scan/position/playheadVelocity are smoothed so automation, freeze toggles,
    // and source swaps never zipper or hard-jump the playhead (QUAL-01).
    scanSmoothed.reset     (sampleRate, 0.02);
    positionSmoothed.reset (sampleRate, 0.02);
    playheadVelocity.reset (sampleRate, 0.02);
    scanSmoothed.setCurrentAndTargetValue     (scanParam->load() / 100.0f);
    positionSmoothed.setCurrentAndTargetValue (positionParam->load());
    playheadVelocity.setCurrentAndTargetValue (0.0f);

    // Rest-ease coefficient from the τ time constant (IN-03) — same glide feel
    // at every sample rate (the old fixed 0.0008/sample was ~2× faster at 96 k).
    restEaseCoeff = 1.0 - std::exp (-1.0 / (kRestEaseTauSeconds * sampleRate));

    // Seed the playhead at its resting point (position% * srcLen is unknown until
    // the source is decoded below; we re-seed once the source length is known).
    playheadPos = 0.0;

    // Decode + resample the active source to the engine rate, OFF the audio
    // thread, and publish it (R8: resample on every prepareToPlay since the
    // engine rate may change). The active source is a built-in
    // ("embedded:<name>"), a picker file (absolute path), or a dropped file
    // ("dropped:<name>" — name only, WKWebView strips disk paths).
    //
    // CR-01 (v1.1.1): a LIVE user source must never be silently clobbered by the
    // built-in fallback just because the host re-prepared (buffer-size change,
    // engine stop/start, offline bounce). The fallback is only for identities we
    // genuinely cannot realise: a restored name-only drop in a fresh instance, or
    // a picker path that no longer exists AND no live buffer to keep.
    bool loadedUserFile = false;
    const juce::String identity = getSourceIdentity();

    if (identity.startsWith ("embedded:"))
    {
        // Skip the re-decode when the published source is already this identity
        // at this engine rate (IN-09) — hosts re-prepare on every buffer-size
        // change / transport-engine restart, and decoding + resampling 10 s of
        // audio each time is a pointless message/host-thread stall.
        bool alreadyCurrent = false;
        {
            const juce::ScopedLock sl (sourceStateLock);
            alreadyCurrent = (currentSource != nullptr && publishedSourceRate == sampleRate);
        }
        // Built-in: pick the index matching the identity (falls back to the
        // sourceSample choice, then fire, if the name is unknown).
        if (! alreadyCurrent)
            loadBuiltInSource (builtInIndexForIdentity (identity), sampleRate);
    }
    else
    {
        if (identity.startsWith ("dropped:"))
        {
            const juce::ScopedLock sl (sourceStateLock);
            if (currentSource != nullptr && publishedSourceRate == sampleRate)
            {
                loadedUserFile = true;               // already at the engine rate — keep as-is
            }
            else if (userSourceBytes.getSize() > 0 && userSourceBytesIdentity == identity)
            {
                // Rate changed and the dropped bytes are retained — re-decode.
                loadedUserFile = decodeAndPublish (userSourceBytes.getData(),
                                                   userSourceBytes.getSize(),
                                                   sampleRate, identity);
            }
            if (! loadedUserFile && currentSource != nullptr)
                loadedUserFile = true;   // bytes gone: keep the live buffer (transposed on a
                                         // rate change) rather than discard the user's sound
        }
        else if (juce::File::isAbsolutePath (identity))   // guard: juce::File(non-path) jasserts
        {
            // Picker file: re-read its path at the new rate.
            const juce::File f (identity);
            if (f.existsAsFile())
            {
                juce::MemoryBlock mb;
                if (f.loadFileAsData (mb)
                    && decodeAndPublish (mb.getData(), mb.getSize(), sampleRate, identity))
                    loadedUserFile = true;
            }
            if (! loadedUserFile)
            {
                // Path vanished mid-session: keep a live buffer over clobbering.
                const juce::ScopedLock sl (sourceStateLock);
                if (currentSource != nullptr)
                    loadedUserFile = true;
            }
        }

        if (! loadedUserFile)
        {
            // Nothing realisable (fresh-instance restore of a name-only identity,
            // or a missing file with no live buffer) — documented fallback.
            setSourceIdentity ("embedded:fire");
            loadBuiltInSource (0, sampleRate);
        }
    }

    // Reap any retired buffers that are now provably unreferenced (the host
    // guarantees prepareToPlay never runs concurrently with processBlock).
    {
        const juce::ScopedLock sl (sourceStateLock);
        reapRetiredSources();
    }

    // Now the source length is known — seed the playhead at the resting point so
    // the first block starts where `position` points (no jump-from-zero glide).
    std::shared_ptr<juce::AudioBuffer<float>> src;
    {
        const juce::ScopedLock sl (sourceStateLock);
        src = currentSource;
    }
    if (src != nullptr && src->getNumSamples() > 0)
    {
        const int srcLen = src->getNumSamples();
        playheadPos = (double) (positionParam->load() / 100.0f) * (double) srcLen;
        playheadPos = juce::jlimit (0.0, (double) (srcLen - 1), playheadPos);
    }
}

//==============================================================================
// Publish a fully-built buffer to the audio thread (caller holds sourceStateLock).
// The outgoing buffer is PARKED, not freed — an in-flight audio block may still
// be reading its raw pointer (see the header note; WR-05).
void OSimpleGrainAudioProcessor::publishSource (std::shared_ptr<juce::AudioBuffer<float>> newBuf)
{
    reapRetiredSources();

    if (currentSource != nullptr)
        retiredSources.push_back ({ currentSource,
                                    blocksRendered.load (std::memory_order_acquire) });

    currentSource = std::move (newBuf);
    audioSourceView.store (currentSource.get(), std::memory_order_release);
}

// Free retired buffers once >= 2 audio blocks have completed since parking: any
// block that could have loaded the old raw pointer has finished by then. If
// audio is suspended the entries simply wait (bounded memory, never unsafe).
void OSimpleGrainAudioProcessor::reapRetiredSources()
{
    const juce::uint64 now = blocksRendered.load (std::memory_order_acquire);
    retiredSources.erase (
        std::remove_if (retiredSources.begin(), retiredSources.end(),
                        [now] (const RetiredSource& r) { return now >= r.parkedAtBlock + 2; }),
        retiredSources.end());
}

//==============================================================================
// Source decode/resample/publish (Stage 2.3). ALL of this runs OFF the audio
// thread (prepareToPlay, the AsyncUpdater for sourceSample changes, the drag-drop
// NativeFunction callbacks, and the FileChooser callback). The audio thread only
// ever sees a fully-built buffer published via the atomic shared_ptr swap (R6).

// Map embedded BinaryData symbols by built-in index. Index order MUST match the
// sourceSample AudioParameterChoice: 0=fire, 1=voice, 2=water, 3=piano. JUCE
// mangles `fire.wav` -> BinaryData::fire_wav / BinaryData::fire_wavSize.
namespace
{
    struct BuiltInBlob { const char* data; int size; };

    BuiltInBlob builtInBlob (int idx) noexcept
    {
        switch (idx)
        {
            case 0:  return { BinaryData::fire_wav,  BinaryData::fire_wavSize  };
            case 1:  return { BinaryData::voice_wav, BinaryData::voice_wavSize };
            case 2:  return { BinaryData::water_wav, BinaryData::water_wavSize };
            case 3:  return { BinaryData::piano_wav, BinaryData::piano_wavSize };
            default: return { BinaryData::fire_wav,  BinaryData::fire_wavSize  };
        }
    }
}

int OSimpleGrainAudioProcessor::builtInIndexForIdentity (const juce::String& identity) const
{
    if (identity.startsWith ("embedded:"))
    {
        const auto name = identity.fromFirstOccurrenceOf ("embedded:", false, false);
        for (int i = 0; i < kNumBuiltIns; ++i)
            if (name == kBuiltInNames[i])
                return i;
    }
    // Fall back to the live choice param, then fire.
    if (sourceSampleParam != nullptr)
        return juce::jlimit (0, kNumBuiltIns - 1, (int) sourceSampleParam->load());
    return 0;
}

// Resample a decoded buffer (srcRate) to engineRate, capped at the source-length
// cap. Returns a fully-built shared_ptr ready to publish. `truncated` is set when
// the source exceeded the cap (the UI surfaces a notice in Stage 3).
std::shared_ptr<juce::AudioBuffer<float>>
OSimpleGrainAudioProcessor::resampleToEngineRate (const juce::AudioBuffer<float>& src,
                                                  double srcRate, double engineRate,
                                                  bool& truncated) const
{
    const int    nCh   = juce::jmax (1, src.getNumChannels());
    const int    nSmp  = src.getNumSamples();
    const double ratio = (srcRate > 0.0 ? srcRate : engineRate) / engineRate; // speedRatio
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

// Decode a raw byte block (a complete .wav/.aiff/.flac file in memory) through
// the format manager, resample to the engine rate, cap, and atomic-publish.
// Shared by the built-in path, the drag-drop base64 path, and the picker path.
bool OSimpleGrainAudioProcessor::decodeAndPublish (const void* data, size_t numBytes,
                                                   double engineRate, const juce::String& identity)
{
    if (data == nullptr || numBytes == 0)
        return false;

    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();                  // WAV / AIFF / FLAC

    std::unique_ptr<juce::AudioFormatReader> reader (
        fmt.createReaderFor (std::make_unique<juce::MemoryInputStream> (data, numBytes, false)));
    if (reader == nullptr)
        return false;                            // unsupported/invalid — caller keeps the previous source

    // WR-02 (v1.1.1): clamp the decode length to the 10 s cap BEFORE allocating.
    // Previously the whole file was decoded (a 45-min WAV = a multi-hundred-MB
    // allocation and stall, 99% then thrown away by the resampler's cap), and
    // lengthInSamples (int64) was truncated straight to int — a hostile header
    // near INT32_MAX drove a multi-GB allocation. +4 keeps the interpolator's
    // guard taps past the cap boundary.
    const juce::int64 len64   = reader->lengthInSamples;
    const double      srcRate = reader->sampleRate > 0.0 ? reader->sampleRate : engineRate;
    if (len64 <= 0)
        return false;

    const juce::int64 maxSrcSmp    = (juce::int64) std::ceil (kMaxSourceSeconds * srcRate) + 4;
    const int         nSmp         = (int) juce::jmin (len64, maxSrcSmp);
    const bool        preTruncated = (len64 > maxSrcSmp);
    const int         nCh          = juce::jmax (1, (int) reader->numChannels);

    // Decode at the source rate (handles 1–2 channels generally; the engine read
    // sums/duplicates as needed — see processBlock's mono channel-0 snapshot).
    juce::AudioBuffer<float> tmp (nCh, nSmp);
    reader->read (&tmp, 0, nSmp, 0, true, true);

    bool truncated = false;
    auto resampled = resampleToEngineRate (tmp, srcRate, engineRate, truncated);

    lastLoadTruncated.store (truncated || preTruncated, std::memory_order_relaxed);

    {
        const juce::ScopedLock sl (sourceStateLock);
        publishSource (std::move (resampled));
        currentSourceIdentity = identity;
        publishedSourceRate   = engineRate;

        // CR-01: retain the raw bytes of a dropped file (no re-readable disk
        // path) so a later prepareToPlay at a NEW engine rate can re-decode
        // instead of clobbering. Skip the self-copy when prepareToPlay feeds the
        // retained block back through this very path; cap so a huge drop can't
        // pin RAM (over the cap the live buffer is kept, transposed, on a rate
        // change). Non-dropped sources release any stale retained bytes.
        if (identity.startsWith ("dropped:"))
        {
            if (data != userSourceBytes.getData())
            {
                if (numBytes <= kMaxRetainedSourceBytes)
                {
                    userSourceBytes.setSize (numBytes);
                    userSourceBytes.copyFrom (data, 0, numBytes);
                    userSourceBytesIdentity = identity;
                }
                else
                {
                    userSourceBytes.reset();
                    userSourceBytesIdentity.clear();
                }
            }
        }
        else
        {
            userSourceBytes.reset();
            userSourceBytesIdentity.clear();
        }
    }

    // Tell the editor timer a new source is live (IN-08) — it emits the
    // "sourceChanged" WebView event on the message thread.
    sourceVersion.fetch_add (1, std::memory_order_relaxed);
    return true;
}

// Decode one embedded built-in by index and publish. OFF the audio thread.
bool OSimpleGrainAudioProcessor::loadBuiltInSource (int builtInIndex, double engineRate)
{
    builtInIndex = juce::jlimit (0, kNumBuiltIns - 1, builtInIndex);
    const auto blob = builtInBlob (builtInIndex);
    const juce::String identity = juce::String ("embedded:") + kBuiltInNames[builtInIndex];
    return decodeAndPublish (blob.data, (size_t) blob.size, engineRate, identity);
}

// Min/max envelope of the currently-published source (UI-02 waveform background).
// Message-thread read: snapshots the owning shared_ptr under the lock (held for
// the call), then reduces channel 0 to `numPairs` min/max pairs in [-1,1]. The
// audio thread is untouched. Returns a flat [min,max,…] vector (empty if no source).
std::vector<float> OSimpleGrainAudioProcessor::getSourceThumbnail (int numPairs) const
{
    numPairs = juce::jlimit (1, 4096, numPairs);

    std::shared_ptr<juce::AudioBuffer<float>> src;   // snapshot — held for this call
    {
        const juce::ScopedLock sl (sourceStateLock);
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

// Rebuild the source from the current sourceSample choice (message thread).
void OSimpleGrainAudioProcessor::rebuildSourceFromChoice()
{
    const int idx = (sourceSampleParam != nullptr)
                  ? juce::jlimit (0, kNumBuiltIns - 1, (int) sourceSampleParam->load()) : 0;
    loadBuiltInSource (idx, currentSampleRate.load (std::memory_order_relaxed));
}

//==============================================================================
// APVTS listener (message thread, possibly off it depending on host). Never
// decodes here — defers to the AsyncUpdater so the decode always runs on the
// message thread (R6 / RT-safety: no decode on the audio thread).
void OSimpleGrainAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == OSimpleGrain::ParamIDs::sourceSample)
    {
        pendingBuiltInIndex.store (juce::jlimit (0, kNumBuiltIns - 1, (int) newValue),
                                   std::memory_order_relaxed);
        triggerAsyncUpdate();
    }
}

void OSimpleGrainAudioProcessor::handleAsyncUpdate()
{
    const int idx = pendingBuiltInIndex.exchange (-1, std::memory_order_relaxed);
    if (idx < 0)
        return;

    // A state restore that lands on a user/dropped file cancels this pending update
    // (setStateInformation publishes the restored source then cancelPendingUpdate()s),
    // so reaching here always means a genuine sourceSample-choice change.
    loadBuiltInSource (idx, currentSampleRate.load (std::memory_order_relaxed));
}

//==============================================================================
// Drag-drop streaming bridge (Stage 2.3). These are the C++ side of the shared
// webview-drop-streaming.js module; Stage 3 wires the JS that CALLS them via
// WebBrowserComponent::Options::withNativeFunction. Names are FIXED by the shared
// module. Single granular source -> the single-file path (Start -> Add -> Commit)
// is the live path; the folder-commit is registered as a clean no-op so the
// shared module binds. All on the message thread; decode via convertFromBase64.
bool OSimpleGrainAudioProcessor::dropSessionStart (const juce::String& sessionId,
                                                   const juce::String& folderName)
{
    if (sessionId.isEmpty())
        return false;

    dropSessionId         = sessionId;
    dropSessionFolderName = folderName;
    dropAccumBase64.clear();
    dropAccumFilename.clear();
    return true;
}

bool OSimpleGrainAudioProcessor::dropSessionAddFile (const juce::String& sessionId,
                                                     const juce::String& filename,
                                                     const juce::String& base64)
{
    if (sessionId != dropSessionId || base64.isEmpty())
        return false;

    // Single source: keep only the most recent file's bytes (a later AddFile for
    // the same session simply replaces it). The commit decodes/publishes.
    dropAccumFilename = filename;
    dropAccumBase64   = base64;
    return true;
}

bool OSimpleGrainAudioProcessor::dropSessionCommitFile (const juce::String& sessionId,
                                                        const juce::String& filename,
                                                        const juce::String& base64)
{
    if (sessionId != dropSessionId)
        return false;

    // Prefer the commit's own base64 (single-file drops send it on commit); fall
    // back to the accumulated AddFile payload.
    const juce::String& b64  = base64.isNotEmpty()  ? base64  : dropAccumBase64;
    const juce::String  name = filename.isNotEmpty() ? filename : dropAccumFilename;
    if (b64.isEmpty())
    {
        endDropSession();
        return false;
    }

    // R7 / §9.2: decode standard btoa() base64 with juce::Base64::convertFromBase64
    // ONLY (MemoryBlock::fromBase64Encoding silently rejects it).
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
    // persist the name; on restore a name-only identity falls back to the default
    // built-in with a notice — handled in setStateInformation/prepareToPlay).
    const juce::String identity = juce::String ("dropped:") + name;
    const bool ok = decodeAndPublish (raw.getData(), raw.getSize(),
                                      currentSampleRate.load (std::memory_order_relaxed), identity);
    endDropSession();
    return ok;
}

bool OSimpleGrainAudioProcessor::dropSessionCommitFolder (const juce::String& sessionId)
{
    // O-simpleGrain has a single source — a folder drop is not meaningful. Accept
    // the call so the shared JS binds cleanly, then clean up the session.
    if (sessionId == dropSessionId)
        endDropSession();
    return false;
}

void OSimpleGrainAudioProcessor::endDropSession() noexcept
{
    dropSessionId.clear();
    dropSessionFolderName.clear();
    dropAccumBase64.clear();
    dropAccumFilename.clear();
}

//==============================================================================
// File-picker fallback (async, message thread). Stage 3 calls this from a
// "Load…" button. Same decode/resample/publish path as drag-drop.
void OSimpleGrainAudioProcessor::loadSourceFromFileChooser()
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
        if (f.loadFileAsData (mb))
            decodeAndPublish (mb.getData(), mb.getSize(),
                              currentSampleRate.load (std::memory_order_relaxed),
                              f.getFullPathName());  // identity = path (re-readable on restore)
    });
}

void OSimpleGrainAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
}

bool OSimpleGrainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void OSimpleGrainAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);   // MidiMessageCollector wants seconds
    midiCollector.addMessageToQueue (msg);
}

void OSimpleGrainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // grain voices ADD into a cleared buffer

    // Merge any on-screen-keyboard notes into the host MIDI stream before the
    // grain voices render below (the WebView keyboard injects via handleUiMidi).
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    // --- Snapshot the source view ONCE (valid for the whole block) ------------
    // Wait-free (WR-05): a single atomic raw-pointer load — no shared_ptr
    // ref-count, no lock. The pointee is immutable and stays alive because a
    // publish PARKS the outgoing buffer until ≥2 blocks after the swap (see
    // publishSource/reapRetiredSources); blocksRendered below is the fence.
    const juce::AudioBuffer<float>* src = audioSourceView.load (std::memory_order_acquire);
    const float* srcPtr = nullptr;
    int          srcLen = 0;
    if (src != nullptr && src->getNumSamples() > 0)
    {
        srcPtr = src->getReadPointer (0);        // mono read in 2.1 (channel 0)
        srcLen = src->getNumSamples();
    }

    // --- Read APVTS atomics once and push to every voice ---------------------
    GrainVoiceParams p;
    p.grainSizeMs    = grainSizeParam->load();
    p.density        = densityParam->load();
    p.windowShape    = (int) windowShapeParam->load();
    p.windowTaper    = windowTaperParam->load() * 0.01f;      // 0..100 % → α 0..1
    p.grainPitch     = grainPitchParam->load();
    p.positionSpray  = positionSprayParam->load();
    p.pitchSpray     = pitchSprayParam->load();
    p.scatter        = scatterParam->load();
    p.panSpray       = panSprayParam->load();
    p.velToDensity   = velToDensityParam->load() * 0.01f;   // 0..100 % → 0..1 depth (GrainVoice consumes a 0..1 depth)
    p.adsrEnabled    = adsrEnabledParam->load() > 0.5f;     // v1.1.0 — envelope bypass
    p.amp = juce::ADSR::Parameters {
        ampAttackParam->load(), ampDecayParam->load(),
        ampSustainParam->load(), ampReleaseParam->load() };

    // --- Global read head (Phase 2.2): advance per sample, freeze-pinnable -----
    // velocity (samples/sample) = (scan/100) * realtime: 100% = forward realtime,
    // -100% = reverse, ±200% = double speed. Freeze targets velocity -> 0 (pin);
    // disengage ramps back to the scan-derived velocity. The SmoothedValue ramp on
    // playheadVelocity makes engage/disengage click-free — the playhead is NEVER
    // hard-jumped (RESEARCH §4.2). `position` sets the resting point the playhead
    // eases toward when scan is ~0 (so the Position knob stays live, click-free).
    const bool  freezeActive = (freezeParam->load() > 0.5f);
    const float scanFrac     = scanParam->load() / 100.0f;         // [-2 .. +2]
    const float srcLenF      = (float) juce::jmax (1, srcLen);

    scanSmoothed.setTargetValue     (scanFrac);
    positionSmoothed.setTargetValue (positionParam->load());

    // Capture the BLOCK-START playhead — this is the snapshot the voices read at
    // spawn (keeps the 2.1 voice spawn signature unchanged; Sequencing Note 3).
    // Stays double end-to-end (IN-04) — no float re-quantization on long sources.
    const double playheadAtBlockStart = playheadPos;

    // Advance the global playhead across the block (per sample) for read-head
    // correctness + the Stage-3 waveform-playhead line. Voices snapshot the
    // block-start value above; the per-sample motion keeps the playhead coherent
    // block-to-block (and feeds the live playhead position to 2.3's viz tap).
    constexpr float kRestEpsilon = 1.0e-4f;          // |velocity| below this = "at rest"
    const double restEase = restEaseCoeff;           // τ-derived in prepareToPlay (IN-03)
    for (int i = 0; i < numSamples; ++i)
    {
        const float scanNow = scanSmoothed.getNextValue();
        const float restPct = positionSmoothed.getNextValue();

        // Freeze pins the target velocity to 0; otherwise it is the scan velocity.
        playheadVelocity.setTargetValue (freezeActive ? 0.0f : scanNow);
        const float vel = playheadVelocity.getNextValue();

        playheadPos += (double) vel;

        // When effectively at rest (scan ~0, not mid-ramp) and NOT frozen, ease
        // toward the resting point so the Position knob stays responsive without a
        // hard jump. Under freeze the playhead holds wherever it was pinned.
        if (! freezeActive && std::abs (vel) < kRestEpsilon)
        {
            const double restTarget = (double) (restPct / 100.0f) * (double) srcLenF;
            playheadPos += (restTarget - playheadPos) * restEase;
        }

        // Wrap to [0, srcLen) for BOTH directions (negative scan = reverse).
        if (playheadPos >= (double) srcLenF) playheadPos -= (double) srcLenF;
        if (playheadPos < 0.0)               playheadPos += (double) srcLenF;
    }

    // --- Grain-event viz frame (Phase 2.3): one frame per block --------------
    // Reset count=0 here; each voice appends a GrainEvent at spawn (bounded by
    // kMaxEvents — extras dropped). Filled below + published after render.
    GrainCloudFrame& frame = grainCloudBuffer.getWriteBuffer();
    frame.count = 0;

    for (auto* gv : grainVoices)                         // cached typed pointers (IN-02)
    {
        gv->setParams (p);
        gv->setSource (srcPtr, srcLen);
        gv->setPlayhead (playheadAtBlockStart);
        gv->setGrainCloudFrame  (&frame);            // viz: grain events
        gv->setActiveGrainCount (&activeGrainCount); // viz: live grain count (UI-05)
    }

    // --- Render the grain voices ---------------------------------------------
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // Detach the per-block viz pointers so a stale frame is never written between
    // blocks (the next block re-sets them before render).
    for (auto* gv : grainVoices)
        gv->setGrainCloudFrame (nullptr);

    // --- Fill the global read-head fields + publish the grain frame ----------
    // (playhead/position/spray/freeze are the read-head context for the cloud
    // scatter (UI-01) + waveform playheads (UI-02).)
    frame.playheadNorm      = (srcLen > 0)
                            ? juce::jlimit (0.0f, 1.0f, (float) (playheadAtBlockStart / (double) srcLenF)) : 0.0f;
    frame.positionNorm      = juce::jlimit (0.0f, 1.0f, positionParam->load() / 100.0f);
    frame.positionSprayNorm = juce::jlimit (0.0f, 1.0f, positionSprayParam->load() / 100.0f);
    frame.frozen            = freezeActive;
    grainCloudBuffer.publish();

    // --- Master output trim (dB->lin, smoothed) + overlap-aware normalization -
    // A single grain peaks near the source level; overlapping grains pile up as
    // the cloud densifies. The *fixed* headroom (was 0.5 = −6 dB) sized for dense
    // clouds left sparse/single-grain patches — i.e. ordinary playing at the
    // default density — far too quiet. Normalize by the square root of the live
    // overlap (grainSize × density): incoherent grains sum in power, so amplitude
    // grows ~√overlap. This keeps dense clouds near the old level while lifting
    // sparse patches by up to +6 dB; clamped so it only ever attenuates (never
    // amplifies above unity). The SmoothedValue ramp on outputGain keeps the gain
    // change click-free as density moves. (Per-voice velToDensity modulation is
    // ignored here — this is a coarse global trim, not a limiter.)
    const float overlap  = (grainSizeParam->load() * 0.001f) * densityParam->load();
    const float normGain = 1.0f / std::sqrt (juce::jmax (1.0f, overlap));
    const float outDb = outputLevelParam->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f) * normGain);
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

    // --- Output scope/spectrum tap (Phase 2.3, UI-04): post-gain mono sum into
    // the lock-free ring (copy-only, NO alloc, NO FFT — the FFT is the Stage-3
    // editor Timer's job). Mirror O-simpleFM PluginProcessor.cpp:317-340: a stack
    // mono[] in <= 4096 chunks (no audio-thread allocation).
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

    // Block-completion fence for the retired-source reap (WR-05): a publish that
    // parked a buffer at count C may free it once >= C+2 — by then this (and any
    // other) block that could have loaded the old pointer has finished.
    blocksRendered.fetch_add (1, std::memory_order_release);
}

//==============================================================================
// Concept-preset tour (Stage 3.3 / FUNC-06). Eight factory snapshots, each
// isolating ONE granular concept. Every preset resets to defaults first (so it
// reproduces regardless of prior state), then applies its named snapshot via
// setValueNotifyingHost — real (scaled) floats through convertTo0to1, choices /
// bool through their index. The WebView relays/attachments propagate every change
// back to the page (no DOM poking). Mirrors O-simpleAdditive::applyFactoryPreset.
//
// Stored-range reminders (parameter-spec.md): position / *Spray / scatter /
// velToDensity are 0–100; grainSize 2–500 ms; density 1–200 g/s; scan ±200 %;
// grainPitch ±24 st; pitchSpray 0–12 st; ampSustain stored 0–1 (NOT %);
// ADSR times in seconds. Choice indices — sourceSample: 0=fire 1=voice 2=water
// 3=piano; windowShape: 0=Rect 1=Tri 2=Welch 3=Gauss 4=Hann 5=Tukey.
void OSimpleGrainAudioProcessor::applyFactoryPreset (const juce::String& name)
{
    using namespace OSimpleGrain::ParamIDs;

    // 1. Reset every parameter to its default EXCEPT sourceSample — a clean
    //    slate per concept, applied to whatever source is loaded (WR-03,
    //    v1.1.1). Previously the reset put sourceSample back to fire, so a
    //    lesson button silently discarded a user-loaded source whenever the
    //    prior choice happened to differ — and kept it when it didn't:
    //    non-deterministic from the user's point of view. Contract now:
    //    presets keep the current source; "Granular Fire" alone force-loads
    //    fire (below), honouring its name.
    // Every write is wrapped in a begin/endChangeGesture pair (IN-06, v1.1.2):
    // hosts recording automation log ungestured setValueNotifyingHost calls
    // oddly (strict hosts warn). A preset press is a user gesture — mark it so.
    auto setGestured = [] (juce::AudioProcessorParameter* p, float norm) {
        p->beginChangeGesture();
        p->setValueNotifyingHost (norm);
        p->endChangeGesture();
    };

    auto* sourceParam = apvts.getParameter (sourceSample);
    for (auto* p : getParameters())
        if (p != sourceParam)
            setGestured (p, p->getDefaultValue());

    // 2. Snapshot helpers (scaled → normalised; choices/bool by index).
    auto setReal = [this, &setGestured] (const char* id, float real) {
        if (auto* p = apvts.getParameter (id))
            setGestured (p, p->convertTo0to1 (real));
    };
    auto setChoice = [this, &setGestured] (const char* id, int index) {
        if (auto* p = apvts.getParameter (id))
            setGestured (p, p->convertTo0to1 ((float) index));
    };
    auto setBool = [this, &setGestured] (const char* id, bool on) {
        if (auto* p = apvts.getParameter (id))
            setGestured (p, on ? 1.0f : 0.0f);
    };

    // ── Single Grain — isolate ONE grain: a long grain fired once per "tick".
    //    grainSize huge, density at the floor → the ear hears separated grains,
    //    not a stream. The atom of granular synthesis.
    if (name == "Single Grain")
    {
        setReal   (grainSize, 60.0f);
        setReal   (density,   1.0f);          // floor → fully separated grains
        setChoice (windowShape, 4);           // Hann — clean, click-free grain
        setReal   (ampAttack, 0.01f);
        setReal   (ampRelease, 0.6f);
    }
    // ── Pitched Buzz — fast, perfectly synchronous, tiny grains. The grain rate
    //    itself becomes an audible pitch (the comb), scatter=0 keeps it discrete.
    else if (name == "Pitched Buzz")
    {
        setReal   (grainSize, 6.0f);          // very short
        setReal   (density,   180.0f);        // fast → grain rate = pitch
        setReal   (scatter,   0.0f);          // synchronous → discrete sidebands
        setChoice (windowShape, 4);           // Hann
        setReal   (ampSustain, 1.0f);
    }
    // ── Fragments — medium grains, sparse. You still recognise chunks of the
    //    source — the in-between of single-grain and a smooth cloud.
    else if (name == "Fragments")
    {
        setReal   (grainSize, 90.0f);         // recognisable source chunks
        setReal   (density,   8.0f);
        setReal   (positionSpray, 25.0f);     // scatter the reads a little
        setChoice (windowShape, 4);           // Hann
        setReal   (ampSustain, 1.0f);
    }
    // ── Smooth Cloud — many overlapping Hann grains fuse into one continuous,
    //    glassy texture. Overlap-add in action (grainSize × density ≫ 1).
    else if (name == "Smooth Cloud")
    {
        setReal   (grainSize, 50.0f);
        setReal   (density,   140.0f);        // dense overlap → fused, continuous
        setChoice (windowShape, 4);           // Hann — smooth crossfades
        setReal   (positionSpray, 12.0f);
        setReal   (ampAttack, 0.3f);
        setReal   (ampSustain, 1.0f);
        setReal   (ampRelease, 0.8f);
    }
    // ── Frozen Pad — freeze pins the read head; pitchSpray shimmers it into a
    //    sustained, evolving pad that never moves through the source.
    else if (name == "Frozen Pad")
    {
        setBool   (freeze,    true);          // pin the read head on one instant
        setReal   (density,   90.0f);
        setReal   (grainSize, 55.0f);
        setReal   (pitchSpray, 4.0f);         // shimmer the frozen instant
        setReal   (panSpray,  60.0f);         // widen it
        setChoice (windowShape, 4);
        setReal   (ampAttack, 0.5f);
        setReal   (ampSustain, 1.0f);
        setReal   (ampRelease, 1.5f);
    }
    // ── Asynchronous Cloud — high scatter randomises the grain period; the comb
    //    dissolves and the spectrum smears toward broadband noise (sync→async).
    else if (name == "Asynchronous Cloud")
    {
        setReal   (scatter,   100.0f);        // fully asynchronous → noisy
        setReal   (density,   130.0f);
        setReal   (grainSize, 40.0f);
        setReal   (positionSpray, 40.0f);
        setChoice (windowShape, 4);
        setReal   (ampSustain, 1.0f);
    }
    // ── Granular Fire — the worked example on the crackling-fire source: a lively
    //    grain/spray set that turns a field recording into a moving granular bed.
    //    The ONE preset that replaces the loaded source: it force-loads fire
    //    directly (WR-03) — setChoice alone is suppressed by the APVTS when the
    //    choice already reads "fire" (e.g. a dropped source is active but the
    //    param never moved), which shipped a "Granular Fire" that didn't load fire.
    else if (name == "Granular Fire")
    {
        setChoice (sourceSample, 0);          // keep the param in sync (may be a same-value no-op)
        loadBuiltInSource (0, currentSampleRate.load (std::memory_order_relaxed));
        setReal   (grainSize, 35.0f);
        setReal   (density,   110.0f);
        setReal   (position,  30.0f);
        setReal   (scan,      40.0f);         // slowly travel through the recording
        setReal   (positionSpray, 30.0f);
        setReal   (pitchSpray, 2.0f);
        setReal   (panSpray,  45.0f);
        setChoice (windowShape, 4);
        setReal   (ampSustain, 1.0f);
    }
    // ── Rect Click — the intentional teaching artifact: a rectangular window has
    //    no fade, so every grain edge CLICKS. Sparse + medium so each click is
    //    audible on its own (a feature, not a bug).
    else if (name == "Rect Click")
    {
        setChoice (windowShape, 0);           // Rectangular → hard edges = clicks
        setReal   (grainSize, 45.0f);
        setReal   (density,   6.0f);          // sparse → each click stands alone
        setReal   (ampSustain, 1.0f);
    }
    // Unknown name → leave at defaults (already reset above).
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleGrainAudioProcessor::createEditor()
{
    return new OSimpleGrainAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleGrainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Serialize the APVTS tree PLUS a custom child holding the loaded-source
    // identity, so a session restores both the params and the active source.
    // Hosts may call this from arbitrary threads (VST3 autosave) — the identity
    // read goes through the lock (CR-02).
    auto state = apvts.copyState();

    auto sourceChild = state.getOrCreateChildWithName (
        juce::Identifier (kSourceStateTag), nullptr);
    sourceChild.setProperty (juce::Identifier (kSourceIdProp),
                             getSourceIdentity(), nullptr);

    // v1.3.0: the interface LANGUAGE rides the same tree as a plain property on
    // the ROOT, not a parameter and not a child of the source node — it has
    // nothing to do with which sound is loaded. Written as a STRING ("en"/"fr")
    // rather than the atomic's int index, so a hand-inspected session file says
    // what it means — and because the ValueTree -> XML round-trip rebuilds every
    // property as a string var anyway
    // (critical_valuetree_xml_roundtrip_loses_type). Storing the code means the
    // value that comes back is the value that went in, with no type predicate to
    // misfire on the way.
    state.setProperty (kUiLanguageProp,
                       languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleGrainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || state.getType() != apvts.state.getType())
        return;

    // Restore the custom loaded-source identity (if present) before handing the
    // tree to the APVTS. Default stays "embedded:fire" when absent (legacy state).
    auto sourceChild = state.getChildWithName (juce::Identifier (kSourceStateTag));
    if (sourceChild.isValid())
        setSourceIdentity (sourceChild.getProperty (
            juce::Identifier (kSourceIdProp), getSourceIdentity()).toString());

    // v1.3.0. A pre-1.3.0 session has no property, getProperty returns a VOID
    // var, and the default (English) stands. isVoid() is the only correct gate —
    // the value comes back as a STRING var, so a type predicate like isInt()
    // would never fire. languageIndex() clamps anything that is not "fr" to 0,
    // so a hand-edited value degrades to English rather than to a bad index.
    // Read from the INCOMING tree, before replaceState below: apvts.state is
    // still the old tree at this point.
    const juce::var lang = state.getProperty (kUiLanguageProp);
    if (! lang.isVoid())
        uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

    // replaceState() fires the sourceSample listener, which queues an AsyncUpdater
    // to rebuild a built-in source. That update is deferred (it runs AFTER this
    // method returns), so a synchronous "suppress" flag can't gate it. Instead we
    // publish the correct source below, then cancelPendingUpdate() to drop the
    // queued rebuild — otherwise a restored user/dropped file would be clobbered by
    // the built-in choice a moment later.
    apvts.replaceState (state);

    // Re-decode the active source at the current engine rate so the restored
    // session sounds the same. OFF the audio thread (setStateInformation runs
    // off the audio thread; hosts may use a non-message thread — identity access
    // stays behind the lock, CR-02). prepareToPlay also does this — but a host
    // can restore state AFTER prepareToPlay, so reload here too.
    const double engineRate = currentSampleRate.load (std::memory_order_relaxed);
    if (engineRate > 0.0)
    {
        const juce::String identity = getSourceIdentity();
        if (identity.startsWith ("embedded:"))
        {
            loadBuiltInSource (builtInIndexForIdentity (identity), engineRate);
        }
        else
        {
            bool ok = false;
            if (identity.startsWith ("dropped:"))
            {
                // Name-only identity: the bytes are not persisted. Same-instance
                // restore (host undo / A-B) may still hold them — reuse; a fresh
                // instance falls back below (documented, with a UI notice). The
                // guard also keeps juce::File() away from a non-path string,
                // which jasserts in debug hosts (CR-01).
                const juce::ScopedLock sl (sourceStateLock);
                if (userSourceBytes.getSize() > 0 && userSourceBytesIdentity == identity)
                    ok = decodeAndPublish (userSourceBytes.getData(), userSourceBytes.getSize(),
                                           engineRate, identity);
            }
            else if (juce::File::isAbsolutePath (identity))
            {
                const juce::File f (identity);
                if (f.existsAsFile())
                {
                    juce::MemoryBlock mb;
                    ok = f.loadFileAsData (mb)
                       && decodeAndPublish (mb.getData(), mb.getSize(), engineRate, identity);
                }
            }
            if (! ok)
            {
                // Missing user file -> fall back to the default built-in (notice in
                // Stage 3 via the identity reverting to embedded:fire).
                setSourceIdentity ("embedded:fire");
                loadBuiltInSource (0, engineRate);
            }
        }
    }

    // Drop the sourceSample-rebuild that replaceState() queued above (see the note
    // before replaceState) — the restored source is already published, so letting
    // the pending built-in load run would only clobber it.
    cancelPendingUpdate();
    pendingBuiltInIndex.store (-1, std::memory_order_relaxed);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleGrainAudioProcessor();
}
