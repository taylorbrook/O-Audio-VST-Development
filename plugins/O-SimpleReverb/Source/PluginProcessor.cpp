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

    Booth, Room, Hall and Ambient run a 16-line feedback delay network
    (Source/dsp/FdnEngine.h) with stereo early reflections at the output
    (Source/dsp/EarlyReflections.h). Each type has its own delay set, DECAY is
    a multiplier on the type's own decay time in seconds, and SIZE scales the
    delay lengths without moving the tail time.

    Plate runs Dattorro's figure-8 tank (Source/dsp/PlateEngine.h), with the
    octave-up shimmer inside its loop and no early reflections.

    Spring runs three dispersive springs (Source/dsp/SpringEngine.h): each
    echo arrives low frequencies first, and the tank passes nothing much above
    4.5 kHz. No early reflections and no pre-delay - the first echo is the
    spring's own.

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is deliberately NOT included at the top of this TU — the
// include lives inside the #if JUCE_WEB_BROWSER guard directly above
// createEditor(), so a console target that compiles this TU with
// JUCE_WEB_BROWSER=0 and no editor sources (scripts/param-dump) links.

// The six types. FdnConfig is { minMs, maxMs, jitter, hfRatio, crossoverHz,
// modMs, modHz, diffusionScale } (Source/dsp/FdnEngine.h).
// wetTrimDb level-matches each type's wet output at default SIZE/DECAY/
// CHARACTER: K-weighted (BS.1770) RMS on pink noise, WET 100 / DRY 0, to
// +6.4 dB re input - where v1.14.0's types sat in stereo (6.3..6.5;
// render-check --levels prints the table these are set from). monoTrimDb
// brings a mono bus to the same figure: the fold assumes L and R share
// nothing, and each type's early reflections and tank share a little.
const OSimpleReverbAudioProcessor::TypePreset OSimpleReverbAudioProcessor::typePresets[6] = {
    // 0: Booth - tight, intimate
    {
        TypePreset::Engine::Fdn,
        0.40f,      // baseT60 (s)
        0.5f, 2.0f, // sizeLo, sizeHi
        { 2.9f, 13.7f, 1, 0.55f, 6000.0f, 0.0f, 0.0f, 0.25f },   // no modulation; short diffusers
        6.9f,       // earlySpanMs
        0.6f,       // earlyLevel
        3.0f,       // preDelayMs (very short)
        150.0f,     // eqFreq (high-pass to remove rumble)
        0.0f,       // eqGain
        0.707f,     // eqQ
        TypePreset::EqType::HighPass,
        6.6f,       // wetTrimDb
        -0.17f      // monoTrimDb
    },
    // 1: Room - natural, versatile
    {
        TypePreset::Engine::Fdn,
        1.1f,       // baseT60 (s)
        0.5f, 2.0f, // sizeLo, sizeHi
        { 8.3f, 37.9f, 2, 0.50f, 5000.0f, 0.06f, 0.7f, 1.0f },
        23.0f,      // earlySpanMs
        0.4f,       // earlyLevel
        15.0f,      // preDelayMs (natural room)
        0.0f,       // eqFreq (no EQ)
        0.0f,       // eqGain
        0.707f,     // eqQ
        TypePreset::EqType::None,
        5.4f,       // wetTrimDb
        -0.47f      // monoTrimDb
    },
    // 2: Hall - large concert hall, spacious
    {
        TypePreset::Engine::Fdn,
        3.0f,       // baseT60 (s)
        0.5f, 2.0f, // sizeLo, sizeHi
        { 21.7f, 83.1f, 3, 0.45f, 4000.0f, 0.15f, 0.5f, 1.0f },
        46.0f,      // earlySpanMs
        0.3f,       // earlyLevel
        50.0f,      // preDelayMs (long for large space)
        3000.0f,    // eqFreq (gentle roll-off)
        -2.0f,      // eqGain (slight high cut for distance)
        0.5f,       // eqQ
        TypePreset::EqType::HighShelf,
        5.9f,       // wetTrimDb
        -0.52f      // monoTrimDb
    },
    // 3: Spring - three dispersive springs. SIZE runs x0.75..x1.33 of their
    // echo times (33 / 37 / 41 ms at 50 %), which keeps the tank inside what
    // real springs do. No early reflections, and no pre-delay: nothing comes
    // out of a spring before its first echo, 25..44 ms after the input.
    {
        TypePreset::Engine::Spring,
        2.5f,       // baseT60 (s)
        0.75f, 1.3333333f, // sizeLo, sizeHi
        {},         // fdn: not an FDN type
        0.0f,       // earlySpanMs
        0.0f,       // earlyLevel
        0.0f,       // preDelayMs
        800.0f,     // eqFreq (resonant mid boost)
        4.0f,       // eqGain (metallic resonance)
        2.5f,       // eqQ (narrow resonance)
        TypePreset::EqType::Peak,
        -0.5f,      // wetTrimDb
        0.14f       // monoTrimDb
    },
    // 4: Plate - Dattorro's tank. SIZE runs x0.40..x0.80 of the paper's
    // lengths (x0.57 at 50 %): above that the tank is audibly sparse. No
    // early reflections - a plate has none.
    {
        TypePreset::Engine::Plate,
        2.5f,       // baseT60 (s)
        0.40f, 0.80f, // sizeLo, sizeHi
        {},         // fdn: not an FDN type
        0.0f,       // earlySpanMs
        0.0f,       // earlyLevel
        8.0f,       // preDelayMs (short for density)
        5000.0f,    // eqFreq (bright shelf)
        3.0f,       // eqGain (add sparkle)
        0.707f,     // eqQ
        TypePreset::EqType::HighShelf,
        1.9f,       // wetTrimDb
        -0.01f      // monoTrimDb
    },
    // 5: Ambient - washy, ethereal, very long
    {
        TypePreset::Engine::Fdn,
        7.0f,       // baseT60 (s)
        0.5f, 2.0f, // sizeLo, sizeHi
        { 30.7f, 121.3f, 4, 0.60f, 3500.0f, 0.25f, 0.3f, 1.0f },
        57.5f,      // earlySpanMs
        0.25f,      // earlyLevel
        35.0f,      // preDelayMs
        2500.0f,    // eqFreq
        -3.0f,      // eqGain (soften highs for washy sound)
        0.5f,       // eqQ
        TypePreset::EqType::HighShelf,
        4.1f,       // wetTrimDb
        -0.15f      // monoTrimDb
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

    // Initialize factory presets (8 per reverb type = 48 total)
    initializeFactoryPresets();
}

OSimpleReverbAudioProcessor::~OSimpleReverbAudioProcessor() = default;

void OSimpleReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxChunkSamples = juce::jmax(1, samplesPerBlock);

    // Prepare DSP spec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxChunkSamples);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare filters using template helper
    warmFilter.prepare(spec);
    warmFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    warmFilter.setResonance(1.0f / juce::MathConstants<float>::sqrt2);  // Butterworth, as makeLowPass
    prepareFilterAsAllPass(brightFilter, spec, sampleRate);
    characterSmoothed.reset(sampleRate, 0.05);
    characterSmoothed.setCurrentAndTargetValue(characterParam->load());
    previousCharacterValue = -1000.0f;  // out of range: forces the first coefficient update
    updateCharacterCoefficients(characterSmoothed.getCurrentValue());

    // User LOW CUT: always running, crossfaded in/out (v1.12.0)
    lowCutFilter.prepare(spec);
    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    lowCutFilter.setResonance(1.0f / juce::MathConstants<float>::sqrt2);  // Butterworth, as makeHighPass
    lowCutBuffer.setSize(static_cast<int>(spec.numChannels), maxChunkSamples);
    lowCutFreqSmoothed.reset(sampleRate, 0.05);
    lowCutFreqSmoothed.setCurrentAndTargetValue(lpFreqParam->load());
    lowCutFilter.setCutoffFrequency(lowCutFreqSmoothed.getCurrentValue());
    lowCutMixSmoothed.reset(sampleRate, 0.02);
    lowCutMixSmoothed.setCurrentAndTargetValue(lpOnParam->load() >= 0.5f ? 1.0f : 0.0f);

    // Reverb slots. Every capacity follows the running rate and the largest
    // value any type asks for; nothing is sized again after this.
    float maxLineMs = 1.0f, maxModMs = 0.0f, maxSpanMs = 1.0f, maxPreDelayMs = 1.0f;
    for (const auto& t : typePresets) {
        maxLineMs = juce::jmax(maxLineMs, t.fdn.maxMs);
        maxModMs = juce::jmax(maxModMs, t.fdn.modMs);
        maxSpanMs = juce::jmax(maxSpanMs, t.earlySpanMs);
        maxPreDelayMs = juce::jmax(maxPreDelayMs, t.preDelayMs);
    }
    const int preDelayCapacity = static_cast<int>(std::ceil(maxPreDelayMs * 0.001 * sampleRate)) + 1;
    for (auto& slot : slots) {
        slot.fdn.prepare(sampleRate, maxLineMs, maxModMs);
        slot.plate.prepare(sampleRate);
        slot.spring.prepare(sampleRate);
        slot.early.prepare(sampleRate, maxSpanMs);
        slot.preDelayL.prepare(preDelayCapacity);
        slot.preDelayR.prepare(preDelayCapacity);
        prepareFilterAsAllPass(slot.eq, spec, sampleRate);
        slot.out.setSize(static_cast<int>(spec.numChannels), maxChunkSamples);
        slot.state = Slot::State::idle;
        slot.type = -1;
    }
    duckCoeff = static_cast<float>(osr::slewCoefficient(0.25 * kTypeDuckMs * 0.001, sampleRate));
    levelCoeff = static_cast<float>(osr::slewCoefficient(0.5 * kLevelSlewMs * 0.001, sampleRate));
    stealStep = 1.0f / (kStealFadeMs * 0.001f * static_cast<float>(sampleRate));
    retireHoldSamples = static_cast<int>(kRetireHoldMs * 0.001 * sampleRate);

    // CR-04: pre-allocate work buffers. processBlock works in chunks of at
    // most this size, so its setSize() calls only ever shrink them.
    dryBuffer.setSize(getTotalNumOutputChannels(), maxChunkSamples);
    wetBuffer.setSize(getTotalNumOutputChannels(), maxChunkSamples);

    // Start on the current TYPE with its input open (no duck)
    currentSlot = 0;
    startSlot(slots[0], juce::jlimit(0, 5, juce::roundToInt(typeParam->load())),
              decayParam->load(), sizeParam->load(), true);

    // WR-03: 20ms wet/dry gain smoothing (zipper-noise-free knob drags/automation)
    wetGainSmoothed.reset(sampleRate, 0.02);
    dryGainSmoothed.reset(sampleRate, 0.02);
    wetGainSmoothed.setCurrentAndTargetValue(wetParam->load() / 100.0f);
    dryGainSmoothed.setCurrentAndTargetValue(dryParam->load() / 100.0f);
}

