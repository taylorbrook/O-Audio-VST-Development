/*
  ==============================================================================

    O-IntonationPad - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/WavetableVoice.h"
#include "DSP/WavetableSound.h"
#include "DSP/ChordGenerator.h"
#include "DSP/TuningEngine.h"
#include "DSP/ScaleGenerator.h"
#include "DSP/TuningExporter.h"
#include "DSP/EmbeddedTunings.h"

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

    // TUNING: Tuning Mode (12-TET, Custom, MTS-ESP)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuning_tuningMode", 1 },
        "Tuning Mode",
        juce::StringArray { "12-TET", "Custom", "MTS-ESP" },
        0));

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

    // INVERSION_RANDOM - Float (0-100%, default: 30%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inversionRandom", 1 },
        "Inversion Random",
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

    // WAVETABLE_POS - Float (0-100%, default: 50%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wavetablePos", 1 },
        "Wavetable Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // LFO_RATE - Float (0.01-20 Hz, default: 0.5 Hz, exponential)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lfoRate", 1 },
        "LFO Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f),  // skew = 0.3 for exponential
        0.5f,
        "Hz"
    ));

    // LFO_DEPTH - Float (0-100%, default: 25%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lfoDepth", 1 },
        "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.25f
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
    parameters.addParameterListener("tuning_tuningMode", this);
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

    // Prepare filter
    filterSpec.sampleRate = sampleRate;
    filterSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    filterSpec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    filter.prepare(filterSpec);
    filter.reset();
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setResonance(0.707f);  // Butterworth response

    // Initialize LFO
    float lfoRate = parameters.getRawParameterValue("lfoRate")->load();
    lfoPhaseIncrement = (lfoRate * juce::MathConstants<double>::twoPi) / sampleRate;
    lfoPhase = 0.0;

    // Update envelope parameters for all voices
    float attackTime = parameters.getRawParameterValue("attackTime")->load();
    float releaseTime = parameters.getRawParameterValue("releaseTime")->load();

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<WavetableVoice*>(synthesiser.getVoice(i)))
        {
            voice->setEnvelopeParameters(attackTime, releaseTime);
        }
    }
}

void OIntonationPadAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2 (DSP)
}

void OIntonationPadAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // Read parameters (atomic, real-time safe)
    float wavetablePos = parameters.getRawParameterValue("wavetablePos")->load();
    float attackTime = parameters.getRawParameterValue("attackTime")->load();
    float releaseTime = parameters.getRawParameterValue("releaseTime")->load();
    int voiceCount = static_cast<int>(parameters.getRawParameterValue("voiceCount")->load());
    float complexity = parameters.getRawParameterValue("complexity")->load();
    int keyRoot = static_cast<int>(parameters.getRawParameterValue("keyRoot")->load());
    float lfoRate = parameters.getRawParameterValue("lfoRate")->load();
    float lfoDepth = parameters.getRawParameterValue("lfoDepth")->load();
    float filterCutoff = parameters.getRawParameterValue("filterCutoff")->load();
    float masterVolume = parameters.getRawParameterValue("masterVolume")->load();
    float inversionRandom = parameters.getRawParameterValue("inversionRandom")->load();
    float detuneRandom = parameters.getRawParameterValue("detuneRandom")->load();
    float timingRandom = parameters.getRawParameterValue("timingRandom")->load();

    // Update LFO phase increment
    lfoPhaseIncrement = (lfoRate * juce::MathConstants<double>::twoPi) / getSampleRate();

    // Calculate LFO value (global, free-running)
    float lfoValue = static_cast<float>(std::sin(lfoPhase)) * lfoDepth;
    lfoPhase += lfoPhaseIncrement * buffer.getNumSamples();
    if (lfoPhase >= juce::MathConstants<double>::twoPi)
        lfoPhase -= juce::MathConstants<double>::twoPi;

    // Modulate wavetable position with LFO
    float modulatedWavetablePos = juce::jlimit(0.0f, 1.0f, wavetablePos + lfoValue);

    // v1.5.0: Check if scale size changed, auto-reset enabled intervals
    int currentScaleSize = tuningEngine.getScaleDegrees();
    if (currentScaleSize != lastKnownScaleSize)
    {
        resetEnabledIntervals();
        lastKnownScaleSize = currentScaleSize;
    }

    // Rebuild audio-thread cache if dirty (lock-free read path for normal operation)
    if (enabledIntervalsDirty.exchange(false))
    {
        cachedEnabledDegrees = getEnabledDegreeOffsets();
        cachedScaleDegreeCount = getScaleDegreeCount();
    }

    // Update all voice parameters before rendering
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<WavetableVoice*>(synthesiser.getVoice(i)))
        {
            voice->setWavetablePosition(modulatedWavetablePos);
            voice->setEnvelopeParameters(attackTime, releaseTime);

            // Store chord generation parameters for voices to use on note-on
            voice->setChordGenerationParams(voiceCount, complexity, keyRoot,
                                            cachedEnabledDegrees, cachedScaleDegreeCount,
                                            inversionRandom, detuneRandom, timingRandom,
                                            &chordGenerator, &tuningEngine, &randomGenerator);
        }
    }

    // Render synthesiser output (handles MIDI internally)
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply filter
    filter.setCutoffFrequency(filterCutoff);
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);

    // Apply master volume
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.applyGain(channel, 0, buffer.getNumSamples(), masterVolume);
    }
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
    else if (parameterID == "tuning_tuningMode")
    {
        // Only apply mode change for 12-TET or Custom presets
        // Non-12-TET built-in presets manage their own mode via setBuiltInPreset()
        auto preset = tuningEngine.getBuiltInPreset();
        if (preset == TuningEngine::BuiltInPreset::Equal12TET ||
            preset == TuningEngine::BuiltInPreset::Custom)
        {
            tuningEngine.setMode(static_cast<TuningEngine::Mode>(static_cast<int>(newValue)));
        }
    }
    else if (parameterID == "tuning_temperamentPreset")
        tuningEngine.setBuiltInPreset(static_cast<TuningEngine::BuiltInPreset>(static_cast<int>(newValue)));
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

    // v1.5.0: Save enabled intervals
    {
        auto ei = getEnabledIntervals();
        juce::String eiStr;
        for (size_t i = 0; i < ei.size(); ++i)
        {
            if (i > 0) eiStr += ",";
            eiStr += ei[i] ? "1" : "0";
        }
        tuningState.setProperty("enabledIntervals", eiStr, nullptr);
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

            // v1.5.0: Restore enabled intervals
            juce::String eiStr = tuningState.getProperty("enabledIntervals", "");
            if (eiStr.isNotEmpty())
            {
                juce::StringArray eiTokens;
                eiTokens.addTokens(eiStr, ",", "");
                std::lock_guard<std::mutex> lock(enabledIntervalsMutex);
                enabledIntervals.clear();
                for (const auto& token : eiTokens)
                    enabledIntervals.push_back(token == "1");
            }
        }
    }
}

std::vector<ActiveNoteInfo> OIntonationPadAudioProcessor::getActiveNotes() const
{
    std::vector<ActiveNoteInfo> notes;

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        auto* voice = dynamic_cast<const WavetableVoice*>(synthesiser.getVoice(i));
        if (voice != nullptr && voice->isVoiceActive())
        {
            int subCount = voice->getActiveSubVoiceCount();
            for (int j = 0; j < subCount; ++j)
            {
                // Base (uninverted) contribution
                float baseGain = voice->getSubVoiceBaseGain(j);
                if (baseGain > 0.01f)
                {
                    const auto& info = voice->getSubVoiceInfo(j);
                    notes.push_back({ info.midiNote, info.frequencyHz, baseGain });
                }

                // Inverted contribution
                float invertedGain = voice->getSubVoiceInvertedGain(j);
                if (invertedGain > 0.01f)
                {
                    const auto& info = voice->getSubVoiceInvertedInfo(j);
                    notes.push_back({ info.midiNote, info.frequencyHz, invertedGain });
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
    std::lock_guard<std::mutex> lock(enabledIntervalsMutex);
    return enabledIntervals;
}

void OIntonationPadAudioProcessor::setIntervalEnabled(int index, bool enabled)
{
    std::lock_guard<std::mutex> lock(enabledIntervalsMutex);
    if (index >= 0 && index < static_cast<int>(enabledIntervals.size()))
    {
        enabledIntervals[static_cast<size_t>(index)] = enabled;
        enabledIntervalsDirty.store(true);
    }
}

void OIntonationPadAudioProcessor::resetEnabledIntervals()
{
    std::lock_guard<std::mutex> lock(enabledIntervalsMutex);
    int scaleSize = tuningEngine.getScaleDegrees();
    enabledIntervals.clear();
    enabledIntervals.resize(static_cast<size_t>(scaleSize), true);
    enabledIntervalsDirty.store(true);
}

int OIntonationPadAudioProcessor::getScaleDegreeCount() const
{
    return tuningEngine.getScaleDegrees();
}

std::vector<int> OIntonationPadAudioProcessor::getEnabledDegreeOffsets() const
{
    std::lock_guard<std::mutex> lock(enabledIntervalsMutex);
    std::vector<int> degrees;
    for (int i = 0; i < static_cast<int>(enabledIntervals.size()); ++i)
    {
        if (enabledIntervals[static_cast<size_t>(i)])
            degrees.push_back(i);
    }
    // Always include degree 0 (root) if nothing is enabled
    if (degrees.empty())
        degrees.push_back(0);
    return degrees;
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OIntonationPadAudioProcessor();
}
