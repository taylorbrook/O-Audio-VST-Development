/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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

    O-Bassoon - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation): silent shell. APVTS + headless TuningEngine + NE drain.
    First audio: Phase 2.1.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"

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
    , presetManager (parameters, "O-Bassoon")
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

    // Phase 2.3: per-voice noise-seed wire (NoiseExciter uses voiceIndex × 31337)
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (i)))
            bv->setVoiceIndex (i);

    // Single shared sound (accepts all notes / all channels)
    synthesiser.addSound (new BassoonSound());

    // Stage 4: write 4 ROADMAP factory presets to disk on first run (idempotent).
    initializeFactoryPresets();
}

OBassoonAudioProcessor::~OBassoonAudioProcessor() = default;

//==============================================================================
void OBassoonAudioProcessor::initializeFactoryPresets()
{
    // Idempotency guard: only write if Factory dir doesn't already contain presets.
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    if (factoryDir.isDirectory()
        && factoryDir.getNumberOfChildFiles (juce::File::findFiles) > 0)
        return;

    presetManager.initializeFactoryPresets (FactoryPresets::build (parameters));
}

//==============================================================================
void OBassoonAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Modal synthesis is feed-forward; latency = 0 — do NOT call setLatencySamples.
    // (getLatencySamples is non-virtual in JUCE 8; default returns 0.)
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    // Phase 2.1: dispatch per-voice prepare hook.
    // juce::SynthesiserVoice has no virtual prepareToPlay in JUCE 8; we add a
    // non-virtual custom method on BassoonVoice and iterate voices here.
    // Mirrors O-Wind FluteSynthVoice / O-Lyrica HarpSynthVoice precedent.
    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
            bv->prepareToPlay (sampleRate, samplesPerBlock);

    // Phase 2.2: tone smoother + dispatch throttle (CONTEXT-rev-2 Q3-rev-2).
    toneSmoother.reset (sampleRate, 0.050);   // 50 ms ramp
    lastDispatchedTone = -1.0f;               // force first dispatch on next processBlock

    // Phase 2.3: output_gain smoother + processor-scope dispatch shadows
    outputGainSmoother.reset (sampleRate, 0.030);   // 30 ms ramp
    outputGainSmoother.setCurrentAndTargetValue (1.0f);

    lastDispatchedAttackMs   = -1.0f;
    lastDispatchedReleaseMs  = -1.0f;
    lastDispatchedVibRate    = -1.0f;
    lastDispatchedVibDepth   = -1.0f;
    lastDispatchedVibOnsetMs = -1.0f;
    lastDispatchedUiBreath   = -1.0f;
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

    const int numSamples = buffer.getNumSamples();

    // Phase 2.4 (FUNC-02): voice_count snapshot at processBlock prologue head.
    // Applies on next note-on; already-active voices unaffected per ROADMAP.
    // Integer-comparison throttle (no float epsilon — AudioParameterInt is exact).
    const int requestedVoices = static_cast<int> (
        parameters.getRawParameterValue ("voice_count")->load());
    if (requestedVoices != lastDispatchedVoiceCount)
    {
        synthesiser.setActiveVoiceCap (requestedVoices);
        lastDispatchedVoiceCount = requestedVoices;
    }

    // Stage 3 / Phase 3.2 — push-channel snapshots (RT-safe; allocation-free; relaxed memory order).
    // Single producer (audio thread) / single consumer (editor's 30 Hz Timer on message thread).
    // No causal dependency between channels — relaxed ordering is sufficient.
    {
        // (1) Live active-voice count: 16-iter loop over isVoiceActive() — same cost as
        // BassoonSynthesiser::findFreeVoice cap-enforce loop (cheap integer flag reads).
        int active = 0;
        const int n = synthesiser.getNumVoices();
        for (int v = 0; v < n; ++v)
            if (synthesiser.getVoice (v)->isVoiceActive())
                ++active;
        currentActiveVoiceCount.store (active, std::memory_order_relaxed);

        // (2) Effective breath snapshot: read first active voice's breathSmoother.
        // breathSmoother already composes ui_breath x cc2_normalised per BassoonVoice.cpp:149,
        // so this is exactly the breath value being applied to the audible voice render.
        // (3) Vibrato envelope snapshot: read first active voice's vibrato.getEnvelope() (0..1).
        float effBreath = 0.0f;
        float vibEnv    = 0.0f;
        for (int v = 0; v < n; ++v)
        {
            if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
            {
                if (bv->isVoiceActive())
                {
                    effBreath = bv->getEffectiveBreath();
                    vibEnv    = bv->getVibratoEnvelope();
                    break;
                }
            }
        }
        currentEffectiveBreath.store  (effBreath, std::memory_order_relaxed);
        currentVibratoEnvelope.store  (vibEnv,    std::memory_order_relaxed);
    }

    // Phase 2.2: tone smoother advance + voice dispatch (CONTEXT-rev-2 Q3/Q4-rev-2).
    // Sits BEFORE the NE drain — same principle as the Phase 2.1 NE-drain-BEFORE-
    // renderNextBlock invariant: voice state is fully up-to-date when JUCE iterates
    // voice events. Throttle epsilon = 0.001 keeps quiescent dispatch off the hot path.
    const float toneTarget   = parameters.getRawParameterValue ("tone")->load();
    toneSmoother.setTargetValue (toneTarget);
    const float toneSmoothed = toneSmoother.skip (juce::jmax (0, numSamples));

    if (std::abs (toneSmoothed - lastDispatchedTone) > 0.001f)
    {
        for (int v = 0; v < synthesiser.getNumVoices(); ++v)
            if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
                bv->setTone (toneSmoothed);
        lastDispatchedTone = toneSmoothed;
    }

    // Phase 2.3: expression dispatch (BEFORE NE drain — locked OQ#6-rev-3).
    // Reads 6 APVTS values; dispatches single aggregated setExpression(...) per voice
    // only when ANY sub-param changes > epsilon (saves ~6 × 16 = 96 virtual hops/block
    // when expression is static, the 99 % case during sustained playback).
    const float attackMs   = parameters.getRawParameterValue ("attack_time")->load();
    const float releaseMs  = parameters.getRawParameterValue ("release_time")->load();
    const float vibRate    = parameters.getRawParameterValue ("vibrato_rate")->load();
    const float vibDepth   = parameters.getRawParameterValue ("vibrato_depth")->load();
    const float vibOnsetMs = parameters.getRawParameterValue ("vibrato_onset")->load();
    const float uiBreath   = parameters.getRawParameterValue ("breath")->load();

    const bool anyChanged =
           std::abs (attackMs   - lastDispatchedAttackMs)   > 0.001f
        || std::abs (releaseMs  - lastDispatchedReleaseMs)  > 0.001f
        || std::abs (vibRate    - lastDispatchedVibRate)    > 0.001f
        || std::abs (vibDepth   - lastDispatchedVibDepth)   > 0.001f
        || std::abs (vibOnsetMs - lastDispatchedVibOnsetMs) > 0.001f
        || std::abs (uiBreath   - lastDispatchedUiBreath)   > 0.001f;

    if (anyChanged)
    {
        for (int v = 0; v < synthesiser.getNumVoices(); ++v)
            if (auto* bv = dynamic_cast<BassoonVoice*> (synthesiser.getVoice (v)))
                bv->setExpression (attackMs, releaseMs, vibRate, vibDepth, vibOnsetMs, uiBreath);

        lastDispatchedAttackMs   = attackMs;
        lastDispatchedReleaseMs  = releaseMs;
        lastDispatchedVibRate    = vibRate;
        lastDispatchedVibDepth   = vibDepth;
        lastDispatchedVibOnsetMs = vibOnsetMs;
        lastDispatchedUiBreath   = uiBreath;
    }

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch.
    // MUST run BEFORE renderNextBlock so per-voice startNote sees pending NE deltas.
    vst3Extensions.drainAndUpdate();

    // Render all voices via synthesiser (handles MIDI routing + voice allocation).
    synthesiser.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // Phase 2.3: output_gain post-summation declick-safe applyGainRamp
    // (locked OQ#1-rev-3: applyGainRamp(0, numSamples, current, smoother.skip(N))
    // is JUCE 8 canonical idiom for SmoothedValue-driven buffer-level gain).
    const float outDb     = parameters.getRawParameterValue ("output_gain")->load();
    const float linearTgt = juce::Decibels::decibelsToGain (outDb);
    const float gainStart = outputGainSmoother.getCurrentValue();
    outputGainSmoother.setTargetValue (linearTgt);
    const float gainEnd   = outputGainSmoother.skip (juce::jmax (0, numSamples));
    buffer.applyGainRamp (0, numSamples, gainStart, gainEnd);
}

//==============================================================================
juce::AudioProcessorEditor* OBassoonAudioProcessor::createEditor()
{
    return new OBassoonAudioProcessorEditor (*this);
}

//==============================================================================
void OBassoonAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Stage 4: delegate to preset manager (XML wrap + currentPreset attr).
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void OBassoonAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml (xmlState.get());
}

//==============================================================================
// Factory function (JUCE plugin entry point)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBassoonAudioProcessor();
}