void OSimpleReverbAudioProcessor::releaseResources()
{
}

void OSimpleReverbAudioProcessor::startSlot(Slot& slot, int typeIndex, float decayValue, float sizeValue, bool inputOpen)
{
    // The slot is silent here (idle, or a stolen slot whose output fade has
    // reached zero), so the delay set can jump and the lines can be cleared.
    const auto& preset = typePresets[typeIndex];
    slot.type = typeIndex;
    slot.state = Slot::State::playing;
    slot.inputGain = slot.inputLead = inputOpen ? 1.0f : 0.0f;
    slot.outputGain = 1.0f;
    slot.quietSamples = 0;
    slot.earlyLevel = OSR_MUTANT(earlyMuted) ? 0.0f : preset.earlyLevel;
    slot.preDelaySamples = juce::jmax(1, juce::roundToInt(preset.preDelayMs * 0.001 * currentSampleRate));

    // A mono bus sums the slot's L and R. The FDN's and the plate's are
    // decorrelated, so 0.7071 keeps the power; the spring's are not.
    slot.monoFold = (preset.engine == TypePreset::Engine::Spring ? osr::SpringEngine::kMonoFold : 0.70710678f)
                    * juce::Decibels::decibelsToGain(preset.monoTrimDb);

    switch (preset.engine) {
        case TypePreset::Engine::Plate:
            slot.plate.setShimmer(OSR_MUTANT(noShimmer) ? 0.0 : osr::PlateEngine::kShimmerPerLoopSecond);
            break;
        case TypePreset::Engine::Spring:
            slot.spring.setVoicing(! OSR_MUTANT(springNoChirp), ! OSR_MUTANT(springOpenBand));
            break;
        case TypePreset::Engine::Fdn: {
            auto fdn = preset.fdn;
            if (OSR_MUTANT(sameDelays)) {
                fdn.minMs = typePresets[2].fdn.minMs;
                fdn.maxMs = typePresets[2].fdn.maxMs;
                fdn.jitter = typePresets[2].fdn.jitter;
            }
            slot.fdn.setType(fdn);
            break;
        }
    }
    slot.early.setType(juce::jmax(1.0f, preset.earlySpanMs));
    driveSlot(slot, decayValue, sizeValue);
    slot.trim = slot.trimLead = slot.trimTarget;    // the slot is silent: no slew to wait for
    clearSlot(slot);
    setTypeEq(slot, typeIndex);
}

