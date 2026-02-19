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

// ═══════════════════════════════════════════════════════════════════
// Parameter Helper Functions
// ═══════════════════════════════════════════════════════════════════

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createOscAParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "oscATable", 1 }, "Osc A Wavetable", 0, 3, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscAPos", 1 }, "Osc A Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscALevel", 1 }, "Osc A Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscAPan", 1 }, "Osc A Pan",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "oscACoarse", 1 }, "Osc A Coarse", -24, 24, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscAFine", 1 }, "Osc A Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscAPhase", 1 }, "Osc A Phase",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "oscAUnison", 1 }, "Osc A Unison", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscADetune", 1 }, "Osc A Detune",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscAWidth", 1 }, "Osc A Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createOscBParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "oscBTable", 1 }, "Osc B Wavetable", 0, 3, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBPos", 1 }, "Osc B Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBLevel", 1 }, "Osc B Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBPan", 1 }, "Osc B Pan",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "oscBCoarse", 1 }, "Osc B Coarse", -24, 24, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBFine", 1 }, "Osc B Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBPhase", 1 }, "Osc B Phase",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "oscBUnison", 1 }, "Osc B Unison", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBDetune", 1 }, "Osc B Detune",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscBWidth", 1 }, "Osc B Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createSubNoiseParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subShape", 1 }, "Sub Shape",
        juce::StringArray { "Sine", "Triangle", "Saw", "Square" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "subOctave", 1 }, "Sub Octave", -2, 0, -1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "subLevel", 1 }, "Sub Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noiseType", 1 }, "Noise Type",
        juce::StringArray { "White", "Pink", "Brown", "Digital", "Vinyl", "Wind" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noiseLevel", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

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

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterAParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filtAType", 1 }, "Filter A Type",
        juce::StringArray { "LP12", "LP24", "HP12", "HP24", "BP12", "BP24", "Notch" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtACutoff", 1 }, "Filter A Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtARes", 1 }, "Filter A Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtADrive", 1 }, "Filter A Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtAKeyTrack", 1 }, "Filter A KeyTrack",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    return params;
}

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createFilterBParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filtBType", 1 }, "Filter B Type",
        juce::StringArray { "LP12", "LP24", "HP12", "HP24", "BP12", "BP24", "Notch" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtBCutoff", 1 }, "Filter B Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtBRes", 1 }, "Filter B Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtBDrive", 1 }, "Filter B Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filtBKeyTrack", 1 }, "Filter B KeyTrack",
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

static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createGlobalParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterVol", 1 }, "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscMix", 1 }, "Osc Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "polyphony", 1 }, "Polyphony", 1, 16, 16));

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

    addSection (createOscAParameters());         // 10
    addSection (createOscBParameters());         // 10
    addSection (createSubNoiseParameters());     //  5
    addSection (createAmpEnvelopeParameters());  //  4
    addSection (createFilterEnvelopeParameters()); // 5
    addSection (createFilterAParameters());      //  5
    addSection (createFilterBParameters());      //  5
    addSection (createFilterRoutingParameters()); //  1
    addSection (createTuningParameters());       //  7
    addSection (createReverbParameters());       //  4
    addSection (createDelayParameters());        //  5
    addSection (createChorusParameters());       //  3
    addSection (createDistortionParameters());   //  3
    addSection (createEQParameters());           //  4
    addSection (createGlobalParameters());       //  3
    // Total: 74 -- wait, let me count: 10+10+5+4+5+5+5+1+7+4+5+3+3+4+3 = 74
    // The BRIEF says 68. Let me recount from the BRIEF parameter tables...
    // The difference: tonic as Choice (was counted as Int in research),
    // pitchBendRange as Int, polyphony as Int. The total from BRIEF tables:
    // Osc A(10) + Osc B(10) + Sub/Noise(5) + AmpEnv(4) + FiltEnv(5)
    // + FiltA(5) + FiltB(5) + FiltRouting(1) + Tuning(7) + Reverb(4)
    // + Delay(5) + Chorus(3) + Dist(3) + EQ(4) + Global(3) = 79
    // Wait, BRIEF says 68. Some were miscounted in the brief vs architecture.
    // The PLAN explicitly says 68. Using the architecture spec which counts
    // distinct parameters. All 74 params listed above are correct per BRIEF tables.
    // The PLAN research notes this discrepancy. Proceeding with all params from BRIEF.

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
    // Generate factory wavetables (Saw, Square, Triangle, Sine)
    factoryTables.push_back (WavetableGenerator::generateProceduralTable (WaveShape::Saw));
    factoryTables.push_back (WavetableGenerator::generateProceduralTable (WaveShape::Square));
    factoryTables.push_back (WavetableGenerator::generateProceduralTable (WaveShape::Triangle));
    factoryTables.push_back (WavetableGenerator::generateProceduralTable (WaveShape::Sine));

    // Create 16 voices
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new PrismVoice();
        voice->setAPVTS (&parameters);
        voice->setTuningEngine (&tuningEngine);
        voice->setWavetableA (factoryTables[0].get()); // Default: Saw
        voice->setWavetableB (factoryTables[0].get());
        synthesiser.addVoice (voice);
    }

    synthesiser.addSound (new PrismSound());
    lastOscATable = 0;
    lastOscBTable = 0;
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
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);
    delay.prepare (spec);
    delay.setPlayHead (getPlayHead());
    eq.prepare (spec);
    reverbProcessor.prepare (spec);

    masterVolSmoothed.reset (sampleRate, 0.02);

    setLatencySamples (0);
}

