/*
  ==============================================================================

    O-Marimba - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MarimbaSound.h"
#include "MarimbaVoice.h"
#include "TuningEngine.h"

juce::AudioProcessorValueTreeState::ParameterLayout OMarimbaAudioProcessor::createParameterLayout()
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

    // v1.6.0: STRIKE_POSITION - Float (0.0 to 1.0)
    // Simulates mallet strike location: 0.0 = edge, 0.5 = center, 1.0 = edge
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STRIKE_POSITION", 1 },
        "Strike Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // v1.6.0: OVERTONE_DAMPING - Float (0.0 to 1.0)
    // Controls how quickly upper modes decay relative to fundamental
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "OVERTONE_DAMPING", 1 },
        "Overtone Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // v1.6.0: TONE - Float (0.0 to 1.0)
    // Post-synthesis brightness control via lowpass filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TONE", 1 },
        "Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.75f
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

    // v1.8.0: Analog EQ Unit parameters (effects tab)
    auto eqParams = AnalogEQUnit::createParameterLayout("fx_eq_");
    for (auto& param : eqParams)
        layout.add(std::move(param));

    // v1.8.1: EQ master bypass (off by default = EQ bypassed)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "fx_eq_enabled", 1 },
        "EQ Enabled",
        false  // Default OFF = bypassed
    ));

    // v1.9.0: Compressor Unit parameters (effects tab, after EQ)
    CompressorUnit::addParameters(layout, "fx_comp_");

    return layout;
}

OMarimbaAudioProcessor::OMarimbaAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , eqUnit(parameters, "fx_eq_")  // v1.8.0: Initialize EQ with parameter prefix
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

    // v1.3.0: Initialize factory presets on first run
    presetManager.initializeFactoryPresets();
}

OMarimbaAudioProcessor::~OMarimbaAudioProcessor()
{
}

void OMarimbaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
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

    // v1.8.0: Prepare Analog EQ
    eqUnit.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // v1.9.0: Prepare Compressor
    compressorUnit.prepare(sampleRate, samplesPerBlock);
}

void OMarimbaAudioProcessor::releaseResources()
{
    // Phase 2.4: Reset body resonance
    bodyResonance.reset();
}

void OMarimbaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // v1.2.6: Extract note-on AND note-off events for polyphonic UI visualization
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            // Note-on with velocity (normalized 0-1)
            midiEventQueue.push({ msg.getNoteNumber(), msg.getFloatVelocity() });
        }
        else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
        {
            // Note-off (velocity 0 note-on is also note-off per MIDI spec)
            midiEventQueue.push({ msg.getNoteNumber(), 0.0f });
        }
    }

    // Phase 2.2: Read parameters (atomic, real-time safe)
    float outputGainDB = parameters.getRawParameterValue("OUTPUT_GAIN")->load();
    float velCurve = parameters.getRawParameterValue("VEL_CURVE")->load();
    float malletHardness = parameters.getRawParameterValue("MALLET_HARDNESS")->load();
    float barMaterial = parameters.getRawParameterValue("BAR_MATERIAL")->load();
    float resonance = parameters.getRawParameterValue("RESONANCE")->load();

    // v1.6.0: Timbre parameters
    float strikePosition = parameters.getRawParameterValue("STRIKE_POSITION")->load();
    float overtoneDamping = parameters.getRawParameterValue("OVERTONE_DAMPING")->load();
    float tone = parameters.getRawParameterValue("TONE")->load();

    // Phase 2.3: Tuning parameters
    int tuningModeInt = static_cast<int>(parameters.getRawParameterValue("TUNING_MODE")->load());
    float referencePitch = parameters.getRawParameterValue("REFERENCE_PITCH")->load();

    // Update tuning engine (atomic updates, safe from audio thread)
    tuningEngine.setMode(static_cast<TuningEngine::Mode>(tuningModeInt));
    tuningEngine.setReferencePitch(static_cast<double>(referencePitch));

    // Update all active voices with current parameters
    // v1.9.8: Output gain removed from voices - now applied once at end of chain
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<MarimbaVoice*>(synthesiser.getVoice(i)))
        {
            voice->setVelocityCurve(velCurve);
            voice->setMalletHardness(malletHardness);
            voice->setBarMaterial(barMaterial);
            voice->setResonance(resonance);
            // v1.6.0: New timbre parameters
            voice->setStrikePosition(strikePosition);
            voice->setOvertoneDamping(overtoneDamping);
            voice->setTone(tone);
        }
    }

    // Phase 2.2: Render synthesiser (processes MIDI and generates audio)
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Phase 2.4: Apply body resonance (after synth rendering)
    // RESONANCE parameter controls both decay time (in voices) AND body mix
    // Scale to 0.5 to avoid too much wetness (resonance = 1.0 → 50% wet)
    bodyResonance.setMix(resonance * 0.5f);
    bodyResonance.process(buffer);

    // v1.8.1: Apply Analog EQ only if enabled (effects tab, end of chain before output gain)
    bool eqEnabled = parameters.getRawParameterValue("fx_eq_enabled")->load() > 0.5f;
    if (eqEnabled)
        eqUnit.process(buffer);

    // v1.9.0: Apply Compressor (after EQ, at end of effects chain)
    compressorUnit.process(buffer, parameters, "fx_comp_");

    // v1.9.8: Output gain applied ONCE at end of chain (after EQ & Compressor)
    // Signal chain: Synth → Body Resonance → EQ (if enabled) → Compressor (if enabled) → Output Gain → VU Meter
    float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDB);
    buffer.applyGain(outputGainLinear);

    // v1.2.3: Write samples to waveform FIFO for oscilloscope display
    // Use left channel (or mono mix if stereo)
    const float* readPtr = buffer.getReadPointer(0);
    waveformFifo.write(readPtr, buffer.getNumSamples());

    // v1.2.5: VU Meter - Calculate peak level after all processing
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        peakLevel = std::max(peakLevel, channelPeak);
    }
    float levelDB = peakLevel > 0.00001f
        ? juce::Decibels::gainToDecibels(peakLevel)
        : -100.0f;
    outputLevelDB.store(levelDB, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* OMarimbaAudioProcessor::createEditor()
{
    return new OMarimbaAudioProcessorEditor(*this);
}

void OMarimbaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.3.0: Use PresetManager to save complete state including tuning
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OMarimbaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // v1.3.0: Use PresetManager to restore complete state including tuning
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        presetManager.setStateFromXml(xmlState.get());
}

// UI keyboard MIDI injection (called from UI thread)
void OMarimbaAudioProcessor::addMidiMessage(const juce::MidiMessage& msg)
{
    const juce::ScopedLock lock(midiLock);
    pendingUiMidi.addEvent(msg, 0);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OMarimbaAudioProcessor();
}
