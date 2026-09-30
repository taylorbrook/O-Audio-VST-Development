/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
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

    O-SimpleReverb - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    v1.13.0 - Real flutter + octave shimmer, DECAY headroom map (see CHANGELOG)

    Each reverb type now has distinct sonic character:
    - Booth: Tight, immediate, minimal reflections
    - Room: Natural early reflections, balanced character
    - Hall: Long pre-delay, spacious early reflections
    - Spring: Chirpy dispersion, pitch flutter
    - Plate: Dense diffusion, octave-up shimmer
    - Ambient: Maximum diffusion, slow modulation, washy tail

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

// Define the type presets with distinct DSP characteristics.
// wetTrimDb (v1.12.0) level-matches each type's wet output to Room at default
// SIZE/DECAY/CHARACTER: K-weighted (BS.1770) RMS on pink noise, WET 100 / DRY 0.
// Before the trim the six types spanned +1.5..+9.2 dB re input.
const OSimpleReverbAudioProcessor::TypePreset OSimpleReverbAudioProcessor::typePresets[6] = {
    // 0: Booth - tight, intimate, minimal reflections
    {
        0.15f,      // baseRoomSize
        0.85f,      // baseDamping (high = fast decay)
        0.6f,       // width (narrow)
        3.0f,       // preDelayMs (very short)
        0.3f,       // earlyReflectionScale (tight)
        0.2f,       // earlyReflectionMix (minimal)
        0.0f,       // modRate (no modulation)
        0.0f,       // modCents
        false,      // useAllPass
        false,      // useShimmer
        150.0f,     // eqFreq (high-pass to remove rumble)
        0.0f,       // eqGain
        0.707f,     // eqQ
        TypePreset::EqType::HighPass,
        4.9f        // wetTrimDb
    },
    // 1: Room - natural, versatile
    {
        0.50f,      // baseRoomSize
        0.50f,      // baseDamping
        1.0f,       // width (full stereo)
        15.0f,      // preDelayMs (natural room)
        1.0f,       // earlyReflectionScale (natural)
        0.4f,       // earlyReflectionMix
        0.0f,       // modRate
        0.0f,       // modCents
        false,      // useAllPass
        false,      // useShimmer
        0.0f,       // eqFreq (no EQ)
        0.0f,       // eqGain
        0.707f,     // eqQ
        TypePreset::EqType::None,
        0.0f        // wetTrimDb
    },
    // 2: Hall - large concert hall, spacious
    {
        0.85f,      // baseRoomSize (large)
        0.25f,      // baseDamping (low = long decay)
        1.0f,       // width
        50.0f,      // preDelayMs (long for large space)
        2.0f,       // earlyReflectionScale (spread out)
        0.5f,       // earlyReflectionMix
        0.15f,      // modRate (very subtle movement)
        3.0f,       // modCents (slow drift in the tail)
        false,      // useAllPass
        false,      // useShimmer
        3000.0f,    // eqFreq (gentle roll-off)
        -2.0f,      // eqGain (slight high cut for distance)
        0.5f,       // eqQ
        TypePreset::EqType::HighShelf,
        -2.1f       // wetTrimDb
    },
    // 3: Spring - metallic chirp, flutter
    {
        0.35f,      // baseRoomSize
        0.40f,      // baseDamping
        0.7f,       // width (narrower)
        20.0f,      // preDelayMs
        0.5f,       // earlyReflectionScale
        0.15f,      // earlyReflectionMix (less - spring character dominates)
        4.5f,       // modRate (flutter speed)
        6.0f,       // modCents (audible spring wobble)
        true,       // useAllPass (spring dispersion!)
        false,      // useShimmer
        800.0f,     // eqFreq (resonant mid boost)
        4.0f,       // eqGain (metallic resonance)
        2.5f,       // eqQ (narrow resonance)
        TypePreset::EqType::Peak,
        2.1f        // wetTrimDb
    },
    // 4: Plate - dense, bright, shimmering
    {
        0.65f,      // baseRoomSize
        0.30f,      // baseDamping
        1.0f,       // width (full stereo)
        8.0f,       // preDelayMs (short for density)
        0.6f,       // earlyReflectionScale
        0.6f,       // earlyReflectionMix (dense early reflections)
        0.0f,       // modRate
        0.0f,       // modCents
        false,      // useAllPass
        true,       // useShimmer (plate shimmer!)
        5000.0f,    // eqFreq (bright shelf)
        3.0f,       // eqGain (add sparkle)
        0.707f,     // eqQ
        TypePreset::EqType::HighShelf,
        -2.1f       // wetTrimDb
    },
    // 5: Ambient - washy, ethereal, infinite
    {
        0.95f,      // baseRoomSize (maximum)
        0.10f,      // baseDamping (very low = infinite)
        1.0f,       // width
        35.0f,      // preDelayMs
        2.5f,       // earlyReflectionScale (very spread)
        0.3f,       // earlyReflectionMix
        0.4f,       // modRate (slow, dreamy movement)
        4.0f,       // modCents
        false,      // useAllPass
        false,      // useShimmer
        2500.0f,    // eqFreq
        -3.0f,      // eqGain (soften highs for washy sound)
        0.5f,       // eqQ
        TypePreset::EqType::HighShelf,
        -2.8f       // wetTrimDb
    }
};

