/*
  ==============================================================================

    O-Contrabass — Audio Processor Implementation
    (Stage 1: Foundation — APVTS shell, silent output, no DSP)

    Parameter ID convention: UPPER_SNAKE_CASE per parameter-spec.md
    (sha256:c47fe7361a55e1d64b906ef7194894f4a2490744b35a644c76b6e1a632282d0d).
    This differs from sibling plugins (which use lowerCamelCase) — IDs are
    a frozen contract; renaming breaks DAW automation persistence.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BowedContrabassVoice.h"
#include <cmath>

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OContrabassAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    using NR  = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // -- Tier 1: Primary Controls (5 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_SPEED", 1},     "Bow Speed",
        NR(0.02f, 1.5f, 0.001f, 0.5f),     0.15f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_PRESSURE", 1},  "Bow Pressure",
        NR(0.05f, 8.0f, 0.01f, 0.5f),      1.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_POSITION", 1},  "Bow Position",
        NR(0.02f, 0.25f, 0.001f),          0.10f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BRIGHTNESS", 1},    "Brightness",
        NR(80.0f, 12000.0f, 1.0f, 0.25f),  4500.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"OUTPUT_GAIN", 1},   "Output Level",
        NR(-60.0f, 12.0f, 0.1f),           0.0f));

    // -- Tier 2: Secondary Controls (5 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"ROSIN", 1},         "Rosin",
        NR(0.0f, 1.0f, 0.001f),            0.65f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_NOISE", 1},     "Bow Noise",
        NR(0.0f, 1.0f, 0.001f),            0.35f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BODY_SIZE", 1},     "Body Size",
        NR(0.0f, 1.0f, 0.001f),            0.75f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BODY_DAMPING", 1},  "Body Damping",
        NR(0.0f, 1.0f, 0.001f),            0.40f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BODY_MIX", 1},      "Body Mix",
        NR(0.0f, 1.0f, 0.001f),            0.80f));

    // -- Tier 3: String Configuration (3 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"STRING_TENSION", 1},   "String Tension",
        NR(0.0f, 1.0f, 0.001f),            0.50f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"STRING_STIFFNESS", 1}, "String Stiffness",
        NR(0.0f, 1.0f, 0.001f),            0.30f));
    layout.add(std::make_unique<API>(juce::ParameterID{"ACTIVE_STRINGS", 1},   "Active Strings",
        1, 4, 4));

    // -- Per-String Detune (4 params, scordatura / just-intonation drones) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_E", 1}, "E String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_A", 1}, "A String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_D", 1}, "D String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"DETUNE_G", 1}, "G String Detune",
        NR(-1200.0f, 1200.0f, 0.1f),       0.0f));

    // -- Expression (6 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_RATE", 1},     "Vibrato Rate",
        NR(0.1f, 12.0f, 0.01f),            5.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_DEPTH", 1},    "Vibrato Depth",
        NR(0.0f, 50.0f, 0.1f),             12.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_ONSET", 1},    "Vibrato Onset",
        NR(0.0f, 3000.0f, 1.0f, 0.5f),     600.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SLOW_LFO_RATE", 1},    "Slow Bow LFO Rate",
        NR(0.05f, 2.0f, 0.001f),           0.3f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SLOW_LFO_DEPTH", 1},   "Slow Bow LFO Depth",
        NR(0.0f, 1.0f, 0.001f),            0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"EXPRESSION_MACRO", 1}, "Expression Macro",
        NR(0.0f, 1.0f, 0.001f),            0.50f));

    // -- Drone Features (2 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"INFINITE_SUSTAIN", 1}, "Infinite Sustain",
        NR(0.0f, 1.0f, 0.001f),            0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SUB_HARMONICS", 1},    "Sub-Harmonics",
        NR(0.0f, 1.0f, 0.001f),            0.0f));

    // -- Output (1 param) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"WIDTH", 1},            "Width",
        NR(0.0f, 2.0f, 0.001f),            1.0f));

    // -- Microtonal Tuning (3 params, Ouaricon convention) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"REFERENCE_PITCH", 1},  "Reference Pitch",
        NR(220.0f, 880.0f, 0.01f),         440.0f));
    layout.add(std::make_unique<APC>(juce::ParameterID{"TUNING_SYSTEM", 1},    "Tuning System",
        juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" }, 2));
    layout.add(std::make_unique<APB>(juce::ParameterID{"NOTE_EXPRESSION", 1},  "Note Expression",
        true));

    return layout;
}

//==============================================================================
OContrabassAudioProcessor::OContrabassAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Phase 2.1a: single E1 voice. Multi-voice / per-string voicing lands in 2.2.
    synth.addVoice(new BowedContrabassVoice(&parameters));

    // MPE legacy mode for non-MPE DAWs (RESEARCH §5 pitfall #8).
    // Pitchbend range 24 semitones, channels 1..16 — covers omni MIDI input.
    synth.enableLegacyMode(/*pitchbendRange*/ 24, juce::Range<int>(1, 16));
}

//==============================================================================
bool OContrabassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OContrabassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<BowedContrabassVoice*>(synth.getVoice(i)))
            v->prepareToPlay(sampleRate, samplesPerBlock);

    // Report oversampler latency to host (RESEARCH §3.1; CLAUDE.md memory:
    // getLatencySamples() is non-virtual in JUCE 8 — never override; always use
    // setLatencySamples in prepareToPlay).
    if (auto* v = dynamic_cast<BowedContrabassVoice*>(synth.getVoice(0)))
        setLatencySamples(static_cast<int>(std::ceil(v->getOversamplingLatency())));
    else
        setLatencySamples(0);
}

void OContrabassAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2.
}

void OContrabassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Synth pattern: clear any stray output channels not backed by inputs.
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // MPESynthesiser additively writes into outputBuffer — clear first so we
    // don't accumulate prior block content (synth.addVoice path uses addSample).
    buffer.clear();

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
void OContrabassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void OContrabassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessorEditor* OContrabassAudioProcessor::createEditor()
{
    return new OContrabassAudioProcessorEditor(*this);
}

//==============================================================================
// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OContrabassAudioProcessor();
}