float OSimpleReverbAudioProcessor::levelLawDb(const TypePreset& preset, float decayValue, float scale)
{
    // 0 dB at DECAY 1.0x, SIZE 50, where wetTrimDb is set.
    const float decayOctaves = std::log2(juce::jlimit(0.5f, 2.0f, decayValue));
    const float sizeOctaves = std::log2(scale / sizeScale(preset, 50.0f));
    return kSizeLevelDbPerOctave[static_cast<int>(preset.engine)] * sizeOctaves - kDecayLevelDbPerOctave * decayOctaves;
}

void OSimpleReverbAudioProcessor::clearSlot(Slot& slot)
{
    // Only the engine the slot's type runs: the others hold nothing that will
    // be heard, and are cleared here when a type that runs them starts.
    slot.preDelayL.reset();
    slot.preDelayR.reset();
    switch (engineOf(slot.type)) {
        case TypePreset::Engine::Plate:  slot.plate.reset(); break;
        case TypePreset::Engine::Spring: slot.spring.reset(); break;
        case TypePreset::Engine::Fdn:    slot.fdn.reset(); break;
    }
    slot.early.reset();
    slot.eq.reset();
}

void OSimpleReverbAudioProcessor::driveSlot(Slot& slot, float decayValue, float sizeValue)
{
    // DECAY is a multiplier on the type's own decay time; SIZE is a length
    // scale and nothing else. The engine turns the decay time into its loop
    // gains from the current lengths, so the tail time holds as SIZE moves.
    const auto& preset = typePresets[slot.type];
    const double scale = sizeScale(preset, OSR_MUTANT(sizeDead) ? 50.0f : sizeValue);
    double t60 = preset.baseT60 * (OSR_MUTANT(decayDead) ? 1.0f : decayValue);
    if (OSR_MUTANT(sizeIsDecay)) t60 *= scale / sizeScale(preset, 50.0f);
    if (OSR_MUTANT(decayStuck)) t60 = 1.0e6;

    slot.trimTarget = wetTrimGain(slot.type)
                      * (OSR_MUTANT(noLevelLaw) ? 1.0f : juce::Decibels::decibelsToGain(levelLawDb(preset, decayValue, static_cast<float>(scale))));
    if (OSR_MUTANT(noGlide)) slot.trim = slot.trimLead = slot.trimTarget;

    if (preset.engine == TypePreset::Engine::Plate) {
        slot.plate.setSize(scale);
        slot.plate.setT60(t60);
        if (OSR_MUTANT(noGlide)) slot.plate.land();
        return;
    }
    if (preset.engine == TypePreset::Engine::Spring) {
        slot.spring.setSize(scale);
        slot.spring.setT60(t60);
        if (OSR_MUTANT(noGlide)) slot.spring.land();
        return;
    }
    slot.fdn.setSize(scale);
    slot.fdn.setT60(t60);
    slot.early.setSize(scale);
    if (OSR_MUTANT(noGlide)) {
        slot.fdn.land();
        slot.early.land();
    }
}

