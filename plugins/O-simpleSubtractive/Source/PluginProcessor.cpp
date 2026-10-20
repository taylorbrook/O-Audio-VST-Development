/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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

    O-simpleSubtractive - Audio Processor (implementation)

    Stage 2 (DSP): audible polyphonic subtractive synth. OscillatorBank ->
    multimode ZDF SVF -> VCA, two independent ADSRs, Poly/Mono/Legato + glide,
    real-time-safe viz tap. No oversampling (PolyBLEP band-limits at source) ->
    zero latency. processBlock is allocation-free (PERF-01).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Shared skews (match ARCHITECTURE.md -> Parameter Mapping).
    constexpr float kAdsrTimeSkew = 0.35f; // perceptual skew for 0–5 s envelope times
    constexpr float kMinAdsrTime  = 0.0f;
    constexpr float kMaxAdsrTime  = 5.0f;

    // 0–5 s ADSR time range with perceptual skew.
    juce::NormalisableRange<float> adsrTimeRange()
    {
        return { kMinAdsrTime, kMaxAdsrTime, 0.0001f, kAdsrTimeSkew };
    }

    // 0–1 normalized "percent" range (stored 0–1; UI scales x100 in Stage 3).
    juce::NormalisableRange<float> unitRange()
    {
        return { 0.0f, 1.0f, 0.0001f };
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleSubtractiveAudioProcessor::createParameterLayout()
{
    using namespace OSimpleSubtractive::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //--- Oscillator / sources ----------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { oscWave, 1 }, "Oscillator Wave",
        juce::StringArray { "Saw", "Square", "Triangle", "Sine" }, 0)); // Saw

    // v1.7.0 — Square duty cycle. 0.5 is the pre-1.7.0 square, bit for bit.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { pulseWidth, 1 }, "Pulse Width",
        juce::NormalisableRange<float> { 0.05f, 0.95f, 0.0001f }, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { subLevel, 1 }, "Sub Level", unitRange(), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { noiseLevel, 1 }, "Noise Level", unitRange(), 0.0f));

    //--- Oscillator 2 (v1.7.0) — silent by default (level 0) ---------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { osc2Wave, 1 }, "Osc 2 Wave",
        juce::StringArray { "Saw", "Square", "Triangle", "Sine" }, 0)); // Saw

    params.push_back (std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { osc2Coarse, 1 }, "Osc 2 Coarse", -24, 24, 0,
        juce::AudioParameterIntAttributes().withLabel ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { osc2Fine, 1 }, "Osc 2 Fine",
        juce::NormalisableRange<float> { -50.0f, 50.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ct")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { osc2Level, 1 }, "Osc 2 Level", unitRange(), 0.0f));

    //--- Filter ------------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { filterType, 1 }, "Filter Type",
        juce::StringArray { "Low-pass", "High-pass", "Band-pass", "Notch" }, 0)); // LP

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { filterSlope, 1 }, "Filter Slope",
        juce::StringArray { "6 dB/oct", "12 dB/oct", "24 dB/oct" }, 2)); // 24 dB/oct

    // Base cutoff: log skew (~0.25) over the full audio band, default 2 kHz.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { cutoff, 1 }, "Cutoff",
        juce::NormalisableRange<float> { 20.0f, 20000.0f, 0.0f, 0.25f }, 2000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { resonance, 1 }, "Resonance", unitRange(), 0.10f));

    // Bipolar filter-envelope depth (-100..+100%), default +50%.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterEnvAmount, 1 }, "Filter Env Amount",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.0001f }, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { keyTrack, 1 }, "Key Track", unitRange(), 0.0f));

    // v1.7.0 — velocity scales the filter-envelope depth. 0 = ignore velocity.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { velToFilter, 1 }, "Velocity to Filter", unitRange(), 0.0f));

    //--- The one global LFO (v1.7.0) — every depth defaults to 0 ------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { lfoShape, 1 }, "LFO Shape",
        juce::StringArray { "Triangle", "Square", "Saw", "S&H" }, 0)); // Triangle

    // 0.02–20 Hz, log-ish skew so 2 Hz sits near the middle of the knob.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { lfoRate, 1 }, "LFO Rate",
        juce::NormalisableRange<float> { 0.02f, 20.0f, 0.0f, 0.3f }, 2.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { lfoToCutoff, 1 }, "LFO to Cutoff", unitRange(), 0.0f));

    // Semitones of swing at full LFO; skewed so the first half of the knob is
    // vibrato territory (0–2 st) and the top reaches an octave.
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { lfoToPitch, 1 }, "LFO to Pitch",
        juce::NormalisableRange<float> { 0.0f, 12.0f, 0.0f, 0.4f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { lfoToPw, 1 }, "LFO to Pulse Width", unitRange(), 0.0f));

    //--- Filter envelope (ADSR -> cutoff) ----------------------------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterAttack, 1 }, "Filter Attack", adsrTimeRange(), 0.005f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterDecay, 1 }, "Filter Decay", adsrTimeRange(), 0.30f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterSustain, 1 }, "Filter Sustain", unitRange(), 0.40f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { filterRelease, 1 }, "Filter Release", adsrTimeRange(), 0.20f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Amplitude envelope (ADSR -> output + voice lifetime) --------------
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampAttack, 1 }, "Amp Attack", adsrTimeRange(), 0.005f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampDecay, 1 }, "Amp Decay", adsrTimeRange(), 0.30f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampSustain, 1 }, "Amp Sustain", unitRange(), 0.80f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ampRelease, 1 }, "Amp Release", adsrTimeRange(), 0.10f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Voicing -----------------------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { voiceMode, 1 }, "Voice Mode",
        juce::StringArray { "Poly", "Mono", "Legato" }, 0)); // Poly

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { glide, 1 }, "Glide",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0001f, 0.5f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    //--- Output ------------------------------------------------------------
    // -60 dB floor represents "-inf" (silence).
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputLevel, 1 }, "Output Level",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f }, -6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

