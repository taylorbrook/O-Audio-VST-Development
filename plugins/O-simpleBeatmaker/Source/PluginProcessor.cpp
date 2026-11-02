/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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

    O-simpleBeatmaker - Audio Processor (implementation)

    Stage 1 (Foundation): a silent, loadable shell. The full 42-parameter APVTS
    is laid out here, and a custom 6x32 step-grid (std::atomic<uint8_t>) is
    persisted alongside the APVTS state in a "PATTERN" ValueTree child. No DSP
    (processBlock outputs silence) and no WebView — those arrive in Stages 2 & 3.

  ==============================================================================
*/

#include "PluginProcessor.h"
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"   // WebView editor — excluded from the headless render-harness
#endif
#include "BeatPresets.h"

namespace
{
    // 0–1 normalized range (stored 0–1; the Stage-3 UI scales for display).
    juce::NormalisableRange<float> unitRange()
    {
        return { 0.0f, 1.0f, 0.0001f };
    }

    // -60..0 dB range; -60 dB represents "-inf" (silence).
    juce::NormalisableRange<float> dbRange()
    {
        return { -60.0f, 0.0f, 0.1f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleBeatmakerAudioProcessor::createParameterLayout()
{
    using namespace OSimpleBeatmaker;
    using namespace OSimpleBeatmaker::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Sequencer / timing-feel (5) ---------------------------------------
    // v1.6.0: swing is gone from the sound and the page. The parameter stays,
    // non-automatable and read by nothing, so that saved sessions and the host's
    // parameter list stay valid. Do not reuse this ID.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { swing, 1 }, "Swing (unused)", unitRange(), 0.0f,
        juce::AudioParameterFloatAttributes().withAutomatable (false)));

    // Stored normalized 0–1. Display: humanize/quantize -> 0-100%.

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { humanize, 1 }, "Humanize", unitRange(), 0.0f));

    // Default 100% = dead tight (humanize fully removed).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { quantizeStrength, 1 }, "Quantize Strength", unitRange(), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { patternLength, 1 }, "Pattern Length",
        juce::StringArray { "8", "16", "32" }, 1)); // 16 steps

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { tempo, 1 }, "Tempo",
        juce::NormalisableRange<float> { 40.0f, 240.0f, 0.01f }, 120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("BPM")));

    //--- Per voice (6 voices x 6 params = 36) ------------------------------
    for (int v = 0; v < kNumVoices; ++v)
    {
        const juce::String name (kVoiceName[(size_t) v]);

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufTune), 1 }, name + " Tune",
            juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("st")));

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufDecay), 1 }, name + " Decay",
            unitRange(), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufTone), 1 }, name + " Tone",
            unitRange(), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufLevel), 1 }, name + " Level",
            dbRange(), 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));

        params.push_back (std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { voiceParamID (v, sufMute), 1 }, name + " Mute", false));

        params.push_back (std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { voiceParamID (v, sufSolo), 1 }, name + " Solo", false));
    }

    //--- Master (1) --------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        dbRange(), -6.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleBeatmakerAudioProcessor::OSimpleBeatmakerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // std::atomic's default constructor does not zero-initialize — do it explicitly.
    clearGrid();
    clearAllSlots();
    cacheParamPointers();
}

OSimpleBeatmakerAudioProcessor::~OSimpleBeatmakerAudioProcessor()
{
    cancelPendingUpdate();
}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Warm up the shared fastSine LUT on the message thread so its function-local
    // static (one-time heap alloc + __cxa_guard) never initialises lazily on the
    // audio thread on the first tonal-voice block (PERF-01: zero alloc/lock in
    // processBlock). The result is discarded.
    juce::ignoreUnused (OSimpleBeatmaker::fastSine (0.0f));

    voices.prepareToPlay (sampleRate, samplesPerBlock);
    clock.prepareToPlay (sampleRate);
    feel.prepareToPlay (sampleRate);                 // pre-seed per-voice RNG (RT-safe)

    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (
        OSimpleBeatmaker::dbToGain (pOutput != nullptr ? pOutput->load() : 0.0f));

    // Pre-allocate the merged MIDI buffer so processBlock never allocates.
    // 16 KB ≈ 1800 note-ons/block headroom (merge is filtered to note-ons only).
    sequencerMidi.ensureSize (16384);

    pendingCount = 0;
    absSamplePos = 0;
    lastSeenParamLength = patternLengthSteps();