juce::AudioProcessorValueTreeState::ParameterLayout OSimpleReverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // TYPE - Choice parameter (6 options: Booth, Room, Hall, Spring, Plate, Ambient)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "TYPE", 1 },
        "Type",
        juce::StringArray { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" },
        1  // Default: Room (index 1)
    ));

    // CHARACTER - Bipolar float (-100% to +100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "CHARACTER", 1 },
        "Character",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f  // Default: Neutral
    ));

    // WET - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "WET", 1 },
        "Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f  // Default: 25%
    ));

    // DRY - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DRY", 1 },
        "Dry",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f  // Default: 100%
    ));

    // DECAY - Float (0.5x to 2.0x multiplier)
    // CR-02: skew 0.6309 puts 1.0x at knob center. convertFrom0to1 maps
    // value = min + (max-min) * norm^(1/skew); solving 0.5 + 1.5*0.5^(1/s) = 1.0
    // gives s = log(0.5)/log(1/3) ≈ 0.6309. The previous 1.585 (the reciprocal)
    // put 1.47x at center, disagreeing with the UI readout and factory presets,
    // which were both authored against this intended curve.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f, 0.6309f),
        1.0f  // Default: 1.0x (no change)
    ));

    // SIZE - Float (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SIZE", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f  // Default: 50% (Medium)
    ));

    // LPFREQ - LOW CUT (high-pass) cutoff, 20Hz to 400Hz. IN-02 (v1.12.0): the
    // host-visible NAME says what the filter is; the ID keeps its historical
    // "LP" so sessions and automation still bind.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LPFREQ", 1 },
        "Low Cut Freq",
        juce::NormalisableRange<float>(20.0f, 400.0f, 1.0f),
        200.0f  // Default: 200Hz
    ));

    // LPON - LOW CUT on/off (0 = off, 1 = on)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "LPON", 1 },
        "Low Cut On",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
        0.0f  // Default: Off
    ));

    return layout;
}

OSimpleReverbAudioProcessor::OSimpleReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-SimpleReverb")
{
    // Cache parameter pointers (these never change after construction)
    typeParam = parameters.getRawParameterValue("TYPE");
    characterParam = parameters.getRawParameterValue("CHARACTER");
    sizeParam = parameters.getRawParameterValue("SIZE");
    decayParam = parameters.getRawParameterValue("DECAY");
    wetParam = parameters.getRawParameterValue("WET");
    dryParam = parameters.getRawParameterValue("DRY");
    lpFreqParam = parameters.getRawParameterValue("LPFREQ");
    lpOnParam = parameters.getRawParameterValue("LPON");

    // Initialize factory presets (4 per reverb type = 24 total)
    initializeFactoryPresets();
}

OSimpleReverbAudioProcessor::~OSimpleReverbAudioProcessor() = default;

void OSimpleReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare DSP spec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Delay capacities follow the running rate. The pre-1.11.0 fixed 9600-sample
    // capacity (50 ms @ 192 kHz) clamped Ambient's 61.5 ms early reflections
    // above 156 kHz.
    const auto msToSamples = [sampleRate](float ms) {
        return static_cast<int>(std::ceil(ms * 0.001 * sampleRate)) + 1;
    };

    // Prepare main reverb
    reverb.prepare(spec);
    reverb.reset();

    // Prepare filters using template helper
    warmFilter.prepare(spec);
    warmFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    warmFilter.setResonance(1.0f / juce::MathConstants<float>::sqrt2);  // Butterworth, as makeLowPass
    prepareFilterAsAllPass(brightFilter, spec, sampleRate);
    characterSmoothed.reset(sampleRate, 0.05);
    characterSmoothed.setCurrentAndTargetValue(characterParam->load());
    previousCharacterValue = -1000.0f;  // out of range: forces the first coefficient update
    updateCharacterCoefficients(characterSmoothed.getCurrentValue());

    for (auto& eq : typeEq)
        prepareFilterAsAllPass(eq, spec, sampleRate);
    eqFadeBuffer.setSize(static_cast<int>(spec.numChannels), samplesPerBlock);
    eqActive = 0;
    eqMix = 0.0f;
    eqMixStep = 1.0f / (kTypeEqFadeMs * 0.001f * static_cast<float>(sampleRate));

    // User LOW CUT: always running, crossfaded in/out (v1.12.0)
    lowCutFilter.prepare(spec);
    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    lowCutFilter.setResonance(1.0f / juce::MathConstants<float>::sqrt2);  // Butterworth, as makeHighPass
    lowCutBuffer.setSize(static_cast<int>(spec.numChannels), samplesPerBlock);
    lowCutFreqSmoothed.reset(sampleRate, 0.05);
    lowCutFreqSmoothed.setCurrentAndTargetValue(lpFreqParam->load());
    lowCutFilter.setCutoffFrequency(lowCutFreqSmoothed.getCurrentValue());
    lowCutMixSmoothed.reset(sampleRate, 0.02);
    lowCutMixSmoothed.setCurrentAndTargetValue(lpOnParam->load() >= 0.5f ? 1.0f : 0.0f);

    // Prepare delay lines
    for (auto* delay : { &preDelayL, &preDelayR }) {
        delay->setMaximumDelayInSamples(msToSamples(kMaxPreDelayMs));
        delay->prepare(spec);
        delay->reset();
    }
    prepareDelayContainer(earlyReflectionsL, spec, msToSamples(kMaxEarlyReflectionMs));
    prepareDelayContainer(earlyReflectionsR, spec, msToSamples(kMaxEarlyReflectionMs));
    prepareDelayContainer(allPassL, spec, msToSamples(kMaxAllPassMs));
    prepareDelayContainer(allPassR, spec, msToSamples(kMaxAllPassMs));

    // Modulation (v1.13.0): the flutter line is sized for the deepest type
    float maxFlutterMs = 0.0f;
    for (const auto& t : typePresets) {
        FlutterDelay probe;
        probe.prepare(sampleRate, 0.0f);
        probe.setModulation(t.modRate, t.modCents);
        maxFlutterMs = juce::jmax(maxFlutterMs, probe.getDepthSamples() * 1000.0f / static_cast<float>(sampleRate));
    }
    flutterL.prepare(sampleRate, maxFlutterMs);
    flutterR.prepare(sampleRate, maxFlutterMs);
    shimmerL.prepare(sampleRate);
    shimmerR.prepare(sampleRate);
    lfoPhase = 0.0f;

    // CR-04: pre-allocate work buffers so the setSize() calls in processBlock
    // are no-op reuse instead of a guaranteed first-callback allocation
    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);

    // Start on the current TYPE with no duck and no EQ crossfade
    const int startType = juce::jlimit(0, 5, juce::roundToInt(typeParam->load()));
    activeType = -1;
    switchChainTo(startType);
    chainGain = 1.0f;
    chainGainStep = 1.0f / (kTypeDuckMs * 0.001f * static_cast<float>(sampleRate));
    setTypeEq(eqActive, startType);
    eqType = startType;

    // WR-03: 20ms wet/dry gain smoothing (zipper-noise-free knob drags/automation)
    wetGainSmoothed.reset(sampleRate, 0.02);
    dryGainSmoothed.reset(sampleRate, 0.02);
    wetGainSmoothed.setCurrentAndTargetValue(wetParam->load() / 100.0f);
    dryGainSmoothed.setCurrentAndTargetValue(dryParam->load() / 100.0f);
}

void OSimpleReverbAudioProcessor::releaseResources()
{
}

void OSimpleReverbAudioProcessor::switchChainTo(int typeIndex)
{
    // Called with chainGain at 0 (or before the first block): the delay times
    // jump and the lines are cleared, which is silent because nothing is being
    // fed to the reverb. Clearing also drops the OLD type's reflections, which
    // would otherwise re-emerge at the new spacing.
    const auto& preset = typePresets[typeIndex];
    activeType = typeIndex;
    const float fs = static_cast<float>(currentSampleRate);

    lfoIncrement = preset.modRate > 0.0f ? juce::MathConstants<float>::twoPi * preset.modRate / fs : 0.0f;
    for (auto* f : { &flutterL, &flutterR }) {
        f->setModulation(preset.modRate, preset.modCents);
        f->reset();
    }
    shimmerL.reset();
    shimmerR.reset();

    const float preDelaySamples = preset.preDelayMs * 0.001f * fs;
    for (auto* d : { &preDelayL, &preDelayR }) {
        d->reset();
        d->setDelay(preDelaySamples);
    }

    for (int i = 0; i < numEarlyReflections; ++i) {
        const float delaySamples = kBaseEarlyDelaysMs[i] * preset.earlyReflectionScale * 0.001f * fs;
        earlyReflectionsL[i].reset();
        earlyReflectionsR[i].reset();
        earlyReflectionsL[i].setDelay(delaySamples);
        earlyReflectionsR[i].setDelay(delaySamples * kEarlyReflectionStereoOffset);
    }

    for (int i = 0; i < numAllPassFilters; ++i) {
        const float delaySamples = allPassDelayMs[i] * 0.001f * fs;
        allPassL[i].reset();
        allPassR[i].reset();
        allPassL[i].setDelay(delaySamples);
        allPassR[i].setDelay(delaySamples * kAllPassStereoOffset);
    }
}