//==============================================================================
OSimpleSubtractiveAudioProcessor::OSimpleSubtractiveAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Pre-allocate all voices up front (no audio-thread allocation later).
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new SubVoice());

    synth.addSound (new SubSound());          // single shared sound, all notes/channels
    synth.setNoteStealingEnabled (true);
}

OSimpleSubtractiveAudioProcessor::~OSimpleSubtractiveAudioProcessor() = default;

//==============================================================================
void OSimpleSubtractiveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // On-screen-keyboard MIDI runs in host sample space.
    midiCollector.reset (sampleRate);

    // No oversampling: PolyBLEP band-limits at source -> zero added latency.
    // getLatencySamples() is non-virtual in JUCE 8 — set the stored value.
    setLatencySamples (0);

    // Synthesiser + per-voice prepare at the HOST rate. SynthesiserVoice has no
    // virtual prepareToPlay in JUCE 8 — dispatch the custom one via dynamic_cast.
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SubVoice*> (synth.getVoice (v)))
            sv->prepareToPlay (sampleRate, samplesPerBlock);

    outputGain.reset (sampleRate, 0.02);
    const float outDb = parameters.getRawParameterValue (OSimpleSubtractive::ParamIDs::outputLevel)->load();
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));

    monoStack.clear();
    lastVoiceMode = (int) parameters.getRawParameterValue (OSimpleSubtractive::ParamIDs::voiceMode)->load();

    // The LFO restarts from phase 0 at prepare (a transport restart on most
    // hosts), never on a note — it is free-running across the whole instrument.
    lfoPhase = 0.0;
    lfoCycle = 0;
}

void OSimpleSubtractiveAudioProcessor::releaseResources()
{
    synth.allNotesOff (0, false);
    monoStack.clear();
}

