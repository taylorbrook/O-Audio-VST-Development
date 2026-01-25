/*
  ==============================================================================

    O-MultiBandCompressor - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Parameter layout creation (BEFORE constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OMultiBandCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ===== GLOBAL PARAMETERS (8) =====

    // INPUT_GAIN: -24 to +24 dB, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "INPUT_GAIN", 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // OUTPUT_GAIN: -24 to +24 dB, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // MIX: 0-100%, default 100%
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MIX", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"
    ));

    // AUTO_MAKEUP: bool, default false
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "AUTO_MAKEUP", 1 },
        "Auto-Makeup",
        false
    ));

    // MS_MODE: choice (Off/Mid/Side/Both), default Off (0)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "MS_MODE", 1 },
        "M/S Mode",
        juce::StringArray { "Off", "Mid", "Side", "Both" },
        0
    ));

    // XOVER1: 20-500 Hz, default 200 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER1", 1 },
        "Crossover 1",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f, 0.3f),
        200.0f,
        "Hz"
    ));

    // XOVER2: 200-5000 Hz, default 2000 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER2", 1 },
        "Crossover 2",
        juce::NormalisableRange<float>(200.0f, 5000.0f, 0.1f, 0.3f),
        2000.0f,
        "Hz"
    ));

    // XOVER3: 2000-16000 Hz, default 8000 Hz (logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "XOVER3", 1 },
        "Crossover 3",
        juce::NormalisableRange<float>(2000.0f, 16000.0f, 0.1f, 0.3f),
        8000.0f,
        "Hz"
    ));

    // ===== PER-BAND PARAMETERS (12 × 4 = 48) =====

    juce::StringArray bandPrefixes = { "LOW", "LOMID", "HIMID", "HIGH" };
    juce::StringArray bandNames = { "Low", "Low-Mid", "High-Mid", "High" };

    for (int i = 0; i < 4; ++i)
    {
        const auto& prefix = bandPrefixes[i];
        const auto& name = bandNames[i];

        // THRESHOLD: -60 to 0 dB, default -20
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_THRESHOLD", 1 },
            name + " Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -20.0f,
            "dB"
        ));

        // RATIO: 1 to 20 (1:1 to 20:1), default 4
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_RATIO", 1 },
            name + " Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
            4.0f,
            ":1"
        ));

        // ATTACK: 0.1 to 200 ms, default 10
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_ATTACK", 1 },
            name + " Attack",
            juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.3f),
            10.0f,
            "ms"
        ));

        // RELEASE: 10 to 2000 ms, default 100
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_RELEASE", 1 },
            name + " Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.3f),
            100.0f,
            "ms"
        ));

        // KNEE: 0 to 24 dB, default 6
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_KNEE", 1 },
            name + " Knee",
            juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
            6.0f,
            "dB"
        ));

        // MAKEUP: -12 to +24 dB, default 0
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_MAKEUP", 1 },
            name + " Makeup",
            juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
            0.0f,
            "dB"
        ));

        // PEAK_RMS: 0-100%, default 50%
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_PEAK_RMS", 1 },
            name + " Peak/RMS",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            50.0f,
            "%"
        ));

        // SOLO: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_SOLO", 1 },
            name + " Solo",
            false
        ));

        // BYPASS: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_BYPASS", 1 },
            name + " Bypass",
            false
        ));

        // SC_HPF: 20-2000 Hz, default 0 (off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_SC_HPF", 1 },
            name + " SC HPF",
            juce::NormalisableRange<float>(0.0f, 2000.0f, 0.1f, 0.3f),
            0.0f,
            "Hz"
        ));

        // SC_LPF: 500-20000 Hz, default 0 (off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "_SC_LPF", 1 },
            name + " SC LPF",
            juce::NormalisableRange<float>(0.0f, 20000.0f, 0.1f, 0.3f),
            0.0f,
            "Hz"
        ));

        // SC_LISTEN: bool, default false
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "_SC_LISTEN", 1 },
            name + " SC Listen",
            false
        ));
    }

    return layout;
}

OMultiBandCompressorAudioProcessor::OMultiBandCompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OMultiBandCompressorAudioProcessor::~OMultiBandCompressorAudioProcessor()
{
}

void OMultiBandCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // DSP initialization will be added in Stage 2
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void OMultiBandCompressorAudioProcessor::releaseResources()
{
    // DSP cleanup will be added in Stage 2
}

void OMultiBandCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Parameter access example (for Stage 2 DSP implementation):
    // auto* inputGainParam = parameters.getRawParameterValue("INPUT_GAIN");
    // float inputGainValue = inputGainParam->load();  // Atomic read (real-time safe)

    // Pass-through for Stage 1 (DSP implementation happens in Stage 2)
    // Audio routing is already handled by JUCE
}

juce::AudioProcessorEditor* OMultiBandCompressorAudioProcessor::createEditor()
{
    return new OMultiBandCompressorAudioProcessorEditor(*this);
}

void OMultiBandCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OMultiBandCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OMultiBandCompressorAudioProcessor();
}
