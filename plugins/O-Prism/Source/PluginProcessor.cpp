/*
  ==============================================================================

    PluginProcessor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/ModulationMatrix.h"

// ═══════════════════════════════════════════════════════════════════
// Parameter Helper Functions
// ═══════════════════════════════════════════════════════════════════

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createOscParameters (const juce::String& prefix)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto label = "Osc " + juce::String::charToString (prefix.getLastCharacter());
    float levelDefault = (prefix == "oscA") ? 0.8f : 0.0f;

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "Table", 1 }, label + " Wavetable", 0, WavetableFactory::kNumFactoryTables - 1, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Pos", 1 }, label + " Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Level", 1 }, label + " Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), levelDefault));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Pan", 1 }, label + " Pan",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "Coarse", 1 }, label + " Coarse", -24, 24, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Fine", 1 }, label + " Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Phase", 1 }, label + " Phase",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "Unison", 1 }, label + " Unison", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Detune", 1 }, label + " Detune",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Width", 1 }, label + " Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createSubNoiseParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subShape", 1 }, "Sub Shape",
        juce::StringArray { "Sine", "Triangle", "Saw", "Square" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subOctave", 1 }, "Sub Octave",
        juce::StringArray { "-1 Oct", "-2 Oct", "-3 Oct", "-4 Oct" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "subLevel", 1 }, "Sub Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noiseType", 1 }, "Noise Type",
        juce::StringArray { "White", "Pink", "Brown", "Digital", "Vinyl", "Wind" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noiseLevel", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subRouting", 1 }, "Sub Routing",
        juce::StringArray { "Post-Filter", "Pre-Filter" }, 0));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createAmpEnvelopeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampAttack", 1 }, "Amp Attack",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampDecay", 1 }, "Amp Decay",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampSustain", 1 }, "Amp Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ampRelease", 1 }, "Amp Release",
        juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.3f), 0.5f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterEnvelopeParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtAttack", 1 }, "Filter Attack",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtDecay", 1 }, "Filter Decay",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.35f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtSustain", 1 }, "Filter Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtRelease", 1 }, "Filter Release",
        juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.3f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtEnvDepth", 1 }, "Filter Env Depth",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterParameters (const juce::String& prefix)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto label = "Filter " + juce::String::charToString (prefix.getLastCharacter());

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "Type", 1 }, label + " Type",
        juce::StringArray { "LP12", "LP24", "HP12", "HP24", "BP12", "BP24", "Notch" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Cutoff", 1 }, label + " Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Res", 1 }, label + " Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "Drive", 1 }, label + " Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "KeyTrack", 1 }, label + " KeyTrack",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterRoutingParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filtRouting", 1 }, "Filter Routing",
        juce::StringArray { "Serial", "Parallel" }, 0));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createTuningParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tuningPreset", 1 }, "Tuning Preset",
        juce::StringArray { "12-TET", "Pythagorean", "Zarlino", "Meantone 1/4",
                            "Werckmeister III", "Kirnberger III", "Vallotti",
                            "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tonic", 1 }, "Tonic",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterTune", 1 }, "Master Tune",
        juce::NormalisableRange<float> (420.0f, 460.0f, 0.1f), 440.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "octaveStretch", 1 }, "Octave Stretch",
        juce::NormalisableRange<float> (0.95f, 1.25f, 0.001f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "pitchBendRange", 1 }, "Pitch Bend Range", 1, 48, 2));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "glideMode", 1 }, "Glide Mode",
        juce::StringArray { "Off", "Legato", "Always" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glideTime", 1 }, "Glide Time",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.35f), 0.1f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createReverbParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "reverbBypass", 1 }, "Reverb Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbSize", 1 }, "Reverb Size",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbDamp", 1 }, "Reverb Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbPredelay", 1 }, "Reverb Pre-delay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f, 0.5f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverbMix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createDelayParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "delayBypass", 1 }, "Delay Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTime", 1 }, "Delay Time",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.35f), 0.375f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayFeedback", 1 }, "Delay Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "delaySync", 1 }, "Delay Sync", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delayMode", 1 }, "Delay Mode",
        juce::StringArray { "Normal", "PingPong" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayMix", 1 }, "Delay Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createChorusParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "chorusBypass", 1 }, "Chorus Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusRate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.4f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusDepth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorusMix", 1 }, "Chorus Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createDistortionParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "distBypass", 1 }, "Distortion Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "distType", 1 }, "Distortion Type",
        juce::StringArray { "SoftClip", "HardClip", "Tube", "Fold" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "distDrive", 1 }, "Distortion Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "distMix", 1 }, "Distortion Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createEQParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "eqBypass", 1 }, "EQ Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqLowGain", 1 }, "EQ Low Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqMidGain", 1 }, "EQ Mid Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqMidFreq", 1 }, "EQ Mid Freq",
        juce::NormalisableRange<float> (200.0f, 8000.0f, 0.1f, 0.35f), 1000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "eqHighGain", 1 }, "EQ High Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    return params;
}

static const juce::StringArray& getLfoDivisionNames()
{
    static const juce::StringArray names {
        "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
        "1/1D", "1/2D", "1/4D", "1/8D", "1/16D", "1/32D",
        "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"
    };
    return names;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createLFOParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto& divNames = getLfoDivisionNames();

    for (int i = 1; i <= 4; ++i)
    {
        auto n = juce::String (i);

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo" + n + "Rate", 1 }, "LFO " + n + " Rate",
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.35f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Shape", 1 }, "LFO " + n + " Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "lfo" + n + "Sync", 1 }, "LFO " + n + " Sync", false));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Division", 1 }, "LFO " + n + " Division",
            divNames, 2)); // default 1/4
    }

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createModMatrixParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto sourceNames = getModSourceNames();
    auto destNames = getModDestNames();

    for (int i = 0; i < 16; ++i)
    {
        auto prefix = "modSlot" + juce::String (i);
        auto label = "Mod " + juce::String (i + 1);

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "Src", 1 }, label + " Source",
            sourceNames, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "Dst", 1 }, label + " Dest",
            destNames, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "Amt", 1 }, label + " Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { prefix + "On", 1 }, label + " Enabled", false));
    }

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createGlobalParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterVol", 1 }, "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stereoWidth", 1 }, "Stereo Width",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscMix", 1 }, "Osc Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "velocityCurve", 1 }, "Velocity Curve",
        juce::StringArray { "Linear", "Soft", "Hard", "Fixed" }, 0));

    return params;
}

// ═══════════════════════════════════════════════════════════════════
// Parameter Layout
// ═══════════════════════════════════════════════════════════════════

juce::AudioProcessorValueTreeState::ParameterLayout OPrismAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> allParams;

    auto addSection = [&allParams] (auto sectionParams) {
        for (auto& p : sectionParams)
            allParams.push_back (std::move (p));
    };

    addSection (createOscParameters ("oscA"));     // 10
    addSection (createOscParameters ("oscB"));     // 10
    addSection (createSubNoiseParameters());       //  5
    addSection (createAmpEnvelopeParameters());    //  4
    addSection (createFilterEnvelopeParameters()); //  5
    addSection (createFilterParameters ("filtA")); //  5
    addSection (createFilterParameters ("filtB")); //  5
    addSection (createFilterRoutingParameters()); //  1
    addSection (createTuningParameters());       //  7
    addSection (createReverbParameters());       //  4
    addSection (createDelayParameters());        //  5
    addSection (createChorusParameters());       //  3
    addSection (createDistortionParameters());   //  3
    addSection (createEQParameters());           //  4
    addSection (createLFOParameters());          //  8 (rate + shape only, routing via matrix)
    addSection (createModMatrixParameters());    // 64 (16 slots x 4 params)
    addSection (createGlobalParameters());       //  3

    return { allParams.begin(), allParams.end() };
}

// ═══════════════════════════════════════════════════════════════════
// Constructor / Destructor
// ═══════════════════════════════════════════════════════════════════

OPrismAudioProcessor::OPrismAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("OPrismParameters"), createParameterLayout())
{
    // Generate factory wavetable library (28 tables)
    auto factoryLib = WavetableFactory::createFactoryLibrary();
    tableInfoList = WavetableFactory::getTableInfoList();
    for (auto& entry : factoryLib)
        factoryTables.push_back (std::move (entry.table));

    // Load user wavetables from persistent directory
    userWavetableManager.loadFromDisk();

    // Create 16 voices
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new PrismVoice();
        voice->setAPVTS (&parameters);
        voice->setTuningEngine (&tuningEngine);
        voice->setProcessor (this);
        voice->setWavetableA (factoryTables[0].get()); // Default: Saw
        voice->setWavetableB (factoryTables[0].get());
        synthesiser.addVoice (voice);
    }

    synthesiser.addSound (new PrismSound());
    lastAssignedTableA = factoryTables[0].get();
    lastAssignedTableB = factoryTables[0].get();
}

OPrismAudioProcessor::~OPrismAudioProcessor() = default;

// ═══════════════════════════════════════════════════════════════════
// Audio Processing
// ═══════════════════════════════════════════════════════════════════

void OPrismAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    // Prepare all voices
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PrismVoice*> (synthesiser.getVoice (i)))
            voice->prepare (sampleRate, samplesPerBlock);
    }

    // Effects chain
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
    distortion.prepare (spec);
    chorus.prepare (spec);
    delay.prepare (spec);
    delay.setPlayHead (getPlayHead());
    eq.prepare (spec);
    reverbProcessor.prepare (spec);

    masterVolSmoothed.reset (sampleRate, 0.02);
    stereoWidthSmoothed.reset (sampleRate, 0.02);

    setLatencySamples (static_cast<int> (distortion.getLatencyInSamples()));
}

void OPrismAudioProcessor::releaseResources() {}

void OPrismAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Read BPM from host transport for tempo-synced LFOs
    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            if (auto bpm = posInfo->getBpm())
                currentBPM.store (*bpm, std::memory_order_relaxed);
        }
    }

    // Update TuningEngine from APVTS
    tuningEngine.setMasterTune (static_cast<double> (parameters.getRawParameterValue ("masterTune")->load()));
    tuningEngine.setOctaveStretch (parameters.getRawParameterValue ("octaveStretch")->load());
    tuningEngine.setPitchBendRange (static_cast<float> (
        parameters.getRawParameterValue ("pitchBendRange")->load()));

    // Forward tuning preset and tonic (only on change to avoid rebuilding frequency table every block)
    int tuningPreset = static_cast<int> (parameters.getRawParameterValue ("tuningPreset")->load());
    int tonic = static_cast<int> (parameters.getRawParameterValue ("tonic")->load());

    if (tuningPreset != lastTuningPreset)
    {
        lastTuningPreset = tuningPreset;
        tuningEngine.setBuiltInPreset (static_cast<TuningEngine::BuiltInPreset> (tuningPreset));
    }

    if (tonic != lastTonic)
    {
        lastTonic = tonic;
        tuningEngine.setTonicNote (tonic);
    }

    // Update wavetable assignments if table selection changed
    updateWavetableAssignments();

    // Track active MIDI notes and extract CC data for mod matrix
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            noteStates[static_cast<size_t> (msg.getNoteNumber())].store (true, std::memory_order_relaxed);
        else if (msg.isNoteOff())
            noteStates[static_cast<size_t> (msg.getNoteNumber())].store (false, std::memory_order_relaxed);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            for (auto& s : noteStates) s.store (false, std::memory_order_relaxed);
        else if (msg.isController() && msg.getControllerNumber() == 1) // Mod wheel
            modWheelValue.store (msg.getControllerValue() / 127.0f, std::memory_order_relaxed);
        else if (msg.isChannelPressure()) // Channel aftertouch
            aftertouchValue.store (msg.getChannelPressureValue() / 127.0f, std::memory_order_relaxed);
    }

    // Render synth voices
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Effects chain (float precision)
    juce::dsp::AudioBlock<float> block (buffer);

    // 1. Distortion
    bool distBypassed = parameters.getRawParameterValue ("distBypass")->load() > 0.5f;
    if (! distBypassed)
    {
        int distType = static_cast<int> (parameters.getRawParameterValue ("distType")->load());
        float distDrive = parameters.getRawParameterValue ("distDrive")->load();
        float distMix = parameters.getRawParameterValue ("distMix")->load();
        distortion.setType (distType);
        distortion.setDrive (distDrive);
        distortion.setMix (distMix);
        if (distMix > 0.001f)
            distortion.process (block);
    }

    // 2. Chorus
    bool chorusBypassed = parameters.getRawParameterValue ("chorusBypass")->load() > 0.5f;
    if (! chorusBypassed)
    {
        float chorusRate = parameters.getRawParameterValue ("chorusRate")->load();
        float chorusDepth = parameters.getRawParameterValue ("chorusDepth")->load();
        float chorusMix = parameters.getRawParameterValue ("chorusMix")->load();
        chorus.setRate (chorusRate);
        chorus.setDepth (chorusDepth);
        chorus.setMix (chorusMix);
        if (chorusMix > 0.001f)
            chorus.process (block);
    }

    // 3. Delay
    bool delayBypassed = parameters.getRawParameterValue ("delayBypass")->load() > 0.5f;
    if (! delayBypassed)
    {
        float delayTime = parameters.getRawParameterValue ("delayTime")->load();
        float delayFeedback = parameters.getRawParameterValue ("delayFeedback")->load();
        int delayMode = static_cast<int> (parameters.getRawParameterValue ("delayMode")->load());
        float delayMix = parameters.getRawParameterValue ("delayMix")->load();
        delay.setTime (delayTime);
        delay.setFeedback (delayFeedback);
        delay.setMode (delayMode);
        delay.setMix (delayMix);
        if (delayMix > 0.001f)
            delay.process (block);
    }

    // 4. EQ
    bool eqBypassed = parameters.getRawParameterValue ("eqBypass")->load() > 0.5f;
    if (! eqBypassed)
    {
        float eqLowGain = parameters.getRawParameterValue ("eqLowGain")->load();
        float eqMidGain = parameters.getRawParameterValue ("eqMidGain")->load();
        float eqMidFreq = parameters.getRawParameterValue ("eqMidFreq")->load();
        float eqHighGain = parameters.getRawParameterValue ("eqHighGain")->load();
        eq.setLowGain (eqLowGain);
        eq.setMidGain (eqMidGain);
        eq.setMidFreq (eqMidFreq);
        eq.setHighGain (eqHighGain);
        if (std::abs (eqLowGain) > 0.1f || std::abs (eqMidGain) > 0.1f || std::abs (eqHighGain) > 0.1f)
            eq.process (block);
    }

    // 5. Reverb
    bool reverbBypassed = parameters.getRawParameterValue ("reverbBypass")->load() > 0.5f;
    if (! reverbBypassed)
    {
        float reverbSize = parameters.getRawParameterValue ("reverbSize")->load();
        float reverbDamp = parameters.getRawParameterValue ("reverbDamp")->load();
        float reverbPredelay = parameters.getRawParameterValue ("reverbPredelay")->load();
        float reverbMix = parameters.getRawParameterValue ("reverbMix")->load();
        reverbProcessor.setSize (reverbSize);
        reverbProcessor.setDamping (reverbDamp);
        reverbProcessor.setPredelay (reverbPredelay);
        reverbProcessor.setMix (reverbMix);
        if (reverbMix > 0.001f)
            reverbProcessor.process (block);
    }

    // Stereo width (mid-side processing) + master volume (smoothed per-sample)
    float stereoWidth = parameters.getRawParameterValue ("stereoWidth")->load();
    float masterVol = parameters.getRawParameterValue ("masterVol")->load();
    stereoWidthSmoothed.setTargetValue (stereoWidth);
    masterVolSmoothed.setTargetValue (masterVol);

    if (buffer.getNumChannels() >= 2)
    {
        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float width = stereoWidthSmoothed.getNextValue();
            float gain = masterVolSmoothed.getNextValue();

            float mid = (leftData[sample] + rightData[sample]) * 0.5f;
            float side = (leftData[sample] - rightData[sample]) * 0.5f;

            leftData[sample] = (mid + side * width) * gain;
            rightData[sample] = (mid - side * width) * gain;
        }
    }
    else
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float gain = masterVolSmoothed.getNextValue();
            buffer.setSample (0, sample, buffer.getSample (0, sample) * gain);
        }
    }
}

void OPrismAudioProcessor::updateWavetableAssignments()
{
    int numFactoryTables = static_cast<int> (factoryTables.size());

    // Determine target table for each oscillator (user override takes priority)
    auto* currentUserA = userTablePtrA.load (std::memory_order_relaxed);
    auto* currentUserB = userTablePtrB.load (std::memory_order_relaxed);

    const WavetableData* targetA;
    const WavetableData* targetB;

    if (currentUserA != nullptr)
    {
        targetA = currentUserA;
    }
    else
    {
        int oscATable = juce::jlimit (0, numFactoryTables - 1,
            static_cast<int> (parameters.getRawParameterValue ("oscATable")->load()));
        targetA = factoryTables[static_cast<size_t> (oscATable)].get();
    }

    if (currentUserB != nullptr)
    {
        targetB = currentUserB;
    }
    else
    {
        int oscBTable = juce::jlimit (0, numFactoryTables - 1,
            static_cast<int> (parameters.getRawParameterValue ("oscBTable")->load()));
        targetB = factoryTables[static_cast<size_t> (oscBTable)].get();
    }

    if (targetA != lastAssignedTableA || targetB != lastAssignedTableB)
    {
        for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<PrismVoice*> (synthesiser.getVoice (i)))
            {
                if (targetA != lastAssignedTableA)
                    voice->setWavetableA (targetA);
                if (targetB != lastAssignedTableB)
                    voice->setWavetableB (targetB);
            }
        }
        lastAssignedTableA = targetA;
        lastAssignedTableB = targetB;
    }
}

// ═══════════════════════════════════════════════════════════════════
// User Wavetable API
// ═══════════════════════════════════════════════════════════════════

void OPrismAudioProcessor::selectUserWavetable (int oscIndex, const juce::String& name)
{
    auto* table = userWavetableManager.getTable (name);
    if (table == nullptr)
        return;

    if (oscIndex == 0)
    {
        userTableNameA = name;
        userTablePtrA.store (table, std::memory_order_relaxed);
    }
    else
    {
        userTableNameB = name;
        userTablePtrB.store (table, std::memory_order_relaxed);
    }
}

void OPrismAudioProcessor::clearUserWavetableOverride (int oscIndex)
{
    if (oscIndex == 0)
    {
        userTableNameA = {};
        userTablePtrA.store (nullptr, std::memory_order_relaxed);
    }
    else
    {
        userTableNameB = {};
        userTablePtrB.store (nullptr, std::memory_order_relaxed);
    }
}

const WavetableData* OPrismAudioProcessor::getActiveOscTable (int oscIndex) const
{
    if (oscIndex == 0)
    {
        auto* userTable = userTablePtrA.load (std::memory_order_relaxed);
        if (userTable != nullptr)
            return userTable;
        int idx = juce::jlimit (0, static_cast<int> (factoryTables.size()) - 1,
            static_cast<int> (parameters.getRawParameterValue ("oscATable")->load()));
        return factoryTables[static_cast<size_t> (idx)].get();
    }
    else
    {
        auto* userTable = userTablePtrB.load (std::memory_order_relaxed);
        if (userTable != nullptr)
            return userTable;
        int idx = juce::jlimit (0, static_cast<int> (factoryTables.size()) - 1,
            static_cast<int> (parameters.getRawParameterValue ("oscBTable")->load()));
        return factoryTables[static_cast<size_t> (idx)].get();
    }
}

juce::String OPrismAudioProcessor::getActiveUserTableName (int oscIndex) const
{
    return oscIndex == 0 ? userTableNameA : userTableNameB;
}

bool OPrismAudioProcessor::isUserTableActive (int oscIndex) const
{
    return oscIndex == 0
        ? userTablePtrA.load (std::memory_order_relaxed) != nullptr
        : userTablePtrB.load (std::memory_order_relaxed) != nullptr;
}

// ═══════════════════════════════════════════════════════════════════
// State Persistence
// ═══════════════════════════════════════════════════════════════════

void OPrismAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Add custom tuning state
    auto tuningState = state.getOrCreateChildWithName ("tuningEngine", nullptr);

    auto intervals = tuningEngine.getIntervals();
    juce::String intervalsStr;
    for (size_t i = 0; i < intervals.size(); ++i)
    {
        if (i > 0) intervalsStr += ",";
        intervalsStr += juce::String (intervals[i], 6);
    }

    tuningState.setProperty ("intervals", intervalsStr, nullptr);
    tuningState.setProperty ("scaleName", tuningEngine.getActiveTuningName(), nullptr);
    tuningState.setProperty ("tonic", tuningEngine.getTonicNote(), nullptr);

    // Save user wavetable selections
    auto userWtState = state.getOrCreateChildWithName ("userWavetables", nullptr);
    userWtState.setProperty ("oscAUserTable", userTableNameA, nullptr);
    userWtState.setProperty ("oscBUserTable", userTableNameB, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OPrismAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        parameters.replaceState (state);

        // Sync cached values so processBlock doesn't overwrite restored TuningEngine state
        lastTuningPreset = static_cast<int> (parameters.getRawParameterValue ("tuningPreset")->load());
        lastTonic = static_cast<int> (parameters.getRawParameterValue ("tonic")->load());

        // Restore tuning state
        auto tuningState = state.getChildWithName ("tuningEngine");
        if (tuningState.isValid())
        {
            juce::String intervalsStr = tuningState.getProperty ("intervals", "");
            if (intervalsStr.isNotEmpty())
            {
                std::vector<double> intervals;
                juce::StringArray tokens;
                tokens.addTokens (intervalsStr, ",", "");
                for (const auto& token : tokens)
                    intervals.push_back (token.getDoubleValue());

                juce::String scaleName = tuningState.getProperty ("scaleName", "Custom");
                tuningEngine.setCustomIntervals (intervals, scaleName);
            }

            int tonic = tuningState.getProperty ("tonic", 0);
            tuningEngine.setTonicNote (tonic);
        }

        // Restore user wavetable selections
        auto userWtState = state.getChildWithName ("userWavetables");
        if (userWtState.isValid())
        {
            juce::String nameA = userWtState.getProperty ("oscAUserTable", "").toString();
            juce::String nameB = userWtState.getProperty ("oscBUserTable", "").toString();

            if (nameA.isNotEmpty())
                selectUserWavetable (0, nameA);
            if (nameB.isNotEmpty())
                selectUserWavetable (1, nameB);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Editor
// ═══════════════════════════════════════════════════════════════════

juce::AudioProcessorEditor* OPrismAudioProcessor::createEditor()
{
    return new OPrismAudioProcessorEditor (*this);
}

// ═══════════════════════════════════════════════════════════════════
// Plugin Instantiation
// ═══════════════════════════════════════════════════════════════════

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OPrismAudioProcessor();
}
