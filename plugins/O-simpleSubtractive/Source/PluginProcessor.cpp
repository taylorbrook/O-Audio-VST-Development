/*
  ==============================================================================

    O-simpleSubtractive - Audio Processor (implementation)

    Stage 1 (Foundation): silent synth shell. Builds the full 20-parameter APVTS
    and persists it as XML. processBlock clears the buffer and consumes MIDI
    without synthesis (no audio until Stage 2).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Shared skews (match ARCHITECTURE.md -> Parameter Mapping).
    constexpr float kAdsrTimeSkew = 0.35f; // perceptual skew for 0–5 s envelope times
    constexpr float kMinAdsrTime  = 0.0f;
    constexpr float kMaxAdsrTime  = 5.0f;

    // 0–5 s ADSR time range with perceptual skew.
    juce::NormalisableRange<float> adsrTimeRange()
    {
        return { kMinAdsrTime, kMaxAdsrTime, 0.0001f, kAdsrTimeSkew };
    }

    // 0–1 normalized "percent" range (stored 0–1; UI scales x100 in Stage 3).
    juce::NormalisableRange<float> unitRange()
    {
        return { 0.0f, 1.0f, 0.0001f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleSubtractiveAudioProcessor::createParameterLayout()
{
    using namespace OSimpleSubtractive::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Oscillator / sources ----------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { oscWave, 1 }, "Oscillator Wave",
        juce::StringArray { "Saw", "Square", "Triangle", "Sine" }, 0)); // Saw

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { subLevel, 1 }, "Sub Level", unitRange(), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { noiseLevel, 1 }, "Noise Level", unitRange(), 0.0f));

    //--- Filter ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { filterType, 1 }, "Filter Type",
        juce::StringArray { "Low-pass", "High-pass", "Band-pass", "Notch" }, 0)); // LP

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { filterSlope, 1 }, "Filter Slope",
        juce::StringArray { "6 dB/oct", "12 dB/oct", "24 dB/oct" }, 2)); // 24 dB/oct

    // Base cutoff: log skew (~0.25) over the full audio band, default 2 kHz.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { cutoff, 1 }, "Cutoff",
        juce::NormalisableRange<float> { 20.0f, 20000.0f, 0.0f, 0.25f }, 2000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { resonance, 1 }, "Resonance", unitRange(), 0.10f));

    // Bipolar filter-envelope depth (-100..+100%), default +50%.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterEnvAmount, 1 }, "Filter Env Amount",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.0001f }, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { keyTrack, 1 }, "Key Track", unitRange(), 0.0f));

    //--- Filter envelope (ADSR -> cutoff) ----------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterAttack, 1 }, "Filter Attack", adsrTimeRange(), 0.005f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterDecay, 1 }, "Filter Decay", adsrTimeRange(), 0.30f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterSustain, 1 }, "Filter Sustain", unitRange(), 0.40f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterRelease, 1 }, "Filter Release", adsrTimeRange(), 0.20f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Amplitude envelope (ADSR -> output + voice lifetime) --------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.005f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.30f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 0.80f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.10f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Voicing -----------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { voiceMode, 1 }, "Voice Mode",
        juce::StringArray { "Poly", "Mono", "Legato" }, 0)); // Poly

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { glide, 1 }, "Glide",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f, 0.5f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Output ------------------------------------------------------------
    // -60 dB floor represents "-inf" (silence).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleSubtractiveAudioProcessor::OSimpleSubtractiveAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Voices, oscillators, filter and envelopes are built in Stage 2 (DSP).
}

OSimpleSubtractiveAudioProcessor::~OSimpleSubtractiveAudioProcessor() = default;

//==============================================================================
void OSimpleSubtractiveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    // Zero latency: no oversampling/look-ahead at foundation. getLatencySamples()
    // is non-virtual in JUCE 8 — set the stored value instead of overriding.
    setLatencySamples (0);

    // Smoothing / DSP prepare is reserved for Stage 2 (no-op now).
}

void OSimpleSubtractiveAudioProcessor::releaseResources()
{
    // Nothing allocated at foundation.
}

bool OSimpleSubtractiveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void OSimpleSubtractiveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Foundation is silent: clear ALL output channels (incl. any beyond the input
    // count) and consume MIDI without synthesizing. RT-safe — no alloc/lock/IO.
    buffer.clear();
    juce::ignoreUnused (midiMessages);
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleSubtractiveAudioProcessor::createEditor()
{
    return new OSimpleSubtractiveAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleSubtractiveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleSubtractiveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleSubtractiveAudioProcessor();
}