bool OSimpleSubtractiveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
void OSimpleSubtractiveAudioProcessor::pushParamsToVoices (int numSamples)
{
    using namespace OSimpleSubtractive::ParamIDs;
    auto get = [this] (const char* id) { return parameters.getRawParameterValue (id)->load(); };

    SubVoiceParams p;
    p.wave            = (int) get (oscWave);
    p.pulseWidth      = get (pulseWidth);
    p.subLevel        = get (subLevel);
    p.noiseLevel      = get (noiseLevel);

    p.osc2Wave        = (int) get (osc2Wave);
    p.osc2Ratio       = (float) std::exp2 (((double) get (osc2Coarse) + (double) get (osc2Fine) / 100.0) / 12.0);
    p.osc2Level       = get (osc2Level);

    p.filterType      = (int) get (filterType);
    p.slope           = (int) get (filterSlope);
    p.cutoffHz        = get (cutoff);
    p.resonance       = get (resonance);
    p.filterEnvAmount = get (filterEnvAmount);
    p.keyTrack        = get (keyTrack);
    p.velToFilter     = get (velToFilter);

    p.filt = { get (filterAttack), get (filterDecay), get (filterSustain), get (filterRelease) };
    p.amp  = { get (ampAttack),    get (ampDecay),    get (ampSustain),    get (ampRelease) };
    p.glideSeconds    = get (glide);

    // The global LFO: this block's start state. The rate is read once per block,
    // so a rate move lands at the next block boundary with the phase continuous.
    lfoIncThisBlock   = (double) get (lfoRate) / juce::jmax (1.0, currentSampleRate);
    p.lfo.shape       = (int) get (lfoShape);
    p.lfo.phase0      = lfoPhase;
    p.lfo.inc         = lfoIncThisBlock;
    p.lfo.cycle0      = lfoCycle;
    p.lfoToCutoff     = get (lfoToCutoff);
    p.lfoToPitchSemis = get (lfoToPitch);
    p.lfoToPw         = get (lfoToPw);

    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SubVoice*> (synth.getVoice (v)))
            sv->setParams (p);

    // Publish the static filter selectors for the display (cutoff/k come from the
    // live lead voice after rendering), and the LFO's value at the END of this
    // block for the LFO canvas playhead.
    displayType.store (p.filterType, std::memory_order_relaxed);
    displaySlope.store (p.slope, std::memory_order_relaxed);
    lfoDisplayValue.store (p.lfo.at (numSamples), std::memory_order_relaxed);
    {
        const double pe = lfoPhase + (double) numSamples * lfoIncThisBlock;
        lfoDisplayPhase.store ((float) (pe - std::floor (pe)), std::memory_order_relaxed);
    }
}

void OSimpleSubtractiveAudioProcessor::advanceLfo (int numSamples)
{
    const double p = lfoPhase + (double) numSamples * lfoIncThisBlock;
    const double c = std::floor (p);
    lfoCycle += (uint32_t) c;
    lfoPhase  = p - c;
}

void OSimpleSubtractiveAudioProcessor::handleUiMidi (int noteNumber, bool noteOn, float velocity)
{
    auto msg = noteOn
        ? juce::MidiMessage::noteOn  (1, noteNumber, juce::jlimit (0.0f, 1.0f, velocity))
        : juce::MidiMessage::noteOff (1, noteNumber);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    midiCollector.addMessageToQueue (msg);
}

