/*
  ==============================================================================

    O-Bells - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (MUST be defined before constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OBellsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Main Panel Parameters (7) ==========

    // STRIKE_POSITION - Center to edge strike
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "strikePosition", 1 },
        "Strike",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // MALLET_HARDNESS - Soft to hard striker
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "malletHardness", 1 },
        "Mallet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // BELL_SIZE - Small hand bell to large church bell
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bellSize", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // DAMPING - Hand-damped to free-ring
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "damping", 1 },
        "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f,
        "%"
    ));

    // BRIGHTNESS - Dark to brilliant
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brightness", 1 },
        "Bright",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // MATERIAL - Bronze → Steel → Glass → Crystal
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "material", 1 },
        "Material",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.25f
    ));

    // INHARMONICITY - Pure harmonic to gamelan
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inharmonicity", 1 },
        "Inharm",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // ========== Ensemble Section Parameters (5) ==========

    // UNISON_COUNT - Number of detuned bell copies (1-4)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "unisonCount", 1 },
        "Unison",
        1,
        4,
        1
    ));

    // UNISON_DETUNE - Detune spread (0-50 cents)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "unisonDetune", 1 },
        "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        10.0f,
        "cents"
    ));

    // OCTAVE_BLEND_SUB - Sub-octave layer mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveBlendSub", 1 },
        "Sub",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // OCTAVE_BLEND_OCT - Upper-octave layer mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveBlendOct", 1 },
        "Oct",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // STEREO_SPREAD - Ensemble panning width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stereoSpread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // ========== Advanced Panel Parameters (10) ==========

    // PARTIAL_TUNING - Fine-tune minor-third partial
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "partialTuning", 1 },
        "Partial Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f,
        "cents"
    ));

    // NONLINEAR_EFFECTS - Bell warping/distortion
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "nonlinearEffects", 1 },
        "Nonlinear",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // SYMPATHETIC_RESONANCE - Cross-voice coupling
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticResonance", 1 },
        "Sympathetic",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // STRIKE_NOISE_CHARACTER - Transient filter type
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "strikeNoiseChar", 1 },
        "Noise",
        juce::StringArray { "Click", "Thud", "Ping" },
        0
    ));

    // DECAY_SHAPE - Envelope curve type
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "decayShape", 1 },
        "Decay",
        juce::StringArray { "Linear", "Exponential", "Multi-stage" },
        1
    ));

    // VELOCITY_CURVE - Velocity response shaping
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "velocityCurve", 1 },
        "Velocity",
        juce::StringArray { "Linear", "Exponential", "Logarithmic" },
        0
    ));

    // PITCH_ENVELOPE - Initial pitch drop amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnvelope", 1 },
        "Pitch Env",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // PITCH_ENV_TIME - Pitch envelope return time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnvTime", 1 },
        "P.Env Time",
        juce::NormalisableRange<float>(5.0f, 200.0f, 1.0f, 0.5f),
        50.0f,
        "ms"
    ));

    // OUTPUT_GAIN - Master output level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1 },
        "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // QUALITY - CPU vs quality tradeoff
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "quality", 1 },
        "Quality",
        juce::StringArray { "Low", "Medium", "High" },
        2
    ));

    return layout;
}

//==============================================================================
OBellsAudioProcessor::OBellsAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OBellsAudioProcessor::~OBellsAudioProcessor()
{
}

//==============================================================================
void OBellsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // DSP initialization will be added in Stage 2 (DSP implementation)
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OBellsAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2 (DSP implementation)
}

void OBellsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Pass-through for Stage 1 (DSP implementation happens in Stage 2)
    // Currently synthesizer produces silence (no audio generation yet)

    // Clear output buffer
    buffer.clear();
}

//==============================================================================
juce::AudioProcessorEditor* OBellsAudioProcessor::createEditor()
{
    return new OBellsAudioProcessorEditor(*this);
}

//==============================================================================
void OBellsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OBellsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBellsAudioProcessor();
}