void OSimpleReverbAudioProcessor::setTypeEq(int slot, int typeIndex)
{
    // CR-03: runs on the audio thread, so ArrayCoefficients (stack std::array)
    // rather than Coefficients::makeXXX, which heap-allocates. Assignment reuses
    // the storage primed in prepareToPlay.
    const auto& preset = typePresets[typeIndex];
    auto& state = *typeEq[static_cast<size_t>(slot)].state;
    switch (preset.eqType) {
        case TypePreset::EqType::None:
            // Identity (not an all-pass, which shifts phase): the instance
            // still runs, so crossfades into and out of Room are seamless.
            state = std::array<float, 6> { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
            break;
        case TypePreset::EqType::LowShelf:
            state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
                currentSampleRate, preset.eqFreq, preset.eqQ, juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::HighShelf:
            state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
                currentSampleRate, preset.eqFreq, preset.eqQ, juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::Peak:
            state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
                currentSampleRate, preset.eqFreq, preset.eqQ, juce::Decibels::decibelsToGain(preset.eqGain));
            break;
        case TypePreset::EqType::HighPass:
            state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(
                currentSampleRate, preset.eqFreq, preset.eqQ);
            break;
    }
}

void OSimpleReverbAudioProcessor::updateCharacterCoefficients(float characterValue)
{
    if (juce::exactlyEqual(characterValue, previousCharacterValue))
        return;
    previousCharacterValue = characterValue;

    // Warm (< -0.5): low-pass 2 kHz..20 kHz, the pre-1.11.0 curve (clamped to
    // 0.45 fs). From 0 up it is parked at 0.49 fs, where the prewarped
    // low-pass is flat across the audible band, so Neutral and Bright sound as
    // they did when this filter was bypassed. -0.5..0 glides between the two
    // so the map has no step for a smoothed crossing to click on.
    const float fs = static_cast<float>(currentSampleRate);
    const float warmTopHz = juce::jmin(20000.0f, 0.45f * fs);
    const float parkHz = 0.49f * fs;
    float cutoffHz;
    if (characterValue < -0.5f)
        cutoffHz = juce::jmin(2000.0f + 18000.0f * juce::jlimit(0.0f, 1.0f, (characterValue + 100.0f) / 99.0f), warmTopHz);
    else if (characterValue < 0.0f)
        cutoffHz = juce::jmap(characterValue, -0.5f, 0.0f, warmTopHz, parkHz);
    else
        cutoffHz = parkHz;
    warmFilter.setCutoffFrequency(cutoffHz);  // RT-safe: one tan(), no allocation

    // Bright: +0..6 dB shelf at 4 kHz, CONTINUOUS from 0 (v1.12.0). The old
    // `> 0.5 ? ... : 0` step from the identity shelf burst at about -51 dB
    // above 3 kHz on every crossing; at +0.5 the shelf is +0.03 dB, inaudible,
    // so the UI's "neutral" zone still reads true.
    const float brightValue = juce::jlimit(0.0f, 1.0f, characterValue / 100.0f);
    *brightFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
        currentSampleRate, 4000.0f, 0.707f, juce::Decibels::decibelsToGain(brightValue * 6.0f));  // CR-03: RT-safe, no heap alloc
}

float OSimpleReverbAudioProcessor::processAllPassChain(float input, bool isLeft)
{
    float output = input;
    auto& delays = isLeft ? allPassL : allPassR;

    // Schroeder all-pass: v[n] = x[n] + g*v[n-D],  y[n] = v[n-D] - g*v[n]
    // H(z) = (-g + z^-D) / (1 - g*z^-D), |H| = 1. The pre-1.11.0 form summed
    // the feed-forward term with the wrong sign, which made the 3-stage chain
    // a comb cascade (+20.5 dB at DC, -42 dB notches) instead of an all-pass.
    for (int i = 0; i < numAllPassFilters; ++i) {
        const float delayed = delays[i].popSample(0);
        const float v = output + allPassCoeff * delayed;
        delays[i].pushSample(0, v);
        output = delayed - allPassCoeff * v;
    }

    return output;
}

void OSimpleReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    if (buffer.getNumSamples() == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (pointers cached in constructor)
    // TYPE: ROUND the raw choice value, as AudioParameterChoice::getIndex() does.
    // The raw value is normalised * 5 with no snapping, so truncation played the
    // type BELOW the one the host and the UI showed for any value not exactly k/5.
    int typeValue = juce::roundToInt(typeParam->load());
    float characterValue = characterParam->load();
    float sizeValue = sizeParam->load();
    float decayValue = decayParam->load();
    float wetValue = wetParam->load();
    float dryValue = dryParam->load();
    float lpFreqValue = lpFreqParam->load();
    bool lpFilterOn = lpOnParam->load() >= 0.5f;

    typeValue = juce::jlimit(0, 5, typeValue);

    // The requested type drives the reverb core (juce::Reverb smooths its own
    // room size, damping and width). The pre-reverb chain, and with it the
    // level trim, runs activeType and follows via the duck below.
    const auto& preset = typePresets[typeValue];

    // Calculate reverb parameters
    // Size affects room size
    float sizeNorm = sizeValue / 100.0f;
    float finalRoomSize = preset.baseRoomSize * (0.5f + sizeNorm * 0.5f);

    // Decay (0.5x to 2.0x) scales room size and inversely scales damping:
    // higher decay = larger room + less damping = longer tail. v1.13.0: above
    // 1.0x it closes the HEADROOM to 1.0 rather than multiplying, so the
    // maximum arrives at 2.0x on every type. The multiply clamped early on big
    // rooms: Ambient at SIZE 100 hit 1.0 at 1.05x, and the top ~47 % of the
    // knob moved only damping. Below 1.0x is unchanged.
    if (decayValue <= 1.0f)
        finalRoomSize *= decayValue;
    else
        finalRoomSize += (1.0f - finalRoomSize) * (decayValue - 1.0f);
    finalRoomSize = juce::jlimit(0.0f, 1.0f, finalRoomSize);

    // Damping: lower values = longer decay, so divide by decay multiplier
    float finalDamping = preset.baseDamping / decayValue;
    finalDamping = juce::jlimit(0.0f, 1.0f, finalDamping);

    // WR-03: smooth wet/dry gains (raw per-block atomic loads step at block rate
    // and zipper on sustained material). Ramp linearly across the block between
    // the smoother's start and end values — equivalent to per-sample smoothing
    // but keeps the channel-major mix loop.
    wetGainSmoothed.setTargetValue(wetValue / 100.0f);
    dryGainSmoothed.setTargetValue(dryValue / 100.0f);
    const float wetGainStart = wetGainSmoothed.getCurrentValue();
    const float dryGainStart = dryGainSmoothed.getCurrentValue();
    wetGainSmoothed.skip(buffer.getNumSamples());
    dryGainSmoothed.skip(buffer.getNumSamples());
    const float wetGainEnd = wetGainSmoothed.getCurrentValue();
    const float dryGainEnd = dryGainSmoothed.getCurrentValue();

    // Store dry signal (pre-allocated buffer, no reallocation)
    dryBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

    // Process sample-by-sample for type-specific DSP
    const int numSamples = buffer.getNumSamples();
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : leftChannel;

    // Prepare wet buffer (using pre-allocated buffer, resize only if needed)
    wetBuffer.setSize(buffer.getNumChannels(), numSamples, false, false, true);
    wetBuffer.clear();

    const TypePreset* chain = &typePresets[activeType];
    float chainTrim = wetTrimGain(activeType);

    for (int sample = 0; sample < numSamples; ++sample) {
        // === 0. TYPE duck-and-swap (v1.12.0) ===
        // Jumping the delay times under signal clicked (60x..6900x). Fall to 0 on
        // the old type, swap with nothing feeding the reverb, rise on the new one.
        if (typeValue != activeType) {
            chainGain -= chainGainStep;
            if (chainGain <= 0.0f) {
                chainGain = 0.0f;
                switchChainTo(typeValue);
                chain = &typePresets[activeType];
                chainTrim = wetTrimGain(activeType);
            }
        } else if (chainGain < 1.0f) {
            chainGain = juce::jmin(1.0f, chainGain + chainGainStep);
        }

        float inputL = leftChannel[sample];
        float inputR = rightChannel[sample];

        // === 1. Pre-delay ===
        preDelayL.pushSample(0, inputL);
        preDelayR.pushSample(0, inputR);
        float preDelayedL = preDelayL.popSample(0);
        float preDelayedR = preDelayR.popSample(0);

        // === 2. Early Reflections ===
        float earlyL = 0.0f;
        float earlyR = 0.0f;

        for (int i = 0; i < numEarlyReflections; ++i) {
            earlyReflectionsL[i].pushSample(0, preDelayedL);
            earlyReflectionsR[i].pushSample(0, preDelayedR);
            earlyL += earlyReflectionsL[i].popSample(0) * earlyReflectionGains[i];
            earlyR += earlyReflectionsR[i].popSample(0) * earlyReflectionGains[i];
        }

        // Mix early reflections
        float processedL = preDelayedL + (earlyL * chain->earlyReflectionMix);
        float processedR = preDelayedR + (earlyR * chain->earlyReflectionMix);

        // === 3. Spring All-Pass Dispersion (if enabled) ===
        if (chain->useAllPass) {
            processedL = processAllPassChain(processedL, true);
            processedR = processAllPassChain(processedR, false);
        }

        // === 4. Pitch flutter (v1.13.0) ===
        // A swept short delay: Spring 6 cents at 4.5 Hz, Hall 3 at 0.15 Hz,
        // Ambient 4 at 0.4 Hz. It replaced a +/-3 % amplitude wobble that
        // measured +/-0.26 dB. L and R share ONE phase: juce::Reverb sums its
        // input to mono before the combs, so an L/R offset (tried: a quarter
        // cycle) became a sweeping comb filter there - a flanger, -3 dB on noise.
        if (chain->modRate > 0.0f) {
            lfoPhase += lfoIncrement;
            if (lfoPhase >= juce::MathConstants<float>::twoPi)
                lfoPhase -= juce::MathConstants<float>::twoPi;
            const float lfo = std::sin(lfoPhase);
            processedL = flutterL.process(processedL, lfo);
            processedR = flutterR.process(processedR, lfo);
        }

        // === 5. Plate shimmer (v1.13.0) ===
        // A real octave up, blended into the reverb's input. It replaced a
        // 1.5 kHz ring modulator, whose sidebands were inharmonic.
        if (chain->useShimmer) {
            processedL += kShimmerMixAmount * shimmerL.process(processedL);
            processedR += kShimmerMixAmount * shimmerR.process(processedR);
        }

        // Write to wet buffer for reverb processing. The type's level trim
        // (v1.12.0) goes in HERE, at the reverb input and inside the duck: on
        // the output it would re-scale the old type's ringing tail on a switch
        // (Ambient -> Booth lifted it 7.7 dB). The reverb is linear, so the
        // steady level is the same either way.
        const float g = chainGain * chainTrim;
        wetBuffer.setSample(0, sample, processedL * g);
        if (wetBuffer.getNumChannels() > 1)
            wetBuffer.setSample(1, sample, processedR * g);
    }

    // === 6. Main Reverb Processing ===
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = finalRoomSize;
    reverbParams.damping = finalDamping;
    reverbParams.width = preset.width;
    reverbParams.wetLevel = 1.0f;   // Full wet (we mix manually)
    reverbParams.dryLevel = 0.0f;   // No dry (we add it back)
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
    reverb.process(wetContext);

    // === 7. Type-Specific EQ (crossfaded on a type change, v1.12.0) ===
    // Start a crossfade only when none is running; a type change arriving
    // mid-fade is picked up by the next block after this one lands.
    if (eqMix <= 0.0f && typeValue != eqType) {
        const int idle = 1 - eqActive;
        setTypeEq(idle, typeValue);
        typeEq[static_cast<size_t>(idle)].reset();
        eqType = typeValue;
        eqMix = eqMixStep;   // > 0: fade running
    }
    if (eqMix > 0.0f) {
        const int idle = 1 - eqActive;
        eqFadeBuffer.setSize(wetBuffer.getNumChannels(), numSamples, false, false, true);
        for (int ch = 0; ch < wetBuffer.getNumChannels(); ++ch)
            eqFadeBuffer.copyFrom(ch, 0, wetBuffer, ch, 0, numSamples);
        juce::dsp::AudioBlock<float> fadeBlock(eqFadeBuffer);
        juce::dsp::ProcessContextReplacing<float> fadeContext(fadeBlock);
        typeEq[static_cast<size_t>(eqActive)].process(wetContext);
        typeEq[static_cast<size_t>(idle)].process(fadeContext);

        float mix = eqMix;
        for (int sample = 0; sample < numSamples; ++sample) {
            for (int ch = 0; ch < wetBuffer.getNumChannels(); ++ch) {
                const float a = wetBuffer.getSample(ch, sample);
                wetBuffer.setSample(ch, sample, a + mix * (eqFadeBuffer.getSample(ch, sample) - a));
            }
            mix = juce::jmin(1.0f, mix + eqMixStep);
        }
        if (mix >= 1.0f) {
            eqActive = idle;
            eqMix = 0.0f;
        } else {
            eqMix = mix;
        }
    } else {
        typeEq[static_cast<size_t>(eqActive)].process(wetContext);
    }

    // === 8. Character ===
    // Smoothed over 50 ms. While it moves, the coefficients follow EVERY sample:
    // stepping the Bright shelf every 32 samples zippered at about -46 dB above
    // 3 kHz (v1.12.0 probe); per sample it is -67 dB, level with a held knob.
    // Settled, one update per block (a no-op: the value has not changed).
    characterSmoothed.setTargetValue(characterValue);
    if (characterSmoothed.isSmoothing()) {
        for (int i = 0; i < numSamples; ++i) {
            updateCharacterCoefficients(characterSmoothed.getNextValue());
            auto one = wetBlock.getSubBlock(static_cast<size_t>(i), 1);
            juce::dsp::ProcessContextReplacing<float> oneContext(one);
            warmFilter.process(oneContext);
            brightFilter.process(oneContext);
        }
    } else {
        updateCharacterCoefficients(characterSmoothed.getCurrentValue());
        warmFilter.process(wetContext);
        brightFilter.process(wetContext);
    }

    // === 8.5. User LOW CUT (v1.12.0) ===
    // Always runs into lowCutBuffer, its cutoff smoothed (50 ms) and set every
    // kCharacterSubBlock samples; ON/OFF crossfades it against the unfiltered
    // wet over 20 ms. v1.11.0 switched it in and out between blocks (a click).
    lowCutFreqSmoothed.setTargetValue(lpFreqValue);
    lowCutMixSmoothed.setTargetValue(lpFilterOn ? 1.0f : 0.0f);
    lowCutBuffer.setSize(wetBuffer.getNumChannels(), numSamples, false, false, true);
    juce::dsp::AudioBlock<float> lowCutBlock(lowCutBuffer);
    for (int start = 0; start < numSamples; start += kCharacterSubBlock) {
        const int len = juce::jmin(kCharacterSubBlock, numSamples - start);
        lowCutFreqSmoothed.skip(len);
        lowCutFilter.setCutoffFrequency(lowCutFreqSmoothed.getCurrentValue());  // RT-safe: one tan()
        auto sub = wetBlock.getSubBlock(static_cast<size_t>(start), static_cast<size_t>(len));
        auto cutSub = lowCutBlock.getSubBlock(static_cast<size_t>(start), static_cast<size_t>(len));
        juce::dsp::ProcessContextNonReplacing<float> cutContext(sub, cutSub);
        lowCutFilter.process(cutContext);
    }

    if (lowCutMixSmoothed.isSmoothing() || lowCutMixSmoothed.getTargetValue() > 0.0f) {
        for (int sample = 0; sample < numSamples; ++sample) {
            const float m = lowCutMixSmoothed.getNextValue();
            for (int ch = 0; ch < wetBuffer.getNumChannels(); ++ch) {
                const float a = wetBuffer.getSample(ch, sample);
                wetBuffer.setSample(ch, sample, a + m * (lowCutBuffer.getSample(ch, sample) - a));
            }
        }
    }

    // === 9. Dry/Wet Mix (gain-ramped, WR-03) ===
    const float rampStep = 1.0f / static_cast<float>(numSamples);  // numSamples > 0 (early return)
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* output = buffer.getWritePointer(channel);
        const float* dry = dryBuffer.getReadPointer(channel);
        const float* wet = wetBuffer.getReadPointer(channel);

        float t = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample, t += rampStep) {
            const float dryGain = dryGainStart + (dryGainEnd - dryGainStart) * t;
            const float wetGain = wetGainStart + (wetGainEnd - wetGainStart) * t;
            output[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
        }
    }

    // === 10. VU Meter - Calculate peak level after all processing ===
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        peakLevel = std::max(peakLevel, channelPeak);
    }
    // Raise the held peak; the editor's exchange() resets it once per read
    float held = outputPeak.load(std::memory_order_relaxed);
    while (peakLevel > held
           && ! outputPeak.compare_exchange_weak(held, peakLevel, std::memory_order_relaxed)) {}
}