#if OUARICON_BUILD_TESTS
    testEmittedHits.reserve (256);
#endif

    // Zero added latency — the Stage-2 scheduling "lookahead" is bookkeeping, not
    // an output delay line. getLatencySamples() is non-virtual in JUCE 8.
    setLatencySamples (0);
}

void OSimpleBeatmakerAudioProcessor::releaseResources() {}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::cacheParamPointers()
{
    using namespace OSimpleBeatmaker;
    using namespace OSimpleBeatmaker::ParamIDs;
    pHumanize   = parameters.getRawParameterValue (humanize);
    pQuant      = parameters.getRawParameterValue (quantizeStrength);
    pPatternLen = parameters.getRawParameterValue (patternLength);
    pTempo      = parameters.getRawParameterValue (tempo);
    pOutput     = parameters.getRawParameterValue (outputLevel);

    for (int v = 0; v < kNumVoices; ++v)
    {
        pTune [(size_t) v] = parameters.getRawParameterValue (voiceParamID (v, sufTune));
        pDecay[(size_t) v] = parameters.getRawParameterValue (voiceParamID (v, sufDecay));
        pTone [(size_t) v] = parameters.getRawParameterValue (voiceParamID (v, sufTone));
        pLevel[(size_t) v] = parameters.getRawParameterValue (voiceParamID (v, sufLevel));
        pMute [(size_t) v] = parameters.getRawParameterValue (voiceParamID (v, sufMute));
        pSolo [(size_t) v] = parameters.getRawParameterValue (voiceParamID (v, sufSolo));
    }
}

int OSimpleBeatmakerAudioProcessor::patternLengthSteps() const noexcept
{
    const int idx = pPatternLen != nullptr ? (int) std::lround (pPatternLen->load()) : 1;
    return idx == 0 ? 8 : (idx == 2 ? 32 : 16);
}

//==============================================================================
bool OSimpleBeatmakerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: output-only. Accept mono or stereo output, no input bus.
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono()
        && out != juce::AudioChannelSet::stereo())
        return false;

    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return true;
}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::emitSequencerHit (int voiceIndex, int stepIndex,
        int offsetInBlock, int finalVel,
        juce::int32 nominalSampleInBar, juce::int32 appliedSampleInBar,
        int appliedOffsetSamples, juce::int64 blockStartAbs) noexcept
{
    using namespace OSimpleBeatmaker;

    // One emit = one viz push, in the SAME path (QUAL-02 by construction).
    sequencerMidi.addEvent (
        juce::MidiMessage::noteOn (1, kGmNotes[(size_t) voiceIndex], (juce::uint8) finalVel),
        offsetInBlock);

    VizEvent ev;
    ev.voiceIndex         = (juce::uint8) voiceIndex;
    ev.stepIndex          = (juce::int16) stepIndex;
    ev.nominalSampleInBar = nominalSampleInBar;
    ev.appliedSampleInBar = appliedSampleInBar;
    ev.velocity           = (juce::uint8) finalVel;
    ev.source             = 0;                          // sequencer
    viz.push (ev);

#if OUARICON_BUILD_TESTS
    testEmittedHits.push_back ({ voiceIndex, stepIndex, finalVel, 0,
                                 blockStartAbs, offsetInBlock, appliedOffsetSamples,
                                 nominalSampleInBar, appliedSampleInBar });
#else
    juce::ignoreUnused (appliedOffsetSamples, blockStartAbs);
#endif
}

void OSimpleBeatmakerAudioProcessor::drainCarryOver (juce::int64 blockStartAbs, int numSamples) noexcept
{
    int w = 0;
    for (int i = 0; i < pendingCount; ++i)
    {
        const auto& p = pending[(size_t) i];
        const juce::int64 rel = p.absTarget - blockStartAbs;

        if (rel < (juce::int64) numSamples)
        {
            const int off = (int) juce::jlimit ((juce::int64) 0, (juce::int64) (numSamples - 1), rel);
            emitSequencerHit (p.voiceIndex, p.stepIndex, off, p.velocity,
                              p.nominalSampleInBar, p.appliedSampleInBar,
                              p.appliedOffsetSamples, blockStartAbs);
        }
        else
        {
            pending[(size_t) w++] = p;                  // still in the future — keep
        }
    }
    pendingCount = w;
}