//==============================================================================
// Concept-preset tour (UI-07 / FUNC-06) — the 8 named patches, each isolating
// ONE subtractive idea. Writes flow through the public host API
// (setValueNotifyingHost), so the relays/attachments sync every knob/combo back
// to the page automatically — no DOM poking, and NO DSP touched here.
// Model: O-simpleGrain::applyFactoryPreset.
//
// Range reminders (createParameterLayout): cutoff 20–20 kHz (Hz); resonance,
// sustains, keyTrack, sub/noiseLevel 0–1; filterEnvAmount −1…+1 (octaves, bipolar);
// ADSR times in seconds (0–5). Choice indices — oscWave: 0 Saw 1 Square 2 Triangle
// 3 Sine; filterType: 0 LP 1 HP 2 BP 3 Notch; filterSlope: 0 = 6 1 = 12 2 = 24 dB;
// voiceMode: 0 Poly 1 Mono 2 Legato. Names MUST match the data-preset keys in
// index.html / the LESSONS map in app.js (a mismatch silently no-ops the button).
void OSimpleSubtractiveAudioProcessor::applyFactoryPreset (const juce::String& name)
{
    using namespace OSimpleSubtractive::ParamIDs;

    // 1. Reset every parameter to its default — a clean slate per concept.
    //    v1.7.2: except Output Level, which is USER-OWNED — no lesson below names
    //    it either, so loading a preset leaves the level where the user put it.
    for (auto* p : getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (isUserOwnedParameter (ranged->getParameterID()))
                continue;
        p->setValueNotifyingHost (p->getDefaultValue());
    }

    // 2. Snapshot helpers (real value → normalised; choices by index).
    auto setReal = [this] (const char* id, float real) {
        if (auto* p = parameters.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };
    auto setChoice = [this] (const char* id, int index) {
        if (auto* p = parameters.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    };

    // ── Saw → LP Sweep — the headline subtraction: a bright saw through a 24 dB
    //    low-pass with a slow filter envelope; harmonics fall under the curve.
    if (name == "Saw Sweep")
    {
        setChoice (oscWave, 0);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 600.0f);  setReal (resonance, 0.25f);  setReal (filterEnvAmount, 0.7f);
        setReal (filterAttack, 0.40f); setReal (filterDecay, 1.50f); setReal (filterSustain, 0.50f); setReal (filterRelease, 0.60f);
        setReal (ampAttack, 0.02f); setReal (ampSustain, 0.90f); setReal (ampRelease, 0.40f);
    }
    // ── Pluck — a fast filter envelope (short decay, zero sustain) snaps the
    //    cutoff bright-then-dark over a quick amp decay → percussive.
    else if (name == "Pluck")
    {
        setChoice (oscWave, 0);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 400.0f);  setReal (resonance, 0.35f);  setReal (filterEnvAmount, 0.85f);
        setReal (filterAttack, 0.002f); setReal (filterDecay, 0.18f); setReal (filterSustain, 0.0f); setReal (filterRelease, 0.15f);
        setReal (ampAttack, 0.002f); setReal (ampDecay, 0.35f); setReal (ampSustain, 0.0f); setReal (ampRelease, 0.18f);
    }
    // ── Brass Stab — positive filter-env amount opens the cutoff with the attack
    //    and holds; key-track makes brightness follow pitch like a blown brass.
    else if (name == "Brass Stab")
    {
        setChoice (oscWave, 0);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 700.0f);  setReal (resonance, 0.20f);  setReal (filterEnvAmount, 0.6f);  setReal (keyTrack, 0.30f);
        setReal (filterAttack, 0.04f); setReal (filterDecay, 0.25f); setReal (filterSustain, 0.70f); setReal (filterRelease, 0.20f);
        setReal (ampAttack, 0.02f); setReal (ampDecay, 0.20f); setReal (ampSustain, 0.75f); setReal (ampRelease, 0.15f);
    }
    // ── Sweep Pad — slow amp swell + a long, deep filter sweep open the spectrum
    //    over seconds. Pads live in slow envelopes.
    else if (name == "Sweep Pad")
    {
        setChoice (oscWave, 0);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 350.0f);  setReal (resonance, 0.18f);  setReal (filterEnvAmount, 0.8f);
        setReal (filterAttack, 1.80f); setReal (filterDecay, 2.50f); setReal (filterSustain, 0.60f); setReal (filterRelease, 1.50f);
        setReal (ampAttack, 1.20f); setReal (ampDecay, 1.00f); setReal (ampSustain, 0.85f); setReal (ampRelease, 1.40f);
    }
    // ── Acid Bass (303) — high resonance + a snappy filter envelope on a saw
    //    through a 24 dB low-pass; mono with a touch of glide → the squelch.
    else if (name == "Acid Bass")
    {
        setChoice (oscWave, 0);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 300.0f);  setReal (resonance, 0.78f);  setReal (filterEnvAmount, 0.85f);  setReal (keyTrack, 0.15f);
        setReal (filterAttack, 0.003f); setReal (filterDecay, 0.22f); setReal (filterSustain, 0.10f); setReal (filterRelease, 0.15f);
        setReal (ampAttack, 0.003f); setReal (ampDecay, 0.40f); setReal (ampSustain, 0.70f); setReal (ampRelease, 0.12f);
        setChoice (voiceMode, 1);  setReal (glide, 0.06f);
    }
    // ── Hollow Square Bass — odd-harmonic square + sub one octave down for
    //    weight, played mono. The same voice a polysynth stacks in parallel.
    else if (name == "Square Bass")
    {
        setChoice (oscWave, 1);  setReal (subLevel, 0.50f);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 500.0f);  setReal (resonance, 0.20f);  setReal (filterEnvAmount, 0.4f);
        setReal (filterAttack, 0.005f); setReal (filterDecay, 0.25f); setReal (filterSustain, 0.30f); setReal (filterRelease, 0.15f);
        setReal (ampAttack, 0.004f); setReal (ampDecay, 0.30f); setReal (ampSustain, 0.85f); setReal (ampRelease, 0.12f);
        setChoice (voiceMode, 1);
    }
    // ── Filtered Noise (wind) — noise source up, band-passed with no harmonic
    //    source. The osc is a Sine well below the 1.5 kHz pass-band (rejected),
    //    so the ear hears sculpted air. Slow envelopes swell it like breath.
    else if (name == "Noise Wind")
    {
        setChoice (oscWave, 3);  setReal (noiseLevel, 1.0f);  setChoice (filterType, 2);  setChoice (filterSlope, 2);
        setReal (cutoff, 1500.0f);  setReal (resonance, 0.45f);  setReal (filterEnvAmount, 0.2f);  setReal (keyTrack, 0.50f);
        setReal (filterAttack, 0.80f); setReal (filterDecay, 1.20f); setReal (filterSustain, 0.70f); setReal (filterRelease, 1.00f);
        setReal (ampAttack, 0.60f); setReal (ampDecay, 0.80f); setReal (ampSustain, 0.80f); setReal (ampRelease, 0.90f);
    }
    // ── Self-Oscillation Sine — resonance at the limit: the filter rings into a
    //    pure sine. cutoff = C4 (ref note 60) with keyTrack = 100 % → fcEff tracks
    //    the keyboard chromatically, so the whistle plays in tune. Env amount 0
    //    keeps it a steady tone; a Sine source reinforces (osc can't be silenced).
    else if (name == "Self-Oscillation")
    {
        setChoice (oscWave, 3);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 261.63f);  setReal (resonance, 1.0f);  setReal (filterEnvAmount, 0.0f);  setReal (keyTrack, 1.0f);
        setReal (filterSustain, 1.0f);
        setReal (ampAttack, 0.05f); setReal (ampDecay, 0.20f); setReal (ampSustain, 0.90f); setReal (ampRelease, 0.40f);
    }
    // ── v1.7.0 — the four lessons for the new sections. lfoShape: 0 Triangle
    //    1 Square 2 Saw 3 S&H; lfoRate Hz; lfoToPitch in semitones; lfoToCutoff /
    //    lfoToPw are 0–1 depths; osc2Coarse semitones; osc2Fine cents.
    //
    // ── PWM Pad — a square whose width the LFO sweeps. The harmonic mix
    //    changes continuously (even harmonics come and go), which the ear hears
    //    as the chorus-like motion of a string machine. No second oscillator.
    else if (name == "PWM Pad")
    {
        setChoice (oscWave, 1);  setReal (pulseWidth, 0.5f);
        setChoice (lfoShape, 0);  setReal (lfoRate, 0.7f);  setReal (lfoToPw, 0.8f);
        setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 1800.0f);  setReal (resonance, 0.15f);  setReal (filterEnvAmount, 0.3f);
        setReal (filterAttack, 0.80f); setReal (filterDecay, 1.50f); setReal (filterSustain, 0.60f); setReal (filterRelease, 1.20f);
        setReal (ampAttack, 0.60f); setReal (ampDecay, 0.80f); setReal (ampSustain, 0.85f); setReal (ampRelease, 1.20f);
    }
    // ── Vibrato Lead — the LFO on PITCH, a third of a semitone at 5.5 Hz: the
    //    singer's vibrato. Legato with a little glide so phrases slur.
    else if (name == "Vibrato Lead")
    {
        setChoice (oscWave, 0);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 1500.0f);  setReal (resonance, 0.20f);  setReal (filterEnvAmount, 0.4f);  setReal (keyTrack, 0.30f);
        setChoice (lfoShape, 0);  setReal (lfoRate, 5.5f);  setReal (lfoToPitch, 0.35f);
        setReal (filterAttack, 0.03f); setReal (filterDecay, 0.40f); setReal (filterSustain, 0.50f); setReal (filterRelease, 0.20f);
        setReal (ampAttack, 0.01f); setReal (ampDecay, 0.30f); setReal (ampSustain, 0.90f); setReal (ampRelease, 0.20f);
        setChoice (voiceMode, 2);  setReal (glide, 0.05f);
    }
    // ── Wobble Bass — the LFO on CUTOFF: a resonant low-pass opened and closed
    //    at 2 Hz over a saw + sub. The filter sweep IS the rhythm.
    else if (name == "Wobble Bass")
    {
        setChoice (oscWave, 0);  setReal (subLevel, 0.40f);  setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 250.0f);  setReal (resonance, 0.55f);  setReal (filterEnvAmount, 0.2f);
        setChoice (lfoShape, 0);  setReal (lfoRate, 2.0f);  setReal (lfoToCutoff, 0.6f);
        setReal (filterAttack, 0.005f); setReal (filterDecay, 0.30f); setReal (filterSustain, 0.50f); setReal (filterRelease, 0.15f);
        setReal (ampAttack, 0.003f); setReal (ampDecay, 0.30f); setReal (ampSustain, 0.90f); setReal (ampRelease, 0.15f);
        setChoice (voiceMode, 1);
    }
    // ── Detuned Saws — the second oscillator at equal level, 12 cents sharp.
    //    Two saws almost in tune beat against each other: the "fat" sound every
    //    two-oscillator synth is bought for. Poly, so chords stack the beating.
    else if (name == "Detuned Saws")
    {
        setChoice (oscWave, 0);  setChoice (osc2Wave, 0);  setReal (osc2Fine, 12.0f);  setReal (osc2Level, 1.0f);
        setChoice (filterType, 0);  setChoice (filterSlope, 2);
        setReal (cutoff, 2500.0f);  setReal (resonance, 0.10f);  setReal (filterEnvAmount, 0.3f);
        setReal (filterAttack, 0.30f); setReal (filterDecay, 1.00f); setReal (filterSustain, 0.50f); setReal (filterRelease, 0.40f);
        setReal (ampAttack, 0.05f); setReal (ampDecay, 0.50f); setReal (ampSustain, 0.85f); setReal (ampRelease, 0.40f);
    }
}

