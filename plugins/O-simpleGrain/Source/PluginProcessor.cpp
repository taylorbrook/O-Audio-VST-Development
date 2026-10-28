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

    // v1.8.0 (research 2026 update §5 item 6, Tempera 2.3). Both default to the
    // v1.7.0 sound: Skip 0 drops nothing, Tilt 0 is the symmetric window.
    // Skip: probability that each scheduled grain is dropped (the clock keeps
    // running, so it thins the pattern into a rhythm rather than slowing it).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { grainSkip, 1 }, "Grain Skip",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    // Tilt: where the window peaks. −100 = plucked (fast attack, long fall),
    // +100 = reverse swell (long rise, quick cut), 0 = symmetric. Bipolar.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { grainTilt, 1 }, "Grain Tilt",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.01f }, 0.0f,
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
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, -6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleGrainAudioProcessor::OSimpleGrainAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      sampleLibrary (defaultLibraryFolder())
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
    grainSkipParam     = apvts.getRawParameterValue (grainSkip);
    grainTiltParam     = apvts.getRawParameterValue (grainTilt);
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
    rngSeed.store ((juce::uint64) juce::Random::getSystemRandom().nextInt64(), std::memory_order_relaxed);
    activeSeed = rngSeed.load (std::memory_order_relaxed);

    for (int i = 0; i < kMaxVoices; ++i)
    {
        auto* v = new GrainVoice();
        v->setWindowLuts (&windowLuts);
        v->setRngSource (&activeSeed, &noteOnCounter);
        synth.addVoice (v);
        grainVoices[(size_t) i] = v;
    }

    synth.addSound (new GrainSound());           // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);         // steal quietest/oldest voice on overflow
    // v1.6.0: sample-exact note events. The default (32, non-strict) moved an
    // event less than 32 samples after the previous one to that sub-block's start,
    // and where sub-blocks start depends on the host block size.
    synth.setMinimumRenderingSubdivisionSize (1, true);

    // Listen for sourceSample selection changes (Stage 2.3). The decode/resample
    // is dispatched to the message thread via AsyncUpdater (never the audio
    // thread) — see parameterChanged / handleAsyncUpdate.
    apvts.addParameterListener (sourceSample, this);

    // v1.9.0 — user presets through the preset-manager module. The sample is NOT
    // a preset parameter (it lives in the library and rides customState as a
    // reference); Output Level is user-owned (v1.5.2). No factory presets are
    // written: the eight concept lessons stay in applyFactoryPreset(), and no
    // file I/O happens at construction (auval/pluginval scans).
    presetManager.setExcludedParameters ({ sourceSample, outputLevel });
    presetManager.setCustomStateCallbacks (
        [this]() { return captureSampleReference(); },
        [this] (const juce::var& custom) { applySampleReference (custom); });
    presetManager.setCurrentPresetName ({});
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

    // v1.6.0: per-sample playhead track (never resized on the audio thread), and
    // a fresh note numbering so a bounce from here repeats the same grains.
    playheadTrack.assign ((size_t) juce::jmax (samplesPerBlock, kMinPlayheadTrack), 0.0);
    activeSeed    = rngSeed.load (std::memory_order_relaxed);
    noteOnCounter = 0;

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
        // User source (dropped or picker). v1.5.6: one ladder for both — the
        // retained copy is the same audio either way, so a rate change re-decodes
        // it rather than re-reading (and re-encoding) the picker file every time.
        {
            const juce::ScopedLock sl (sourceStateLock);
            if (currentSource != nullptr && publishedSourceRate == sampleRate)
            {
                loadedUserFile = true;               // already at the engine rate — keep as-is
            }
            else if (userSourceBytes.getSize() > 0 && userSourceBytesIdentity == identity)
            {
                // Rate changed and the copy is retained — re-decode.
                loadedUserFile = decodeAndPublish (userSourceBytes.getData(),
                                                   userSourceBytes.getSize(),
                                                   sampleRate, identity);
            }
        }

        // No retained copy: a picker file can still be re-read from its path.
        // The isAbsolutePath guard keeps juce::File away from a non-path string
        // (jasserts in debug hosts).
        if (! loadedUserFile && ! identity.startsWith ("dropped:")
            && juce::File::isAbsolutePath (identity))
        {
            const juce::File f (identity);
            if (f.existsAsFile())
            {
                juce::MemoryBlock mb;
                if (f.loadFileAsData (mb)
                    && decodeAndPublish (mb.getData(), mb.getSize(), sampleRate, identity))
                    loadedUserFile = true;
            }
        }

        if (! loadedUserFile)
        {
            // Nothing re-decodable: keep the live buffer (transposed on a rate
            // change) rather than discard the user's sound.
            const juce::ScopedLock sl (sourceStateLock);
            if (currentSource != nullptr)
                loadedUserFile = true;
        }

        if (! loadedUserFile)
            fallBackToDefaultSource (identity, sampleRate);   // nothing realisable at all
    }

    // Reap any retired buffers that are now provably unreferenced (the host
    // guarantees prepareToPlay never runs concurrently with processBlock).
    {
        const juce::ScopedLock sl (sourceStateLock);
        reapRetiredSources();
    }

    // Now the source length is known — seed the playhead at the resting point so
    // the first block starts where `position` points (no jump-from-zero glide).
    std::shared_ptr<const GrainSource> src;
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
void OSimpleGrainAudioProcessor::publishSource (std::shared_ptr<const GrainSource> newSource)
{
    reapRetiredSources();

    if (currentSource != nullptr)
        retiredSources.push_back ({ currentSource,
                                    blocksRendered.load (std::memory_order_acquire) });

    currentSource = std::move (newSource);
    // seq_cst (v1.6.0): pairs with processBlock's blocksEntered increment + view
    // load for the quiescence reap — store/load ordering acquire/release can't give.
    audioSourceView.store (currentSource.get(), std::memory_order_seq_cst);
}

