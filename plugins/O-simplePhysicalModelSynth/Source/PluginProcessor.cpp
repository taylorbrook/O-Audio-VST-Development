/*
  ==============================================================================

    O-simplePhysicalModelSynth - Audio Processor (implementation)

    Stage 1 (Foundation): silent synth shell. Builds the full 17-parameter APVTS
    (verbatim from the locked parameter-spec.md) and persists it via plain APVTS
    XML round-trip. 16-voice juce::Synthesiser with header-only silent voices;
    processBlock clears the buffer and renders the (currently silent) voices into
    it (no audio until Stage 2).

    ⚠ D3 hazard (RESEARCH §1/§3): the percent params are stored on an explicit
    0–100 range — NOT O-simpleFM's 0–1 unitRange(). Copying unitRange() here would
    silently break the zero-drift contract (wrong endpoints + wrong defaults).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "FactoryPresets.h"

// The WebView editor is only compiled into the plugin TU (JUCE_WEB_BROWSER=1).
// The render-harness compiles this file at JUCE_WEB_BROWSER=0 and does NOT compile
// PluginEditor.cpp — guarding the include keeps the harness free of WebView types.
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"
#endif

namespace
{
    // Perceptual skew for amp-envelope time params (matches the O-simpleFM idiom).
    // Endpoints/defaults are unchanged by skew, so the zero-drift contract holds —
    // skew only reshapes the automation/control curve between the fixed endpoints.
    constexpr float kTimeSkew = 0.35f;

    // ⚠ D3: locked 0–100 percent range (NOT 0–1). All eight percent params use this.
    juce::NormalisableRange<float> percentRange()
    {
        return { 0.0f, 100.0f, 0.01f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimplePhysicalModelSynthAudioProcessor::createParameterLayout()
{
    using namespace ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Excitation (how energy is injected) -------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { excitationType, 1 }, "Excitation Type",
        juce::StringArray { "Pluck", "Strike", "Bow" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { excitationPosition, 1 }, "Excitation Position",
        percentRange(), 25.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { excitationColor, 1 }, "Excitation Color",
        percentRange(), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { bowForce, 1 }, "Bow Force",
        percentRange(), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Resonator (what carries the energy) -------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { resonatorType, 1 }, "Resonator Type",
        juce::StringArray { "String", "Modal" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { stringModel, 1 }, "String Model",
        juce::StringArray { "Karplus-Strong", "Waveguide" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { inharmonicity, 1 }, "Inharmonicity",
        percentRange(), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modeBrightness, 1 }, "Mode Brightness",
        percentRange(), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Material / Damping (how energy is lost) ---------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { damping, 1 }, "Damping",
        percentRange(), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { decay, 1 }, "Decay",
        percentRange(), 70.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Material is a META parameter: moving it writes BOTH damping and decay (the
    // steel↔nylon macro, parameterChanged below). withMeta(true) tells the host/AU
    // it changes other params — without it auval's param-stability check fails
    // ("Meta Param Flag is NOT set on a parameter that will change other params").
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { material, 1 }, "Material",
        percentRange(), 30.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%").withMeta (true)));

    //--- Tuning ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { coarseTune, 1 }, "Coarse Tune", -24, 24, 0,
        juce::AudioParameterIntAttributes().withLabel ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { fineTune, 1 }, "Fine Tune",
        juce::NormalisableRange<float> { -100.0f, 100.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("cents")));

    //--- Amp + dynamics ----------------------------------------------------
    // Mild perceptual skew on the time params (endpoints/defaults intact).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack",
        juce::NormalisableRange<float> { 0.0f, 2.0f, 0.0001f, kTimeSkew }, 0.001f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release",
        juce::NormalisableRange<float> { 0.0f, 5.0f, 0.0001f, kTimeSkew }, 0.2f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToBrightness, 1 }, "Velocity -> Brightness",
        percentRange(), 60.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    //--- Output ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, -6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimplePhysicalModelSynthAudioProcessor::OSimplePhysicalModelSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager (parameters, "O-simplePhysicalModelSynth")
{
    // Seed the 6 concept-isolating factory presets (FUNC-07). Raw values are
    // converted to normalized in FactoryPresets::build via convertTo0to1.
    presetManager.initializeFactoryPresets (FactoryPresets::build (parameters));

    // Pre-allocate all voices up front (no audio-thread allocation later). Each
    // voice carries its index so its noise generators get a deterministic seed.
    for (int i = 0; i < kNumVoices; ++i)
    {
        auto* v = new PhysicalModelVoice();
        v->setVoiceIndex (i);
        v->setVizTap (&viz);
        v->setTriggerCounter (&triggerSeqCounter);
        synth.addVoice (v);
    }

    synth.addSound (new PhysicalModelSound());   // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);

    cacheParamPointers();
    parameters.addParameterListener (ParamIDs::material, this);   // Material macro
}

OSimplePhysicalModelSynthAudioProcessor::~OSimplePhysicalModelSynthAudioProcessor()
{
    parameters.removeParameterListener (ParamIDs::material, this);
    cancelPendingUpdate();   // AsyncUpdater must not have a queued macro apply at teardown
}

//==============================================================================
void OSimplePhysicalModelSynthAudioProcessor::cacheParamPointers()
{
    using namespace ParamIDs;
    raw.excitationType     = parameters.getRawParameterValue (excitationType);
    raw.excitationPosition = parameters.getRawParameterValue (excitationPosition);
    raw.excitationColor    = parameters.getRawParameterValue (excitationColor);
    raw.bowForce           = parameters.getRawParameterValue (bowForce);
    raw.resonatorType      = parameters.getRawParameterValue (resonatorType);
    raw.inharmonicity      = parameters.getRawParameterValue (inharmonicity);
    raw.modeBrightness     = parameters.getRawParameterValue (modeBrightness);
    raw.damping            = parameters.getRawParameterValue (damping);
    raw.decay              = parameters.getRawParameterValue (decay);
    raw.coarseTune         = parameters.getRawParameterValue (coarseTune);
    raw.fineTune           = parameters.getRawParameterValue (fineTune);
    raw.ampAttack          = parameters.getRawParameterValue (ampAttack);
    raw.ampRelease         = parameters.getRawParameterValue (ampRelease);
    raw.velToBrightness    = parameters.getRawParameterValue (velToBrightness);
    raw.outputLevel        = parameters.getRawParameterValue (outputLevel);
}

//==============================================================================
// Build the per-block voice param snapshot from the cached atomics. `damping`
// and `decay` are read straight from APVTS — the Material macro has already
// written them on the message thread, so they are the EFFECTIVE values here.
PhysicalModelParams OSimplePhysicalModelSynthAudioProcessor::readParams() const
{
    PhysicalModelParams pp;
    pp.excitationType     = (int) raw.excitationType->load();
    pp.resonatorType      = (int) raw.resonatorType->load();
    pp.excitationPosition =       raw.excitationPosition->load();
    pp.excitationColor    =       raw.excitationColor->load();
    pp.bowForce           =       raw.bowForce->load();
    pp.inharmonicity      =       raw.inharmonicity->load();
    pp.modeBrightness     =       raw.modeBrightness->load();
    pp.damping            =       raw.damping->load();
    pp.decay              =       raw.decay->load();
    pp.coarseTune         = (int) raw.coarseTune->load();
    pp.fineTune           =       raw.fineTune->load();
    pp.ampAttack          =       raw.ampAttack->load();
    pp.ampRelease         =       raw.ampRelease->load();
    pp.velToBrightness    =       raw.velToBrightness->load();
    return pp;
}

//==============================================================================
// Material macro. material ∈ [0,100]: 0 = steel (bright/long), 100 = nylon
// (dark/short). Writes BOTH visible knobs so they truthfully track the macro
// (RESEARCH §2.4). Only fires on `material` changes → no recursion.
//
// Threading (WR-02): this listener fires synchronously on whatever thread changed
// the value — host automation lands on the AUDIO thread, where setValueNotifyingHost
// (→ host performEdit) is a VST3 violation. Message thread applies immediately
// (preset loads are message-thread and rely on apply ORDER — CR-03); any other
// thread stashes the targets and defers to handleAsyncUpdate.
void OSimplePhysicalModelSynthAudioProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID != ParamIDs::material)
        return;

    // CR-03 (state path): session restore delivers all 17 params explicitly —
    // re-deriving damping/decay from the restored material would stomp them.
    if (restoringState.load())
        return;

    const float m  = juce::jlimit (0.0f, 100.0f, newValue) / 100.0f;
    const float fc = std::exp (juce::jmap (m, std::log (10000.0f), std::log (2000.0f)));  // 10 k…2 k
    const float g  = juce::jmap (m, 0.995f, 0.93f);                                       // long…short

    float dampingPct = 100.0f * std::log (12000.0f / fc) / std::log (12000.0f / 1500.0f);
    float decayPct   = 100.0f * (g - 0.80f) / (0.999f - 0.80f);
    pendingDamping.store (juce::jlimit (0.0f, 100.0f, dampingPct));
    pendingDecay.store   (juce::jlimit (0.0f, 100.0f, decayPct));

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        applyMaterialMacro();
    else
        triggerAsyncUpdate();
}

void OSimplePhysicalModelSynthAudioProcessor::handleAsyncUpdate()
{
    if (! restoringState.load())
        applyMaterialMacro();
}

void OSimplePhysicalModelSynthAudioProcessor::applyMaterialMacro()
{
    if (auto* dp = parameters.getParameter (ParamIDs::damping))
        dp->setValueNotifyingHost (dp->convertTo0to1 (pendingDamping.load()));
    if (auto* dc = parameters.getParameter (ParamIDs::decay))
        dc->setValueNotifyingHost (dc->convertTo0to1 (pendingDecay.load()));
}

//==============================================================================
void OSimplePhysicalModelSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // No oversampling at v1.0 (D4) — report zero added latency. setLatencySamples
    // is the JUCE-8 way (getLatencySamples() is non-virtual / not overridable).
    setLatencySamples (0);

    synth.setCurrentPlaybackSampleRate (sampleRate);

    // On-screen-keyboard MIDI runs in host sample space — reset the collector at the
    // host rate so queued note on/off events land at the right block offsets.
    midiCollector.reset (sampleRate);

    // juce::SynthesiserVoice has no virtual prepareToPlay in JUCE 8 — dispatch the
    // voice's non-virtual prepare via dynamic_cast.
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* pv = dynamic_cast<PhysicalModelVoice*> (synth.getVoice (v)))
            pv->prepareToPlay (sampleRate, samplesPerBlock);

    outputGain.reset (sampleRate, 0.02);   // 20 ms ramp on the master level
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (raw.outputLevel->load()));
}

void OSimplePhysicalModelSynthAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
}

bool OSimplePhysicalModelSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

//==============================================================================
// On-screen keyboard → synth. Builds a note on/off with a seconds timestamp (the
// MidiMessageCollector wants seconds) and queues it for the next processBlock drain.
void OSimplePhysicalModelSynthAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    midiCollector.addMessageToQueue (msg);
}

void OSimplePhysicalModelSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();   // synth voices ADD into the cleared buffer

    // Merge any on-screen-keyboard notes into the host MIDI stream BEFORE rendering
    // (drained at the top so the synth sees them this block).
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    // Push the current parameter snapshot to every voice before rendering (no
    // per-sample APVTS access on the audio thread).
    const PhysicalModelParams pp = readParams();
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* pv = dynamic_cast<PhysicalModelVoice*> (synth.getVoice (v)))
            pv->setParams (pp);

    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // Master output level (dB→gain), smoothed.
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (raw.outputLevel->load()));
    if (outputGain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float goo = outputGain.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * goo);
        }
    }
    else
    {
        buffer.applyGain (outputGain.getCurrentValue());
    }

    publishViz (numSamples, buffer);
}

//==============================================================================
// Audio-thread copy-only viz publish (PERF-01): master waveform → ring; the lead
// (most-recently-triggered active) voice publishes its loop energy + modal stems.
void OSimplePhysicalModelSynthAudioProcessor::publishViz (int numSamples,
                                                          const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() > 0)
        viz.waveform.write (buffer.getReadPointer (0), numSamples);

    PhysicalModelVoice* lead = nullptr;
    juce::uint64 best = 0;
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* pv = dynamic_cast<PhysicalModelVoice*> (synth.getVoice (v)))
            if (pv->isVoiceActive() && (lead == nullptr || pv->getTriggerSeq() >= best))
            {
                best = pv->getTriggerSeq();
                lead = pv;
            }

    if (lead != nullptr)
        lead->publishViz();
}

//==============================================================================
juce::AudioProcessorEditor* OSimplePhysicalModelSynthAudioProcessor::createEditor()
{
   #if JUCE_WEB_BROWSER
    return new OSimplePhysicalModelSynthAudioProcessorEditor (*this);   // WebView UI (Stage 3)
   #else
    return new juce::GenericAudioProcessorEditor (*this);               // harness build (=0)
   #endif
}

//==============================================================================
void OSimplePhysicalModelSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Route through the preset manager so the current-preset name persists alongside
    // the APVTS tree (getStateAsXml falls back to plain APVTS Xml). Stage 3 / D4.
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary (*xml, destData);
}

void OSimplePhysicalModelSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        // CR-03: the XML carries ALL params — suppress the Material macro so the
        // restored `material` can't re-derive and stomp restored damping/decay.
        restoringState = true;
        presetManager.setStateFromXml (xml.get());
        // VR-01: a macro apply queued from audio-thread automation BEFORE this
        // restore would fire after restoringState clears and stomp the restored
        // damping/decay — kill it while the guard is still up (dtor does the same).
        cancelPendingUpdate();
        restoringState = false;
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimplePhysicalModelSynthAudioProcessor();
}