#if JUCE_WEB_BROWSER
#include "PluginEditor.h"
#endif

juce::AudioProcessorEditor* OSimpleReverbAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OSimpleReverbAudioProcessorEditor(*this);
#else
    // The param-dump console target builds with JUCE_WEB_BROWSER=0 and no
    // editor sources. It never opens an editor; this keeps the TU linkable.
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

void OSimpleReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // v1.6.0: the UI language rides the same tree as one more plain property.
    // Written BEFORE getStateAsXml(), because that method serialises
    // parameters.copyState() and would otherwise take a snapshot without it.
    // Written as a STRING ("en"/"fr") rather than the atomic's int index, so a
    // hand-inspected session file says what it means.
    parameters.state.setProperty("uiLanguage",
                                 languageCode(uiLanguage.load(std::memory_order_acquire)),
                                 nullptr);

    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OSimpleReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());

    // v1.6.0: the UI language. Read AFTER setStateFromXml, which calls
    // parameters.replaceState() and therefore rebuilds the whole tree.
    //
    // isVoid() is the ONLY correct guard and toString() the only correct read.
    // getStateInformation writes a STRING var, but even a bool or int written
    // there would not survive: the XML round-trip does not preserve the type,
    // because NamedValueSet::setFromXmlAttributes rebuilds every property as
    // `var (value)` over the attribute STRING
    // (critical_valuetree_xml_roundtrip_loses_type). A pre-1.6.0 session has no
    // such property at all and the default (English) stands. languageIndex()
    // clamps anything that is not "fr" to 0, so a hand-edited value degrades to
    // English rather than to a bad index.
    //
    // A PRESET LOAD CANNOT CHANGE THE LANGUAGE. loadPreset() also calls
    // parameters.replaceState(), which overwrites the property in the tree —
    // but this atomic is read only here, in the HOST state path, so the value
    // the user chose survives a preset change and is re-written from the atomic
    // on the next save.
    //
    // The editor PULLS this through the getUiLanguage native fn at page init
    // rather than being pushed from here — a push would race the WebView's load.
    const juce::var lang = parameters.state.getProperty("uiLanguage");

    if (! lang.isVoid())
        uiLanguage.store(languageIndex(lang.toString()), std::memory_order_release);
}

