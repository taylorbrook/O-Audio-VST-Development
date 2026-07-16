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
#include "PluginEditor.h"
#include "SampleSound.h"
#include "SampleVoice.h"
#include "BinaryData.h"      // embedded piano.wav (Source/samples/piano.wav)
#include <algorithm>         // std::remove_if (retired-source reaper)

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
        juce::StringArray { "piano", "vocal", "flute", "vinyl" }, 0));

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

    // Retired-source reaper (CR-01). Always running (started here rather than at
    // retire time so no cross-thread start/stop choreography is needed); an empty
    // list makes the tick a no-op. 500 ms is far above any block duration, so a
    // retired buffer's last audio-thread snapshot is long gone by the first reap.
    retiredSources.reserve (8);
    startTimer (500);
}

OSimpleSamplerAudioProcessor::~OSimpleSamplerAudioProcessor()
{
    stopTimer();
    apvts.removeParameterListener (OSimpleSampler::ParamIDs::sourceSample, this);
    cancelPendingUpdate();
    // retiredSources drains here — safe: the host guarantees processing has
    // stopped before destruction, so this is an off-audio free.
}

//==============================================================================
void OSimpleSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // The sampler design adds no inherent latency (no oversampling / lookahead).
    // NB: getLatencySamples() is non-virtual in JUCE 8 — never override it.
    setLatencySamples (0);

    // Synthesiser + per-voice prepare. juce::SynthesiserVoice has no virtual
    // prepareToPlay in JUCE 8 — dispatch the custom one via dynamic_cast.
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SampleVoice*> (synth.getVoice (v)))
            sv->prepareToPlay (sampleRate, samplesPerBlock);

    // Output trim (dB->lin, 20 ms smoothing).
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (outputLevelParam->load(), -60.0f));

    // Decode + resample the active source to the engine rate, OFF the audio thread,
    // and publish it via the atomic shared_ptr swap (resample on every prepareToPlay
    // since the engine rate may change). The active source is either the restored
    // built-in (identity "embedded:<name>") or — for a user-file path, a Stage 2.3
    // feature — falls back to the embedded piano for Phase 2.1.
    // CR-02: prepareToPlay may run on a host audio-setup thread concurrently with
    // setStateInformation (arbitrary host thread) at project load — the whole
    // identity-read → load → seed-guard sequence is one critical section so the
    // restored identity can't be clobbered mid-publish (torn buffer/identity pair).
    {
        const juce::ScopedLock sl (sourcePublishLock);

        if (currentSourceIdentity.startsWith ("embedded:"))
        {
            loadBuiltInSource (builtInIndexForIdentity (currentSourceIdentity), sampleRate);
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
        if (! stateWasRestored && ! rootSeeded)
        {
            seedRootForSource (builtInIndexForIdentity (currentSourceIdentity));
            rootSeeded = true;
        }
    }
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

// Map embedded BinaryData symbols by built-in index. For Phase 2.1 only piano.wav
// is embedded (CONTEXT D1); indices 1–3 fall back to the piano blob so no source
// selection yields silence. JUCE mangles `piano.wav` -> BinaryData::piano_wav /
// BinaryData::piano_wavSize.
namespace
{
    struct BuiltInBlob { const char* data; int size; };

    BuiltInBlob builtInBlob (int idx) noexcept
    {
        switch (idx)
        {
            case 0:  return { BinaryData::piano_wav, BinaryData::piano_wavSize };
            // TODO(Stage 2.3): real vocal/flute/vinyl blobs. Until those assets are
            // embedded, indices 1–3 fall back to the piano blob (documented) so no
            // source selection is silent.
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

    // CR-02: one publisher at a time. Serializes prepareToPlay / handleAsyncUpdate
    // / setStateInformation so the buffer swap and the identity write below are one
    // atomic unit (no torn buffer/identity pair, no racing juce::String assignment).
    // Recursive, so callers already holding the lock (prepareToPlay) nest safely.
    // Never contended by the audio thread (which never takes this lock).
    const juce::ScopedLock sl (sourcePublishLock);

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
    juce::ignoreUnused (truncated);              // 2.1 does not surface truncation (Stage 3 UI notice)

    // CR-01: retire the outgoing buffer BEFORE the swap drops our reference. If
    // atomicStore released it here, an in-flight processBlock snapshot could become
    // the LAST owner and free() up to ~46 MB on the audio thread at end-of-block.
    // Parked on retiredSources instead, the message-thread reaper (timerCallback)
    // frees it once no audio-thread snapshot remains (use_count()==1).
    auto old = atomicLoad (currentSource);
    atomicStore (currentSource, std::move (resampled));   // ATOMIC PUBLISH
    if (old != nullptr)
        retiredSources.push_back (std::move (old));

    currentSourceIdentity = identity;
    return true;
}

// Reaper (CR-01): message-thread timer. Frees retired source buffers whose only
// remaining owner is the retired list itself. Try-lock: if a publisher is mid-
// decode, skip this tick rather than stall the message thread.
void OSimpleSamplerAudioProcessor::timerCallback()
{
    const juce::ScopedTryLock stl (sourcePublishLock);
    if (! stl.isLocked() || retiredSources.empty())
        return;

    retiredSources.erase (
        std::remove_if (retiredSources.begin(), retiredSources.end(),
                        [] (const std::shared_ptr<juce::AudioBuffer<float>>& b)
                        { return b.use_count() == 1; }),
        retiredSources.end());
}

// Decode one embedded built-in by index and publish. OFF the audio thread.
bool OSimpleSamplerAudioProcessor::loadBuiltInSource (int builtInIndex, double engineRate)
{
    builtInIndex = juce::jlimit (0, kNumBuiltIns - 1, builtInIndex);
    const auto blob = builtInBlob (builtInIndex);
    const juce::String identity = juce::String ("embedded:") + kBuiltInNames[builtInIndex];
    return decodeAndPublish (blob.data, (size_t) blob.size, engineRate, identity);
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
// APVTS listener (message thread, possibly off it depending on host). Never
// decodes here — defers to the AsyncUpdater so the decode always runs on the
// message thread (RT-safety: no decode on the audio thread).
void OSimpleSamplerAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == OSimpleSampler::ParamIDs::sourceSample)
    {
        pendingBuiltInIndex.store (juce::jlimit (0, kNumBuiltIns - 1, (int) newValue),
                                   std::memory_order_relaxed);
        triggerAsyncUpdate();
    }
}

void OSimpleSamplerAudioProcessor::handleAsyncUpdate()
{
    // CR-02: exchange + load + seed under the publish lock. setStateInformation
    // clears pendingBuiltInIndex under the same lock, so a restore racing this
    // callback either runs first (we exchange -1 and bail) or runs after (its
    // publish overwrites ours) — the restored source always wins. Without the
    // lock, a restore landing between our exchange and load could be clobbered
    // by this stale built-in choice (and its root re-seed).
    const juce::ScopedLock sl (sourcePublishLock);

    const int idx = pendingBuiltInIndex.exchange (-1, std::memory_order_relaxed);
    if (idx < 0)
        return;

    // A state restore that lands on a source cancels this pending update
    // (setStateInformation publishes the restored source then cancelPendingUpdate()s),
    // so reaching here always means a genuine user sourceSample-choice change — seed
    // the per-source root so an explicit pick retunes the keyboard.
    loadBuiltInSource (idx, currentSampleRate);
    seedRootForSource (idx);
}

//==============================================================================
void OSimpleSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // sampler voices ADD into a cleared buffer

    // --- Snapshot the source buffer ONCE (held alive for the whole block) -----
    auto src = atomicLoad (currentSource);
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

    // Region: start/end as % of the source length, in the SOURCE frame. Each NEW
    // note plays [startSamp, endSamp); the voice clamps its reads via
    // readSourceLagrange. startSamp clamped to [0, srcLen-1] so the endSamp jlimit
    // lower bound (startSamp+1) never exceeds srcLen even at start = 100 %.
    if (srcLen > 0)
    {
        const float startPct = juce::jlimit (0.0f, 100.0f, startParam->load());
        const float endPct   = juce::jlimit (0.0f, 100.0f, endParam->load());
        p.startSamp = juce::jlimit (0, srcLen - 1, (int) (startPct * 0.01f * (float) srcLen));
        p.endSamp   = (int) (endPct * 0.01f * (float) srcLen);
        p.endSamp   = juce::jlimit (p.startSamp + 1, srcLen, p.endSamp);
    }
    else
    {
        p.startSamp = 0;
        p.endSamp   = 0;
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
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleSamplerAudioProcessor::createEditor()
{
    return new OSimpleSamplerAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Serialize the APVTS tree PLUS a custom child holding the loaded-source
    // identity, so a session restores both the params and the active source.
    auto state = apvts.copyState();

    // CR-02: identity is written by concurrent publishers — snapshot it under the
    // publish lock (getStateInformation is never the audio thread).
    const juce::String identitySnapshot = getSourceIdentity();

    auto sourceChild = state.getOrCreateChildWithName (
        juce::Identifier (kSourceStateTag), nullptr);
    sourceChild.setProperty (juce::Identifier (kSourceIdProp),
                             identitySnapshot, nullptr);

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

    // CR-02: setStateInformation can arrive on an arbitrary host thread while
    // prepareToPlay runs on another (both fire back-to-back at project load).
    // The identity write, restore flag, and reload below are one critical section
    // so the restored source can't be clobbered by a concurrent prepare-time
    // reload (and the non-atomic juce::String assignment can't race).
    const juce::ScopedLock sl (sourcePublishLock);

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
    // (Safe under the lock: the listener only stores an atomic + triggerAsyncUpdate;
    // the deferred handleAsyncUpdate acquires the lock itself when it eventually runs.)
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
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleSamplerAudioProcessor();
}