void OSimpleReverbAudioProcessor::setTypeEq(Slot& slot, int typeIndex)
{
    // CR-03: runs on the audio thread, so ArrayCoefficients (stack std::array)
    // rather than Coefficients::makeXXX, which heap-allocates. Assignment reuses
    // the storage primed in prepareToPlay.
    const auto& preset = typePresets[typeIndex];
    auto& state = *slot.eq.state;
    switch (preset.eqType) {
        case TypePreset::EqType::None:
            // Identity (not an all-pass, which shifts phase)
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
    slot.eq.reset();
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

// The steal fade's gain for its position 0..1: a smoothstep, which starts and
// ends with no slope. A straight ramp has a corner at each end, and under a
// tone each corner is a small click.
static float fadeShape(float position)
{
    return position * position * (3.0f - 2.0f * position);
}

// True if no sample is NaN or infinite. Reads the exponent bits, so it means
// the same thing under any floating-point optimisation setting.
static bool allFinite(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i) {
        juce::uint32 bits;
        std::memcpy(&bits, data + i, sizeof(bits));
        if ((bits & 0x7f800000u) == 0x7f800000u)
            return false;
    }
    return true;
}

void OSimpleReverbAudioProcessor::renderSlot(Slot& slot, const float* inL, const float* inR, int numSamples)
{
    const bool mono = slot.out.getNumChannels() == 1;
    float* outL = slot.out.getWritePointer(0);
    float* outR = mono ? nullptr : slot.out.getWritePointer(1);
    const float gainTarget = slot.state == Slot::State::playing ? 1.0f : 0.0f;
    const bool useEarly = slot.earlyLevel > 0.0f;
    const auto engine = engineOf(slot.type);

    for (int i = 0; i < numSamples; ++i) {
        // The duck: two one-poles in series, each a quarter of kTypeDuckMs. A
        // TYPE change can arrive while the duck is still on its way, and a
        // ramp that turns round mid-way has a corner (a small click: -49 dB
        // above 3 kHz with TYPE switching every 16 ms). Through two poles the
        // gain has no corner whenever the target flips.
        if (! juce::exactlyEqual(slot.inputGain, gainTarget)) {
            slot.inputLead += (gainTarget - slot.inputLead) * duckCoeff;
            slot.inputGain += (slot.inputLead - slot.inputGain) * duckCoeff;
            if (std::abs(gainTarget - slot.inputGain) < 1.0e-5f && std::abs(gainTarget - slot.inputLead) < 1.0e-5f)
                slot.inputGain = slot.inputLead = gainTarget;
        }

        // The type's level trim goes in HERE, at the input and inside the
        // duck: on the output it would re-scale a tail that is ringing out
        // (v1.11.0: Ambient -> Booth lifted it 7.7 dB). The engines are
        // linear, so the steady level is the same either way. It follows
        // DECAY and SIZE (the level law) through two poles, as the duck does.
        if (! juce::exactlyEqual(slot.trim, slot.trimTarget)) {
            slot.trimLead += (slot.trimTarget - slot.trimLead) * levelCoeff;
            slot.trim += (slot.trimLead - slot.trim) * levelCoeff;
            if (std::abs(slot.trimTarget - slot.trim) < 1.0e-6f && std::abs(slot.trimTarget - slot.trimLead) < 1.0e-6f)
                slot.trim = slot.trimLead = slot.trimTarget;
        }
        const float g = slot.inputGain * slot.trim;
        float xL = inL[i] * g, xR = inR[i] * g;
       #if OSIMPLEREVERB_TEST_HOOKS
        if (testInjectNaN && slot.state == Slot::State::playing) {
            xL = std::numeric_limits<float>::quiet_NaN();
            testInjectNaN = false;
        }
       #endif

        // Pre-delay, then the taps and the tank side by side
        const float pL = slot.preDelayL.at(slot.preDelaySamples);
        const float pR = slot.preDelayR.at(slot.preDelaySamples);
        slot.preDelayL.push(xL);
        slot.preDelayR.push(xR);

        float l, r;
        if (engine == TypePreset::Engine::Fdn)        slot.fdn.process(pL, pR, l, r);
        else if (engine == TypePreset::Engine::Plate) slot.plate.process(pL, pR, l, r);
        else                                          slot.spring.process(pL, pR, l, r);
        if (useEarly) {
            float earlyL, earlyR;
            slot.early.process(pL, pR, earlyL, earlyR);
            l += slot.earlyLevel * earlyL;
            r += slot.earlyLevel * earlyR;
        }
        if (OSR_MUTANT(monoTail)) r = l;

        // Mono bus: the engines still run in stereo, and the fold keeps the
        // power (see startSlot).
        if (mono) {
            outL[i] = slot.monoFold * (l + r);
        } else {
            outL[i] = l;
            outR[i] = r;
        }
    }

    // A non-finite value anywhere in a feedback loop stays there. Clear the
    // slot and give this chunk silence; the next chunk starts from a clean
    // tank. Nothing latches: the slot keeps its state and its input.
    bool finite = true;
    for (int ch = 0; ch < slot.out.getNumChannels(); ++ch)
        finite = finite && allFinite(slot.out.getReadPointer(ch), numSamples);
    if (! finite && ! OSR_MUTANT(noNanGuard)) {
        clearSlot(slot);
        slot.out.clear();
    }

    juce::dsp::AudioBlock<float> block(slot.out);
    juce::dsp::ProcessContextReplacing<float> context(block);
    slot.eq.process(context);
}

void OSimpleReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalSamples = buffer.getNumSamples();
    if (totalSamples == 0)
        return;

    // Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, totalSamples);

    // A host may hand over more samples than it prepared for. The work
    // buffers are sized for the prepared block, so a larger one is processed
    // in pieces; growing a buffer here would allocate on the audio thread.
    const int chunk = OSR_MUTANT(noChunking) ? totalSamples : maxChunkSamples;
    for (int start = 0; start < totalSamples; start += chunk)
        processChunk(buffer, start, juce::jmin(chunk, totalSamples - start));

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

void OSimpleReverbAudioProcessor::processChunk(juce::AudioBuffer<float>& buffer, int start, int numSamples)
{
    const int numChannels = buffer.getNumChannels();

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

    // WR-03: smooth wet/dry gains (raw per-block atomic loads step at block rate
    // and zipper on sustained material). Ramp linearly across the block between
    // the smoother's start and end values — equivalent to per-sample smoothing
    // but keeps the channel-major mix loop.
    wetGainSmoothed.setTargetValue(wetValue / 100.0f);
    dryGainSmoothed.setTargetValue(dryValue / 100.0f);
    const float wetGainStart = wetGainSmoothed.getCurrentValue();
    const float dryGainStart = dryGainSmoothed.getCurrentValue();
    wetGainSmoothed.skip(numSamples);
    dryGainSmoothed.skip(numSamples);
    const float wetGainEnd = wetGainSmoothed.getCurrentValue();
    const float dryGainEnd = dryGainSmoothed.getCurrentValue();

    // Store dry signal (pre-allocated buffer, no reallocation)
    dryBuffer.setSize(numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, start, numSamples);

    // === 1. TYPE change: the playing slot rings out, the other takes over ===
    {
        auto& playing = slots[static_cast<size_t>(currentSlot)];
        if (playing.state == Slot::State::playing && playing.type != typeValue) {
            auto& other = slots[static_cast<size_t>(1 - currentSlot)];
            playing.state = OSR_MUTANT(noRingOut) ? Slot::State::idle : Slot::State::ringing;
            playing.quietSamples = 0;

            if (other.state == Slot::State::idle)
                startSlot(other, typeValue, decayValue, sizeValue, false);
            else if (other.state == Slot::State::ringing && other.type == typeValue)
                other.state = Slot::State::playing;      // its own tail is still going: reopen the input
            else if (OSR_MUTANT(hardSteal))
                startSlot(other, typeValue, decayValue, sizeValue, false);
            else
                other.state = Slot::State::stolen;       // fades out below, then starts the new type
            currentSlot = 1 - currentSlot;
        }
    }

    // === 2. SIZE and DECAY reach the playing slot only ===
    // A ringing slot keeps the size and decay it had when it was left, so a
    // preset change does not bend or shorten the tail of the preset before.
    if (slots[static_cast<size_t>(currentSlot)].state == Slot::State::playing)
        driveSlot(slots[static_cast<size_t>(currentSlot)], decayValue, sizeValue);

    // === 3. The slots, summed into the wet buffer ===
    wetBuffer.setSize(numChannels, numSamples, false, false, true);
    wetBuffer.clear();
    const float* inL = dryBuffer.getReadPointer(0);
    const float* inR = numChannels > 1 ? dryBuffer.getReadPointer(1) : inL;

    for (auto& slot : slots) {
        if (slot.state == Slot::State::idle)
            continue;

        slot.out.setSize(numChannels, numSamples, false, false, true);
        renderSlot(slot, inL, inR, numSamples);

        const bool fading = slot.state == Slot::State::stolen;
        float peak = 0.0f, gainAtEnd = slot.outputGain;
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* src = slot.out.getReadPointer(ch);
            float* dst = wetBuffer.getWritePointer(ch);
            float position = slot.outputGain;
            for (int i = 0; i < numSamples; ++i) {
                if (fading) position = juce::jmax(0.0f, position - stealStep);
                dst[i] += src[i] * fadeShape(position);
                peak = juce::jmax(peak, std::abs(src[i]));
            }
            gainAtEnd = position;
        }
        slot.outputGain = gainAtEnd;

        if (slot.state == Slot::State::stolen && slot.outputGain <= 0.0f) {
            // Only the slot about to play is ever stolen
            jassert(&slot == &slots[static_cast<size_t>(currentSlot)]);
            startSlot(slot, typeValue, decayValue, sizeValue, false);
        } else if (slot.state == Slot::State::ringing) {
            slot.quietSamples = peak < kRetireLevel ? slot.quietSamples + numSamples : 0;
            if (slot.quietSamples >= retireHoldSamples)
                slot.state = Slot::State::idle;
        }
    }

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);

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
    for (int offset = 0; offset < numSamples; offset += kCharacterSubBlock) {
        const int len = juce::jmin(kCharacterSubBlock, numSamples - offset);
        lowCutFreqSmoothed.skip(len);
        lowCutFilter.setCutoffFrequency(lowCutFreqSmoothed.getCurrentValue());  // RT-safe: one tan()
        auto sub = wetBlock.getSubBlock(static_cast<size_t>(offset), static_cast<size_t>(len));
        auto cutSub = lowCutBlock.getSubBlock(static_cast<size_t>(offset), static_cast<size_t>(len));
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
    for (int channel = 0; channel < numChannels; ++channel) {
        float* output = buffer.getWritePointer(channel) + start;
        const float* dry = dryBuffer.getReadPointer(channel);
        const float* wet = wetBuffer.getReadPointer(channel);

        float t = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample, t += rampStep) {
            const float dryGain = dryGainStart + (dryGainEnd - dryGainStart) * t;
            const float wetGain = wetGainStart + (wetGainEnd - wetGainStart) * t;
            output[sample] = (dry[sample] * dryGain) + (wet[sample] * wetGain);
        }
    }
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
    // Authored in the parameters' own units and converted once through each
    // range below (v1.14.0; the table used to hold hand-normalised fractions).
    //   type: 0 Booth, 1 Room, 2 Hall, 3 Spring, 4 Plate, 5 Ambient
    //   character -100..100 (dark..bright), wet/dry %, decay x, size %,
    //   low cut Hz and on/off.
    // v2.0.0: re-voiced for the new engines. DECAY is a multiple of the type's
    // own decay time (Booth 0.40 s, Room 1.1 s, Hall 3.0 s, Spring and Plate
    // 2.5 s, Ambient 7.0 s), so each row's tail length is the figure in its
    // comment, and a name that promises a real space gets that space's time.
    // SIZE no longer moves the tail: it is how far apart the echoes are.
    // "Send" presets are 100 % wet / 0 % dry, for an aux bus. Insert presets
    // stay at or below +5 dB re input (render-check section 9).
    struct Def { const char* name; int type; float character, wet, dry, decay, size, lowCutHz; bool lowCut; };
    static constexpr Def bank[] = {
        // name                          type  char   wet    dry   decay  size  lowcut           tail
        // === BOOTH ===
        { "Booth - Vocal Booth",           0,    0.0f, 20.0f, 100.0f, 0.60f, 30.0f, 199.0f, false },  // 0.24 s
        { "Booth - Drum Close",            0,  -30.0f, 15.0f, 100.0f, 0.62f, 20.0f, 119.0f, true  },  // 0.25 s
        { "Booth - Tight Room",            0,    0.0f, 25.0f, 100.0f, 1.00f, 60.0f, 199.0f, false },  // 0.40 s
        { "Booth - Whisper",               0,   40.0f, 30.0f, 100.0f, 0.70f, 15.0f, 199.0f, false },  // 0.28 s
        { "Booth - Snare Ambience",        0,   25.0f, 22.0f, 100.0f, 1.25f, 55.0f, 160.0f, true  },  // 0.50 s
        { "Booth - Voiceover",             0,  -20.0f, 12.0f, 100.0f, 0.50f, 20.0f, 120.0f, true  },  // 0.20 s
        { "Booth - Dark Closet",           0,  -65.0f, 30.0f, 100.0f, 0.55f,  5.0f, 199.0f, false },  // 0.22 s
        { "Booth - Send",                  0,    0.0f,100.0f,   0.0f, 1.00f, 50.0f, 150.0f, true  },  // 0.40 s

        // === ROOM ===
        { "Room - Small Room",             1,    0.0f, 25.0f, 100.0f, 0.55f, 25.0f, 199.0f, false },  // 0.61 s
        { "Room - Live Room",              1,    0.0f, 35.0f, 100.0f, 0.90f, 55.0f, 199.0f, false },  // 0.99 s
        { "Room - Studio A",               1,   10.0f, 30.0f, 100.0f, 0.72f, 45.0f, 199.0f, false },  // 0.79 s
        { "Room - Jazz Club",              1,  -20.0f, 35.0f, 100.0f, 1.10f, 60.0f, 119.0f, true  },  // 1.21 s
        { "Room - Drum Room",              1,   15.0f, 30.0f, 100.0f, 0.65f, 40.0f,  90.0f, true  },  // 0.72 s
        { "Room - Wood Room",              1,  -45.0f, 30.0f, 100.0f, 0.82f, 50.0f, 199.0f, false },  // 0.90 s
        { "Room - Bright Chamber",         1,   55.0f, 32.0f, 100.0f, 1.50f, 70.0f, 150.0f, true  },  // 1.65 s
        { "Room - Send",                   1,    0.0f,100.0f,   0.0f, 1.00f, 50.0f, 120.0f, true  },  // 1.10 s

        // === HALL ===
        { "Hall - Concert Hall",           2,    0.0f, 30.0f, 100.0f, 0.67f, 60.0f, 199.0f, false },  // 2.0 s
        { "Hall - Cathedral",              2,  -10.0f, 40.0f,  85.0f, 1.80f, 95.0f, 100.0f, true  },  // 5.4 s
        { "Hall - Theater",                2,   10.0f, 30.0f, 100.0f, 0.50f, 35.0f, 199.0f, false },  // 1.5 s
        { "Hall - Ballroom",               2,   20.0f, 35.0f, 100.0f, 0.87f, 70.0f, 199.0f, false },  // 2.6 s
        { "Hall - Strings Hall",           2,  -25.0f, 35.0f, 100.0f, 0.80f, 65.0f, 100.0f, true  },  // 2.4 s
        { "Hall - Dark Hall",              2,  -65.0f, 40.0f,  95.0f, 1.10f, 75.0f, 120.0f, true  },  // 3.3 s
        { "Hall - Choir Loft",             2,   35.0f, 38.0f,  95.0f, 1.30f, 80.0f, 140.0f, true  },  // 3.9 s
        { "Hall - Send",                   2,    0.0f,100.0f,   0.0f, 1.00f, 60.0f, 120.0f, true  },  // 3.0 s

        // === SPRING === (SIZE is the echo time: 25 ms at 0, 33 at 50, 44 at 100)
        { "Spring - Vintage Spring",       3,  -10.0f, 30.0f, 100.0f, 1.00f, 50.0f, 199.0f, false },  // 2.5 s
        { "Spring - Surf Guitar",          3,   20.0f, 45.0f, 100.0f, 1.20f, 65.0f, 199.0f, false },  // 3.0 s
        { "Spring - Dub Spring",           3,  -30.0f, 45.0f,  90.0f, 1.40f, 80.0f, 142.0f, true  },  // 3.5 s
        { "Spring - Twang",                3,   40.0f, 35.0f, 100.0f, 0.70f, 30.0f, 199.0f, false },  // 1.75 s
        { "Spring - Amp Spring",           3,    0.0f, 25.0f, 100.0f, 0.80f, 40.0f, 199.0f, false },  // 2.0 s
        { "Spring - Dark Tank",            3,  -55.0f, 38.0f, 100.0f, 1.10f, 60.0f, 140.0f, true  },  // 2.75 s
        { "Spring - Bright Tank",          3,   65.0f, 40.0f,  95.0f, 1.30f, 70.0f, 160.0f, true  },  // 3.25 s
        { "Spring - Send",                 3,    0.0f,100.0f,   0.0f, 1.00f, 50.0f, 150.0f, true  },  // 2.5 s

        // === PLATE === (SIZE is spread wide: the plate's range is the narrowest)
        { "Plate - Studio Plate",          4,   10.0f, 30.0f, 100.0f, 0.80f, 40.0f, 199.0f, false },  // 2.0 s
        { "Plate - Shimmer Plate",         4,   40.0f, 35.0f, 100.0f, 1.40f, 85.0f, 199.0f, false },  // 3.5 s
        { "Plate - Vocal Plate",           4,    0.0f, 25.0f, 100.0f, 0.70f, 30.0f, 199.0f, false },  // 1.75 s
        { "Plate - Lush Plate",            4,  -10.0f, 40.0f,  95.0f, 1.30f, 70.0f, 119.0f, true  },  // 3.25 s
        { "Plate - Snare Plate",           4,   30.0f, 30.0f, 100.0f, 0.50f, 10.0f, 180.0f, true  },  // 1.25 s
        { "Plate - Dark Plate",            4,  -45.0f, 35.0f, 100.0f, 1.10f, 60.0f, 120.0f, true  },  // 2.75 s
        { "Plate - Long Plate",            4,   15.0f, 38.0f,  95.0f, 1.80f, 90.0f, 120.0f, true  },  // 4.5 s
        { "Plate - Send",                  4,    0.0f,100.0f,   0.0f, 1.00f, 50.0f, 150.0f, true  },  // 2.5 s

        // === AMBIENT ===
        { "Ambient - Pad Wash",            5,  -20.0f, 45.0f,  80.0f, 1.20f, 75.0f, 119.0f, true  },  // 8.4 s
        { "Ambient - Infinite Drone",      5,  -30.0f, 55.0f,  55.0f, 2.00f,100.0f, 100.0f, true  },  // 14 s
        { "Ambient - Ethereal",            5,   10.0f, 50.0f,  70.0f, 1.30f, 85.0f, 199.0f, false },  // 9.1 s
        { "Ambient - Cloud Nine",          5,    0.0f, 55.0f,  65.0f, 1.50f, 90.0f, 142.0f, true  },  // 10.5 s
        { "Ambient - Frozen Lake",         5,  -55.0f, 50.0f,  60.0f, 1.80f,100.0f, 140.0f, true  },  // 12.6 s
        { "Ambient - Glass Haze",          5,   50.0f, 40.0f,  80.0f, 1.00f, 70.0f, 160.0f, true  },  // 7.0 s
        { "Ambient - Soft Halo",           5,  -10.0f, 35.0f,  90.0f, 0.70f, 55.0f, 100.0f, true  },  // 4.9 s
        { "Ambient - Send",                5,    0.0f,100.0f,   0.0f, 1.00f, 60.0f, 120.0f, true  },  // 7.0 s
    };

    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets;
    for (const auto& d : bank) {
        auto norm = [this](const char* id, float v) { return parameters.getParameter(id)->convertTo0to1(v); };
        factoryPresets.push_back({ d.name, {
            { "TYPE", norm("TYPE", (float) d.type) }, { "CHARACTER", norm("CHARACTER", d.character) },
            { "WET", norm("WET", d.wet) }, { "DRY", norm("DRY", d.dry) },
            { "DECAY", norm("DECAY", d.decay) }, { "SIZE", norm("SIZE", d.size) },
            { "LPFREQ", norm("LPFREQ", d.lowCutHz) }, { "LPON", d.lowCut ? 1.0f : 0.0f }
        }, juce::var() });
    }

    // v1.14.0: the module (re)writes only the files the table names, so a
    // renamed preset ("Dub Echo") would stay in the installed bank. When the
    // bank is about to be rewritten (version sentinel differs), first remove
    // factory files the table no longer names.
    const auto factoryDir = presetManager.getFactoryPresetsDirectory();
    const auto sentinel = factoryDir.getChildFile(".factory-version");
    if (! (sentinel.existsAsFile() && sentinel.loadFileAsString().trim() == JucePlugin_VersionString)) {
        juce::StringArray names;
        for (const auto& d : bank) names.add(d.name);
        for (const auto& f : factoryDir.findChildFiles(juce::File::findFiles, false, "*.json"))
            if (! names.contains(f.getFileNameWithoutExtension()))
                f.deleteFile();
    }

    presetManager.initializeFactoryPresets(factoryPresets);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleReverbAudioProcessor();
}