// Free retired buffers once >= 2 audio blocks have completed since parking (any
// block that could have loaded the old raw pointer has finished by then), or
// when no block is in flight at all (v1.6.0 — the +2 rule never fires while the
// host has stopped calling processBlock). Exits are read FIRST: entries >= exits
// always, so equality means nothing was in flight when entries was read, and a
// block entering later loads the already-published new pointer.
void OSimpleGrainAudioProcessor::reapRetiredSources()
{
    const juce::uint64 now     = blocksRendered.load (std::memory_order_seq_cst);
    const juce::uint64 entries = blocksEntered.load  (std::memory_order_seq_cst);
    const bool quiescent = (entries == now);
    retiredSources.erase (
        std::remove_if (retiredSources.begin(), retiredSources.end(),
                        [now, quiescent] (const RetiredSource& r)
                        { return quiescent || now >= r.parkedAtBlock + 2; }),
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
// cap, then builds the band-limited mipmap (v1.7.0; tens of ms for 10 s — this
// runs on the decoding thread, never the audio thread). Returns a fully-built
// source ready to publish. `truncated` is set when the source exceeded the cap
// (the UI surfaces a notice in Stage 3).
std::shared_ptr<const GrainSource>
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

    auto out = std::make_shared<GrainSource>();
    out->buffer.setSize (nCh, numOut);
    out->buffer.clear();
    for (int ch = 0; ch < nCh; ++ch)
    {
        juce::LagrangeInterpolator interp;       // streaming one-shot — correct for a whole-buffer resample
        interp.reset();
        interp.process (ratio, src.getReadPointer (ch), out->buffer.getWritePointer (ch), numOut);
    }
    out->build();
    return out;
}

// v1.5.6: encode a decoded user source (at its own rate) as a complete audio file
// in memory, for the session copy. Integer sources -> FLAC at 16 or 24 bit
// (lossless for 8/16/24-bit files); float sources -> 32-bit float WAV, so peaks
// over 0 dBFS survive (FLAC is integer-only and would clip them). FLAC also tops
// out at 8 channels. Returns an empty block if the writer refuses.
static juce::MemoryBlock encodeSessionCopy (const juce::AudioBuffer<float>& buf, double rate,
                                            bool isFloat, int srcBits)
{
    using SampleFormat = juce::AudioFormatWriterOptions::SampleFormat;
    const bool useFlac = ! isFloat && buf.getNumChannels() <= 8;

    std::unique_ptr<juce::AudioFormat> format;
    if (useFlac) format = std::make_unique<juce::FlacAudioFormat>();
    else         format = std::make_unique<juce::WavAudioFormat>();

    const auto options = juce::AudioFormatWriterOptions{}
                             .withSampleRate (rate)
                             .withNumChannels (buf.getNumChannels())
                             .withBitsPerSample (useFlac ? (srcBits <= 16 ? 16 : 24) : 32)
                             .withSampleFormat (useFlac ? SampleFormat::integral
                                                        : SampleFormat::floatingPoint);

    juce::MemoryBlock out;
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (out, false);
    auto writer = format->createWriterFor (stream, options);
    if (writer == nullptr)
        return {};

    const bool ok = writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
    writer.reset();                      // finalises the header and trims `out` to size
    return ok ? out : juce::MemoryBlock{};
}

juce::String OSimpleGrainAudioProcessor::displayNameForIdentity (const juce::String& identity)
{
    if (identity.startsWith ("dropped:"))
        return identity.fromFirstOccurrenceOf ("dropped:", false, false);
    if (juce::File::isAbsolutePath (identity))      // guard: juce::File(non-path) jasserts
        return juce::File (identity).getFileName();
    return {};
}

juce::String OSimpleGrainAudioProcessor::getUserSourceName() const
{
    return displayNameForIdentity (getSourceIdentity());
}

juce::String OSimpleGrainAudioProcessor::takeRestoreFailedName()
{
    const juce::ScopedLock sl (sourceStateLock);
    juce::String name;
    name.swapWith (restoreFailedName);
    return name;
}

void OSimpleGrainAudioProcessor::fallBackToDefaultSource (const juce::String& lostIdentity,
                                                          double engineRate)
{
    {
        const juce::ScopedLock sl (sourceStateLock);
        restoreFailedName     = displayNameForIdentity (lostIdentity);
        currentSourceIdentity = "embedded:fire";
    }
    loadBuiltInSource (0, engineRate);
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

    // v1.5.6: a user source gets a self-contained copy (see userSourceBytes) —
    // the decoded, already-capped audio re-encoded at its own rate, so the
    // session carries ~10 s of audio rather than the whole dropped file. Skipped
    // when prepareToPlay / setStateInformation feed the retained copy back
    // through this very path. Encoded before taking the lock (FLAC encode of
    // 10 s is tens of ms).
    const bool isUserSource = ! identity.startsWith ("embedded:");
    bool needsCopy = false;
    if (isUserSource)
    {
        const juce::ScopedLock sl (sourceStateLock);
        needsCopy = (data != userSourceBytes.getData());
    }
    juce::MemoryBlock sessionCopy;
    if (needsCopy)
    {
        sessionCopy = encodeSessionCopy (tmp, srcRate, reader->usesFloatingPointData,
                                         (int) reader->bitsPerSample);
        if (sessionCopy.isEmpty() && numBytes <= kMaxRetainedSourceBytes)
            sessionCopy.append (data, numBytes);          // encoder refused: keep the raw file
    }

    {
        const juce::ScopedLock sl (sourceStateLock);
        publishSource (std::move (resampled));
        currentSourceIdentity = identity;
        publishedSourceRate   = engineRate;

        // CR-01: retain the copy so a later prepareToPlay at a NEW engine rate can
        // re-decode instead of clobbering, and getStateInformation can embed it.
        // Cap so a pathological source can't pin RAM (over the cap the live buffer
        // is kept, transposed, on a rate change). Built-ins release any stale copy.
        if (isUserSource)
        {
            restoreFailedName.clear();                   // a user sound is live again
            if (needsCopy)
            {
                if (sessionCopy.getSize() > 0 && sessionCopy.getSize() <= kMaxRetainedSourceBytes)
                {
                    userSourceBytes.swapWith (sessionCopy);
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

    std::shared_ptr<const GrainSource> src;   // snapshot — held for this call
    {
        const juce::ScopedLock sl (sourceStateLock);
        src = currentSource;
    }
    if (src == nullptr || src->getNumSamples() <= 0)
        return {};

    const float* data = src->buffer.getReadPointer (0);    // mono read (channel 0), matches the engine
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
    if (ok)
        addCurrentSourceToLibrary();   // v1.9.0 — every dropped sound joins the library
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
        if (f.loadFileAsData (mb)
            && decodeAndPublish (mb.getData(), mb.getSize(),
                                 currentSampleRate.load (std::memory_order_relaxed),
                                 f.getFullPathName()))   // identity = path (re-readable on restore)
            addCurrentSourceToLibrary();   // v1.9.0 — and joins the library
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
    // Reap fence, entry side (v1.6.0): FIRST, before the source view is loaded.
    // Every path out of processBlock must reach the blocksRendered increment.
    blocksEntered.fetch_add (1, std::memory_order_seq_cst);

    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // grain voices ADD into a cleared buffer

    // Seed restored / changed since the last block: restart the note numbering.
    if (const auto seed = rngSeed.load (std::memory_order_relaxed); seed != activeSeed)
    {
        activeSeed    = seed;
        noteOnCounter = 0;
    }

    // Merge any on-screen-keyboard notes into the host MIDI stream before the
    // grain voices render below (the WebView keyboard injects via handleUiMidi).
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    // --- Snapshot the source view ONCE (valid for the whole block) ------------
    // Wait-free (WR-05): a single atomic raw-pointer load — no shared_ptr
    // ref-count, no lock. The pointee is immutable and stays alive because a
    // publish PARKS the outgoing buffer until ≥2 blocks after the swap (see
    // publishSource/reapRetiredSources); blocksRendered below is the fence.
    const GrainSource* src = audioSourceView.load (std::memory_order_seq_cst);
    const float* srcPtr = nullptr;
    const GrainSource::Snapshot* srcMip = nullptr;   // v1.7.0 — read by rate > 1 grains
    int          srcLen = 0;
    if (src != nullptr && src->getNumSamples() > 0)
    {
        srcPtr = src->buffer.getReadPointer (0); // mono read in 2.1 (channel 0)
        srcMip = &src->mip;
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
    p.grainSkip      = juce::jlimit (0.0f, 1.0f, grainSkipParam->load() * 0.01f);    // v1.8.0
    p.grainTilt      = juce::jlimit (-1.0f, 1.0f, grainTiltParam->load() * 0.01f);   // v1.8.0
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

    // Block-start playhead for the viz frame (UI-02). Stays double end-to-end
    // (IN-04) — no float re-quantization on long sources.
    const double playheadAtBlockStart = playheadPos;

    // Advance the global playhead across the block (per sample), recording each
    // sample's value in playheadTrack: a grain spawned on sample i reads from
    // track[i] (v1.6.0 — was the block-start value, so the block size moved grains).
    constexpr float kRestEpsilon = 1.0e-4f;          // |velocity| below this = "at rest"
    const double restEase = restEaseCoeff;           // τ-derived in prepareToPlay (IN-03)
    const int trackLen = juce::jmin (numSamples, (int) playheadTrack.size());
    for (int i = 0; i < numSamples; ++i)
    {
        if (i < trackLen)
            playheadTrack[(size_t) i] = playheadPos;

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
        gv->setSource (srcPtr, srcMip, srcLen);
        gv->setPlayheadTrack (playheadTrack.data(), trackLen);
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
    //
    // v1.5.5: two corrections. (1) The overlap respects kMaxGrainsPerVoice —
    // past the cap only 24 grains can sound, each cut short by the steal, so the
    // uncapped figure attenuated far too much (500 ms × 200 g/s: 14.7 dB too
    // low). (2) Overlap is weighted by the window's power μ₂ relative to Hann,
    // so switching windows no longer changes the cloud level (rect was +4.2 dB
    // over Hann). Hann's ratio is exactly 1, so
    // Hann patches under the cap are bit-identical to v1.5.4.
    const int   shapeNow  = juce::jlimit (0, WindowLuts::kNumShapes - 1, p.windowShape);
    const float sizeSamp  = grainSizeParam->load() * 0.001f
                          * (float) currentSampleRate.load (std::memory_order_relaxed);
    const float taperEnd  = WindowLuts::taperEndFor (shapeNow, p.windowTaper, sizeSamp,
                                                     currentSampleRate.load (std::memory_order_relaxed));
    const bool  usesTaper = (shapeNow == WindowLuts::kShapeRect || shapeNow == WindowLuts::kShapeTukey);
    // v1.8.0 — the tilted window's power, the same way the voices build it. For
    // the table shapes the warp keeps μ₂ (ratio unchanged to ~1e-3); for rect /
    // Tukey the floored tapers move it a little, and past the cap the stolen
    // grains' LEADING part is what sounds, which tilt changes a lot (a plucked
    // grain keeps its peak, a swell loses it) — so tilt joins the cache key.
    const bool tiltedNow = (p.grainTilt != 0.0f);
    const auto tiltNow   = WindowLuts::tiltFor (shapeNow, p.grainTilt, taperEnd, juce::jmax (2.0f, sizeSamp),
                                                currentSampleRate.load (std::memory_order_relaxed));
    if (shapeNow != normShapeCache || (usesTaper && taperEnd != normTaperEndCache)
        || tiltedNow != normTiltedCache
        || (tiltedNow && (tiltNow.peak != normTiltPeakCache || tiltNow.teA != normTiltTeACache
                          || tiltNow.teR != normTiltTeRCache)))
    {
        normShapeCache    = shapeNow;
        normTaperEndCache = taperEnd;
        normTiltedCache   = tiltedNow;
        normTiltPeakCache = tiltNow.peak;
        normTiltTeACache  = tiltNow.teA;
        normTiltTeRCache  = tiltNow.teR;
        normPowerRatio    = tiltedNow
                          ? windowLuts.meanSquareTilted (shapeNow, tiltNow)
                              / windowLuts.meanSquare (WindowLuts::kShapeHann)
                          : (shapeNow == WindowLuts::kShapeHann)
                          ? 1.0f
                          : windowLuts.meanSquare (shapeNow, taperEnd)
                              / windowLuts.meanSquare (WindowLuts::kShapeHann);
    }
    // Past the cap, steal-oldest keeps exactly kMaxGrainsPerVoice grains live and
    // each one only reaches phase cap/overlap before it is stolen, so the power
    // is the cap × the mean w² over that leading part of the window.
    // v1.8.0: Skip thins the grains that actually start, so the overlap counts
    // only the (1 − skip) share that sounds (at Skip 0 the term is skipped, so
    // the figure is bit-identical to v1.7.0).
    float rawOverlap = (grainSizeParam->load() * 0.001f) * densityParam->load();
    if (p.grainSkip > 0.0f)
        rawOverlap *= (1.0f - p.grainSkip);
    const float overlap = (rawOverlap <= (float) kMaxGrainsPerVoice)
        ? rawOverlap * normPowerRatio
        : (float) kMaxGrainsPerVoice
            * (tiltedNow ? windowLuts.meanSquareTilted (shapeNow, tiltNow, (float) kMaxGrainsPerVoice / rawOverlap)
                         : windowLuts.meanSquare (shapeNow, taperEnd, (float) kMaxGrainsPerVoice / rawOverlap))
            / windowLuts.meanSquare (WindowLuts::kShapeHann);
    const float normGain = 1.0f / std::sqrt (juce::jmax (1.0f, overlap));
    const float outDb = outputLevelParam->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f) * normGain);
    // v1.6.0: per sample while ramping. applyGainRamp between per-block skip()
    // endpoints interpolated straight across a ramp that ended mid-block, so the
    // gain near a note depended on the host block size.
    if (outputGain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outputGain.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[i] *= g;
        }
    }
    else
    {
        buffer.applyGain (outputGain.getCurrentValue());
    }

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

    // v1.9.0: a lesson is not a user preset — the preset bar stops naming one.
    {
        const juce::ScopedLock pl (presetLock);
        presetManager.setCurrentPresetName ({});
    }

    // v1.5.2: Output Level is USER-OWNED — skipped here, and no lesson below
    // names it, so loading a preset leaves the level where the user put it.
    auto* sourceParam = apvts.getParameter (sourceSample);
    for (auto* p : getParameters())
    {
        if (p == sourceParam)
            continue;
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (isUserOwnedParameter (ranged->getParameterID()))
                continue;
        setGestured (p, p->getDefaultValue());
    }

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
// v1.9.0 — USER PRESETS + SAMPLE LIBRARY (see the header note).
//==============================================================================
juce::File OSimpleGrainAudioProcessor::defaultLibraryFolder()
{
    // Sibling of the preset-manager's ~/Library/O-simpleGrain/Presets/ — one
    // product folder holds both, and neither lives inside the other.
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
        .getChildFile ("Library")
        .getChildFile ("O-simpleGrain")
        .getChildFile ("Samples");
}

juce::String OSimpleGrainAudioProcessor::extensionForAudioBytes (const juce::MemoryBlock& bytes)
{
    if (bytes.getSize() >= 4)
    {
        const auto* b = static_cast<const char*> (bytes.getData());
        if (std::memcmp (b, "fLaC", 4) == 0) return ".flac";
        if (std::memcmp (b, "FORM", 4) == 0) return ".aiff";
    }
    return ".wav";   // RIFF/RF64 (the float session copy), or anything else WAV-shaped
}

juce::String OSimpleGrainAudioProcessor::currentLibraryEntryName()
{
    const juce::String identity = getSourceIdentity();
    if (identity.startsWith ("embedded:"))
        return {};

    // Loaded FROM the library: the identity is the entry's own path.
    if (juce::File::isAbsolutePath (identity))
    {
        const juce::File f (identity);
        if (sampleLibrary.contains (f) && f.existsAsFile())
            return f.getFileName();
    }

    // Copied INTO the library on drop/pick (identity stays dropped:/path).
    juce::String name;
    {
        const juce::ScopedLock sl (sourceStateLock);
        if (libraryEntryIdentity == identity)
            name = libraryEntryName;
    }
    if (name.isNotEmpty() && sampleLibrary.fileFor (name) == juce::File{})
        name.clear();   // removed from the library since
    return name;
}

juce::String OSimpleGrainAudioProcessor::addCurrentSourceToLibrary()
{
    juce::String identity;
    juce::MemoryBlock bytes;
    {
        const juce::ScopedLock sl (sourceStateLock);
        identity = currentSourceIdentity;
        if (identity.startsWith ("embedded:") || userSourceBytesIdentity != identity)
            return {};
        bytes = userSourceBytes;   // copy out; the file write happens outside the lock
    }

    // Already a library file? Nothing to add.
    if (juce::File::isAbsolutePath (identity) && sampleLibrary.contains (juce::File (identity)))
        return juce::File (identity).getFileName();

    const auto entry = sampleLibrary.add (bytes, displayNameForIdentity (identity),
                                          extensionForAudioBytes (bytes));
    if (entry.isNotEmpty())
    {
        const juce::ScopedLock sl (sourceStateLock);
        if (currentSourceIdentity == identity)
        {
            libraryEntryIdentity = identity;
            libraryEntryName     = entry;
        }
    }
    return entry;
}

juce::var OSimpleGrainAudioProcessor::captureSampleReference()
{
    auto* sample = new juce::DynamicObject();
    const juce::String identity = getSourceIdentity();

    if (identity.startsWith ("embedded:"))
    {
        sample->setProperty ("kind", "builtin");
        sample->setProperty ("name", identity.fromFirstOccurrenceOf ("embedded:", false, false));
    }
    else
    {
        // A user sound restored from an older session (or whose library copy was
        // removed) is added now, so the preset's reference resolves on this machine.
        auto entry = currentLibraryEntryName();
        if (entry.isEmpty())
            entry = addCurrentSourceToLibrary();

        const auto file = sampleLibrary.fileFor (entry);
        if (file != juce::File{})
        {
            sample->setProperty ("kind", "library");
            sample->setProperty ("name", entry);
            sample->setProperty ("hash", SampleLibrary::hashOf (file));
            sample->setProperty ("size", juce::String (file.getSize()));   // string: int64-safe
        }
        else
        {
            sample->setProperty ("kind", "none");
            sample->setProperty ("name", displayNameForIdentity (identity));
        }
    }

    auto* custom = new juce::DynamicObject();
    custom->setProperty ("sample", juce::var (sample));
    return juce::var (custom);
}

void OSimpleGrainAudioProcessor::applySampleReference (const juce::var& customState)
{
    auto* custom = customState.getDynamicObject();
    if (custom == nullptr)
        return;                                    // no reference (void var): keep the sound
    auto* sample = custom->getProperty ("sample").getDynamicObject();
    if (sample == nullptr || getSampleLock())
        return;                                    // Sample Lock: presets never change the sound

    const juce::String kind = sample->getProperty ("kind").toString();
    const juce::String name = sample->getProperty ("name").toString();
    const double engineRate = currentSampleRate.load (std::memory_order_relaxed);

    if (kind == "builtin")
    {
        int idx = -1;
        for (int i = 0; i < kNumBuiltIns; ++i)
            if (name == kBuiltInNames[i])
                idx = i;
        if (idx < 0)
        {
            presetSampleMissing = name;
            return;
        }
        if (getSourceIdentity() == juce::String ("embedded:") + kBuiltInNames[idx])
            return;                                // already playing — no reload, no glitch

        // Keep the choice param in step, then load directly (the param write is a
        // no-op when the choice already reads this index — the WR-03 trap) and
        // drop the async rebuild the write queued.
        if (auto* p = apvts.getParameter (OSimpleGrain::ParamIDs::sourceSample))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
            p->endChangeGesture();
        }
        loadBuiltInSource (idx, engineRate);
        cancelPendingUpdate();
        pendingBuiltInIndex.store (-1, std::memory_order_relaxed);
        return;
    }

    if (kind == "library")
    {
        const juce::String hash = sample->getProperty ("hash").toString();
        const juce::int64  size = sample->getProperty ("size").toString().getLargeIntValue();

        // name + same content, then the same content under any name (renamed),
        // then the name alone (edited in place).
        juce::String entry;
        const auto byName = sampleLibrary.fileFor (name);
        if (byName != juce::File{} && byName.getSize() == size && SampleLibrary::hashOf (byName) == hash)
            entry = name;
        if (entry.isEmpty())
            entry = sampleLibrary.findByContent (size, hash);
        if (entry.isEmpty() && byName != juce::File{})
            entry = name;

        if (entry.isEmpty())
        {
            presetSampleMissing = name;
            return;
        }
        if (currentLibraryEntryName() == entry)
            return;                                // already playing
        if (! loadLibrarySample (entry))
            presetSampleMissing = name;
        return;
    }

    if (name.isNotEmpty())
        presetSampleMissing = name;                // kind "none": saved without a library copy
}

juce::var OSimpleGrainAudioProcessor::getPresetInfo()
{
    juce::StringArray list;
    juce::String current;
    {
        const juce::ScopedLock pl (presetLock);
        list    = presetManager.getPresetList();
        current = presetManager.getCurrentPresetName();
    }
    juce::Array<juce::var> arr;
    for (const auto& n : list)
        arr.add (n);
    auto* info = new juce::DynamicObject();
    info->setProperty ("presets", juce::var (arr));
    info->setProperty ("current", current);
    return juce::var (info);
}

juce::var OSimpleGrainAudioProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = rawName.trim().substring (0, 64);
    bool ok = false;
    juce::String saved;
    if (name.isNotEmpty() && ! name.startsWithChar ('.'))
    {
        const juce::ScopedLock pl (presetLock);
        ok    = presetManager.savePreset (name);   // savePreset(name) keeps the factory guard
        saved = presetManager.getCurrentPresetName();
    }
    auto* r = new juce::DynamicObject();
    r->setProperty ("ok", ok);
    r->setProperty ("name", saved);
    return juce::var (r);
}

juce::var OSimpleGrainAudioProcessor::loadUserPreset (const juce::String& name)
{
    bool ok = false;
    juce::String loaded, missing;
    {
        const juce::ScopedLock pl (presetLock);
        presetSampleMissing.clear();
        ok      = presetManager.loadPreset (name);
        loaded  = presetManager.getCurrentPresetName();
        missing = presetSampleMissing;
        presetSampleMissing.clear();
    }
    auto* r = new juce::DynamicObject();
    r->setProperty ("ok", ok);
    r->setProperty ("name", loaded);
    r->setProperty ("missing", missing);
    return juce::var (r);
}

juce::var OSimpleGrainAudioProcessor::stepUserPreset (int direction)
{
    juce::String target;
    {
        const juce::ScopedLock pl (presetLock);
        if (presetManager.getPresetList().isEmpty())
            target.clear();
        else
            target = direction < 0 ? presetManager.getPreviousPreset()
                                   : presetManager.getNextPreset();
    }
    if (target.isEmpty())
    {
        auto* r = new juce::DynamicObject();
        r->setProperty ("ok", false);
        r->setProperty ("name", juce::String());
        r->setProperty ("missing", juce::String());
        return juce::var (r);
    }
    return loadUserPreset (target);
}

bool OSimpleGrainAudioProcessor::deleteUserPreset (const juce::String& name)
{
    const juce::ScopedLock pl (presetLock);
    const bool ok = presetManager.deletePreset (name);
    if (ok && presetManager.getCurrentPresetName() == "Default")
        presetManager.setCurrentPresetName ({});   // the module's fallback name is not a preset here
    return ok;
}

juce::var OSimpleGrainAudioProcessor::getLibraryInfo()
{
    juce::Array<juce::var> arr;
    for (const auto& n : sampleLibrary.list())
        arr.add (n);
    auto* info = new juce::DynamicObject();
    info->setProperty ("entries", juce::var (arr));
    info->setProperty ("current", currentLibraryEntryName());
    return juce::var (info);
}

bool OSimpleGrainAudioProcessor::loadLibrarySample (const juce::String& entryName)
{
    const auto file = sampleLibrary.fileFor (entryName);
    if (file == juce::File{})
        return false;
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb))
        return false;
    // Identity = the library file's path: a session re-reads it from the library
    // first and falls back to its embedded copy if the entry is gone.
    return decodeAndPublish (mb.getData(), mb.getSize(),
                             currentSampleRate.load (std::memory_order_relaxed),
                             file.getFullPathName());
}

bool OSimpleGrainAudioProcessor::removeLibrarySample (const juce::String& entryName)
{
    // The live sound keeps playing from memory and stays embedded in the session.
    return sampleLibrary.remove (entryName);
}

void OSimpleGrainAudioProcessor::revealSampleLibrary()
{
    auto folder = sampleLibrary.getFolder();
    folder.createDirectory();
    folder.revealToUser();
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

    // v1.5.6: a user source also embeds its retained copy, so a reopened project
    // (fresh instance, no bytes in RAM, picker file maybe moved) gets its sound
    // back. Copy under the lock, base64 outside it.
    juce::String identity;
    juce::MemoryBlock sourceCopy;
    {
        const juce::ScopedLock sl (sourceStateLock);
        identity = currentSourceIdentity;
        if (! identity.startsWith ("embedded:") && userSourceBytesIdentity == identity)
            sourceCopy = userSourceBytes;
    }

    auto sourceChild = state.getOrCreateChildWithName (
        juce::Identifier (kSourceStateTag), nullptr);
    sourceChild.setProperty (juce::Identifier (kSourceIdProp), identity, nullptr);
    if (sourceCopy.getSize() > 0)
        sourceChild.setProperty (juce::Identifier (kSourceDataProp),
                                 juce::Base64::toBase64 (sourceCopy.getData(), sourceCopy.getSize()),
                                 nullptr);   // standard alphabet, never MemoryBlock::toBase64Encoding
    else
        sourceChild.removeProperty (juce::Identifier (kSourceDataProp), nullptr);

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

    // v1.6.0: the RNG seed, so a reopened session renders the same grains. A
    // STRING of the signed int64 (getLargeIntValue clamps above INT64_MAX).
    state.setProperty (kRngSeedProp,
                       juce::String ((juce::int64) rngSeed.load (std::memory_order_relaxed)), nullptr);

    // v1.9.0: the loaded user preset's name and Sample Lock (root properties).
    juce::String presetName;
    {
        const juce::ScopedLock pl (presetLock);
        presetName = presetManager.getCurrentPresetName();
    }
    state.setProperty (kPresetNameProp, presetName, nullptr);
    state.setProperty (kSampleLockProp, getSampleLock() ? "1" : "0", nullptr);

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
    {
        const juce::String identity = sourceChild.getProperty (
            juce::Identifier (kSourceIdProp), getSourceIdentity()).toString();
        setSourceIdentity (identity);

        // v1.5.6: the embedded copy of a user source. Taken into userSourceBytes
        // (where the restore ladder below and prepareToPlay look), then stripped
        // from the tree so apvts.state doesn't hold a second multi-MB string —
        // getStateInformation re-embeds from userSourceBytes.
        const juce::String b64 = sourceChild.getProperty (juce::Identifier (kSourceDataProp)).toString();
        sourceChild.removeProperty (juce::Identifier (kSourceDataProp), nullptr);
        if (b64.isNotEmpty() && ! identity.startsWith ("embedded:"))
        {
            juce::MemoryBlock copy;
            bool decoded = false;
            {
                juce::MemoryOutputStream mos (copy, false);   // trims `copy` on destruction
                decoded = juce::Base64::convertFromBase64 (mos, b64);
            }
            if (decoded && copy.getSize() > 0 && copy.getSize() <= kMaxRetainedSourceBytes)
            {
                const juce::ScopedLock sl (sourceStateLock);
                userSourceBytes.swapWith (copy);
                userSourceBytesIdentity = identity;
            }
        }
    }

    // v1.3.0. A pre-1.3.0 session has no property, getProperty returns a VOID
    // var, and the default (English) stands. isVoid() is the only correct gate —
    // the value comes back as a STRING var, so a type predicate like isInt()
    // would never fire. languageIndex() clamps anything unrecognised to 0,
    // so a hand-edited value degrades to English rather than to a bad index.
    // Read from the INCOMING tree, before replaceState below: apvts.state is
    // still the old tree at this point.
    const juce::var lang = state.getProperty (kUiLanguageProp);
    if (! lang.isVoid())
        uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

    // v1.6.0: a pre-1.6.0 session has no seed and keeps this instance's random one.
    const juce::String seed = state.getProperty (kRngSeedProp).toString();
    if (seed.isNotEmpty())
        rngSeed.store ((juce::uint64) seed.getLargeIntValue(), std::memory_order_relaxed);

    // v1.9.0: preset name + Sample Lock. A pre-1.9.0 session has neither: no
    // preset named, lock off. Compared as STRINGS (the XML round-trip).
    {
        const juce::ScopedLock pl (presetLock);
        presetManager.setCurrentPresetName (state.getProperty (kPresetNameProp).toString());
    }
    setSampleLock (state.getProperty (kSampleLockProp).toString() == "1");

    // replaceState() fires the sourceSample listener, which queues an AsyncUpdater
    // to rebuild a built-in source. That update is deferred (it runs AFTER this
    // method returns), so a synchronous "suppress" flag can't gate it. Instead we
    // publish the correct source below, then cancelPendingUpdate() to drop the
    // queued rebuild — otherwise a restored user/dropped file would be clobbered by
    // the built-in choice a moment later.
    // v1.8.0: a session saved before Grain Skip / Grain Tilt existed has no
    // entry for them, and replaceState() would leave whatever this instance was
    // set to. Give the missing ones their defaults (Skip 0, Tilt 0), so an old
    // session renders exactly as it did in v1.7.0.
    for (const char* id : { OSimpleGrain::ParamIDs::grainSkip, OSimpleGrain::ParamIDs::grainTilt })
    {
        if (! state.getChildWithProperty ("id", juce::String (id)).isValid())
            if (auto* param = apvts.getParameter (id))
            {
                juce::ValueTree child ("PARAM");
                child.setProperty ("id", juce::String (id), nullptr);
                child.setProperty ("value", param->convertFrom0to1 (param->getDefaultValue()), nullptr);
                state.appendChild (child, nullptr);
            }
    }

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
            // Picker file: its path first, so a session follows the file on disk.
            // The isAbsolutePath guard keeps juce::File() away from a non-path
            // string ("dropped:<name>"), which jasserts in debug hosts (CR-01).
            if (! identity.startsWith ("dropped:") && juce::File::isAbsolutePath (identity))
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
                // The retained copy: embedded in the session (v1.5.6), or still in
                // RAM on a same-instance restore (host undo / A-B). The only route
                // for a dropped file; the fallback for a picker file that moved.
                const juce::ScopedLock sl (sourceStateLock);
                if (userSourceBytes.getSize() > 0 && userSourceBytesIdentity == identity)
                    ok = decodeAndPublish (userSourceBytes.getData(), userSourceBytes.getSize(),
                                           engineRate, identity);
            }
            if (! ok)
                fallBackToDefaultSource (identity, engineRate);   // the editor toasts the name
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