void OSimpleBeatmakerAudioProcessor::fireSegment (int segStart, int segEnd, int numSamples,
        bool synced, bool playing, double bpm, double ppqStart, juce::int64 blockStartAbs,
        double q, double humanize01, bool& drained, float& playheadPhaseOut) noexcept
{
    using namespace OSimpleBeatmaker;

    // The ppq at this segment's first sample. A block with no switch is one
    // segment from 0, so it enumerates exactly as a whole block always did.
    const double segPpq = ppqStart + (double) segStart * bpm / (60.0 * currentSampleRate.load (std::memory_order_relaxed));

    float phase = 0.0f;
    bool  discontinuity = false;
    const int nCols = clock.process (firingColumns.data(), kMaxFiringColumns,
                                     synced, playing, bpm, segPpq, getEffectivePatternLength(),
                                     segEnd - segStart, phase, discontinuity);
    if (discontinuity)
        pendingCount = 0;                               // relocate/loop -> drop stale carry-over

    if (! drained)
    {
        drained = true;
        playheadPhaseOut = phase;
        drainCarryOver (blockStartAbs, numSamples);     // late hits from prior blocks first
    }

    for (int c = 0; c < nCols; ++c)
    {
        const auto& col = firingColumns[(size_t) c];
        for (int v = 0; v < kNumVoices; ++v)
        {
            const int stepVel = getStep (v, col.stepIndex);
            if (stepVel <= 0)                 continue; // cell off
            if (! router.isVoiceAudible (v))  continue; // mute/solo gate at emit -> no audio, no viz

            const auto hit = feel.compute (v, col.nominalSampleInBar, stepVel, q, humanize01);
            const int finalOffset = segStart + col.nominalOffsetInBlock + hit.appliedOffsetSamples;

            if (finalOffset < numSamples)
            {
                emitSequencerHit (v, col.stepIndex, juce::jmax (0, finalOffset), hit.finalVelocity,
                                  hit.nominalSampleInBar, hit.appliedSampleInBar,
                                  hit.appliedOffsetSamples, blockStartAbs);
            }
            else if (pendingCount < kMaxPending)        // late: carry to a later block
            {
                pending[(size_t) pendingCount++] = {
                    blockStartAbs + finalOffset, v, col.stepIndex, hit.finalVelocity,
                    hit.nominalSampleInBar, hit.appliedSampleInBar, hit.appliedOffsetSamples };
            }
        }
    }
}

void OSimpleBeatmakerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midiMessages)
{
    using namespace OSimpleBeatmaker;
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                                     // voices ADD into a cleared buffer

    // --- 1. Read transport once (getPosition() is legal only here) -----------
    bool   synced  = false;
    bool   playing = false;
    double bpm      = (pTempo != nullptr ? (double) pTempo->load() : 120.0);
    double ppqStart = 0.0;
    bool   hostTempo = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            // v1.5.0: the host tempo wins while stopped too, so Preview runs at
            // the song's tempo. The Tempo knob is for hosts that report none.
            if (auto b = pos->getBpm())
                if (*b > 0.0) { bpm = *b; hostTempo = true; }
            if (playing)
                if (auto p = pos->getPpqPosition()) { ppqStart = *p; synced = true; }
        }

    // v1.5.0: the sequencer runs only with the host playing or Preview on. DAW
    // play ends Preview for good, so that DAW stop means silence.
    if (playing)
        previewOn.store (false, std::memory_order_relaxed);
    const bool restartPreview = previewRestart.exchange (false, std::memory_order_relaxed);
    const bool running = playing || (previewOn.load (std::memory_order_relaxed) && ! isNonRealtime());

    if (restartPreview)
        clock.resetFreeRun();
    if (! running)
    {
        clock.stop();
        pendingCount = 0;                               // a stopped transport fires nothing late
    }

    // Advisory taps for the Stage-3 UI (read-only on the message thread).
    lastKnownBpm.store (bpm, std::memory_order_relaxed);
    hostSynced.store (playing, std::memory_order_relaxed);
    hostTempoActive.store (hostTempo, std::memory_order_relaxed);
    sequencerRunning.store (running, std::memory_order_relaxed);

    // --- Read the 42 params once / push to voices + router -------------------
    const double humanize01 = pHumanize != nullptr ? (double) pHumanize->load() : 0.0;
    const double q          = pQuant    != nullptr ? (double) pQuant->load()    : 1.0;

    // v1.5.0: a host or user change of the length parameter beats a slot's override.
    const int paramLen = patternLengthSteps();
    if (paramLen != lastSeenParamLength)
    {
        lastSeenParamLength = paramLen;
        lengthOverride.store (0, std::memory_order_relaxed);
    }

    for (int v = 0; v < kNumVoices; ++v)
    {
        voices.setParams (v,
                          pTune [(size_t) v]->load(), pDecay[(size_t) v]->load(),
                          pTone [(size_t) v]->load(), pLevel[(size_t) v]->load());
        muteArr[(size_t) v] = pMute[(size_t) v]->load() > 0.5f;
        soloArr[(size_t) v] = pSolo[(size_t) v]->load() > 0.5f;
    }
    router.setMuteSolo (muteArr, soloArr);

    // --- 2+3. Firing columns, feel compose + emit, split at pattern switches ---
    // v1.5.0: notes 0-11 recall a slot at the note's own sample, so the result
    // does not depend on the block size: steps from that sample on read the new
    // pattern, and hits already computed (humanize carry-over) keep the old one.
    // Empty slots ignore the note. A switch while stopped still takes effect.
    sequencerMidi.clear();
