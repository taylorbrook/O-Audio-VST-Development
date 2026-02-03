/*
  ==============================================================================

    O-Bells - Audio Processor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (MUST be defined before constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OBellsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Main Panel Parameters (7) ==========

    // STRIKE_POSITION - Center to edge strike
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "strikePosition", 1 },
        "Strike",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // MALLET_HARDNESS - Soft to hard striker
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "malletHardness", 1 },
        "Mallet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // DAMPING - Hand-damped to free-ring
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "damping", 1 },
        "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f,
        "%"
    ));

    // BRIGHTNESS - Dark to brilliant
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brightness", 1 },
        "Bright",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // MATERIAL - Discrete choice parameter (v1.3.0)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "material", 1 },
        "Material",
        juce::StringArray { "Bronze", "Brass", "Steel", "Aluminum", "Cast Iron" },
        0  // Default: Bronze
    ));

    // INHARMONICITY - Pure harmonic to gamelan
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inharmonicity", 1 },
        "Inharm",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // BLOOM - Spectral swelling before decay (v1.2.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloom", 1 },
        "Bloom",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // SHIMMER - Frequency modulation that increases during decay (v1.2.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "shimmer", 1 },
        "Shimmer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.2f,
        "%"
    ));

    // ========== Ensemble Section Parameters (5) ==========

    // UNISON_COUNT - Number of detuned bell copies (1-4)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "unisonCount", 1 },
        "Unison",
        1,
        4,
        1
    ));

    // UNISON_DETUNE - Detune spread (0-50 cents)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "unisonDetune", 1 },
        "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f),
        10.0f,
        "cents"
    ));

    // OCTAVE_BLEND_SUB - Sub-octave layer mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveBlendSub", 1 },
        "Sub",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // OCTAVE_BLEND_OCT - Upper-octave layer mix
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "octaveBlendOct", 1 },
        "Oct",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // STEREO_SPREAD - Ensemble panning width
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stereoSpread", 1 },
        "Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // ========== Advanced Panel Parameters (10) ==========

    // PARTIAL_TUNING - Fine-tune minor-third partial
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "partialTuning", 1 },
        "Partial Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f,
        "cents"
    ));

    // NONLINEAR_EFFECTS - Bell warping/distortion
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "nonlinearEffects", 1 },
        "Nonlinear",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // STRIKE_NOISE_CHARACTER - Transient filter type
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "strikeNoiseChar", 1 },
        "Noise",
        juce::StringArray { "Click", "Thud", "Ping" },
        0
    ));

    // ATTACK_LEVEL - Transient volume control (v1.3.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackLevel", 1 },
        "Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,  // Default 50% (natural level)
        "%"
    ));

    // DECAY_SHAPE parameter removed in v1.2.0 - always use multi-stage decay

    // VELOCITY_CURVE - Velocity response shaping
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "velocityCurve", 1 },
        "Velocity",
        juce::StringArray { "Linear", "Exponential", "Logarithmic" },
        0
    ));

    // PITCH_ENVELOPE - Initial pitch drop amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnvelope", 1 },
        "Pitch Env",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    // PITCH_ENV_TIME - Pitch envelope return time
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "pitchEnvTime", 1 },
        "P.Env Time",
        juce::NormalisableRange<float>(5.0f, 200.0f, 1.0f, 0.5f),
        50.0f,
        "ms"
    ));

    // ========== Multi-Stage Envelope Parameters (4) ==========
    // Only active when decayShape == 2 (Multi-stage)

    // STRIKE_TIME - Duration of bright metallic transient
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "strikeTime", 1 },
        "Strike Time",
        juce::NormalisableRange<float>(5.0f, 100.0f, 0.1f),
        30.0f,
        "ms"
    ));

    // BRILLIANCE - High-frequency sustain (0=warm/woody, 100=bright/glassy)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brilliance", 1 },
        "Brilliance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // BODY_TIME - Duration of main tonal decay phase
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyTime", 1 },
        "Body Time",
        juce::NormalisableRange<float>(100.0f, 5000.0f, 1.0f),
        1500.0f,
        "ms"
    ));

    // HUM_SUSTAIN - Extension of low partial sustain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "humSustain", 1 },
        "Hum Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        "%"
    ));

    // REVERB_MIX - Spaciousness control (0-100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverbMix", 1 },
        "Reverb",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f,
        "%"
    ));

    // OUTPUT_GAIN - Master output level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1 },
        "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    return layout;
}

//==============================================================================
OBellsAudioProcessor::OBellsAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Bells")
{
    // Add 8 bell voices
    for (int i = 0; i < 8; ++i)
        synthesiser.addVoice(new BellVoice());

    // Add one sound (all notes trigger bell sounds)
    synthesiser.addSound(new BellSound());

    // Initialize factory presets (only on first run)
    initializeFactoryPresets();
}

OBellsAudioProcessor::~OBellsAudioProcessor()
{
}

//==============================================================================
void OBellsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare synthesiser with sample rate
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare all voices
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BellVoice*>(synthesiser.getVoice(i)))
        {
            voice->prepare(sampleRate, samplesPerBlock);
        }
    }

    // Prepare reverb DSP
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    reverb.prepare(spec);

    // Configure reverb for spacious bell sound
    reverbParams.roomSize = BellReverbSpec::roomSize;
    reverbParams.damping = BellReverbSpec::damping;
    reverbParams.width = BellReverbSpec::width;
    reverbParams.freezeMode = BellReverbSpec::freezeMode;
    reverbParams.wetLevel = 0.3f;  // Will be modulated by reverbMix
    reverbParams.dryLevel = 0.7f;
    reverb.setParameters(reverbParams);

    // Cache parameter pointers (atomic reads in processBlock)
    // Main Panel
    strikePositionParam = parameters.getRawParameterValue("strikePosition");
    malletHardnessParam = parameters.getRawParameterValue("malletHardness");
    dampingParam = parameters.getRawParameterValue("damping");
    brightnessParam = parameters.getRawParameterValue("brightness");
    materialParam = parameters.getRawParameterValue("material");
    inharmonicityParam = parameters.getRawParameterValue("inharmonicity");
    bloomParam = parameters.getRawParameterValue("bloom");
    shimmerParam = parameters.getRawParameterValue("shimmer");
    // Ensemble
    unisonCountParam = parameters.getRawParameterValue("unisonCount");
    unisonDetuneParam = parameters.getRawParameterValue("unisonDetune");
    octaveBlendSubParam = parameters.getRawParameterValue("octaveBlendSub");
    octaveBlendOctParam = parameters.getRawParameterValue("octaveBlendOct");
    stereoSpreadParam = parameters.getRawParameterValue("stereoSpread");
    // Advanced
    partialTuningParam = parameters.getRawParameterValue("partialTuning");
    nonlinearEffectsParam = parameters.getRawParameterValue("nonlinearEffects");
    strikeNoiseCharParam = parameters.getRawParameterValue("strikeNoiseChar");
    attackLevelParam = parameters.getRawParameterValue("attackLevel");
    // decayShapeParam removed in v1.2.0 - always use multi-stage
    velocityCurveParam = parameters.getRawParameterValue("velocityCurve");
    pitchEnvelopeParam = parameters.getRawParameterValue("pitchEnvelope");
    pitchEnvTimeParam = parameters.getRawParameterValue("pitchEnvTime");
    // Multi-stage envelope
    strikeTimeParam = parameters.getRawParameterValue("strikeTime");
    brillianceParam = parameters.getRawParameterValue("brilliance");
    bodyTimeParam = parameters.getRawParameterValue("bodyTime");
    humSustainParam = parameters.getRawParameterValue("humSustain");
    // Output
    reverbMixParam = parameters.getRawParameterValue("reverbMix");
    outputGainParam = parameters.getRawParameterValue("outputGain");
}

void OBellsAudioProcessor::releaseResources()
{
    // Release reverb resources
    reverb.reset();
}

void OBellsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer
    buffer.clear();

    // Read parameters (atomic, real-time safe)
    float inharmonicity = inharmonicityParam->load();
    float damping = dampingParam->load();
    float brightness = brightnessParam->load();
    float strikePosition = strikePositionParam->load();
    float malletHardness = malletHardnessParam->load();
    float material = materialParam->load();
    float bloom = bloomParam->load();
    float shimmer = shimmerParam->load();
    int unisonCount = static_cast<int>(unisonCountParam->load());
    float unisonDetune = unisonDetuneParam->load();
    float octaveBlendSub = octaveBlendSubParam->load();
    float octaveBlendOct = octaveBlendOctParam->load();
    float stereoSpread = stereoSpreadParam->load();
    float partialTuning = partialTuningParam->load();
    float pitchEnvelope = pitchEnvelopeParam->load();
    float pitchEnvTime = pitchEnvTimeParam->load();
    // decayShape removed - always use multi-stage in v1.2.0
    int velocityCurve = static_cast<int>(velocityCurveParam->load());
    float nonlinearEffects = nonlinearEffectsParam->load();
    int strikeNoiseChar = static_cast<int>(strikeNoiseCharParam->load());
    float attackLevel = attackLevelParam->load();
    // Multi-stage envelope params (always active in v1.2.0)
    float strikeTime = strikeTimeParam->load();
    float brilliance = brillianceParam->load();
    float bodyTime = bodyTimeParam->load();
    float humSustain = humSustainParam->load();
    float outputGain = outputGainParam->load();

    // Update all voice parameters
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BellVoice*>(synthesiser.getVoice(i)))
        {
            voice->updateParameters(
                inharmonicity, damping, brightness,
                strikePosition, malletHardness, material, bloom, shimmer,
                unisonCount, unisonDetune,
                octaveBlendSub, octaveBlendOct, stereoSpread,
                partialTuning, pitchEnvelope, pitchEnvTime,
                velocityCurve, nonlinearEffects,
                strikeNoiseChar, attackLevel, outputGain,
                strikeTime, brilliance, bodyTime, humSustain
            );
        }
    }

    // Process MIDI and render audio
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Read reverb mix parameter and apply reverb
    float reverbMix = reverbMixParam->load();
    reverbParams.wetLevel = reverbMix;
    reverbParams.dryLevel = 1.0f - (reverbMix * 0.5f);  // Keep some dry signal even at 100%
    reverb.setParameters(reverbParams);

    // Process reverb (stereo in-place)
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    // Calculate output levels for metering (peak detection)
    const int numSamples = buffer.getNumSamples();
    float peakLeft = 0.0f;
    float peakRight = 0.0f;

    if (buffer.getNumChannels() >= 1)
        peakLeft = buffer.getMagnitude(0, 0, numSamples);
    if (buffer.getNumChannels() >= 2)
        peakRight = buffer.getMagnitude(1, 0, numSamples);

    // Store with ballistics (slight hold for visual smoothness)
    const float decay = 0.85f;  // Meter decay rate
    outputLevelLeft.store(std::max(peakLeft, outputLevelLeft.load() * decay));
    outputLevelRight.store(std::max(peakRight, outputLevelRight.load() * decay));
}

//==============================================================================
juce::AudioProcessorEditor* OBellsAudioProcessor::createEditor()
{
    return new OBellsAudioProcessorEditor(*this);
}

//==============================================================================
void OBellsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OBellsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

//==============================================================================
void OBellsAudioProcessor::initializeFactoryPresets()
{
    // Only initialize if factory presets don't exist yet
    if (presetManager.factoryPresetsExist())
        return;

    std::vector<OuariconPresetManager::FactoryPresetDef> presets;

    // ========== ORCHESTRAL ==========
    presets.push_back({ "Orchestral", "Tubular Bells", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.6f}, {"damping", 0.8f},
        {"brightness", 0.55f}, {"material", 0.1f}, {"inharmonicity", 0.45f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Orchestral", "Concert Chimes", {
        {"strikePosition", 0.5f}, {"malletHardness", 0.7f}, {"damping", 0.85f},
        {"brightness", 0.65f}, {"material", 0.15f}, {"inharmonicity", 0.5f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Orchestral", "Glockenspiel", {
        {"strikePosition", 0.6f}, {"malletHardness", 0.8f}, {"damping", 0.7f},
        {"brightness", 0.75f}, {"material", 0.4f}, {"inharmonicity", 0.35f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Orchestral", "Celesta Mallet", {
        {"strikePosition", 0.35f}, {"malletHardness", 0.45f}, {"damping", 0.6f},
        {"brightness", 0.6f}, {"material", 0.7f}, {"inharmonicity", 0.25f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Orchestral", "Vibraphone", {
        {"strikePosition", 0.45f}, {"malletHardness", 0.5f}, {"damping", 0.75f},
        {"brightness", 0.5f}, {"material", 0.25f}, {"inharmonicity", 0.3f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.16f}, {"octaveBlendSub", 0.2f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    // ========== SACRED ==========
    presets.push_back({ "Sacred", "Church Bell", {
        {"strikePosition", 0.3f}, {"malletHardness", 0.65f}, {"damping", 0.95f},
        {"brightness", 0.5f}, {"material", 0.15f}, {"inharmonicity", 0.6f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Sacred", "Cathedral Carillon", {
        {"strikePosition", 0.35f}, {"malletHardness", 0.55f}, {"damping", 0.9f},
        {"brightness", 0.45f}, {"material", 0.1f}, {"inharmonicity", 0.55f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Sacred", "Meditation Bowl", {
        {"strikePosition", 0.25f}, {"malletHardness", 0.3f}, {"damping", 0.85f},
        {"brightness", 0.4f}, {"material", 0.2f}, {"inharmonicity", 0.3f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Sacred", "Temple Gong", {
        {"strikePosition", 0.2f}, {"malletHardness", 0.5f}, {"damping", 0.95f},
        {"brightness", 0.35f}, {"material", 0.12f}, {"inharmonicity", 0.65f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Sacred", "Singing Bowl", {
        {"strikePosition", 0.3f}, {"malletHardness", 0.25f}, {"damping", 0.9f},
        {"brightness", 0.5f}, {"material", 0.3f}, {"inharmonicity", 0.25f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.16f}, {"octaveBlendSub", 0.2f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    // ========== WORLD ==========
    presets.push_back({ "World", "Gamelan Saron", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.6f}, {"damping", 0.5f},
        {"brightness", 0.6f}, {"material", 0.15f}, {"inharmonicity", 0.85f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.0f}
    }, {} });

    presets.push_back({ "World", "Gamelan Bonang", {
        {"strikePosition", 0.5f}, {"malletHardness", 0.55f}, {"damping", 0.6f},
        {"brightness", 0.55f}, {"material", 0.2f}, {"inharmonicity", 0.75f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.16f}, {"octaveBlendSub", 0.2f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.0f}
    }, {} });

    presets.push_back({ "World", "Tibetan Bowl", {
        {"strikePosition", 0.25f}, {"malletHardness", 0.2f}, {"damping", 0.88f},
        {"brightness", 0.45f}, {"material", 0.25f}, {"inharmonicity", 0.35f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "World", "Steel Pan", {
        {"strikePosition", 0.6f}, {"malletHardness", 0.65f}, {"damping", 0.65f},
        {"brightness", 0.7f}, {"material", 0.35f}, {"inharmonicity", 0.4f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.16f}, {"octaveBlendSub", 0.2f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "World", "Kalimba Bell", {
        {"strikePosition", 0.45f}, {"malletHardness", 0.4f}, {"damping", 0.55f},
        {"brightness", 0.65f}, {"material", 0.1f}, {"inharmonicity", 0.2f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}, {"decayShape", 0.5f}
    }, {} });

    // ========== AMBIENT ==========
    presets.push_back({ "Ambient", "Frozen Shimmer", {
        {"strikePosition", 0.6f}, {"malletHardness", 0.7f}, {"damping", 1.0f},
        {"brightness", 0.8f}, {"material", 0.85f}, {"inharmonicity", 0.4f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.3f}, {"octaveBlendSub", 0.3f},
        {"octaveBlendOct", 0.2f}, {"stereoSpread", 0.8f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Ambient", "Bell Pad", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.35f}, {"damping", 0.95f},
        {"brightness", 0.5f}, {"material", 0.5f}, {"inharmonicity", 0.5f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.3f}, {"octaveBlendSub", 0.3f},
        {"octaveBlendOct", 0.2f}, {"stereoSpread", 0.8f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Ambient", "Crystal Drone", {
        {"strikePosition", 0.5f}, {"malletHardness", 0.45f}, {"damping", 1.0f},
        {"brightness", 0.7f}, {"material", 0.95f}, {"inharmonicity", 0.35f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Ambient", "Ethereal Chime", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.6f}, {"damping", 0.9f},
        {"brightness", 0.75f}, {"material", 0.8f}, {"inharmonicity", 0.3f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.3f}, {"octaveBlendSub", 0.3f},
        {"octaveBlendOct", 0.2f}, {"stereoSpread", 0.8f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Ambient", "Submerged Bells", {
        {"strikePosition", 0.3f}, {"malletHardness", 0.25f}, {"damping", 0.92f},
        {"brightness", 0.3f}, {"material", 0.6f}, {"inharmonicity", 0.55f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    // ========== CINEMATIC ==========
    presets.push_back({ "Cinematic", "Epic Bell", {
        {"strikePosition", 0.35f}, {"malletHardness", 0.7f}, {"damping", 0.95f},
        {"brightness", 0.6f}, {"material", 0.15f}, {"inharmonicity", 0.55f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.24f}, {"octaveBlendSub", 0.5f},
        {"octaveBlendOct", 0.3f}, {"stereoSpread", 1.0f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Cinematic", "Tension Chime", {
        {"strikePosition", 0.7f}, {"malletHardness", 0.85f}, {"damping", 0.6f},
        {"brightness", 0.85f}, {"material", 0.45f}, {"inharmonicity", 0.7f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.24f}, {"octaveBlendSub", 0.5f},
        {"octaveBlendOct", 0.3f}, {"stereoSpread", 1.0f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.1f}, {"pitchEnvTime", 0.5f},
        {"nonlinearEffects", 0.2f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.5f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Cinematic", "Horror Stinger", {
        {"strikePosition", 0.8f}, {"malletHardness", 0.95f}, {"damping", 0.4f},
        {"brightness", 0.9f}, {"material", 0.5f}, {"inharmonicity", 0.8f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f}, {"octaveBlendSub", 0.0f},
        {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.5f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.3f}, {"pitchEnvTime", 0.3f},
        {"nonlinearEffects", 0.3f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.5f}, {"decayShape", 0.0f}
    }, {} });

    presets.push_back({ "Cinematic", "Dramatic Swell", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.55f}, {"damping", 0.98f},
        {"brightness", 0.55f}, {"material", 0.2f}, {"inharmonicity", 0.5f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.0f}, {"pitchEnvTime", 0.23f},
        {"nonlinearEffects", 0.0f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}, {"decayShape", 0.5f}
    }, {} });

    presets.push_back({ "Cinematic", "Distant Thunder", {
        {"strikePosition", 0.2f}, {"malletHardness", 0.4f}, {"damping", 1.0f},
        {"brightness", 0.25f}, {"material", 0.1f}, {"inharmonicity", 0.65f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.4f}, {"octaveBlendSub", 0.4f},
        {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.9f},
        {"partialTuning", 0.5f}, {"pitchEnvelope", 0.05f}, {"pitchEnvTime", 0.8f},
        {"nonlinearEffects", 0.1f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}, {"decayShape", 0.5f}
    }, {} });

    presetManager.initializeFactoryPresets(presets);
}

//==============================================================================
// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBellsAudioProcessor();
}
