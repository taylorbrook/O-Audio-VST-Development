#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "HiHatVoice.h"
#include "HiHatSound.h"

OrganicHatsAudioProcessor::OrganicHatsAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Add 16 voices for polyphony (8 closed + 8 open typical use)
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new HiHatVoice(parameters));

    // Add hi-hat sound descriptor
    synth.addSound(new HiHatSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout OrganicHatsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Closed Hi-Hat parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CLOSED_TONE", 1 },
        "Closed Tone",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CLOSED_DECAY", 1 },
        "Closed Decay",
        juce::NormalisableRange<float>(20.0f, 200.0f, 0.1f),
        80.0f,
        "ms"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CLOSED_NOISE_COLOR", 1 },
        "Closed Noise Color",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    // Open Hi-Hat parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OPEN_TONE", 1 },
        "Open Tone",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OPEN_RELEASE", 1 },
        "Open Release",
        juce::NormalisableRange<float>(100.0f, 1000.0f, 0.1f),
        400.0f,
        "ms"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OPEN_NOISE_COLOR", 1 },
        "Open Noise Color",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f,
        "%"
    ));

    return layout;
}

OrganicHatsAudioProcessor::~OrganicHatsAudioProcessor()
{
}

void OrganicHatsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare synthesiser with sample rate
    synth.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare all voices for DSP processing (Phase 4.2)
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<HiHatVoice*>(synth.getVoice(i)))
            voice->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void OrganicHatsAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 4
}

void OrganicHatsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer before synthesiser adds to it
    buffer.clear();

    // Implement choke logic (Phase 4.3): Closed hi-hat cuts open hi-hat
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int noteNumber = msg.getNoteNumber();

            // If closed hi-hat (C1 = 36), choke all active open hi-hats
            if (noteNumber == 36)
            {
                for (int i = 0; i < synth.getNumVoices(); ++i)
                {
                    if (auto* voice = dynamic_cast<HiHatVoice*>(synth.getVoice(i)))
                    {
                        if (voice->isOpen() && voice->isVoiceActive())
                        {
                            voice->forceRelease();
                        }
                    }
                }
            }
        }
    }

    // Render MIDI-triggered hi-hat voices
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* OrganicHatsAudioProcessor::createEditor()
{
    return new OrganicHatsAudioProcessorEditor(*this);
}

void OrganicHatsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OrganicHatsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrganicHatsAudioProcessor();
}
