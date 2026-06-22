/*
  ==============================================================================

    O-simpleAdditive - Audio Processor (implementation)

    Stage 2 (complete): 16-voice additive Synthesiser. Per block: read all 33
    APVTS params → resolve Frame B + the global scan LFO + spectral-decay sources
    + the bit-depth choice → push to voices → render → smoothed output trim → NaN
    scrub → visualization tap (mono-sum into the lock-free VizRing + publish the
    newest sounding voice's active-spectrum snapshot). Zero latency — no
    oversampling (each voice band-limits its single-cycle table exactly).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Shared skews (match O-simpleFM → ARCHITECTURE.md Parameter Mapping).
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
OSimpleAdditiveAudioProcessor::createParameterLayout()
{
    using namespace OSimpleAdditive::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Additive spectrum — Frame A: 16 harmonic drawbars (stored 0–1) ----
    // Default: H1 = 1.0 (100%), partials 2..16 = 0.0 → pure sine on load.
    const char* const partialIds[16] = {
        partial1,  partial2,  partial3,  partial4,
        partial5,  partial6,  partial7,  partial8,
        partial9,  partial10, partial11, partial12,
        partial13, partial14, partial15, partial16
    };

    for (int k = 0; k < 16; ++k)
    {
        const float defaultLevel = (k == 0) ? 1.0f : 0.0f;
        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { partialIds[k], 1 },
            "Partial " + juce::String (k + 1) + " Level",
            unitRange(), defaultLevel));
    }

    //--- Wavetable dimension — scan / morph --------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { frameBSource, 1 }, "Frame B Source",
        juce::StringArray { "Sine", "Saw", "Square", "Odd" }, 1 /*default = Saw*/));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { scanPosition, 1 }, "Scan Position",
        unitRange(), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { scanLfoRate, 1 }, "Scan LFO Rate",
        juce::NormalisableRange<float> { 0.01f, 20.0f, 0.0f, 0.3f }, 0.5f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { scanLfoDepth, 1 }, "Scan LFO Depth",
        unitRange(), 0.0f));

    // Bipolar mod-env amount routed to scan (-1..1).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { scanEnvAmount, 1 }, "Scan Env Amount",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.0001f }, 0.0f));

    //--- Spectral shaping --------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { spectralDecay, 1 }, "Spectral Decay",
        unitRange(), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { bitDepth, 1 }, "Bit Depth",
        juce::StringArray { "Off", "12", "10", "8", "6", "4", "2" }, 0 /*default = Off*/));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToDecay, 1 }, "Velocity -> Decay",
        unitRange(), 0.0f));

    //--- Amplitude envelope (ADSR -> per-voice output) ---------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.005f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.1f));

    //--- Modulation envelope (ADSR -> scan position) -----------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modAttack, 1 }, "Mod Attack", adsrTimeRange(), 0.005f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modDecay, 1 }, "Mod Decay", adsrTimeRange(), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modSustain, 1 }, "Mod Sustain", unitRange(), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modRelease, 1 }, "Mod Release", adsrTimeRange(), 0.1f));

    //--- Output ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Foundation contract: exactly 33 parameters (16 drawbars + 9 scan/spectral
    // + amp ADSR×4 + mod ADSR×4 + outputLevel). Guard against accidental drift.
    jassert (params.size() == 33);

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleAdditiveAudioProcessor::OSimpleAdditiveAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Pre-allocate all voices up front (no audio-thread allocation later).
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new AdditiveVoice());

    synth.addSound (new AdditiveSound());     // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);
}

OSimpleAdditiveAudioProcessor::~OSimpleAdditiveAudioProcessor() = default;

//==============================================================================
void OSimpleAdditiveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // On-screen-keyboard MIDI queue: timestamps are converted from the host clock
    // against this rate when drained in processBlock.
    midiCollector.reset (sampleRate);

    // Synthesiser + per-voice prepare. juce::SynthesiserVoice has no virtual
    // prepareToPlay in JUCE 8 — dispatch the custom one via dynamic_cast.
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* av = dynamic_cast<AdditiveVoice*> (synth.getVoice (v)))
            av->prepareToPlay (sampleRate, samplesPerBlock);

    // Smoothed dB->lin output trim, seeded from the current parameter value (20 ms).
    outputGain.reset (sampleRate, 0.02);
    const float outDb = parameters.getRawParameterValue (OSimpleAdditive::ParamIDs::outputLevel)->load();
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));

    // Pre-allocate the viz mono down-mix scratch so the audio-thread tap needs no
    // allocation (PERF-01). Sized to the host's max block; the tap copies only the
    // live numSamples each block.
    monoScratch.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);

    // No oversampling — additive band-limits exactly (partials k>Kmax omitted per
    // voice). getLatencySamples() is non-virtual in JUCE 8; set the stored value.
    setLatencySamples (0);
}

void OSimpleAdditiveAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
}

bool OSimpleAdditiveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void OSimpleAdditiveAudioProcessor::pushParamsToVoices (int numSamples)
{
    using namespace OSimpleAdditive::ParamIDs;
    auto get = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };

    // Frame A — the 16 harmonic drawbars (stored 0–1).
    static const char* const partialIds[AdditiveVoice::kNumPartials] = {
        partial1,  partial2,  partial3,  partial4,
        partial5,  partial6,  partial7,  partial8,
        partial9,  partial10, partial11, partial12,
        partial13, partial14, partial15, partial16
    };

    float frameA[AdditiveVoice::kNumPartials];
    for (int k = 0; k < AdditiveVoice::kNumPartials; ++k)
        frameA[k] = get (partialIds[k]);

    // Frame B — resolve the selected preset vector once for all voices (the
    // per-voice dirty-check absorbs the unchanged-block case).
    float frameB[AdditiveVoice::kNumPartials];
    OSimpleAdditive::fillFrameB ((int) get (frameBSource), frameB);

    // Global scan LFO — one sine shared by all voices so notes morph in phase
    // (ARCHITECTURE.md §Scan LFO: global, sine, advanced once per block). Bipolar
    // [-1,1] × depth swings the scan around the manual position.
    const float lfoVal   = std::sin (lfoPhase * juce::MathConstants<float>::twoPi);
    const float scanBase = get (scanPosition) + lfoVal * get (scanLfoDepth);

    if (currentSampleRate > 0.0)
    {
        lfoPhase += (float) (get (scanLfoRate) * (double) numSamples / currentSampleRate);
        lfoPhase -= std::floor (lfoPhase);     // wrap to [0,1)
    }

    const float scanEnvAmt = get (scanEnvAmount);

    // Spectral-decay sources (2.3). The voice combines these with its own velocity
    // into an effective per-partial rate; pushed raw so the rate tracks live edits.
    const float spectralDecayV = get (spectralDecay);
    const float velToDecayV     = get (velToDecay);

    // Bit-depth choice → bit count (0 = Off). Resolve the choice INDEX here so the
    // voice receives a plain int. Layout: {Off, 12, 10, 8, 6, 4, 2}.
    static constexpr int kBitTable[] = { 0, 12, 10, 8, 6, 4, 2 };
    const int bitIdx  = juce::jlimit (0, (int) std::size (kBitTable) - 1,
                                      (int) std::round (get (bitDepth)));
    const int bitBits = kBitTable[bitIdx];

    const juce::ADSR::Parameters ampP { get (ampAttack), get (ampDecay),
                                        get (ampSustain), get (ampRelease) };
    const juce::ADSR::Parameters modP { get (modAttack), get (modDecay),
                                        get (modSustain), get (modRelease) };

    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* av = dynamic_cast<AdditiveVoice*> (synth.getVoice (v)))
            av->setParams (frameA, frameB, scanBase, scanEnvAmt,
                           spectralDecayV, velToDecayV, bitBits, ampP, modP);
}

void OSimpleAdditiveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();

    buffer.clear();                           // voices ADD into a cleared buffer

    // Merge any on-screen-keyboard notes into the host MIDI stream before rendering
    // (covers both host MIDI and the WebView keyboard from one path).
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    pushParamsToVoices (numSamples);          // advances the global scan LFO by the block
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // Active-spectrum snapshot (2.3) — pick the ACTIVE voice with the largest
    // noteAge (the newest sounding note) and publish its morphed + decayed 16
    // amplitudes for the message-thread drawbar display. This read of voice state
    // runs on the audio thread (same thread as renderNextBlock above) → no
    // voice→processor atomics needed; only the snapshot crosses to the editor.
    // If nothing is sounding the snapshot is left unchanged (Stage 3 handles idle).
    {
        const AdditiveVoice* primary = nullptr;
        std::uint64_t bestAge = 0;
        for (int v = 0; v < synth.getNumVoices(); ++v)
            if (auto* av = dynamic_cast<AdditiveVoice*> (synth.getVoice (v)))
                if (av->isAmpActive() && av->getNoteAge() >= bestAge)
                {
                    bestAge = av->getNoteAge();
                    primary = av;
                }

        if (primary != nullptr)
        {
            const float* spec = primary->getActiveSpectrum();
            for (int k = 0; k < AdditiveVoice::kNumPartials; ++k)
                activeSpectrumSnapshot[(size_t) k].store (spec[k], std::memory_order_relaxed);
        }

        // Drives the editor's idle handling: when nothing sounds, the drawbar
        // live-glow snaps to each drawbar's set level rather than the stale snapshot.
        anyVoiceSounding.store (primary != nullptr, std::memory_order_relaxed);
    }

    // Master output trim (dB->lin, smoothed).
    const float outDb = parameters.getRawParameterValue (OSimpleAdditive::ParamIDs::outputLevel)->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));
    const float g0 = outputGain.getCurrentValue();
    const float g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, numSamples, g0, g1);

    // Suite-wide NaN insurance (finite scrub) after summing voices.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }

    // Visualization tap (2.3) — post-gain, post-scrub mono sum → lock-free ring
    // (copy-only, NO alloc/FFT/locks on the audio thread; PERF-01). monoScratch is
    // pre-allocated in prepareToPlay; guard against a host block larger than the
    // prepared size (clamp the copy length — never reallocate on the audio thread).
    if (numCh == 1)
    {
        viz.write (buffer.getReadPointer (0), numSamples);
    }
    else if (numCh > 1)
    {
        const int n        = juce::jmin (numSamples, (int) monoScratch.size());
        const float invCh  = 1.0f / (float) numCh;
        float* const mono  = monoScratch.data();
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                s += buffer.getReadPointer (ch)[i];
            mono[i] = s * invCh;
        }
        viz.write (mono, n);
    }
}