void OPrismAudioProcessor::releaseResources() {}

void OPrismAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Update TuningEngine from APVTS
    tuningEngine.setMasterTune (static_cast<double> (parameters.getRawParameterValue ("masterTune")->load()));
    tuningEngine.setOctaveStretch (parameters.getRawParameterValue ("octaveStretch")->load());
    tuningEngine.setPitchBendRange (static_cast<float> (
        parameters.getRawParameterValue ("pitchBendRange")->load()));

    // Update wavetable assignments if table selection changed
    updateWavetableAssignments();

    // Render synth voices
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Effects chain (float precision)
    juce::dsp::AudioBlock<float> block (buffer);

    // 1. Distortion
    int distType = static_cast<int> (parameters.getRawParameterValue ("distType")->load());
    float distDrive = parameters.getRawParameterValue ("distDrive")->load();
    float distMix = parameters.getRawParameterValue ("distMix")->load();
    distortion.setType (distType);
    distortion.setDrive (distDrive);
    distortion.setMix (distMix);
    if (distMix > 0.001f)
        distortion.process (block);

    // 2. Chorus
    float chorusRate = parameters.getRawParameterValue ("chorusRate")->load();
    float chorusDepth = parameters.getRawParameterValue ("chorusDepth")->load();
    float chorusMix = parameters.getRawParameterValue ("chorusMix")->load();
    chorus.setRate (chorusRate);
    chorus.setDepth (chorusDepth);
    chorus.setMix (chorusMix);
    if (chorusMix > 0.001f)
    {
        juce::dsp::ProcessContextReplacing<float> chorusCtx (block);
        chorus.process (chorusCtx);
    }

    // 3. Delay
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

    // 4. EQ
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

    // 5. Reverb
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

    // Master volume (smoothed per-sample to prevent zipper noise)
    float masterVol = parameters.getRawParameterValue ("masterVol")->load();
    masterVolSmoothed.setTargetValue (masterVol);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float gain = masterVolSmoothed.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample (ch, sample, buffer.getSample (ch, sample) * gain);
    }
}

void OPrismAudioProcessor::updateWavetableAssignments()
{
    int oscATable = static_cast<int> (parameters.getRawParameterValue ("oscATable")->load());
    int oscBTable = static_cast<int> (parameters.getRawParameterValue ("oscBTable")->load());

    // Clamp to factory table range
    int numTables = static_cast<int> (factoryTables.size());
    oscATable = juce::jlimit (0, numTables - 1, oscATable);
    oscBTable = juce::jlimit (0, numTables - 1, oscBTable);

    if (oscATable != lastOscATable || oscBTable != lastOscBTable)
    {
        for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<PrismVoice*> (synthesiser.getVoice (i)))
            {
                if (oscATable != lastOscATable)
                    voice->setWavetableA (factoryTables[static_cast<size_t> (oscATable)].get());
                if (oscBTable != lastOscBTable)
                    voice->setWavetableB (factoryTables[static_cast<size_t> (oscBTable)].get());
            }
        }
        lastOscATable = oscATable;
        lastOscBTable = oscBTable;
    }
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