void OSimpleReverbAudioProcessor::initializeFactoryPresets()
{
    // TYPE is normalised over 6 choices: index k -> k / 5 (NOT k / 6 — the
    // pre-1.11.0 table used sixths, which recalled the wrong type for 20 of 24).
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // === BOOTH PRESETS (4) ===
        { "Booth - Vocal Booth", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.5f}, {"WET", 0.20f}, {"DRY", 1.0f},
            {"DECAY", 0.33f}, {"SIZE", 0.30f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Booth - Drum Close", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.35f}, {"WET", 0.15f}, {"DRY", 1.0f},
            {"DECAY", 0.20f}, {"SIZE", 0.20f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Booth - Tight Room", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.50f}, {"WET", 0.25f}, {"DRY", 1.0f},
            {"DECAY", 0.40f}, {"SIZE", 0.40f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Booth - Whisper", {
            {"TYPE", 0.0f}, {"CHARACTER", 0.70f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.25f}, {"SIZE", 0.15f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },

        // === ROOM PRESETS (4) ===
        { "Room - Small Room", {
            {"TYPE", 0.2f}, {"CHARACTER", 0.50f}, {"WET", 0.25f}, {"DRY", 1.0f},
            {"DECAY", 0.33f}, {"SIZE", 0.35f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Room - Live Room", {
            {"TYPE", 0.2f}, {"CHARACTER", 0.45f}, {"WET", 0.35f}, {"DRY", 1.0f},
            {"DECAY", 0.50f}, {"SIZE", 0.55f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Room - Studio A", {
            {"TYPE", 0.2f}, {"CHARACTER", 0.55f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.45f}, {"SIZE", 0.50f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Room - Jazz Club", {
            {"TYPE", 0.2f}, {"CHARACTER", 0.40f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.55f}, {"SIZE", 0.60f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },

        // === HALL PRESETS (4) ===
        { "Hall - Concert Hall", {
            {"TYPE", 0.4f}, {"CHARACTER", 0.50f}, {"WET", 0.35f}, {"DRY", 1.0f},
            {"DECAY", 0.60f}, {"SIZE", 0.75f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Hall - Cathedral", {
            {"TYPE", 0.4f}, {"CHARACTER", 0.45f}, {"WET", 0.45f}, {"DRY", 0.85f},
            {"DECAY", 0.80f}, {"SIZE", 0.90f}, {"LPFREQ", 0.21f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Hall - Theater", {
            {"TYPE", 0.4f}, {"CHARACTER", 0.55f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.55f}, {"SIZE", 0.65f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Hall - Ballroom", {
            {"TYPE", 0.4f}, {"CHARACTER", 0.60f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.70f}, {"SIZE", 0.80f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },

        // === SPRING PRESETS (4) ===
        { "Spring - Vintage Spring", {
            {"TYPE", 0.6f}, {"CHARACTER", 0.45f}, {"WET", 0.35f}, {"DRY", 1.0f},
            {"DECAY", 0.50f}, {"SIZE", 0.50f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Spring - Surf Guitar", {
            {"TYPE", 0.6f}, {"CHARACTER", 0.60f}, {"WET", 0.45f}, {"DRY", 1.0f},
            {"DECAY", 0.55f}, {"SIZE", 0.55f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Spring - Dub Echo", {
            {"TYPE", 0.6f}, {"CHARACTER", 0.35f}, {"WET", 0.50f}, {"DRY", 0.90f},
            {"DECAY", 0.65f}, {"SIZE", 0.60f}, {"LPFREQ", 0.32f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Spring - Twang", {
            {"TYPE", 0.6f}, {"CHARACTER", 0.70f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.45f}, {"SIZE", 0.45f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },

        // === PLATE PRESETS (4) ===
        { "Plate - Studio Plate", {
            {"TYPE", 0.8f}, {"CHARACTER", 0.55f}, {"WET", 0.30f}, {"DRY", 1.0f},
            {"DECAY", 0.50f}, {"SIZE", 0.55f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Plate - Shimmer Plate", {
            {"TYPE", 0.8f}, {"CHARACTER", 0.70f}, {"WET", 0.40f}, {"DRY", 1.0f},
            {"DECAY", 0.65f}, {"SIZE", 0.70f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Plate - Vocal Plate", {
            {"TYPE", 0.8f}, {"CHARACTER", 0.50f}, {"WET", 0.25f}, {"DRY", 1.0f},
            {"DECAY", 0.45f}, {"SIZE", 0.50f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Plate - Lush Plate", {
            {"TYPE", 0.8f}, {"CHARACTER", 0.45f}, {"WET", 0.45f}, {"DRY", 0.95f},
            {"DECAY", 0.70f}, {"SIZE", 0.75f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },

        // === AMBIENT PRESETS (4) ===
        { "Ambient - Pad Wash", {
            {"TYPE", 1.0f}, {"CHARACTER", 0.40f}, {"WET", 0.50f}, {"DRY", 0.80f},
            {"DECAY", 0.75f}, {"SIZE", 0.85f}, {"LPFREQ", 0.26f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Ambient - Infinite Drone", {
            {"TYPE", 1.0f}, {"CHARACTER", 0.35f}, {"WET", 0.60f}, {"DRY", 0.60f},
            {"DECAY", 1.0f}, {"SIZE", 1.0f}, {"LPFREQ", 0.21f}, {"LPON", 1.0f}
        }, juce::var() },
        { "Ambient - Ethereal", {
            {"TYPE", 1.0f}, {"CHARACTER", 0.55f}, {"WET", 0.55f}, {"DRY", 0.75f},
            {"DECAY", 0.80f}, {"SIZE", 0.90f}, {"LPFREQ", 0.47f}, {"LPON", 0.0f}
        }, juce::var() },
        { "Ambient - Cloud Nine", {
            {"TYPE", 1.0f}, {"CHARACTER", 0.50f}, {"WET", 0.65f}, {"DRY", 0.70f},
            {"DECAY", 0.85f}, {"SIZE", 0.95f}, {"LPFREQ", 0.32f}, {"LPON", 1.0f}
        }, juce::var() }
    };

    presetManager.initializeFactoryPresets(factoryPresets);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleReverbAudioProcessor();
}
