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
#include "PluginEditor.h"

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
    // Stored normalized 0–1. Display: swing -> 0-75%, humanize/quantize -> 0-100%.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { swing, 1 }, "Swing", unitRange(), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { humanize, 1 }, "Humanize", unitRange(), 0.0f));

    // Default 100% = dead tight (humanize fully removed; swing still survives).
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
        dbRange(), 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));

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
    cacheParamPointers();
}

OSimpleBeatmakerAudioProcessor::~OSimpleBeatmakerAudioProcessor() = default;

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
    sequencerMidi.ensureSize (4096);

    pendingCount = 0;
    absSamplePos = 0;

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
    pSwing      = parameters.getRawParameterValue (swing);
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
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (playing)
            {
                if (auto b = pos->getBpm())         { if (*b > 0.0) bpm = *b; }
                if (auto p = pos->getPpqPosition()) { ppqStart = *p; synced = true; }
            }
        }

    // --- Read the 42 params once / push to voices + router -------------------
    const double swing01    = pSwing    != nullptr ? (double) pSwing->load()    : 0.0;
    const double humanize01 = pHumanize != nullptr ? (double) pHumanize->load() : 0.0;
    const double q          = pQuant    != nullptr ? (double) pQuant->load()    : 1.0;
    const int    patternLen = patternLengthSteps();

    for (int v = 0; v < kNumVoices; ++v)
    {
        voices.setParams (v,
                          pTune [(size_t) v]->load(), pDecay[(size_t) v]->load(),
                          pTone [(size_t) v]->load(), pLevel[(size_t) v]->load());
        muteArr[(size_t) v] = pMute[(size_t) v]->load() > 0.5f;
        soloArr[(size_t) v] = pSolo[(size_t) v]->load() > 0.5f;
    }
    router.setMuteSolo (muteArr, soloArr);

    // --- 2. Enumerate firing step-columns for this block ---------------------
    float playheadPhase = 0.0f;
    bool  discontinuity = false;
    const int nCols = clock.process (firingColumns.data(), kMaxFiringColumns,
                                     synced, playing, bpm, ppqStart, patternLen, numSamples,
                                     playheadPhase, discontinuity);
    if (discontinuity)
        pendingCount = 0;                               // relocate/loop -> drop stale carry-over

    // --- 3. Feel compose + emit into the sequencer buffer --------------------
    sequencerMidi.clear();
#if OUARICON_BUILD_TESTS
    testEmittedHits.clear();
#endif
    const juce::int64 blockStartAbs = absSamplePos;

    drainCarryOver (blockStartAbs, numSamples);         // late hits from prior blocks first

    for (int c = 0; c < nCols; ++c)
    {
        const auto& col = firingColumns[(size_t) c];
        for (int v = 0; v < kNumVoices; ++v)
        {
            const int stepVel = getStep (v, col.stepIndex);
            if (stepVel <= 0)                 continue; // cell off
            if (! router.isVoiceAudible (v))  continue; // mute/solo gate at emit -> no audio, no viz

            const auto hit = feel.compute (v, col.stepIndex, col.nominalSampleInBar,
                                           stepVel, bpm, swing01, q, humanize01);
            const int finalOffset = col.nominalOffsetInBlock + hit.appliedOffsetSamples;

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

    // --- 4. Host-MIDI viz readout (source=1) + merge into the same stream ----
    for (const auto meta : midiMessages)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn() && router.voiceForNote (m.getNoteNumber()) >= 0)
        {
            VizEvent ev;
            ev.voiceIndex         = (juce::uint8) router.voiceForNote (m.getNoteNumber());
            ev.stepIndex          = -1;
            ev.nominalSampleInBar = meta.samplePosition;
            ev.appliedSampleInBar = meta.samplePosition;   // host MIDI: Δt = 0
            ev.velocity           = (juce::uint8) m.getVelocity();
            ev.source             = 1;
            viz.push (ev);
        }
    }
    sequencerMidi.addEvents (midiMessages, 0, numSamples, 0);   // host merged, stays sorted

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
juce::AudioProcessorEditor* OSimpleBeatmakerAudioProcessor::createEditor()
{
    return new OSimpleBeatmakerAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // copyState() returns a COPY; mutating it never touches the live APVTS tree.
    auto state = parameters.copyState();

    // Defensive: never let a stale PATTERN child accumulate across save cycles.
    state.removeChild (state.getChildWithName ("PATTERN"), nullptr);
    state.appendChild (buildPatternTree(), nullptr);

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

            restorePatternTree (tree.getChildWithName ("PATTERN"));

            // Keep the live APVTS tree free of PATTERN; the grid lives in atomics.
            tree.removeChild (tree.getChildWithName ("PATTERN"), nullptr);
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
