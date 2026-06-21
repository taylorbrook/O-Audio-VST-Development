/*
  ==============================================================================

    O-simpleFM - Audio Processor (implementation)

    Stage 1 (Foundation): silent synth shell. Builds the full 17-parameter APVTS
    and persists it. processBlock clears the buffer (no audio until Stage 2).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Shared skews (match ARCHITECTURE.md → Parameter Mapping).
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
OSimpleFMAudioProcessor::createParameterLayout()
{
    using namespace OSimpleFM::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Core FM controls --------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ratio, 1 }, "Ratio (C:M)",
        juce::NormalisableRange<float> { 0.5f, 16.0f, 0.01f }, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ratioSnap, 1 }, "Ratio Snap", false));

    // Raw radian modulation index I (0–20), skew biases control toward the
    // musically dense low end (perceptual taper applied in DSP at Stage 2).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modIndex, 1 }, "Modulation Index",
        juce::NormalisableRange<float> { 0.0f, 20.0f, 0.001f, 0.3f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { feedback, 1 }, "Feedback",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f, 0.5f }, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { modFixedMode, 1 }, "Modulator Fixed Mode", false));

    // Fixed modulator frequency (active only in fixed mode); log skew.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modFixedHz, 1 }, "Modulator Fixed Hz",
        juce::NormalisableRange<float> { 1.0f, 8000.0f, 0.01f, 0.25f }, 220.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modEnvToIndex, 1 }, "Mod Env -> Index",
        unitRange(), 1.0f)); // headline feature ON by default (depth 1.0)

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToIndex, 1 }, "Velocity -> Index",
        unitRange(), 0.0f));

    //--- Modulator envelope (ADSR -> index) --------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modAttack, 1 }, "Mod Attack", adsrTimeRange(), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modDecay, 1 }, "Mod Decay", adsrTimeRange(), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modSustain, 1 }, "Mod Sustain", unitRange(), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modRelease, 1 }, "Mod Release", adsrTimeRange(), 0.3f));

    //--- Amplitude envelope (ADSR -> carrier + voice lifetime) -------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.3f));

    //--- Output ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleFMAudioProcessor::OSimpleFMAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

OSimpleFMAudioProcessor::~OSimpleFMAudioProcessor() = default;

//==============================================================================
void OSimpleFMAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Stage 1: nothing to prepare yet (no voices/DSP). Stage 2 wires the
    // Synthesiser, ADSRs, oversampling and smoothers here.
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void OSimpleFMAudioProcessor::releaseResources()
{
}

bool OSimpleFMAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void OSimpleFMAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Stage 1: silent shell. Clear any garbage in the output buffer; the synth
    // voices that consume MIDI arrive in Stage 2.
    juce::ignoreUnused (midiMessages);
    buffer.clear();
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleFMAudioProcessor::createEditor()
{
    return new OSimpleFMAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleFMAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void OSimpleFMAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleFMAudioProcessor();
}
