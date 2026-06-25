/*
  ==============================================================================

    O-simpleSampler - Audio Processor (implementation)

    Stage 1 (Foundation): silent 16-voice synth shell. Builds the full 21-parameter
    APVTS and persists it alongside a custom loaded-source identity. processBlock
    clears the buffer and consumes MIDI (no audio until Stage 2). Allocation-free.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
}

OSimpleSamplerAudioProcessor::~OSimpleSamplerAudioProcessor() = default;

//==============================================================================
void OSimpleSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::ignoreUnused (samplesPerBlock);

    // The sampler design adds no inherent latency (no oversampling / lookahead).
    // NB: getLatencySamples() is non-virtual in JUCE 8 — never override it.
    setLatencySamples (0);

    // Stage 2: synth/voice prepare, source decode/resample, filter/ADSR prepare.
}

void OSimpleSamplerAudioProcessor::releaseResources()
{
    // Stage 2: release sampler-engine resources here.
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

void OSimpleSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Silent shell: clear the output buffer (no sampler voices yet — Stage 2).
    buffer.clear();

    // Consume MIDI so notes don't queue up. Stage 2 routes these to the sampler
    // voices; for now we simply drain the stream. Allocation-free.
    juce::ignoreUnused (midiMessages);
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

    apvts.replaceState (state);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleSamplerAudioProcessor();
}
