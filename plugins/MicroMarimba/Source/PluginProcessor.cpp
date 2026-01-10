/*
  ==============================================================================

    Ouaricon Marimba - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MarimbaSound.h"
#include "MarimbaVoice.h"
#include "TuningEngine.h"

juce::AudioProcessorValueTreeState::ParameterLayout MicroMarimbaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MALLET_HARDNESS - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "MALLET_HARDNESS", 1 },
        "Mallet Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // BAR_MATERIAL - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "BAR_MATERIAL", 1 },
        "Bar Material",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // RESONANCE - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "RESONANCE", 1 },
        "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    // TUNING_MODE - Choice (0-2)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "TUNING_MODE", 1 },
        "Tuning Mode",
        juce::StringArray { "12-TET", "Scala", "MTS-ESP" },
        0  // Default: 12-TET
    ));

    // REFERENCE_PITCH - Float (400.0 to 480.0 Hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "REFERENCE_PITCH", 1 },
        "Reference Pitch",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    // VEL_CURVE - Float (0.0 to 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "VEL_CURVE", 1 },
        "Velocity Curve",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // OUTPUT_GAIN - Float (-24.0 to 12.0 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OUTPUT_GAIN", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    return layout;
}

MicroMarimbaAudioProcessor::MicroMarimbaAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Phase 2.1: Initialize synthesiser with 16 voices
    // Add a single MarimbaSound (all notes can play this sound)
    synthesiser.addSound(new MarimbaSound());

    // Add 16 voices for polyphony
    for (int i = 0; i < 16; ++i)
    {
        synthesiser.addVoice(new MarimbaVoice());
    }

    // Phase 2.3: Pass tuning engine to all voices
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MarimbaVoice*>(synthesiser.getVoice(i)))
        {
            voice->setTuningEngine(&tuningEngine);
        }
    }
}

MicroMarimbaAudioProcessor::~MicroMarimbaAudioProcessor()
{
}

void MicroMarimbaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Phase 2.1: Prepare synthesiser
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // Set sample rate for each voice (needed for envelope timing and oscillator)
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MarimbaVoice*>(synthesiser.getVoice(i)))
        {
            voice->setSampleRate(sampleRate);
        }
    }

    // Phase 2.4: Prepare body resonance
    bodyResonance.prepare(sampleRate, samplesPerBlock);
}

void MicroMarimbaAudioProcessor::releaseResources()
{
    // Phase 2.4: Reset body resonance
    bodyResonance.reset();
}

void MicroMarimbaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer (synth generates audio from scratch, no input)
    buffer.clear();

    // Inject any pending MIDI from UI keyboard
    {
        const juce::ScopedLock lock(midiLock);
        midiMessages.addEvents(pendingUiMidi, 0, -1, 0);
        pendingUiMidi.clear();
    }

    // Extract note-on events for UI notification (pitch circle flash)
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            lastPlayedNote.store(msg.getNoteNumber());
            hasNewNote.store(true);
        }
    }

    // Phase 2.2: Read parameters (atomic, real-time safe)
    auto* outputGainParam = parameters.getRawParameterValue("OUTPUT_GAIN");
    float outputGainDB = outputGainParam->load();

    auto* velCurveParam = parameters.getRawParameterValue("VEL_CURVE");
    float velCurve = velCurveParam->load();

    auto* malletHardnessParam = parameters.getRawParameterValue("MALLET_HARDNESS");
    float malletHardness = malletHardnessParam->load();

    auto* barMaterialParam = parameters.getRawParameterValue("BAR_MATERIAL");
    float barMaterial = barMaterialParam->load();

    auto* resonanceParam = parameters.getRawParameterValue("RESONANCE");
    float resonance = resonanceParam->load();

    // Phase 2.3: Read tuning parameters
    auto* tuningModeParam = parameters.getRawParameterValue("TUNING_MODE");
    int tuningModeInt = static_cast<int>(tuningModeParam->load());

    auto* referencePitchParam = parameters.getRawParameterValue("REFERENCE_PITCH");
    float referencePitch = referencePitchParam->load();

    // Update tuning engine (atomic updates, safe from audio thread)
    tuningEngine.setMode(static_cast<TuningEngine::Mode>(tuningModeInt));
    tuningEngine.setReferencePitch(static_cast<double>(referencePitch));

    // Update all active voices with current parameters
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MarimbaVoice*>(synthesiser.getVoice(i)))
        {
            voice->setOutputGain(outputGainDB);
            voice->setVelocityCurve(velCurve);
            voice->setMalletHardness(malletHardness);
            voice->setBarMaterial(barMaterial);
            voice->setResonance(resonance);
        }
    }

    // Phase 2.2: Render synthesiser (processes MIDI and generates audio)
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Phase 2.4: Apply body resonance (after synth rendering)
    // RESONANCE parameter controls both decay time (in voices) AND body mix
    // Scale to 0.5 to avoid too much wetness (resonance = 1.0 → 50% wet)
    bodyResonance.setMix(resonance * 0.5f);
    bodyResonance.process(buffer);

    // Apply output gain (after all processing)
    float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDB);
    buffer.applyGain(outputGainLinear);
}

juce::AudioProcessorEditor* MicroMarimbaAudioProcessor::createEditor()
{
    return new MicroMarimbaAudioProcessorEditor(*this);
}

void MicroMarimbaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MicroMarimbaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// UI keyboard MIDI injection (called from UI thread)
void MicroMarimbaAudioProcessor::addMidiMessage(const juce::MidiMessage& msg)
{
    const juce::ScopedLock lock(midiLock);
    pendingUiMidi.addEvent(msg, 0);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicroMarimbaAudioProcessor();
}
