/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
#include <cmath>

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

bool OMarimbaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // WR-08: accept only mono or stereo output; reject anything else the host proposes.
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void OMarimbaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // WR-07: resolve cached atomic parameter pointers once (avoids per-block string lookups)
    pOutputGain      = parameters.getRawParameterValue("OUTPUT_GAIN");
    pVelCurve        = parameters.getRawParameterValue("VEL_CURVE");
    pMalletHardness  = parameters.getRawParameterValue("MALLET_HARDNESS");
    pBarMaterial     = parameters.getRawParameterValue("BAR_MATERIAL");
    pResonance       = parameters.getRawParameterValue("RESONANCE");
    pStrikePosition  = parameters.getRawParameterValue("STRIKE_POSITION");
    pOvertoneDamping = parameters.getRawParameterValue("OVERTONE_DAMPING");
    pTone            = parameters.getRawParameterValue("TONE");
    pTuningMode      = parameters.getRawParameterValue("TUNING_MODE");
    pReferencePitch  = parameters.getRawParameterValue("REFERENCE_PITCH");
    pEqEnabled       = parameters.getRawParameterValue("fx_eq_enabled");

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

    // Phase 2.2: Read parameters (WR-07: cached atomic pointers, no per-block string lookups)
    float outputGainDB = pOutputGain->load();
    float velCurve = pVelCurve->load();
    float malletHardness = pMalletHardness->load();
    float barMaterial = pBarMaterial->load();
    float resonance = pResonance->load();

    // v1.6.0: Timbre parameters
    float strikePosition = pStrikePosition->load();
    float overtoneDamping = pOvertoneDamping->load();
    float tone = pTone->load();

    // Phase 2.3: Tuning parameters
    int tuningModeInt = static_cast<int>(pTuningMode->load());
    float referencePitch = pReferencePitch->load();

    // Update tuning engine (atomic updates, safe from audio thread)
    tuningEngine.setMode(static_cast<TuningEngine::Mode>(tuningModeInt));
    tuningEngine.setReferencePitch(static_cast<double>(referencePitch));
    // WR-02: complete any frequency-table rebuild that was deferred while the message
    // thread held the tuning lock (non-blocking; no-op when the table is current).
    tuningEngine.serviceRebuild();

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
    bool eqEnabled = pEqEnabled->load() > 0.5f;  // WR-07: cached pointer
    if (eqEnabled)
        eqUnit.process(buffer);

    // v1.9.0: Apply Compressor (after EQ, at end of effects chain)
    compressorUnit.process(buffer, parameters, "fx_comp_");

    // v1.9.8: Output gain applied ONCE at end of chain (after EQ & Compressor)
    // Signal chain: Synth → Body Resonance → EQ (if enabled) → Compressor (if enabled) → Output Gain → VU Meter
    float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDB);
    buffer.applyGain(outputGainLinear);

    // WR-06: master output safety net. Replace any non-finite sample with 0 and clamp to a
    // safe ceiling (well above the ~+/-1 nominal signal) so a resonator/biquad blow-up or
    // runaway feedback can't propagate a NaN/Inf into the waveform FIFO, the VU calc, or
    // the host. Transparent for normal-level audio.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* out = buffer.getWritePointer(ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float x = out[n];
            if (! std::isfinite(x))
                x = 0.0f;
            out[n] = juce::jlimit(-2.0f, 2.0f, x);
        }
    }

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
    {
        // v1.13.0: the UI language rides the session XML as a plain attribute.
        //
        // Written on the XmlElement PresetManager already built, not on
        // parameters.state: getStateAsXml() calls parameters.copyState() and
        // createXml() internally, so by this line the tree has already been
        // snapshotted and a setProperty on the live APVTS tree would never reach
        // the bytes serialised below. Setting the attribute here is exactly
        // equivalent — a ValueTree property IS an XML attribute — and leaves
        // PresetManager, and therefore the JSON preset path, untouched.
        xml->setAttribute("uiLanguage", languageCode(uiLanguage.load(std::memory_order_acquire)));

        copyXmlToBinary(*xml, destData);
    }
}

void OMarimbaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // v1.3.0: Use PresetManager to restore complete state including tuning
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        presetManager.setStateFromXml(xmlState.get());

        // v1.13.0: read the language back from the RESTORED XML.
        //
        // hasAttribute() is the guard, and getStringAttribute() the read.
        // NamedValueSet::setFromXmlAttributes rebuilds every property as a var
        // over the attribute STRING, so a type predicate such as isInt() is
        // false for every session ever saved
        // (critical_valuetree_xml_roundtrip_loses_type) — and here the value
        // never leaves string form at all. A session written before v1.13.0 has
        // no such attribute and simply leaves the language where it is: English
        // on a fresh instance.
        if (xmlState->hasAttribute("uiLanguage"))
            uiLanguage.store(languageIndex(xmlState->getStringAttribute("uiLanguage")),
                             std::memory_order_release);
    }
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
