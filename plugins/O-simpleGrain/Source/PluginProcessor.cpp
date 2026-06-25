/*
  ==============================================================================

    O-simpleGrain - Audio Processor (implementation)

    Stage 1 (Foundation): silent 8-voice synth shell. Builds the full 18-parameter
    APVTS and persists it alongside a custom loaded-source identity. processBlock
    clears the buffer and consumes MIDI (no audio until Stage 2). Allocation-free.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GrainVoice.h"
#include "GrainSound.h"
#include "BinaryData.h"      // embedded fire/voice/water/piano .wav (Stage 2.3)

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
    // Grain size 2–200 ms; skew ~0.4 biases control toward the fine low end
    // (the buzz↔fragments axis lives down there).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { grainSize, 1 }, "Grain Size",
        juce::NormalisableRange<float> { 2.0f, 200.0f, 0.01f, 0.4f }, 30.0f,
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
    // Per-grain amplitude envelope. Default Hann = index 4. Rectangular
    // intentionally clicks (teaching artifact, not a bug).
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { windowShape, 1 }, "Window",
        juce::StringArray { "Rectangular", "Triangular", "Welch", "Gaussian", "Hann" }, 4));

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
    outputLevelParam   = apvts.getRawParameterValue (outputLevel);

    // Preallocate all grain voices up front (no audio-thread allocation later).
    // Hand each voice the shared window-LUT table set (built at construction,
    // before the synth member, so the pointer is valid here).
    for (int i = 0; i < kMaxVoices; ++i)
    {
        auto* v = new GrainVoice();
        v->setWindowLuts (&windowLuts);
        synth.addVoice (v);
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
    currentSampleRate = sampleRate;

    // Granular processing has no inherent latency in this design.
    // NB: getLatencySamples() is non-virtual in JUCE 8 — never override it.
    setLatencySamples (0);

    // Synthesiser + per-voice prepare. juce::SynthesiserVoice has no virtual
    // prepareToPlay in JUCE 8 — dispatch the custom one via dynamic_cast.
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* gv = dynamic_cast<GrainVoice*> (synth.getVoice (v)))
            gv->prepareToPlay (sampleRate, samplesPerBlock);

    // Reset the shared viz grain count (voices forget their contribution in their
    // own prepareToPlay, so a clean delta is recomputed on the first render).
    activeGrainCount.store (0, std::memory_order_relaxed);

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

    // Seed the playhead at its resting point (position% * srcLen is unknown until
    // the source is decoded below; we re-seed once the source length is known).
    playheadPos = 0.0;

    // Decode + resample the active source to the engine rate, OFF the audio
    // thread, and publish it via the atomic shared_ptr swap (R8: resample on
    // every prepareToPlay since the engine rate may change). The active source is
    // either a restored user file (currentSourceIdentity = a path) or the
    // selected built-in (currentSourceIdentity = "embedded:<name>").
    bool loadedUserFile = false;
    if (currentSourceIdentity.startsWith ("embedded:"))
    {
        // Built-in: pick the index matching the identity (falls back to the
        // sourceSample choice, then fire, if the name is unknown).
        loadBuiltInSource (builtInIndexForIdentity (currentSourceIdentity), sampleRate);
    }
    else
    {
        // Restored user file: re-read its path. If missing, fall back to the
        // default built-in with a notice (the identity reverts to embedded:fire).
        const juce::File f (currentSourceIdentity);
        if (f.existsAsFile())
        {
            juce::MemoryBlock mb;
            if (f.loadFileAsData (mb)
                && decodeAndPublish (mb.getData(), mb.getSize(), sampleRate, currentSourceIdentity))
                loadedUserFile = true;
        }
        if (! loadedUserFile)
        {
            currentSourceIdentity = "embedded:fire";
            loadBuiltInSource (0, sampleRate);
        }
    }

    // Now the source length is known — seed the playhead at the resting point so
    // the first block starts where `position` points (no jump-from-zero glide).
    if (auto src = atomicLoad (currentSource); src != nullptr && src->getNumSamples() > 0)
    {
        const int srcLen = src->getNumSamples();
        playheadPos = (double) (positionParam->load() / 100.0f) * (double) srcLen;
        playheadPos = juce::jlimit (0.0, (double) (srcLen - 1), playheadPos);
    }
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

    const int    nCh    = juce::jmax (1, (int) reader->numChannels);
    const int    nSmp   = (int) reader->lengthInSamples;
    const double srcRate = reader->sampleRate > 0.0 ? reader->sampleRate : engineRate;
    if (nSmp <= 0)
        return false;

    // Decode at the source rate (handles 1–2 channels generally; the engine read
    // sums/duplicates as needed — see processBlock's mono channel-0 snapshot).
    juce::AudioBuffer<float> tmp (nCh, nSmp);
    reader->read (&tmp, 0, nSmp, 0, true, true);

    bool truncated = false;
    auto resampled = resampleToEngineRate (tmp, srcRate, engineRate, truncated);

    lastLoadTruncated.store (truncated, std::memory_order_relaxed);
    atomicStore (currentSource, std::move (resampled));
    currentSourceIdentity = identity;
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
// Message-thread read: snapshots the atomic shared_ptr (held for the call), then
// reduces channel 0 to `numPairs` min/max pairs in [-1,1]. The audio thread is
// untouched. Returns a flat [min,max,…] vector (empty if no source).
std::vector<float> OSimpleGrainAudioProcessor::getSourceThumbnail (int numPairs) const
{
    numPairs = juce::jlimit (1, 4096, numPairs);

    auto src = atomicLoad (currentSource);          // snapshot — held for this call
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
    loadBuiltInSource (idx, currentSampleRate);
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

    // Guard against a state-restore race: replaceState() fires the sourceSample
    // listener, but if the restored identity is a user/dropped file we must NOT
    // clobber it with a built-in. Only the user actively turning the choice knob
    // (which leaves a stale "embedded:" / file identity) should swap to a
    // built-in — and selecting a built-in is itself an "embedded:" action, so a
    // pending built-in load always wins UNLESS we are mid-restore of a user file.
    if (suppressChoiceRebuild)
        return;

    loadBuiltInSource (idx, currentSampleRate);
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
    const bool ok = decodeAndPublish (raw.getData(), raw.getSize(), currentSampleRate, identity);
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
            decodeAndPublish (mb.getData(), mb.getSize(), currentSampleRate,
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

void OSimpleGrainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // grain voices ADD into a cleared buffer

    // --- Snapshot the source buffer ONCE (held alive for the whole block) -----
    auto src = atomicLoad (currentSource);
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
    p.grainPitch     = grainPitchParam->load();
    p.positionSpray  = positionSprayParam->load();
    p.pitchSpray     = pitchSprayParam->load();
    p.scatter        = scatterParam->load();
    p.panSpray       = panSprayParam->load();
    p.velToDensity   = velToDensityParam->load();
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
    const float playheadAtBlockStart = (float) playheadPos;

    // Advance the global playhead across the block (per sample) for read-head
    // correctness + the Stage-3 waveform-playhead line. Voices snapshot the
    // block-start value above; the per-sample motion keeps the playhead coherent
    // block-to-block (and feeds the live playhead position to 2.3's viz tap).
    constexpr float kRestEpsilon = 1.0e-4f;          // |velocity| below this = "at rest"
    constexpr float kRestEase    = 0.0008f;           // gentle per-sample ease toward resting point
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
            playheadPos += (restTarget - playheadPos) * (double) kRestEase;
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

    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* gv = dynamic_cast<GrainVoice*> (synth.getVoice (v)))
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
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* gv = dynamic_cast<GrainVoice*> (synth.getVoice (v)))
            gv->setGrainCloudFrame (nullptr);

    // --- Fill the global read-head fields + publish the grain frame ----------
    // (playhead/position/spray/freeze are the read-head context for the cloud
    // scatter (UI-01) + waveform playheads (UI-02).)
    frame.playheadNorm      = (srcLen > 0)
                            ? juce::jlimit (0.0f, 1.0f, playheadAtBlockStart / srcLenF) : 0.0f;
    frame.positionNorm      = juce::jlimit (0.0f, 1.0f, positionParam->load() / 100.0f);
    frame.positionSprayNorm = juce::jlimit (0.0f, 1.0f, positionSprayParam->load() / 100.0f);
    frame.frozen            = freezeActive;
    grainCloudBuffer.publish();

    // --- Master output trim (dB->lin, smoothed) + fixed headroom -------------
    // Overlapping grains sum; a fixed headroom factor keeps dense clouds below
    // clipping before the user trim (overlap-aware normalization is a 2.x
    // refinement — the bounded pool already caps the peak).
    constexpr float kHeadroom = 0.5f;
    const float outDb = outputLevelParam->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f) * kHeadroom);
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
    auto state = apvts.copyState();

    auto sourceChild = state.getOrCreateChildWithName (
        juce::Identifier (kSourceStateTag), nullptr);
    sourceChild.setProperty (juce::Identifier (kSourceIdProp),
                             currentSourceIdentity, nullptr);

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
        currentSourceIdentity = sourceChild.getProperty (
            juce::Identifier (kSourceIdProp), currentSourceIdentity).toString();

    // If the restored source is a user/dropped file (not an "embedded:" built-in),
    // suppress the sourceSample listener's AsyncUpdater so replaceState() does not
    // clobber the restored file with the built-in choice.
    const bool restoringUserFile = ! currentSourceIdentity.startsWith ("embedded:");
    suppressChoiceRebuild = restoringUserFile;

    apvts.replaceState (state);

    suppressChoiceRebuild = false;

    // Re-decode the active source at the current engine rate so the restored
    // session sounds the same. OFF the audio thread (setStateInformation runs on
    // the message thread). prepareToPlay also does this — but a host can restore
    // state AFTER prepareToPlay, so reload here too.
    if (currentSampleRate > 0.0)
    {
        if (currentSourceIdentity.startsWith ("embedded:"))
        {
            loadBuiltInSource (builtInIndexForIdentity (currentSourceIdentity), currentSampleRate);
        }
        else
        {
            const juce::File f (currentSourceIdentity);
            bool ok = false;
            if (f.existsAsFile())
            {
                juce::MemoryBlock mb;
                ok = f.loadFileAsData (mb)
                   && decodeAndPublish (mb.getData(), mb.getSize(), currentSampleRate, currentSourceIdentity);
            }
            if (! ok)
            {
                // Missing user file -> fall back to the default built-in (notice in
                // Stage 3 via the identity reverting to embedded:fire).
                currentSourceIdentity = "embedded:fire";
                loadBuiltInSource (0, currentSampleRate);
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleGrainAudioProcessor();
}
