/*
  ==============================================================================

    O-Contrabass — Audio Processor Implementation
    (Stage 1: Foundation — APVTS shell, silent output, no DSP)

    Parameter ID convention: UPPER_SNAKE_CASE per parameter-spec.md
    (sha256:ae956e9487465dcaa57cf1d1cf6a640f0856614cb2e1b4c93d240cf789490a52).
    This differs from sibling plugins (which use lowerCamelCase) — IDs are
    a frozen contract; renaming breaks DAW automation persistence.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BowedContrabassVoice.h"
#include <cmath>

// Phase 2.4a HR-7 — production-side weak default for harness wedge-math bypass.
// Harness binary overrides this with a strong symbol in tests/render-harness/main.cpp.
// Outside the harness this returns false unconditionally, leaving the production
// Schelleng wedge / calibration-polynomial path always live.
//
// macOS / Linux: weak symbols compose with extern "C" linkage so the harness
// strong symbol takes precedence at link time. Windows MSVC would require
// __declspec(selectany) — out of scope for Phase 2.4a (O-Contrabass macOS-AU first).
extern "C" __attribute__((weak)) bool isMatrixStabilityModeActive() noexcept
{
    return false;
}

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
        NR(0.0f, 50.0f, 0.1f),             0.0f));    // Phase 2.3 — default flipped 12.0 → 0.0 to preserve Phase 2.2 strict byte-equal regression bar (HR-1 short-circuit). Mirrors EXPRESSION_MACRO Q7a precedent.
    layout.add(std::make_unique<APF>(juce::ParameterID{"VIBRATO_ONSET", 1},    "Vibrato Onset",
        NR(0.0f, 3000.0f, 1.0f, 0.5f),     600.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SLOW_LFO_RATE", 1},    "Slow Bow LFO Rate",
        NR(0.05f, 2.0f, 0.001f),           0.3f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SLOW_LFO_DEPTH", 1},   "Slow Bow LFO Depth",
        NR(0.0f, 1.0f, 0.001f),            0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"EXPRESSION_MACRO", 1}, "Expression Macro",
        NR(0.0f, 1.0f, 0.001f),            0.0f));   // Phase 2.3 Q7a — preserves Phase 2.2 strict byte-equal regression bar.

    // -- Drone Features (2 params) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"INFINITE_SUSTAIN", 1}, "Infinite Sustain",
        NR(0.0f, 1.0f, 0.001f),            0.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"SUB_HARMONICS", 1},    "Sub-Harmonics",
        NR(0.0f, 1.0f, 0.001f),            0.0f));

    // -- Output (1 param) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"WIDTH", 1},            "Width",
        NR(0.0f, 2.0f, 0.001f),            1.0f));

    // -- Output Chain (Phase 2.6a additions) --
    layout.add(std::make_unique<APF>(juce::ParameterID{"MASTER_SAT_AMOUNT", 1},  "Master Saturator",
        NR(0.0f, 1.0f, 0.001f),            0.50f));   // 50% wet/dry default
    layout.add(std::make_unique<APF>(juce::ParameterID{"LIMITER_CEILING_DB", 1}, "Limiter Ceiling",
        NR(-6.0f, 0.0f, 0.01f),            -0.3f));   // -0.3 dBFS per CONTEXT rev-11 Q4 LOCKED

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
    // Phase 2.6b R40a — pass tuningEngine ptr to voice so noteStarted /
    // notePitchbendChanged can resolve frequency through the engine
    // (ESCALATION-RP1 voice-side ratio multiplication; cache field Q17).
    // tuningEngine is declared BEFORE synth in the header → constructed
    // first → pointer is valid here (Risk #32 mitigation).
    synth.addVoice(new BowedContrabassVoice(&parameters, &tuningEngine));

    // MPE legacy mode for non-MPE DAWs (RESEARCH §5 pitfall #8).
    // Pitchbend range 24 semitones, channels 1..16 — covers omni MIDI input.
    synth.enableLegacyMode(/*pitchbendRange*/ 24, juce::Range<int>(1, 16));

    // Phase 2.6b R40a — APVTS Listener registration for TUNING_SYSTEM Choice.
    // Initial seed: dispatch parameterChanged once to set initial mode.
    parameters.addParameterListener ("TUNING_SYSTEM", this);
    parameterChanged ("TUNING_SYSTEM",
                      parameters.getRawParameterValue ("TUNING_SYSTEM")->load());
}

