/*
  ==============================================================================

    O-Chorus - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout OChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rate", 1}, "Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.35f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1}, "Depth", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voices", 1}, "Voices", 1, 8, 4));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"spread", 1}, "Spread", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"width", 1}, "Width", 0.0f, 1.0f, 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tone", 1}, "Tone",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1}, "Drive", 0.0f, 1.0f, 0.3f));

    return {params.begin(), params.end()};
}

OChorusAudioProcessor::OChorusAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Chorus")
{
    initializeFactoryPresets();
}

void OChorusAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    chorusEngine.prepare(sampleRate, samplesPerBlock);
}

void OChorusAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read all parameters via atomic loads (real-time safe)
    float rate   = parameters.getRawParameterValue("rate")->load();
    float depth  = parameters.getRawParameterValue("depth")->load();
    int   voices = static_cast<int>(parameters.getRawParameterValue("voices")->load());
    float spread = parameters.getRawParameterValue("spread")->load();
    float width  = parameters.getRawParameterValue("width")->load();
    float tone   = parameters.getRawParameterValue("tone")->load();
    float mix    = parameters.getRawParameterValue("mix")->load();
    float drive  = parameters.getRawParameterValue("drive")->load();

    chorusEngine.process(buffer, rate, depth, voices, spread, width, tone, mix, drive);
}

juce::AudioProcessorEditor* OChorusAudioProcessor::createEditor()
{
    return new OChorusAudioProcessorEditor(*this);
}

void OChorusAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OChorusAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

//==============================================================================
// Program (Preset) API
//==============================================================================
int OChorusAudioProcessor::getNumPrograms()
{
    auto list = presetManager.getPresetList();
    return juce::jmax(1, list.size());
}

int OChorusAudioProcessor::getCurrentProgram()
{
    auto list = presetManager.getPresetList();
    return juce::jmax(0, list.indexOf(presetManager.getCurrentPresetName()));
}

void OChorusAudioProcessor::setCurrentProgram(int index)
{
    auto list = presetManager.getPresetList();
    if (index >= 0 && index < list.size())
        presetManager.loadPreset(list[index]);
}

const juce::String OChorusAudioProcessor::getProgramName(int index)
{
    auto list = presetManager.getPresetList();
    if (index >= 0 && index < list.size())
        return list[index];
    return {};
}

//==============================================================================
// Factory Presets
//==============================================================================
void OChorusAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();

    if (factoryDir.isDirectory() && factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
        return;

    std::vector<OuariconPresetManager::FactoryPresetDef> presets = {
        {
            "Classic",
            {{"rate", 0.432f}, {"depth", 0.4f}, {"voices", 0.143f},
             {"spread", 0.3f}, {"width", 0.7f}, {"tone", 0.5f},
             {"mix", 0.5f}, {"drive", 0.2f}},
            juce::var()
        },
        {
            "Lush",
            {{"rate", 0.352f}, {"depth", 0.6f}, {"voices", 0.714f},
             {"spread", 0.8f}, {"width", 0.9f}, {"tone", 0.4f},
             {"mix", 0.6f}, {"drive", 0.3f}},
            juce::var()
        },
        {
            "Shimmer",
            {{"rate", 0.722f}, {"depth", 0.3f}, {"voices", 0.429f},
             {"spread", 0.5f}, {"width", 0.8f}, {"tone", 0.75f},
             {"mix", 0.4f}, {"drive", 0.15f}},
            juce::var()
        },
        {
            "Ensemble",
            {{"rate", 0.517f}, {"depth", 0.5f}, {"voices", 1.0f},
             {"spread", 1.0f}, {"width", 1.0f}, {"tone", 0.35f},
             {"mix", 0.55f}, {"drive", 0.25f}},
            juce::var()
        },
        {
            "Vibrato",
            {{"rate", 0.834f}, {"depth", 0.7f}, {"voices", 0.0f},
             {"spread", 0.0f}, {"width", 0.0f}, {"tone", 0.5f},
             {"mix", 1.0f}, {"drive", 0.0f}},
            juce::var()
        },
        {
            "Warm",
            {{"rate", 0.517f}, {"depth", 0.45f}, {"voices", 0.286f},
             {"spread", 0.4f}, {"width", 0.6f}, {"tone", 0.25f},
             {"mix", 0.5f}, {"drive", 0.5f}},
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(presets);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OChorusAudioProcessor();
}
