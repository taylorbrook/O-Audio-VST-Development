/*
  ==============================================================================

    O-Bassoon - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain.
    First audio: Phase 2.1.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (frozen 10-parameter spec — see parameter-spec-draft.md)
juce::AudioProcessorValueTreeState::ParameterLayout OBassoonAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Vibrato (3) ==========

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibrato_rate", 1 },
        "Vibrato Rate",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f, 1.0f),
        5.0f,
        " Hz"
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibrato_depth", 1 },
        "Vibrato Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        15.0f,
        " cents"
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vibrato_onset", 1 },
        "Vibrato Onset",
        juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f),
        400.0f,
        " ms"
    ));

    // ========== Expression / Dynamics (3) ==========

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "breath", 1 },
        "Breath",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.7f
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 },
        "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack_character", 1 },
        "Attack Character",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f
    ));

    // ========== Envelope (2) ==========

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack_time", 1 },
        "Attack Time",
        juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f),
        300.0f,
        " ms"
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release_time", 1 },
        "Release Time",
        juce::NormalisableRange<float> (0.0f, 3000.0f, 1.0f),
        800.0f,
        " ms"
    ));

    // ========== Voicing / Output (2) ==========

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "voice_count", 1 },
        "Voice Count",
        1, 16, 8
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output_gain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f,
        " dB"
    ));

    return layout;
}

//==============================================================================
OBassoonAudioProcessor::OBassoonAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , parameters (*this, nullptr, "Parameters", createParameterLayout())
{
    // Pre-allocate 16 voices (matches max voice_count).
    // Voice manager will enforce the runtime cap; pre-allocating prevents
    // processBlock allocations when the user raises the cap (PERF-01).
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new BassoonVoice();
        voice->setAPVTS (&parameters);
        voice->setTuningEngine (&tuningEngine);                                // D2: global namespace
        voice->setPendingTuningSource (&vst3Extensions.getPendingTable());     // module-owned table
        synthesiser.addVoice (voice);
    }

    // Single shared sound (accepts all notes / all channels)
    synthesiser.addSound (new BassoonSound());
}

OBassoonAudioProcessor::~OBassoonAudioProcessor() = default;

//==============================================================================
void OBassoonAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    // Modal synthesis is feed-forward; latency = 0 — do NOT call setLatencySamples.
    // (getLatencySamples is non-virtual in JUCE 8; default returns 0.)
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);
}

void OBassoonAudioProcessor::releaseResources()
{
    // Stage 1: nothing to release (no externally-allocated resources).
}

bool OBassoonAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: stereo output only, no input bus.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Reject any input bus (synths are output-only).
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
}

void OBassoonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Synth: clear all channels (no input; voices add to buffer).
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch.
    // MUST run BEFORE renderNextBlock so per-voice startNote sees pending NE deltas.
    vst3Extensions.drainAndUpdate();

    // Render all voices via synthesiser (handles MIDI routing + voice allocation).
    // Stage 1: BassoonVoice::renderNextBlock is a silent no-op — output stays clear.
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
juce::AudioProcessorEditor* OBassoonAudioProcessor::createEditor()
{
    return new OBassoonAudioProcessorEditor (*this);
}

//==============================================================================
void OBassoonAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Standard APVTS XML round-trip.
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void OBassoonAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// Factory function (JUCE plugin entry point)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBassoonAudioProcessor();
}
