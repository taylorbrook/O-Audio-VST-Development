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
}

OSimpleGrainAudioProcessor::~OSimpleGrainAudioProcessor() = default;

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

    // Decode + resample the default source to the engine rate, OFF the audio
    // thread, and publish it via the atomic shared_ptr swap (2.3 reuses this
    // exact publish path with the embedded BinaryData bytes instead of a file).
    loadDefaultSource (sampleRate);

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
// Decode plugins/O-simpleGrain/Source/samples/fire.wav, resample to engineRate,
// cap at kMaxSourceSeconds, and atomic-publish. NEVER called on the audio thread.
// Phase 2.1 reads the file directly via AudioFormatManager::createReaderFor(File);
// Phase 2.3 replaces the byte source with juce_add_binary_data + MemoryInputStream.
void OSimpleGrainAudioProcessor::loadDefaultSource (double engineRate)
{
    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();

    // Locate the source relative to this translation unit so a dev build finds
    // the bundled .wav without an install step. (2.3 embeds it; this file read
    // is the throwaway 2.1 path to get audible quickly.)
    const juce::File thisFile (juce::String (__FILE__));
    const juce::File wav = thisFile.getParentDirectory()
                                   .getChildFile ("samples")
                                   .getChildFile ("fire.wav");

    std::unique_ptr<juce::AudioFormatReader> reader (fmt.createReaderFor (wav));
    if (reader == nullptr)
        return;                                   // keep silence; processBlock handles a null source

    const int    nCh    = juce::jmax (1, (int) reader->numChannels);
    const int    nSmp   = (int) reader->lengthInSamples;
    const double srcRate = reader->sampleRate > 0.0 ? reader->sampleRate : engineRate;
    if (nSmp <= 0)
        return;

    // Decode into a temp buffer at the source rate.
    juce::AudioBuffer<float> tmp (nCh, nSmp);
    reader->read (&tmp, 0, nSmp, 0, true, true);

    // Resample srcRate -> engineRate per channel, capped at kMaxSourceSeconds.
    const double ratio   = srcRate / engineRate;  // LagrangeInterpolator speedRatio
    const int    maxOut  = (int) (kMaxSourceSeconds * engineRate);
    int          numOut  = (int) std::floor ((double) nSmp / ratio);
    numOut = juce::jlimit (1, maxOut, numOut);

    auto resampled = std::make_shared<juce::AudioBuffer<float>> (nCh, numOut);
    resampled->clear();
    for (int ch = 0; ch < nCh; ++ch)
    {
        juce::LagrangeInterpolator interp;       // streaming/one-shot — correct for a whole-buffer resample
        interp.reset();
        interp.process (ratio, tmp.getReadPointer (ch), resampled->getWritePointer (ch), numOut);
    }

    atomicStore (currentSource, std::move (resampled));
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

    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* gv = dynamic_cast<GrainVoice*> (synth.getVoice (v)))
        {
            gv->setParams (p);
            gv->setSource (srcPtr, srcLen);
            gv->setPlayhead (playheadAtBlockStart);
        }

    // --- Render the grain voices ---------------------------------------------
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

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

    apvts.replaceState (state);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleGrainAudioProcessor();
}