#if OUARICON_BUILD_TESTS
    testEmittedHits.clear();
#endif
    const juce::int64 blockStartAbs = absSamplePos;
    float playheadPhase = 0.0f;
    bool  drained   = false;
    int   segStart  = 0;

    for (const auto meta : midiMessages)
    {
        if (meta.numBytes != 3 || (meta.data[0] & 0xF0) != 0x90 || meta.data[2] == 0
            || meta.data[1] >= kNumPatternSlots
            || slots[(size_t) meta.data[1]].length.load (std::memory_order_acquire) == 0)
            continue;

        const int at = juce::jlimit (0, juce::jmax (0, numSamples - 1), meta.samplePosition);
        if (running && at > segStart)
        {
            fireSegment (segStart, at, numSamples, synced, playing, bpm, ppqStart, blockStartAbs,
                         q, humanize01, drained, playheadPhase);
            segStart = at;
        }
        recallSlotFromAudioThread ((int) meta.data[1]);
    }

    if (running && numSamples > 0)
        fireSegment (segStart, numSamples, numSamples, synced, playing, bpm, ppqStart, blockStartAbs,
                     q, humanize01, drained, playheadPhase);

    // --- 4. Host-MIDI viz readout (source=1) + merge into the same stream ----
    // Gated by isVoiceAudible like the sequencer path (:321) — a muted voice's
    // trigger is dropped by handleTrigger, so drawing its dot would show a hit
    // that makes no sound.
    // Raw-byte gate (same as the merge below): a multi-KB SysEx must never
    // construct a heap-backed MidiMessage on the audio thread.
    for (const auto meta : midiMessages)
    {
        if (meta.numBytes != 3 || (meta.data[0] & 0xF0) != 0x90 || meta.data[2] == 0)
            continue;
        const int hostVoice = router.voiceForNote ((int) meta.data[1]);
        if (hostVoice >= 0 && router.isVoiceAudible (hostVoice))
        {
            VizEvent ev;
            ev.voiceIndex         = (juce::uint8) hostVoice;
            ev.stepIndex          = -1;
            ev.nominalSampleInBar = meta.samplePosition;
            ev.appliedSampleInBar = meta.samplePosition;   // host MIDI: Δt = 0
            ev.velocity           = meta.data[2];
            ev.source             = 1;
            viz.push (ev);
        }
    }
    // Merge note-ons only (all the router ever consumes). Raw-byte check so a
    // multi-KB SysEx never constructs a heap-backed MidiMessage or grows
    // sequencerMidi's storage on the audio thread.
    for (const auto meta : midiMessages)
        if (meta.numBytes == 3 && (meta.data[0] & 0xF0) == 0x90 && meta.data[2] != 0
            && meta.samplePosition < numSamples)
            sequencerMidi.addEvent (meta.getMessage(), meta.samplePosition);

    // --- 5. Sub-slice render on event offsets --------------------------------
    router.renderMerged (buffer, sequencerMidi, voices);

    // --- 6. Master gain (smoothed) -------------------------------------------
    outputGain.setTargetValue (dbToGain (pOutput != nullptr ? pOutput->load() : 0.0f));
    if (numCh > 0)
    {
        auto* const* wp = buffer.getArrayOfWritePointers();
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outputGain.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
                wp[ch][i] *= g;
        }
    }

    // Block-level NaN/inf scrub (suite-wide insurance).
    for (int ch = 0; ch < numCh; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }

    // --- 7. Viz playhead + bookkeeping ---------------------------------------
    viz.setPlayheadStepPhase (playheadPhase);
    viz.bumpFrame();
    absSamplePos += numSamples;
}