//==============================================================================
void OSimpleAdditiveAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);   // collector wants seconds
    midiCollector.addMessageToQueue (msg);
}

//==============================================================================
// Lesson preset tour — full APVTS snapshots applied via setValueNotifyingHost so
// the WebView controls update through their relays. Real (scaled) values are passed
// through convertTo0to1; choices pass their index. Each lesson resets to defaults
// first so it reproduces regardless of the prior state.
void OSimpleAdditiveAudioProcessor::applyFactoryPreset (const juce::String& name)
{
    using namespace OSimpleAdditive::ParamIDs;

    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    auto setReal = [this] (const char* id, float real) {
        if (auto* p = parameters.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };
    auto setChoice = [this] (const char* id, int index) {
        if (auto* p = parameters.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    };
    auto drawbars = [&setReal] (std::initializer_list<float> levels) {
        static const char* const ids[16] = {
            partial1,  partial2,  partial3,  partial4,  partial5,  partial6,
            partial7,  partial8,  partial9,  partial10, partial11, partial12,
            partial13, partial14, partial15, partial16 };
        int k = 0;
        for (float v : levels) { if (k < 16) setReal (ids[k], juce::jlimit (0.0f, 1.0f, v)); ++k; }
    };
    const auto inv = [] (float k) { return 1.0f / k; };   // 1/k harmonic amplitude

    if (name == "Pure Sine")
    {
        drawbars ({ 1.0f });                              // defaults are already a pure sine
    }
    else if (name == "Sawtooth")                          // every harmonic at 1/k
    {
        drawbars ({ inv(1), inv(2), inv(3), inv(4),  inv(5),  inv(6),  inv(7),  inv(8),
                    inv(9), inv(10), inv(11), inv(12), inv(13), inv(14), inv(15), inv(16) });
    }
    else if (name == "Square")                            // odd harmonics only, 1/k — hollow
    {
        drawbars ({ inv(1), 0, inv(3), 0, inv(5), 0, inv(7), 0,
                    inv(9), 0, inv(11), 0, inv(13), 0, inv(15), 0 });
    }
    else if (name == "Organ")                             // Hammond-style registration
    {
        drawbars ({ 1.0f, 0.85f, 0.6f, 0.45f, 0.2f, 0.3f, 0.0f, 0.25f });
        setReal (ampSustain, 1.0f);
        setReal (ampRelease, 0.08f);
    }
    else if (name == "Morph Pad")                         // evolving pad: LFO morph A→Square
    {
        drawbars ({ 1.0f, 0.5f, 0.3f, 0.2f, 0.12f, 0.08f });
        setChoice (frameBSource, 2);                      // Square
        setReal (scanPosition, 0.15f);
        setReal (scanLfoRate, 0.25f);
        setReal (scanLfoDepth, 0.7f);
        setReal (ampAttack, 0.6f);
        setReal (ampDecay, 1.0f);
        setReal (ampSustain, 0.85f);
        setReal (ampRelease, 1.2f);
    }
    else if (name == "Lo-Fi Bells")                       // bell spectrum + spectral-decay + bit-crush
    {
        drawbars ({ 1.0f, 0.2f, 0.0f, 0.6f, 0.0f, 0.3f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.25f });
        setReal (spectralDecay, 0.6f);
        setChoice (bitDepth, 3);                          // "8"
        setReal (velToDecay, 0.3f);
        setReal (ampAttack, 0.002f);
        setReal (ampDecay, 1.5f);
        setReal (ampSustain, 0.0f);
        setReal (ampRelease, 1.0f);
    }
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleAdditiveAudioProcessor::createEditor()
{
    return new OSimpleAdditiveAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleAdditiveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Plain APVTS XML (no preset-manager wrapper at Foundation; Stage 4 adds it).
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleAdditiveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleAdditiveAudioProcessor();
}
