/*
  ==============================================================================

    O-simpleAdditive - Audio Processor (implementation)

    Stage 1 (Foundation): silent synth shell. Builds the full 33-parameter APVTS
    and persists it. processBlock clears the buffer and applies a smoothed output
    trim (no audio until Stage 2). Zero latency — no oversampling.

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
    // No voice allocation at Foundation — the additive Synthesiser arrives in Stage 2.
}

OSimpleAdditiveAudioProcessor::~OSimpleAdditiveAudioProcessor() = default;

//==============================================================================
void OSimpleAdditiveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;

    // Smoothed dB->lin output trim, seeded from the current parameter value (20 ms).
    outputGain.reset (sampleRate, 0.02);
    const float outDb = parameters.getRawParameterValue (OSimpleAdditive::ParamIDs::outputLevel)->load();
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));

    // No oversampling — additive band-limits exactly (omit partials k>Kmax in Stage 2).
    // getLatencySamples() is non-virtual in JUCE 8; set the stored value instead.
    setLatencySamples (0);
}

void OSimpleAdditiveAudioProcessor::releaseResources()
{
    // Nothing to release yet (voices/DSP arrive in Stage 2).
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

void OSimpleAdditiveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages); // MIDI accepted but unused until Stage 2

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();

    // Silent shell: no voices render yet. Clear the buffer (Stage 2 voices ADD into it).
    buffer.clear();

    // Master output trim (dB->lin, smoothed). Applied to silence now; in place so the
    // ramp infrastructure is already wired when voices arrive in Stage 2.
    const float outDb = parameters.getRawParameterValue (OSimpleAdditive::ParamIDs::outputLevel)->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));
    const float g0 = outputGain.getCurrentValue();
    const float g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, numSamples, g0, g1);

    // Suite-wide NaN insurance (finite scrub).
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
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
