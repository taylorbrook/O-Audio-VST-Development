/*
  ==============================================================================

    O-simpleBeatmaker - Audio Processor (implementation)

    Stage 1 (Foundation): a silent, loadable shell. The full 42-parameter APVTS
    is laid out here, and a custom 6x32 step-grid (std::atomic<uint8_t>) is
    persisted alongside the APVTS state in a "PATTERN" ValueTree child. No DSP
    (processBlock outputs silence) and no WebView — those arrive in Stages 2 & 3.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // 0–1 normalized range (stored 0–1; the Stage-3 UI scales for display).
    juce::NormalisableRange<float> unitRange()
    {
        return { 0.0f, 1.0f, 0.0001f };
    }

    // -60..0 dB range; -60 dB represents "-inf" (silence).
    juce::NormalisableRange<float> dbRange()
    {
        return { -60.0f, 0.0f, 0.1f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleBeatmakerAudioProcessor::createParameterLayout()
{
    using namespace OSimpleBeatmaker;
    using namespace OSimpleBeatmaker::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Sequencer / timing-feel (5) ---------------------------------------
    // Stored normalized 0–1. Display: swing -> 0-75%, humanize/quantize -> 0-100%.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { swing, 1 }, "Swing", unitRange(), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { humanize, 1 }, "Humanize", unitRange(), 0.0f));

    // Default 100% = dead tight (humanize fully removed; swing still survives).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { quantizeStrength, 1 }, "Quantize Strength", unitRange(), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { patternLength, 1 }, "Pattern Length",
        juce::StringArray { "8", "16", "32" }, 1)); // 16 steps

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { tempo, 1 }, "Tempo",
        juce::NormalisableRange<float> { 40.0f, 240.0f, 0.01f }, 120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("BPM")));

    //--- Per voice (6 voices x 6 params = 36) ------------------------------
    for (int v = 0; v < kNumVoices; ++v)
    {
        const juce::String name (kVoiceName[(size_t) v]);

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufTune), 1 }, name + " Tune",
            juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("st")));

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufDecay), 1 }, name + " Decay",
            unitRange(), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufTone), 1 }, name + " Tone",
            unitRange(), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { voiceParamID (v, sufLevel), 1 }, name + " Level",
            dbRange(), 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));

        params.push_back (std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { voiceParamID (v, sufMute), 1 }, name + " Mute", false));

        params.push_back (std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { voiceParamID (v, sufSolo), 1 }, name + " Solo", false));
    }

    //--- Master (1) --------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        dbRange(), 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleBeatmakerAudioProcessor::OSimpleBeatmakerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // std::atomic's default constructor does not zero-initialize — do it explicitly.
    clearGrid();
}

OSimpleBeatmakerAudioProcessor::~OSimpleBeatmakerAudioProcessor() = default;

//==============================================================================
void OSimpleBeatmakerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Zero added latency — the Stage-2 scheduling "lookahead" is bookkeeping, not
    // an output delay line. getLatencySamples() is non-virtual in JUCE 8.
    setLatencySamples (0);
}

void OSimpleBeatmakerAudioProcessor::releaseResources() {}

//==============================================================================
bool OSimpleBeatmakerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: output-only. Accept mono or stereo output, no input bus.
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono()
        && out != juce::AudioChannelSet::stereo())
        return false;

    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return true;
}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    // Foundation: silent shell. First audio (voices + sequencer) arrives in Stage 2.
    buffer.clear();
}

//==============================================================================
// Step-grid API (message thread writes / audio thread reads via atomics).
void OSimpleBeatmakerAudioProcessor::setStep (int voice, int step, int velocity) noexcept
{
    if (! inRange (voice, step))
        return;

    grid[(size_t) cellIndex (voice, step)]
        .store ((uint8_t) juce::jlimit (0, 127, velocity), std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::setStepVelocity (int voice, int step, int velocity) noexcept
{
    if (! inRange (voice, step))
        return;

    // Only meaningful for an already-on cell — keep "off" off.
    auto& cell = grid[(size_t) cellIndex (voice, step)];
    if (cell.load (std::memory_order_relaxed) == 0)
        return;

    cell.store ((uint8_t) juce::jlimit (1, 127, velocity), std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::toggleStep (int voice, int step) noexcept
{
    if (! inRange (voice, step))
        return;

    auto& cell = grid[(size_t) cellIndex (voice, step)];
    const uint8_t cur = cell.load (std::memory_order_relaxed);
    cell.store (cur > 0 ? (uint8_t) 0 : (uint8_t) kDefaultStepVelocity, std::memory_order_relaxed);
}

int OSimpleBeatmakerAudioProcessor::getStep (int voice, int step) const noexcept
{
    if (! inRange (voice, step))
        return 0;

    return grid[(size_t) cellIndex (voice, step)].load (std::memory_order_relaxed);
}

void OSimpleBeatmakerAudioProcessor::clearGrid() noexcept
{
    for (auto& cell : grid)
        cell.store (0, std::memory_order_relaxed);
}

//==============================================================================
// PATTERN <-> ValueTree. The grid is encoded as a base64 byte blob (JUCE's own
// MemoryBlock format, used symmetrically for save+restore — this is NOT
// JS-interop base64, so the standard-vs-JUCE encoding distinction is moot here).
juce::ValueTree OSimpleBeatmakerAudioProcessor::buildPatternTree() const
{
    juce::ValueTree pattern ("PATTERN");
    pattern.setProperty ("rows", kNumVoices, nullptr);
    pattern.setProperty ("cols", kMaxSteps, nullptr);

    juce::MemoryBlock mb ((size_t) (kNumVoices * kMaxSteps));
    auto* bytes = static_cast<uint8_t*> (mb.getData());
    for (int i = 0; i < kNumVoices * kMaxSteps; ++i)
        bytes[i] = grid[(size_t) i].load (std::memory_order_relaxed);

    pattern.setProperty ("cells", mb.toBase64Encoding(), nullptr);
    return pattern;
}

void OSimpleBeatmakerAudioProcessor::restorePatternTree (const juce::ValueTree& pattern)
{
    clearGrid();
    if (! pattern.isValid())
        return;

    const int cols = (int) pattern.getProperty ("cols", kMaxSteps);
    const int rows = (int) pattern.getProperty ("rows", kNumVoices);

    juce::MemoryBlock mb;
    if (! mb.fromBase64Encoding (pattern.getProperty ("cells").toString()))
        return;

    const auto* bytes = static_cast<const uint8_t*> (mb.getData());
    const int   avail = (int) mb.getSize();

    // Source stride is the SAVED column count, so this round-trips even if
    // kMaxSteps ever changes. Cells beyond the saved range stay 0 (cleared above).
    for (int v = 0; v < rows && v < kNumVoices; ++v)
        for (int s = 0; s < cols && s < kMaxSteps; ++s)
        {
            const int src = v * cols + s;
            if (src < avail)
                grid[(size_t) cellIndex (v, s)].store (bytes[src], std::memory_order_relaxed);
        }
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleBeatmakerAudioProcessor::createEditor()
{
    return new OSimpleBeatmakerAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleBeatmakerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // copyState() returns a COPY; mutating it never touches the live APVTS tree.
    auto state = parameters.copyState();

    // Defensive: never let a stale PATTERN child accumulate across save cycles.
    state.removeChild (state.getChildWithName ("PATTERN"), nullptr);
    state.appendChild (buildPatternTree(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleBeatmakerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (parameters.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);

            restorePatternTree (tree.getChildWithName ("PATTERN"));

            // Keep the live APVTS tree free of PATTERN; the grid lives in atomics.
            tree.removeChild (tree.getChildWithName ("PATTERN"), nullptr);
            parameters.replaceState (tree);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleBeatmakerAudioProcessor();
}