//==============================================================================
// Step-grid API (message thread writes / audio thread reads via atomics).
void OSimpleBeatmakerAudioProcessor::setStep (int voice, int step, int velocity) noexcept
{
    if (! inRange (voice, step))
        return;

    grid[(size_t) cellIndex (voice, step)]
        .store ((uint8_t) juce::jlimit (0, 127, velocity), std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::setStepVelocity (int voice, int step, int velocity) noexcept
{
    if (! inRange (voice, step))
        return;

    // Only meaningful for an already-on cell — keep "off" off.
    auto& cell = grid[(size_t) cellIndex (voice, step)];
    if (cell.load (std::memory_order_relaxed) == 0)
        return;

    cell.store ((uint8_t) juce::jlimit (1, 127, velocity), std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::toggleStep (int voice, int step) noexcept
{
    if (! inRange (voice, step))
        return;

    auto& cell = grid[(size_t) cellIndex (voice, step)];
    const uint8_t cur = cell.load (std::memory_order_relaxed);
    cell.store (cur > 0 ? (uint8_t) 0 : (uint8_t) kDefaultStepVelocity, std::memory_order_relaxed);
}

int OSimpleBeatmakerAudioProcessor::getStep (int voice, int step) const noexcept
{
    if (! inRange (voice, step))
        return 0;

    return grid[(size_t) cellIndex (voice, step)].load (std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::clearGrid() noexcept
{
    for (auto& cell : grid)
        cell.store (0, std::memory_order_relaxed);
}

//==============================================================================
// Preview + pattern slots (v1.5.0).
void OSimpleBeatmakerAudioProcessor::setPreview (bool on) noexcept
{
    if (on && ! previewOn.load (std::memory_order_relaxed))
        previewRestart.store (true, std::memory_order_relaxed);
    previewOn.store (on, std::memory_order_relaxed);
}

int OSimpleBeatmakerAudioProcessor::getEffectivePatternLength() const noexcept
{
    const int over = lengthOverride.load (std::memory_order_relaxed);
    return over > 0 ? over : patternLengthSteps();
}

void OSimpleBeatmakerAudioProcessor::storePatternSlot (int slot) noexcept
{
    if (slot < 0 || slot >= kNumPatternSlots)
        return;

    auto& dst = slots[(size_t) slot];
    for (size_t i = 0; i < grid.size(); ++i)
        dst.cells[i].store (grid[i].load (std::memory_order_relaxed), std::memory_order_relaxed);
    dst.length.store (getEffectivePatternLength(), std::memory_order_release);
    activeSlot.store (slot, std::memory_order_relaxed);
}

bool OSimpleBeatmakerAudioProcessor::loadPatternSlot (int slot)
{
    if (slot < 0 || slot >= kNumPatternSlots)
        return false;

    auto& src = slots[(size_t) slot];
    const int len = src.length.load (std::memory_order_acquire);
    if (len == 0)
        return false;

    for (size_t i = 0; i < grid.size(); ++i)
        grid[i].store (src.cells[i].load (std::memory_order_relaxed), std::memory_order_relaxed);

    lengthOverride.store (0, std::memory_order_relaxed);
    setPatternLengthParam (len);
    activeSlot.store (slot, std::memory_order_relaxed);
    patternGeneration.fetch_add (1, std::memory_order_relaxed);
    return true;
}

void OSimpleBeatmakerAudioProcessor::clearPatternSlot (int slot) noexcept
{
    if (slot < 0 || slot >= kNumPatternSlots)
        return;

    slots[(size_t) slot].length.store (0, std::memory_order_release);
    int expected = slot;
    activeSlot.compare_exchange_strong (expected, -1, std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::recallSlotFromAudioThread (int slot) noexcept
{
    auto& src = slots[(size_t) slot];
    for (size_t i = 0; i < grid.size(); ++i)
        grid[i].store (src.cells[i].load (std::memory_order_relaxed), std::memory_order_relaxed);

    const int len = src.length.load (std::memory_order_acquire);
    const bool differs = len != patternLengthSteps();
    lengthOverride.store (differs ? len : 0, std::memory_order_relaxed);
    if (differs)
        triggerAsyncUpdate();

    activeSlot.store (slot, std::memory_order_relaxed);
    patternGeneration.fetch_add (1, std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::handleAsyncUpdate()
{
    const int over = lengthOverride.load (std::memory_order_relaxed);
    if (over > 0)
        setPatternLengthParam (over);
}

void OSimpleBeatmakerAudioProcessor::setPatternLengthParam (int steps)
{
    if (auto* prm = parameters.getParameter (OSimpleBeatmaker::ParamIDs::patternLength))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (steps == 8 ? 0 : (steps == 32 ? 2 : 1))));
}

void OSimpleBeatmakerAudioProcessor::clearAllSlots() noexcept
{
    for (auto& slot : slots)
    {
        for (auto& cell : slot.cells)
            cell.store (0, std::memory_order_relaxed);
        slot.length.store (0, std::memory_order_relaxed);
    }
    activeSlot.store (-1, std::memory_order_relaxed);
}

int OSimpleBeatmakerAudioProcessor::getFilledSlotMask() const noexcept
{
    int mask = 0;
    for (int i = 0; i < kNumPatternSlots; ++i)
        if (slots[(size_t) i].length.load (std::memory_order_relaxed) != 0)
            mask |= 1 << i;
    return mask;
}

bool OSimpleBeatmakerAudioProcessor::isActiveSlotModified() const noexcept
{
    const int active = getActiveSlot();
    if (active < 0)
        return false;

    const auto& slot = slots[(size_t) active];
    if (slot.length.load (std::memory_order_relaxed) != getEffectivePatternLength())
        return true;

    for (size_t i = 0; i < grid.size(); ++i)
        if (slot.cells[i].load (std::memory_order_relaxed) != grid[i].load (std::memory_order_relaxed))
            return true;
    return false;
}

//==============================================================================
// Concept-isolating factory presets (FUNC-05). Message thread only.
void OSimpleBeatmakerAudioProcessor::applyConceptPreset (int index)
{
    if (index < 0 || index >= (int) OSimpleBeatmaker::kBeatPresets.size())
        return;

    // v1.5.0: a lesson replaces the grid, so no slot is current any more. The
    // length this preset writes must not lose to a slot's pending override.
    activeSlot.store (-1, std::memory_order_relaxed);
    lengthOverride.store (0, std::memory_order_relaxed);

    const auto& p = OSimpleBeatmaker::kBeatPresets[(size_t) index];
    using namespace OSimpleBeatmaker::ParamIDs;

    // 0. Reset ALL 42 params to defaults first. The lesson presets only set the
    //    5 timing-feel params + grid; without this, stale mute/solo/level/tune
    //    state silently carries into the lesson (a soloed kick makes the "Ghost
    //    Notes" snare inaudible with no visible cause).
    //    v1.4.2: except the master Output Level, which is USER-OWNED — no preset
    //    names it either, so loading one leaves the level where the user put it.
    //    (The per-voice Level knobs stay preset-owned and still reset here.)
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! isUserOwnedParameter (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());

    // 1. Timing-feel params, host-notifying. setValueNotifyingHost takes a
    //    NORMALISED 0..1 value, so convert each real value through the param's own
    //    NormalisableRange (convertTo0to1). This updates host automation AND the
    //    two-way-bound JS knobs/combo for free.
    auto setReal = [this] (const char* id, float real)
    {
        if (auto* prm = parameters.getParameter (id))   // RangedAudioParameter*
            prm->setValueNotifyingHost (prm->convertTo0to1 (real));
    };

    setReal (humanize,         p.humanize01);
    setReal (quantizeStrength, p.quantize01);
    setReal (tempo,            p.tempoBpm);

    // Choice param: normalise the choice index over its {0, numChoices-1, 1} range.
    if (auto* prm = parameters.getParameter (patternLength))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) p.patternLengthChoice));

    // 2. Grid via the existing thread-safe atomic writers. clearGrid() zeros all
    //    6×32 cells (so columns 16..31 are silent), then stamp the 16-col preset.
    clearGrid();
    for (int v = 0; v < OSimpleBeatmaker::kNumVoices; ++v)
        for (int s = 0; s < 16; ++s)
            if (p.grid[(size_t) v][(size_t) s] > 0)
                setStep (v, s, p.grid[(size_t) v][(size_t) s]);
}

//==============================================================================
// PATTERN <-> ValueTree. The grid is encoded as a base64 byte blob (JUCE's own
// MemoryBlock format, used symmetrically for save+restore — this is NOT
// JS-interop base64, so the standard-vs-JUCE encoding distinction is moot here).
juce::ValueTree OSimpleBeatmakerAudioProcessor::buildPatternTree() const
{
    juce::ValueTree pattern ("PATTERN");
    pattern.setProperty ("rows", kNumVoices, nullptr);
    pattern.setProperty ("cols", kMaxSteps, nullptr);

    juce::MemoryBlock mb ((size_t) (kNumVoices * kMaxSteps));
    auto* bytes = static_cast<uint8_t*> (mb.getData());
    for (int i = 0; i < kNumVoices * kMaxSteps; ++i)
        bytes[i] = grid[(size_t) i].load (std::memory_order_relaxed);

    pattern.setProperty ("cells", mb.toBase64Encoding(), nullptr);
    return pattern;
}

void OSimpleBeatmakerAudioProcessor::restorePatternTree (const juce::ValueTree& pattern)
{
    clearGrid();
    if (! pattern.isValid())
        return;

    const int cols = (int) pattern.getProperty ("cols", kMaxSteps);
    const int rows = (int) pattern.getProperty ("rows", kNumVoices);

    juce::MemoryBlock mb;
    if (! mb.fromBase64Encoding (pattern.getProperty ("cells").toString()))
        return;

    const auto* bytes = static_cast<const uint8_t*> (mb.getData());
    const int   avail = (int) mb.getSize();

    // Source stride is the SAVED column count, so this round-trips even if
    // kMaxSteps ever changes. Cells beyond the saved range stay 0 (cleared above).
    for (int v = 0; v < rows && v < kNumVoices; ++v)
        for (int s = 0; s < cols && s < kMaxSteps; ++s)
        {
            const int src = v * cols + s;
            if (src < avail)
                grid[(size_t) cellIndex (v, s)].store (bytes[src], std::memory_order_relaxed);
        }
}

//==============================================================================
// PATTERN_SLOTS <-> ValueTree (v1.5.0). Only filled slots are written. A session
// from before v1.5.0 has no such child and restores with every slot empty.
juce::ValueTree OSimpleBeatmakerAudioProcessor::buildSlotsTree() const
{
    juce::ValueTree tree ("PATTERN_SLOTS");
    tree.setProperty ("active", getActiveSlot(), nullptr);
    tree.setProperty ("rows", kNumVoices, nullptr);
    tree.setProperty ("cols", kMaxSteps, nullptr);

    for (int s = 0; s < kNumPatternSlots; ++s)
    {
        const auto& slot = slots[(size_t) s];
        const int len = slot.length.load (std::memory_order_acquire);
        if (len == 0)
            continue;

        juce::MemoryBlock mb ((size_t) (kNumVoices * kMaxSteps));
        auto* bytes = static_cast<uint8_t*> (mb.getData());
        for (int i = 0; i < kNumVoices * kMaxSteps; ++i)
            bytes[i] = slot.cells[(size_t) i].load (std::memory_order_relaxed);

        juce::ValueTree child ("SLOT");
        child.setProperty ("index", s, nullptr);
        child.setProperty ("length", len, nullptr);
        child.setProperty ("cells", mb.toBase64Encoding(), nullptr);
        tree.appendChild (child, nullptr);
    }
    return tree;
}

void OSimpleBeatmakerAudioProcessor::restoreSlotsTree (const juce::ValueTree& tree)
{
    clearAllSlots();
    if (! tree.isValid())
        return;

    const int cols = (int) tree.getProperty ("cols", kMaxSteps);
    const int rows = (int) tree.getProperty ("rows", kNumVoices);

    for (int c = 0; c < tree.getNumChildren(); ++c)
    {
        const auto child = tree.getChild (c);
        const int index = (int) child.getProperty ("index", -1);
        const int len   = (int) child.getProperty ("length", 0);
        juce::MemoryBlock mb;

        if (index < 0 || index >= kNumPatternSlots || (len != 8 && len != 16 && len != 32)
            || ! mb.fromBase64Encoding (child.getProperty ("cells").toString()))
            continue;

        const auto* bytes = static_cast<const uint8_t*> (mb.getData());
        const int   avail = (int) mb.getSize();
        auto&       slot  = slots[(size_t) index];

        for (int v = 0; v < rows && v < kNumVoices; ++v)
            for (int s = 0; s < cols && s < kMaxSteps; ++s)
            {
                const int src = v * cols + s;
                if (src < avail)
                    slot.cells[(size_t) cellIndex (v, s)].store (bytes[src], std::memory_order_relaxed);
            }
        slot.length.store (len, std::memory_order_release);
    }

    // An active index naming a slot that did not restore must not survive.
    const int active = (int) tree.getProperty ("active", -1);
    if (active >= 0 && active < kNumPatternSlots
        && slots[(size_t) active].length.load (std::memory_order_relaxed) != 0)
        activeSlot.store (active, std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleBeatmakerAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OSimpleBeatmakerAudioProcessorEditor (*this);
#else
    // Headless render-harness build (JUCE_WEB_BROWSER=0): no WebView editor is
    // compiled in. The harness never opens an editor; this fallback just gives
    // the linker a valid createEditor symbol. The shipping plugin always builds
    // with JUCE_WEB_BROWSER=1, so its behaviour is unchanged.
    return new juce::GenericAudioProcessorEditor (*this);
#endif
}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // copyState() returns a COPY; mutating it never touches the live APVTS tree.
    auto state = parameters.copyState();

    // v1.1.0: the interface LANGUAGE rides the same tree as a plain property on
    // the ROOT — not a parameter, and not part of any lesson preset. Written as
    // a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means, and because the ValueTree
    // -> XML round trip rebuilds every property as a string var anyway
    // (critical_valuetree_xml_roundtrip_loses_type). Storing the code means the
    // value that comes back is the value that went in, with no type predicate to
    // misfire on the way.
    //
    // It is set on the COPY, after copyState() rather than before it: this
    // processor writes its own XML from `state` directly, so there is no later
    // copy for the property to miss.
    state.setProperty (kUiLanguageProp,
                       languageCode (uiLanguage.load (std::memory_order_acquire)),
                       nullptr);

    // Defensive: never let a stale PATTERN child accumulate across save cycles.
    state.removeChild (state.getChildWithName ("PATTERN"), nullptr);
    state.appendChild (buildPatternTree(), nullptr);
    state.removeChild (state.getChildWithName ("PATTERN_SLOTS"), nullptr);
    state.appendChild (buildSlotsTree(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleBeatmakerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (parameters.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);

            // v1.1.0. Read from the INCOMING tree, before replaceState() below:
            // AudioProcessorValueTreeState::replaceState() assigns the tree
            // wholesale (state = newState), so parameters.state is the OLD tree
            // until it returns. Reading `tree` sidesteps the ordering entirely.
            //
            // A pre-1.1.0 session has no property, getProperty returns a VOID
            // var, and the default (English) stands. isVoid() is the only
            // correct gate — the value comes back as a STRING var, so a type
            // predicate like isInt() would never fire. languageIndex() clamps
            // anything that is not "fr" to 0, so a hand-edited value degrades to
            // English rather than to a bad index.
            const juce::var lang = tree.getProperty (kUiLanguageProp);
            if (! lang.isVoid())
                uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

            restorePatternTree (tree.getChildWithName ("PATTERN"));
            restoreSlotsTree (tree.getChildWithName ("PATTERN_SLOTS"));
            lengthOverride.store (0, std::memory_order_relaxed);
            previewOn.store (false, std::memory_order_relaxed);

            // Keep the live APVTS tree free of PATTERN; the grid lives in atomics.
            tree.removeChild (tree.getChildWithName ("PATTERN"), nullptr);
            tree.removeChild (tree.getChildWithName ("PATTERN_SLOTS"), nullptr);
            parameters.replaceState (tree);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleBeatmakerAudioProcessor();
}