OContrabassAudioProcessor::~OContrabassAudioProcessor()
{
    parameters.removeParameterListener ("TUNING_SYSTEM", this);
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

    // Phase 2.6a — master output chain prepare.
    masterSaturator.prepare(sampleRate);
    masterLimiter.prepare(sampleRate);
    stereoWidth.prepare(sampleRate, samplesPerBlock);
    outputGainSmoothed.reset(sampleRate, 0.030);
    outputGainSmoothed.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(parameters.getRawParameterValue("OUTPUT_GAIN")->load()));

   #if defined (OCBS_DISABLE_DECORRELATOR)
    // Phase 2.6a-bis Risk #22 — bit-equivalence build path. Force APVTS to
    // bypass values (sat=0 / limiter=0 dB / width=1.0) and seed master-chain
    // smoothers immediately so default-state output matches Phase 2.5 sha256s
    // byte-for-byte (output chain becomes a mathematical no-op when
    // decorrelator is off at compile time AND user-set bypass params are
    // active). Test-only build path; production builds compile this block
    // out entirely and preserve the post-Phase-2.6a R39 audible-golden bar.
    if (auto* p = parameters.getParameter("MASTER_SAT_AMOUNT"))
        p->setValueNotifyingHost(0.0f);                  // norm 0 → raw 0.0
    if (auto* p = parameters.getParameter("LIMITER_CEILING_DB"))
        p->setValueNotifyingHost(1.0f);                  // norm 1.0 → raw 0.0 dB
    if (auto* p = parameters.getParameter("WIDTH"))
        p->setValueNotifyingHost(0.5f);                  // norm 0.5 → raw 1.0
    masterSaturator.setAmountImmediate(0.0f);
    masterLimiter.setCeilingDbImmediate(0.0f);
   #endif

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
    // Phase 2.6a — master output chain reset.
    masterSaturator.reset();
    masterLimiter.reset();
    stereoWidth.reset();
    outputGainSmoothed.reset(1.0f);
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

    // ─── Phase 2.6a — Master Output Chain ──────────────────────────────────
    // Voice writes mono L=R clone (no per-voice OUTPUT_GAIN applied).
    // Chain: Saturator → Limiter → StereoWidth → OUTPUT_GAIN.

    // Step 10: Master Saturator (polynomial x − x³/3, wet/dry mix).
    masterSaturator.setAmount(parameters.getRawParameterValue("MASTER_SAT_AMOUNT")->load());
    masterSaturator.processBlock(buffer);

    // Step 11: Zero-latency feedforward limiter (3 ms attack / 50 ms release).
    masterLimiter.setCeilingDb(parameters.getRawParameterValue("LIMITER_CEILING_DB")->load());
    masterLimiter.processBlock(buffer);

    // Step 12: Stereo width (allpass decorrelator + M/S width).
    stereoWidth.setWidth(parameters.getRawParameterValue("WIDTH")->load());
    stereoWidth.processBlock(buffer);

    // Step 13: Output Gain (relocated from voice-side; ARCHITECTURE §258 final stage).
    const float gainTarget = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("OUTPUT_GAIN")->load());
    outputGainSmoothed.setTargetValue(gainTarget);

    {
        const int numSamples = buffer.getNumSamples();
        const int numChans   = buffer.getNumChannels();
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outputGainSmoothed.getNextValue();
            for (int ch = 0; ch < numChans; ++ch)
                buffer.getWritePointer(ch)[i] *= g;
        }
    }
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
// Phase 2.3 R29 — harness instrumentation accessor (lastSafeDepth read).
BowedContrabassVoice* OContrabassAudioProcessor::getActiveVoice() noexcept
{
    if (synth.getNumVoices() == 0)
        return nullptr;
    return dynamic_cast<BowedContrabassVoice*> (synth.getVoice(0));
}

//==============================================================================
// Phase 2.6b R40a — APVTS Listener: TUNING_SYSTEM Choice → TuningEngine::Mode.
// ESCALATION-MTS1 LOCK: defer the actual setMode call to the message thread
// via MessageManager::callAsync. setMode briefly holds intervalMutex inside
// rebuildFrequencyTable — not RT-safe. callAsync is RT-safe to invoke from
// the audio thread per JUCE docs, so this callback is safe even under
// audio-thread parameter automation fuzz (pluginval-10 Parameter thread
// safety). Choice index → module Mode index swap (plugin index 0 "Scala/TUN"
// = module Mode::Scala; plugin index 2 "12-TET" = module Mode::TwelveTET).
void OContrabassAudioProcessor::parameterChanged (const juce::String& parameterID,
                                                  float newValue)
{
    if (parameterID != "TUNING_SYSTEM")
        return;

    const int choiceIdx = static_cast<int> (newValue);
    TuningEngine::Mode mode;
    switch (choiceIdx)
    {
        case 0:  mode = TuningEngine::Mode::Scala;     break;
        case 1:  mode = TuningEngine::Mode::MTSESP;    break;
        case 2:
        default: mode = TuningEngine::Mode::TwelveTET; break;
    }
    juce::MessageManager::callAsync ([this, mode]() { tuningEngine.setMode (mode); });
}

//==============================================================================
// Phase 2.6b R40a — Scala/TUN file-load entry point (ESCALATION-FPK1).
// Public method invoked from the harness `--microtonal --scl <path>` handler
// on the main thread before render begins; Stage 3 GUI Editor replaces this
// invocation with juce::FileChooser::launchAsync + AsyncUpdater callback.
// Module's loadScalaFile briefly holds intervalMutex during
// setCustomIntervals → rebuildFrequencyTable; main-thread invocation is the
// design contract.
bool OContrabassAudioProcessor::loadScalaFile (const juce::File& sclFile)
{
    return tuningEngine.loadScalaFile (sclFile);
}

//==============================================================================
// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OContrabassAudioProcessor();
}