//==============================================================================
// Mono / Legato: drive synth voice 0 directly with last-note priority + glide.
// mode: 1 = Mono (retrigger), 2 = Legato (slur — suppress retrigger while held).
void OSimpleSubtractiveAudioProcessor::renderMonoLegato (juce::AudioBuffer<float>& buffer,
                                                         juce::MidiBuffer& midi,
                                                         int numSamples, int mode)
{
    auto* v0 = dynamic_cast<SubVoice*> (synth.getVoice (0));
    if (v0 == nullptr) return;

    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos)
        {
            v0->renderNextBlock (buffer, pos, evPos - pos);
            pos = evPos;
        }

        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const bool wasHeld = ! monoStack.empty();
            monoStack.push (m.getNoteNumber(), m.getFloatVelocity());
            const bool retrigger = (mode == 1) || ! wasHeld;   // Legato slurs while held
            v0->noteOnDirect (m.getNoteNumber(), m.getFloatVelocity(), retrigger);
        }
        else if (m.isNoteOff())
        {
            monoStack.remove (m.getNoteNumber());
            if (monoStack.empty())
                v0->noteOffDirect (true);                      // release tail
            else
                v0->setPitchTargetNote (monoStack.topNote());  // fall back (glide, no retrigger)
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            monoStack.clear();
            v0->noteOffDirect (false);
        }
        else if (m.isPitchWheel())
        {
            v0->pitchWheelMoved (m.getPitchWheelValue());
        }
    }

    if (pos < numSamples)
        v0->renderNextBlock (buffer, pos, numSamples - pos);
}

