/*
  ==============================================================================

    O-IntonationPad - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/WavetableSound.h"

juce::AudioProcessorValueTreeState::ParameterLayout OIntonationPadAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // VOICE_COUNT - Int (2-12, default: 5)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "voiceCount", 1 },
        "Voice Count",
        2, 12,
        5
    ));

    // COMPLEXITY - Float (0-100%, default: 50%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "complexity", 1 },
        "Complexity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // KEY_ROOT - Choice (C-B, default: C)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "keyRoot", 1 },
        "Key Root",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
        0
    ));

    // v1.5.0: keyScale parameter removed — replaced by dynamic interval selection

    // TUNING: Master Tune (A4 reference, 400-480 Hz, default 440)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tuning_masterTune", 1 },
        "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // TUNING: Octave Stretch (0.95-1.25, default 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tuning_octaveStretch", 1 },
        "Octave Stretch",
        juce::NormalisableRange<float>(0.95f, 1.25f, 0.001f),
        1.0f));

    // TUNING: Pitch Bend Range (1-48 semitones, default 2)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tuning_pitchBendRange", 1 },
        "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 48.0f, 1.0f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    // TUNING: Temperament Preset
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuning_temperamentPreset", 1 },
        "Temperament",
        juce::StringArray {
            "Equal 12-TET", "Pythagorean", "Zarlino", "Meantone (1/4)",
            "Werckmeister III", "Kirnberger III", "Vallotti",
            "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom" },
        8));  // Default: Just Intonation (restores pre-v1.3.0 behavior)

    // v1.13.0: STEREO_SPREAD - Float (0-100%, default: 50%) — per-voice stereo panning
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stereoSpread", 1 },
        "Stereo Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // SPACING - Float (0-100%, default: 0%) — shifts voices UP by 1-3 octaves
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spacing", 1 },
        "Spacing",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // INVERSION - Float (0-100%, default: 30%) — shifts voices DOWN by 1-3 octaves
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inversion", 1 },
        "Inversion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // TIMING_RANDOM - Float (0-100ms, default: 10ms)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "timingRandom", 1 },
        "Timing Random",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        10.0f,
        "ms"
    ));

    // DETUNE_RANDOM - Float (0-50 cents, default: 5 cents)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "detuneRandom", 1 },
        "Detune Random",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        5.0f,
        "cents"
    ));

    // WAVETABLE_BANK - Choice (16 banks, default: 0 = JI Harmonic)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "wavetableBank", 1 },
        "Wavetable A",
        juce::StringArray {
            "JI Harmonic", "Warm Analog", "Choir", "Strings",
            "Glass", "Evolving", "Organ", "Ethereal", "Dark Matter",
            "Sine", "Square", "Triangle",
            "Spectral Cloud", "Metallic Resonance", "Formant Vowel", "Warm Sub" },
        0
    ));

    // WAVETABLE_POS - Float (0-100%, default: 50%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wavetablePos", 1 },
        "Position A",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // v1.8.0: WAVETABLE_BANK2 - Second oscillator bank
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "wavetableBank2", 1 },
        "Wavetable B",
        juce::StringArray {
            "JI Harmonic", "Warm Analog", "Choir", "Strings",
            "Glass", "Evolving", "Organ", "Ethereal", "Dark Matter",
            "Sine", "Square", "Triangle",
            "Spectral Cloud", "Metallic Resonance", "Formant Vowel", "Warm Sub" },
        1
    ));

    // v1.8.0: WAVETABLE_POS2 - Second oscillator position
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wavetablePos2", 1 },
        "Position B",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // v1.9.0: Independent gain controls for Osc A and Osc B
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gainA", 1 },
        "Gain A",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "gainB", 1 },
        "Gain B",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // LFO_RATE - Float (0.01-20 Hz, default: 0.5 Hz, exponential)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lfoRate", 1 },
        "LFO Rate A",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f),  // skew = 0.3 for exponential
        0.5f,
        "Hz"
    ));

    // v1.10.0: LFO_RATE2 - Independent LFO rate for Osc B
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lfoRate2", 1 },
        "LFO Rate B",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f),
        0.5f,
        "Hz"
    ));

    // LFO_DEPTH - Float (0-100%, default: 25%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lfoDepth", 1 },
        "LFO Depth A",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.25f
    ));

    // v1.8.0: LFO_DEPTH2 - Independent LFO depth for Osc B
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lfoDepth2", 1 },
        "LFO Depth B",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // ATTACK_TIME - Float (1-5000 ms, default: 500 ms, exponential)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackTime", 1 },
        "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f),  // skew = 0.3 for exponential
        0.5f,
        "s"
    ));

    // RELEASE_TIME - Float (10-10000 ms, default: 2000 ms, exponential)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "releaseTime", 1 },
        "Release",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.4f),  // skew = 0.4 for exponential
        2.0f,
        "s"
    ));

    // FILTER_CUTOFF - Float (20-20000 Hz, default: 8000 Hz, logarithmic)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "filterCutoff", 1 },
        "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),  // skew = 0.25 for logarithmic
        8000.0f,
        "Hz"
    ));

    // MASTER_VOLUME - Float (0.0-1.26 gain, default: 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterVolume", 1 },
        "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.26f, 0.01f),
        1.0f
    ));

    // ═══════════════════════════════════════════════════════════════════
    // v1.11.0: EFFECTS PARAMETERS (Chorus, Delay, EQ, Reverb)
    // ═══════════════════════════════════════════════════════════════════

    // --- Chorus ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "chorusBypass", 1 }, "Chorus Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // --- Delay ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "delayBypass", 1 }, "Delay Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTime", 1 }, "Delay Time",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.35f), 0.375f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayFeedback", 1 }, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "delayMode", 1 }, "Delay Mode",
        juce::StringArray { "Normal", "PingPong" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayMix", 1 }, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // --- EQ (3-band) ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "eqBypass", 1 }, "EQ Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqLowGain", 1 }, "EQ Low Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqMidGain", 1 }, "EQ Mid Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqMidFreq", 1 }, "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 0.1f, 0.35f), 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "eqHighGain", 1 }, "EQ High Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // --- Reverb ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "reverbBypass", 1 }, "Reverb Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbSize", 1 }, "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbDamp", 1 }, "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbPredelay", 1 }, "Reverb Pre-delay",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f, 0.5f), 20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    return layout;
}

OIntonationPadAudioProcessor::OIntonationPadAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize synthesiser with 8 voices (Phase 2.1: single oscillator per voice)
    synthesiser.addSound(new WavetableSound());

    for (int i = 0; i < 8; ++i)
    {
        synthesiser.addVoice(new WavetableVoice());
    }

    // Register tuning parameter listeners
    parameters.addParameterListener("tuning_masterTune", this);
    parameters.addParameterListener("tuning_octaveStretch", this);
    parameters.addParameterListener("tuning_pitchBendRange", this);
    parameters.addParameterListener("tuning_temperamentPreset", this);

    // Initialize TuningEngine to match parameter defaults
    // JUCE doesn't fire parameterChanged for initial values, so set explicitly
    tuningEngine.setBuiltInPreset(TuningEngine::BuiltInPreset::JustIntonation);

    // v1.5.0: Initialize enabled intervals (all enabled by default)
    resetEnabledIntervals();
}

OIntonationPadAudioProcessor::~OIntonationPadAudioProcessor()
{
}

void OIntonationPadAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare synthesiser
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // Shared ProcessSpec for all DSP modules
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    filter.prepare(spec);
    filter.reset();
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setResonance(0.707f);  // Butterworth response

    // Initialize LFOs (independent per oscillator)
    // Phase increments are computed per-block in processBlock — no need to seed here
    lfoPhaseA = 0.0;
    lfoPhaseB = 0.0;

    // Update envelope parameters for all voices
    float attackTime = parameters.getRawParameterValue("attackTime")->load();
    float releaseTime = parameters.getRawParameterValue("releaseTime")->load();

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = static_cast<WavetableVoice*>(synthesiser.getVoice(i)))
        {
            voice->prepare(samplesPerBlock);
            voice->setEnvelopeParameters(attackTime, releaseTime);
        }
    }

    // v1.11.0: Prepare effects chain
    chorus.prepare(spec);
    chorus.reset();
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    delay.prepare(spec);
    delay.reset();

    eq.prepare(spec);
    eq.reset();

    reverbProcessor.prepare(spec);
    reverbProcessor.reset();
}

void OIntonationPadAudioProcessor::releaseResources()
{
}

void OIntonationPadAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // Read parameters (atomic, real-time safe)
    int wavetableBank = static_cast<int>(parameters.getRawParameterValue("wavetableBank")->load());
    float wavetablePos = parameters.getRawParameterValue("wavetablePos")->load();
    int wavetableBank2 = static_cast<int>(parameters.getRawParameterValue("wavetableBank2")->load());
    float wavetablePos2 = parameters.getRawParameterValue("wavetablePos2")->load();
    float gainA = parameters.getRawParameterValue("gainA")->load();
    float gainB = parameters.getRawParameterValue("gainB")->load();
    float attackTime = parameters.getRawParameterValue("attackTime")->load();
    float releaseTime = parameters.getRawParameterValue("releaseTime")->load();
    int voiceCount = static_cast<int>(parameters.getRawParameterValue("voiceCount")->load());
    float complexity = parameters.getRawParameterValue("complexity")->load();
    int keyRoot = static_cast<int>(parameters.getRawParameterValue("keyRoot")->load());
    float lfoRate = parameters.getRawParameterValue("lfoRate")->load();
    float lfoRate2 = parameters.getRawParameterValue("lfoRate2")->load();
    float lfoDepth = parameters.getRawParameterValue("lfoDepth")->load();
    float lfoDepth2 = parameters.getRawParameterValue("lfoDepth2")->load();
    float filterCutoff = parameters.getRawParameterValue("filterCutoff")->load();
    float masterVolume = parameters.getRawParameterValue("masterVolume")->load();
    float stereoSpread = parameters.getRawParameterValue("stereoSpread")->load();
    float spacing = parameters.getRawParameterValue("spacing")->load();
    float inversion = parameters.getRawParameterValue("inversion")->load();
    float detuneRandom = parameters.getRawParameterValue("detuneRandom")->load();
    float timingRandom = parameters.getRawParameterValue("timingRandom")->load();

    // Update LFO phase increments (independent per oscillator)
    lfoPhaseIncrementA = (lfoRate * juce::MathConstants<double>::twoPi) / getSampleRate();
    lfoPhaseIncrementB = (lfoRate2 * juce::MathConstants<double>::twoPi) / getSampleRate();

    // v1.14.0: Capture current LFO phases before advancing (passed to voices for per-sub-voice offsets)
    float currentLfoPhaseA = static_cast<float>(lfoPhaseA);
    float currentLfoPhaseB = static_cast<float>(lfoPhaseB);

    // Advance LFO phases for next block
    lfoPhaseA += lfoPhaseIncrementA * buffer.getNumSamples();
    lfoPhaseB += lfoPhaseIncrementB * buffer.getNumSamples();
    if (lfoPhaseA >= juce::MathConstants<double>::twoPi)
        lfoPhaseA -= juce::MathConstants<double>::twoPi;
    if (lfoPhaseB >= juce::MathConstants<double>::twoPi)
        lfoPhaseB -= juce::MathConstants<double>::twoPi;

    // v1.15.2: Read interval snapshot (lock-free, no mutex — published by UI thread)
    auto snap = std::atomic_load(&intervalSnapshot_);
    const auto& currentEnabledDegrees = snap ? snap->enabledDegrees : std::vector<int>{ 0 };
    int currentScaleDegreeCount = snap ? snap->scaleDegreeCount : 12;

    // Update all voice parameters before rendering
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = static_cast<WavetableVoice*>(synthesiser.getVoice(i)))
        {
            voice->setWavetableBank(wavetableBank);
            voice->setWavetablePositionWithLFO(wavetablePos, currentLfoPhaseA, lfoDepth);
            voice->setWavetableBank2(wavetableBank2);
            voice->setWavetablePosition2WithLFO(wavetablePos2, currentLfoPhaseB, lfoDepth2);
            voice->setGainA(gainA);
            voice->setGainB(gainB);
            voice->setStereoSpread(stereoSpread);
            voice->setEnvelopeParameters(attackTime, releaseTime);

            // Store chord generation parameters for voices to use on note-on
            voice->setChordGenerationParams(voiceCount, complexity, keyRoot,
                                            currentEnabledDegrees, currentScaleDegreeCount,
                                            spacing, inversion, detuneRandom, timingRandom,
                                            &chordGenerator, &tuningEngine, &randomGenerator);
        }
    }

    // Render synthesiser output (handles MIDI internally)
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply filter
    filter.setCutoffFrequency(filterCutoff);
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> filterContext(block);
    filter.process(filterContext);

    // ═══════════════════════════════════════════════════════════════════
    // v1.11.0: EFFECTS CHAIN (Chorus -> Delay -> EQ -> Reverb)
    // ═══════════════════════════════════════════════════════════════════

    // Chorus
    bool chorusBypassed = parameters.getRawParameterValue("chorusBypass")->load() > 0.5f;
    if (!chorusBypassed)
    {
        float chorusRate = parameters.getRawParameterValue("chorusRate")->load();
        float chorusDepth = parameters.getRawParameterValue("chorusDepth")->load();
        float chorusMix = parameters.getRawParameterValue("chorusMix")->load();
        chorus.setRate(chorusRate);
        chorus.setDepth(chorusDepth);
        chorus.setMix(chorusMix);
        if (chorusMix > 0.001f)
        {
            juce::dsp::ProcessContextReplacing<float> chorusCtx(block);
            chorus.process(chorusCtx);
        }
    }

    // Delay
    bool delayBypassed = parameters.getRawParameterValue("delayBypass")->load() > 0.5f;
    if (!delayBypassed)
    {
        float delayTime = parameters.getRawParameterValue("delayTime")->load();
        float delayFeedback = parameters.getRawParameterValue("delayFeedback")->load();
        int delayModeVal = static_cast<int>(parameters.getRawParameterValue("delayMode")->load());
        float delayMix = parameters.getRawParameterValue("delayMix")->load();
        delay.setTime(delayTime);
        delay.setFeedback(delayFeedback);
        delay.setMode(delayModeVal);
        delay.setMix(delayMix);
        if (delayMix > 0.001f)
            delay.process(block);
    }

    // EQ
    bool eqBypassed = parameters.getRawParameterValue("eqBypass")->load() > 0.5f;
    if (!eqBypassed)
    {
        float eqLowGain = parameters.getRawParameterValue("eqLowGain")->load();
        float eqMidGain = parameters.getRawParameterValue("eqMidGain")->load();
        float eqMidFreq = parameters.getRawParameterValue("eqMidFreq")->load();
        float eqHighGain = parameters.getRawParameterValue("eqHighGain")->load();
        eq.setLowGain(eqLowGain);
        eq.setMidGain(eqMidGain);
        eq.setMidFreq(eqMidFreq);
        eq.setHighGain(eqHighGain);
        if (std::abs(eqLowGain) > 0.1f || std::abs(eqMidGain) > 0.1f || std::abs(eqHighGain) > 0.1f)
            eq.process(block);
    }

    // Reverb
    bool reverbBypassed = parameters.getRawParameterValue("reverbBypass")->load() > 0.5f;
    if (!reverbBypassed)
    {
        float reverbSize = parameters.getRawParameterValue("reverbSize")->load();
        float reverbDamp = parameters.getRawParameterValue("reverbDamp")->load();
        float reverbPredelay = parameters.getRawParameterValue("reverbPredelay")->load();
        float reverbMix = parameters.getRawParameterValue("reverbMix")->load();
        reverbProcessor.setSize(reverbSize);
        reverbProcessor.setDamping(reverbDamp);
        reverbProcessor.setPredelay(reverbPredelay);
        reverbProcessor.setMix(reverbMix);
        if (reverbMix > 0.001f)
            reverbProcessor.process(block);
    }

    // Apply master volume
    buffer.applyGain(masterVolume);
}

juce::AudioProcessorEditor* OIntonationPadAudioProcessor::createEditor()
{
    return new OIntonationPadAudioProcessorEditor(*this);
}

void OIntonationPadAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "tuning_masterTune")
        tuningEngine.setMasterTune(static_cast<double>(newValue));
    else if (parameterID == "tuning_octaveStretch")
        tuningEngine.setOctaveStretch(newValue);
    else if (parameterID == "tuning_pitchBendRange")
        tuningEngine.setPitchBendRange(newValue);
    else if (parameterID == "tuning_temperamentPreset")
    {
        tuningEngine.setBuiltInPreset(static_cast<TuningEngine::BuiltInPreset>(static_cast<int>(newValue)));
        checkAndResetForScaleChange();
    }
}

void OIntonationPadAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Save tuning engine custom state
    auto tuningState = state.getOrCreateChildWithName("tuningEngine", nullptr);
    auto intervals = tuningEngine.getIntervals();
    juce::String intervalsStr;
    for (size_t i = 0; i < intervals.size(); ++i)
    {
        if (i > 0) intervalsStr += ",";
        intervalsStr += juce::String(intervals[i], 6);
    }
    tuningState.setProperty("intervals", intervalsStr, nullptr);
    tuningState.setProperty("scaleName", tuningEngine.getActiveTuningName(), nullptr);
    tuningState.setProperty("tonic", tuningEngine.getTonicNote(), nullptr);
    tuningState.setProperty("preset", static_cast<int>(tuningEngine.getBuiltInPreset()), nullptr);

    // v1.5.0: Save enabled intervals (lock-free snapshot read)
    {
        auto snap = std::atomic_load(&intervalSnapshot_);
        if (snap)
        {
            juce::String eiStr;
            for (size_t i = 0; i < snap->enabledFlags.size(); ++i)
            {
                if (i > 0) eiStr += ",";
                eiStr += snap->enabledFlags[i] ? "1" : "0";
            }
            tuningState.setProperty("enabledIntervals", eiStr, nullptr);
        }
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OIntonationPadAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        parameters.replaceState(state);

        // Restore tuning engine custom state
        auto tuningState = state.getChildWithName("tuningEngine");
        if (tuningState.isValid())
        {
            juce::String intervalsStr = tuningState.getProperty("intervals", "");
            if (intervalsStr.isNotEmpty())
            {
                std::vector<double> intervals;
                juce::StringArray tokens;
                tokens.addTokens(intervalsStr, ",", "");
                for (const auto& token : tokens)
                    intervals.push_back(token.getDoubleValue());

                juce::String scaleName = tuningState.getProperty("scaleName", "Custom");
                tuningEngine.setCustomIntervals(intervals, scaleName);
            }

            int tonic = tuningState.getProperty("tonic", 0);
            tuningEngine.setTonicNote(tonic);

            // v1.5.0: Restore enabled intervals (lock-free snapshot publish)
            juce::String eiStr = tuningState.getProperty("enabledIntervals", "");
            if (eiStr.isNotEmpty())
            {
                juce::StringArray eiTokens;
                eiTokens.addTokens(eiStr, ",", "");
                auto snap = std::make_shared<IntervalSnapshot>();
                snap->scaleDegreeCount = tuningEngine.getScaleDegrees();
                for (const auto& token : eiTokens)
                    snap->enabledFlags.push_back(token == "1");
                for (int i = 0; i < static_cast<int>(snap->enabledFlags.size()); ++i)
                    if (snap->enabledFlags[static_cast<size_t>(i)])
                        snap->enabledDegrees.push_back(i);
                if (snap->enabledDegrees.empty())
                    snap->enabledDegrees.push_back(0);
                lastKnownScaleSize_ = snap->scaleDegreeCount;
                std::atomic_store(&intervalSnapshot_, std::shared_ptr<const IntervalSnapshot>(std::move(snap)));
            }
        }
    }
}

std::vector<ActiveNoteInfo> OIntonationPadAudioProcessor::getActiveNotes() const
{
    std::vector<ActiveNoteInfo> notes;

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        auto* voice = static_cast<const WavetableVoice*>(synthesiser.getVoice(i));
        if (voice != nullptr && voice->isVoiceActive())
        {
            int subCount = voice->getActiveSubVoiceCount();
            for (int j = 0; j < subCount; ++j)
            {
                // Base contribution
                float baseGain = voice->getSubVoiceBaseGain(j);
                if (baseGain > 0.01f)
                {
                    const auto& info = voice->getSubVoiceInfo(j);
                    notes.push_back({ info.midiNote, info.frequencyHz, baseGain });
                }

                // Spacing (up) contribution
                float spacingGain = voice->getSubVoiceSpacingGain(j);
                if (spacingGain > 0.01f)
                {
                    const auto& info = voice->getSubVoiceSpacingInfo(j);
                    notes.push_back({ info.midiNote, info.frequencyHz, spacingGain });
                }

                // Inversion (down) contribution
                float inversionGain = voice->getSubVoiceInversionGain(j);
                if (inversionGain > 0.01f)
                {
                    const auto& info = voice->getSubVoiceInversionInfo(j);
                    notes.push_back({ info.midiNote, info.frequencyHz, inversionGain });
                }
            }
        }
    }

    return notes;
}

// ═══════════════════════════════════════════════════════════════════
// v1.5.0: ENABLED INTERVAL MANAGEMENT
// ═══════════════════════════════════════════════════════════════════

std::vector<bool> OIntonationPadAudioProcessor::getEnabledIntervals() const
{
    auto snap = std::atomic_load(&intervalSnapshot_);
    return snap ? snap->enabledFlags : std::vector<bool>{};
}

void OIntonationPadAudioProcessor::setIntervalEnabled(int index, bool enabled)
{
    auto current = std::atomic_load(&intervalSnapshot_);
    if (!current || index < 0 || index >= static_cast<int>(current->enabledFlags.size()))
        return;

    auto snap = std::make_shared<IntervalSnapshot>();
    snap->enabledFlags = current->enabledFlags;
    snap->scaleDegreeCount = current->scaleDegreeCount;
    snap->enabledFlags[static_cast<size_t>(index)] = enabled;

    for (int i = 0; i < static_cast<int>(snap->enabledFlags.size()); ++i)
        if (snap->enabledFlags[static_cast<size_t>(i)])
            snap->enabledDegrees.push_back(i);
    if (snap->enabledDegrees.empty())
        snap->enabledDegrees.push_back(0);

    std::atomic_store(&intervalSnapshot_, std::shared_ptr<const IntervalSnapshot>(std::move(snap)));
}

void OIntonationPadAudioProcessor::resetEnabledIntervals()
{
    int scaleSize = tuningEngine.getScaleDegrees();
    auto snap = std::make_shared<IntervalSnapshot>();
    snap->scaleDegreeCount = scaleSize;
    snap->enabledFlags.resize(static_cast<size_t>(scaleSize), true);
    for (int i = 0; i < scaleSize; ++i)
        snap->enabledDegrees.push_back(i);
    std::atomic_store(&intervalSnapshot_, std::shared_ptr<const IntervalSnapshot>(std::move(snap)));
    lastKnownScaleSize_ = scaleSize;
}

void OIntonationPadAudioProcessor::checkAndResetForScaleChange()
{
    int currentScaleSize = tuningEngine.getScaleDegrees();
    if (currentScaleSize != lastKnownScaleSize_)
        resetEnabledIntervals();
}

int OIntonationPadAudioProcessor::getScaleDegreeCount() const
{
    return tuningEngine.getScaleDegrees();
}

std::vector<int> OIntonationPadAudioProcessor::getEnabledDegreeOffsets() const
{
    auto snap = std::atomic_load(&intervalSnapshot_);
    return snap ? snap->enabledDegrees : std::vector<int>{ 0 };
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OIntonationPadAudioProcessor();
}