//==============================================================================
void OSimpleSubtractiveAudioProcessor::updateDisplayFromLeadVoice()
{
    // Lead voice = the active voice with the largest current amp-env value
    // (the most prominent timbre). Drives the headline filter curve + dual-ADSR.
    SubVoice* lead = nullptr;
    float bestAmp = -1.0f;
    for (int v = 0; v < synth.getNumVoices(); ++v)
        if (auto* sv = dynamic_cast<SubVoice*> (synth.getVoice (v)))
            if (sv->isSounding())
            {
                const float a = sv->getAmpEnvVal();
                if (a > bestAmp) { bestAmp = a; lead = sv; }
            }

    if (lead != nullptr)
    {
        displayCutoffHz.store (lead->getLastCutoffHz(), std::memory_order_relaxed);
        displayK.store        (lead->getLastK(),        std::memory_order_relaxed);
        filterEnvValue.store  (lead->getFilterEnvVal(), std::memory_order_relaxed);
        ampEnvValue.store     (lead->getAmpEnvVal(),    std::memory_order_relaxed);
    }
    else
    {
        filterEnvValue.store (0.0f, std::memory_order_relaxed);
        ampEnvValue.store    (0.0f, std::memory_order_relaxed);
    }
}

//==============================================================================
void OSimpleSubtractiveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    buffer.clear();                              // voices ADD into a cleared buffer

    // Merge any on-screen-keyboard notes into the host MIDI stream.
    midiCollector.removeNextBlockOfMessages (midiMessages, numSamples);

    pushParamsToVoices (numSamples);

    const int mode = (int) parameters.getRawParameterValue (OSimpleSubtractive::ParamIDs::voiceMode)->load();

    // Hard-reset on a Poly<->Mono/Legato switch to avoid stuck/frozen voices.
    if (mode != lastVoiceMode)
    {
        synth.allNotesOff (0, false);
        monoStack.clear();
        lastVoiceMode = mode;
    }

    if (mode == 0)
    {
        synth.renderNextBlock (buffer, midiMessages, 0, numSamples);   // Poly
    }
    else
    {
        // Mono/Legato render voice 0 directly; the Synthesiser machinery (note
        // allocation) is bypassed, so pass nothing to renderNextBlock.
        renderMonoLegato (buffer, midiMessages, numSamples, mode);
    }

    // The global LFO ran this block in every voice; move its clock on.
    advanceLfo (numSamples);

    // Master output trim (dB->lin, smoothed).
    const float outDb = parameters.getRawParameterValue (OSimpleSubtractive::ParamIDs::outputLevel)->load();
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb, -60.0f));
    const float g0 = outputGain.getCurrentValue();
    const float g1 = outputGain.skip (numSamples);
    buffer.applyGainRamp (0, numSamples, g0, g1);

    // Suite-wide NaN insurance.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }

    updateDisplayFromLeadVoice();

    // Visualization tap: post-gain mono sum -> lock-free ring (copy-only, no alloc).
    if (numCh == 1)
    {
        vizRing.write (buffer.getReadPointer (0), numSamples);
    }
    else if (numCh > 1)
    {
        constexpr int kChunk = 4096;
        float mono[kChunk];
        int done = 0;
        while (done < numSamples)
        {
            const int n = juce::jmin (kChunk, numSamples - done);
            for (int i = 0; i < n; ++i)
            {
                float s = 0.0f;
                for (int ch = 0; ch < numCh; ++ch)
                    s += buffer.getReadPointer (ch)[done + i];
                mono[i] = s / (float) numCh;
            }
            vizRing.write (mono, n);
            done += n;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* OSimpleSubtractiveAudioProcessor::createEditor()
{
    return new OSimpleSubtractiveAudioProcessorEditor (*this);
}

//==============================================================================
void OSimpleSubtractiveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // v1.3.0: the interface LANGUAGE rides the same tree as a plain property,
    // not a parameter. Written as a STRING ("en"/"fr") rather than the atomic's
    // int index, so a hand-inspected session file says what it means — and
    // because the ValueTree -> XML round-trip rebuilds every property as a
    // string var anyway (critical_valuetree_xml_roundtrip_loses_type). Storing
    // the code means the value that comes back is the value that went in, with
    // no type predicate to misfire on the way.
    state.setProperty ("uiLanguage",
                       languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleSubtractiveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xml));

            // v1.3.0. A pre-1.3.0 session has no property, getProperty returns
            // a VOID var, and the default (English) stands. isVoid() is the
            // only correct gate — the value comes back as a STRING var, so a
            // type predicate like isInt() would never fire. languageIndex()
            // clamps anything that is not "fr" to 0, so a hand-edited value
            // degrades to English rather than to a bad index.
            const juce::var lang = parameters.state.getProperty ("uiLanguage");
            if (! lang.isVoid())
                uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OSimpleSubtractiveAudioProcessor();
}
