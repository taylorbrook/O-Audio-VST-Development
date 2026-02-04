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

    // OVERTONE BRIGHTNESS - Dark to brilliant (initial partial amplitudes)
    // v2.0.0: Renamed from "brightness" - BREAKING CHANGE
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "overtoneBrightness", 1 },
        "Overtone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        "%"
    ));

    // ACOUSTIC BRIGHTNESS - Controls high-frequency decay rate (v2.0.0)
    // 0% = dark (higher partials decay 4x faster), 100% = bright (normal decay)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "acousticBrightness", 1 },
        "Acoustic",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f,  // Default: slightly natural/warm
        "%"
    ));

    // AIR ABSORPTION - Time-varying lowpass filter simulating air absorption (v2.1.0)
    // 0% = no filtering (transparent), 100% = progressive HF rolloff over decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airAbsorption", 1 },
        "Air",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,  // Default: off (preserves existing sound)
        "%"
    ));

    // AIR ABSORPTION TIME - Independent time control for filter sweep (v2.2.0)
    // 0.1s to 10s - decoupled from note decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airAbsorptionTime", 1 },
        "Air Time",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f, 0.5f),  // Skewed toward shorter times
        2.0f,  // Default: 2 seconds
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                if (value < 1.0f)
                    return juce::String(juce::roundToInt(value * 1000)) + " ms";
                return juce::String(value, 1) + " s";
            })
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

    // BLOOM SPEED - How fast partials swell (v1.4.0: split from bloom)
    // v1.5.1: Display as milliseconds (uses mid-partial range 25-400ms as representative)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeed", 1 },
        "Bloom Speed",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,  // Default: medium speed
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 25.0f, 400.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    // BLOOM AMOUNT - Intensity of spectral swelling (v1.4.0: split from bloom)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmount", 1 },
        "Bloom Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,  // Default: off
        "%"
    ));

    // ========== Bloom Fine Controls (v1.5.0) ==========
    // Per-band bloom control when fine controls are enabled

    // BLOOM_FINE_ENABLED - Toggle for fine controls (0=off, 1=on)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bloomFineEnabled", 1 },
        "Bloom Fine Controls",
        false  // Default: off (use main sliders)
    ));

    // Per-band Speed controls (v1.5.1: Display as milliseconds)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeedLow", 1 },
        "Bloom Speed Low",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 15.0f, 250.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeedMid", 1 },
        "Bloom Speed Mid",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 25.0f, 400.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomSpeedHigh", 1 },
        "Bloom Speed High",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float ms = juce::jmap(value, 50.0f, 800.0f);
                return juce::String(juce::roundToInt(ms)) + " ms";
            })
    ));

    // Per-band Amount controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmountLow", 1 },
        "Bloom Amount Low",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmountMid", 1 },
        "Bloom Amount Mid",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f,
        "%"
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bloomAmountHigh", 1 },
        "Bloom Amount High",
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

    // ATTACK_LEVEL - Transient volume control (v1.3.0, renamed v1.5.2)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackLevel", 1 },
        "Attack Amount",
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
    overtoneBrightnessParam = parameters.getRawParameterValue("overtoneBrightness");
    acousticBrightnessParam = parameters.getRawParameterValue("acousticBrightness");
    airAbsorptionParam = parameters.getRawParameterValue("airAbsorption");
    airAbsorptionTimeParam = parameters.getRawParameterValue("airAbsorptionTime");
    materialParam = parameters.getRawParameterValue("material");
    inharmonicityParam = parameters.getRawParameterValue("inharmonicity");
    bloomSpeedParam = parameters.getRawParameterValue("bloomSpeed");
    bloomAmountParam = parameters.getRawParameterValue("bloomAmount");
    // v1.5.0: Bloom fine controls
    bloomFineEnabledParam = parameters.getRawParameterValue("bloomFineEnabled");
    bloomSpeedLowParam = parameters.getRawParameterValue("bloomSpeedLow");
    bloomSpeedMidParam = parameters.getRawParameterValue("bloomSpeedMid");
    bloomSpeedHighParam = parameters.getRawParameterValue("bloomSpeedHigh");
    bloomAmountLowParam = parameters.getRawParameterValue("bloomAmountLow");
    bloomAmountMidParam = parameters.getRawParameterValue("bloomAmountMid");
    bloomAmountHighParam = parameters.getRawParameterValue("bloomAmountHigh");
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
    float overtoneBrightness = overtoneBrightnessParam->load();
    float acousticBrightness = acousticBrightnessParam->load();
    float airAbsorption = airAbsorptionParam->load();
    float airAbsorptionTime = airAbsorptionTimeParam->load();
    float strikePosition = strikePositionParam->load();
    float malletHardness = malletHardnessParam->load();
    float material = materialParam->load();
    float bloomSpeed = bloomSpeedParam->load();
    float bloomAmount = bloomAmountParam->load();
    // v1.5.0: Bloom fine controls
    bool bloomFineEnabled = bloomFineEnabledParam->load() > 0.5f;
    float bloomSpeedLow = bloomSpeedLowParam->load();
    float bloomSpeedMid = bloomSpeedMidParam->load();
    float bloomSpeedHigh = bloomSpeedHighParam->load();
    float bloomAmountLow = bloomAmountLowParam->load();
    float bloomAmountMid = bloomAmountMidParam->load();
    float bloomAmountHigh = bloomAmountHighParam->load();
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
                inharmonicity, damping, overtoneBrightness, acousticBrightness,
                airAbsorption, airAbsorptionTime,
                strikePosition, malletHardness, material, bloomSpeed, bloomAmount,
                bloomFineEnabled, bloomSpeedLow, bloomSpeedMid, bloomSpeedHigh,
                bloomAmountLow, bloomAmountMid, bloomAmountHigh,
                shimmer,
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

    // ==========================================================================
    // v1.6.0 FACTORY PRESETS - Research-Informed Bell Sounds
    // ==========================================================================
    // Designed using acoustic research on real bells:
    // - Church bell partial ratios: Hum(0.25), Prime(0.5), Tierce(0.6), Quint(0.75), Nominal(1.0)
    // - Gamelan inharmonicity from bronze metallophone spectra
    // - Singing bowl beating frequencies and harmonic structures
    // - Tubular bell 4th partial strike pitch phenomenon
    // - Steel pan harmonic/nonlinear tone generation
    //
    // Material mapping: 0=Bronze, 1=Brass, 2=Steel, 3=Aluminum, 4=Cast Iron
    // All presets utilize bloom, shimmer, multi-stage decay, and ensemble features
    // ==========================================================================

    // ========== ORCHESTRAL (5 presets) ==========
    // Professional mallet instruments and orchestral bells

    // Tubular Bells - Steel tubes struck at 4th partial, bright twangy character
    // Research: Strike pitch from 4th/5th/6th partials in ratio 2:3:4
    presets.push_back({ "Orchestral", "Westminster Chimes", {
        {"strikePosition", 0.45f}, {"malletHardness", 0.72f}, {"damping", 0.82f},
        {"overtoneBrightness", 0.68f}, {"acousticBrightness", 0.7f}, {"material", 0.5f}, {"inharmonicity", 0.48f},
        {"bloomSpeed", 0.35f}, {"bloomAmount", 0.15f}, {"shimmer", 0.18f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.45f},
        {"strikeTime", 0.32f}, {"brilliance", 0.65f}, {"bodyTime", 0.55f}, {"humSustain", 0.4f},
        {"attackLevel", 0.55f}, {"reverbMix", 0.35f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.5f}, {"velocityCurve", 0.0f}
    }, {} });

    // Glockenspiel - Steel bars with suppressed overtones, pure fundamental
    // Research: Short bars suppress higher partials, supports mainly fundamental
    presets.push_back({ "Orchestral", "Crystal Glockenspiel", {
        {"strikePosition", 0.62f}, {"malletHardness", 0.85f}, {"damping", 0.58f},
        {"overtoneBrightness", 0.82f}, {"acousticBrightness", 0.7f}, {"material", 0.5f}, {"inharmonicity", 0.28f},
        {"bloomSpeed", 0.2f}, {"bloomAmount", 0.08f}, {"shimmer", 0.12f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.08f}, {"stereoSpread", 0.5f},
        {"strikeTime", 0.15f}, {"brilliance", 0.85f}, {"bodyTime", 0.35f}, {"humSustain", 0.2f},
        {"attackLevel", 0.7f}, {"reverbMix", 0.28f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Vibraphone - Aluminum bars with motor-driven vibrato, warm mellow tone
    // Research: Resonator tubes amplify fundamental, modal ratios 1:2.76:5.4
    presets.push_back({ "Orchestral", "Jazz Vibes", {
        {"strikePosition", 0.42f}, {"malletHardness", 0.48f}, {"damping", 0.78f},
        {"overtoneBrightness", 0.52f}, {"acousticBrightness", 0.7f}, {"material", 0.75f}, {"inharmonicity", 0.32f},
        {"bloomSpeed", 0.55f}, {"bloomAmount", 0.22f}, {"shimmer", 0.35f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.18f},
        {"octaveBlendSub", 0.12f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.65f},
        {"strikeTime", 0.28f}, {"brilliance", 0.48f}, {"bodyTime", 0.65f}, {"humSustain", 0.55f},
        {"attackLevel", 0.45f}, {"reverbMix", 0.32f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}
    }, {} });

    // Crotales - Small bronze discs, very bright and sustaining
    // Research: Nearly harmonic overtones, extremely long sustain
    presets.push_back({ "Orchestral", "Antique Crotales", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.78f}, {"damping", 0.92f},
        {"overtoneBrightness", 0.88f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.18f},
        {"bloomSpeed", 0.4f}, {"bloomAmount", 0.12f}, {"shimmer", 0.25f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.15f}, {"stereoSpread", 0.55f},
        {"strikeTime", 0.12f}, {"brilliance", 0.9f}, {"bodyTime", 0.75f}, {"humSustain", 0.3f},
        {"attackLevel", 0.6f}, {"reverbMix", 0.4f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Celesta - Hammered steel plates over wooden resonators
    // Research: Piano-like mechanism, softer attack than glockenspiel
    presets.push_back({ "Orchestral", "Nutcracker Celesta", {
        {"strikePosition", 0.38f}, {"malletHardness", 0.42f}, {"damping", 0.65f},
        {"overtoneBrightness", 0.62f}, {"acousticBrightness", 0.7f}, {"material", 0.5f}, {"inharmonicity", 0.22f},
        {"bloomSpeed", 0.3f}, {"bloomAmount", 0.1f}, {"shimmer", 0.15f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.4f},
        {"strikeTime", 0.22f}, {"brilliance", 0.58f}, {"bodyTime", 0.45f}, {"humSustain", 0.35f},
        {"attackLevel", 0.4f}, {"reverbMix", 0.25f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}
    }, {} });

    // ========== SACRED (5 presets) ==========
    // Church bells, meditation bowls, ceremonial instruments

    // Church Bell - Bronze with characteristic minor third partial
    // Research: True harmonic tuning with Hum:Prime:Tierce:Quint:Nominal = 0.25:0.5:0.6:0.75:1.0
    presets.push_back({ "Sacred", "Flemish Carillon", {
        {"strikePosition", 0.28f}, {"malletHardness", 0.62f}, {"damping", 0.95f},
        {"overtoneBrightness", 0.48f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.58f},
        {"bloomSpeed", 0.7f}, {"bloomAmount", 0.28f}, {"shimmer", 0.22f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.22f},
        {"octaveBlendSub", 0.35f}, {"octaveBlendOct", 0.18f}, {"stereoSpread", 0.85f},
        {"strikeTime", 0.45f}, {"brilliance", 0.42f}, {"bodyTime", 0.85f}, {"humSustain", 0.75f},
        {"attackLevel", 0.55f}, {"reverbMix", 0.55f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Russian Orthodox Bell - Cast iron with complex beating partials
    // Research: Intentionally detuned partials create wavering tone
    presets.push_back({ "Sacred", "Russian Zvon", {
        {"strikePosition", 0.22f}, {"malletHardness", 0.55f}, {"damping", 0.98f},
        {"overtoneBrightness", 0.38f}, {"acousticBrightness", 0.7f}, {"material", 1.0f}, {"inharmonicity", 0.68f},
        {"bloomSpeed", 0.8f}, {"bloomAmount", 0.35f}, {"shimmer", 0.28f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.35f},
        {"octaveBlendSub", 0.45f}, {"octaveBlendOct", 0.25f}, {"stereoSpread", 0.95f},
        {"strikeTime", 0.55f}, {"brilliance", 0.35f}, {"bodyTime", 0.92f}, {"humSustain", 0.85f},
        {"attackLevel", 0.5f}, {"reverbMix", 0.65f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Tibetan Singing Bowl - Bronze with beating frequencies
    // Research: Asymmetric bowl creates 2-3Hz monaural beats between split modes
    presets.push_back({ "Sacred", "Himalayan Bowl", {
        {"strikePosition", 0.25f}, {"malletHardness", 0.28f}, {"damping", 0.92f},
        {"overtoneBrightness", 0.45f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.25f},
        {"bloomSpeed", 0.65f}, {"bloomAmount", 0.4f}, {"shimmer", 0.45f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.08f},
        {"octaveBlendSub", 0.2f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.55f},
        {"strikeTime", 0.35f}, {"brilliance", 0.4f}, {"bodyTime", 0.88f}, {"humSustain", 0.7f},
        {"attackLevel", 0.35f}, {"reverbMix", 0.48f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Temple Gong - Large bronze with complex inharmonic spectrum
    // Research: Low strike, long buildup, extremely long decay
    presets.push_back({ "Sacred", "Temple Tam-Tam", {
        {"strikePosition", 0.18f}, {"malletHardness", 0.45f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.32f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.72f},
        {"bloomSpeed", 0.85f}, {"bloomAmount", 0.55f}, {"shimmer", 0.3f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.28f},
        {"octaveBlendSub", 0.55f}, {"octaveBlendOct", 0.15f}, {"stereoSpread", 1.0f},
        {"strikeTime", 0.65f}, {"brilliance", 0.28f}, {"bodyTime", 0.95f}, {"humSustain", 0.9f},
        {"attackLevel", 0.4f}, {"reverbMix", 0.6f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.5f}, {"velocityCurve", 0.0f}
    }, {} });

    // Hand Bell - Small brass bell with clear fundamental
    // Research: Handbell has prominent octave partial, no subjective strike note
    presets.push_back({ "Sacred", "Sanctus Handbell", {
        {"strikePosition", 0.48f}, {"malletHardness", 0.58f}, {"damping", 0.72f},
        {"overtoneBrightness", 0.65f}, {"acousticBrightness", 0.7f}, {"material", 0.25f}, {"inharmonicity", 0.38f},
        {"bloomSpeed", 0.25f}, {"bloomAmount", 0.12f}, {"shimmer", 0.18f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.25f}, {"stereoSpread", 0.5f},
        {"strikeTime", 0.2f}, {"brilliance", 0.6f}, {"bodyTime", 0.52f}, {"humSustain", 0.45f},
        {"attackLevel", 0.6f}, {"reverbMix", 0.38f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}
    }, {} });

    // ========== WORLD (5 presets) ==========
    // Gamelan, steel pan, and ethnic metallophones

    // Gamelan Saron - Bronze bar with extreme inharmonicity
    // Research: Sléndro tuning derived from inharmonic bonang spectrum, ~430Hz fundamental
    presets.push_back({ "World", "Javanese Saron", {
        {"strikePosition", 0.58f}, {"malletHardness", 0.65f}, {"damping", 0.48f},
        {"overtoneBrightness", 0.62f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.88f},
        {"bloomSpeed", 0.15f}, {"bloomAmount", 0.05f}, {"shimmer", 0.08f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.45f},
        {"strikeTime", 0.1f}, {"brilliance", 0.7f}, {"bodyTime", 0.28f}, {"humSustain", 0.15f},
        {"attackLevel", 0.75f}, {"reverbMix", 0.22f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Gamelan Bonang - Knobbed gong essential to sléndro scale
    // Research: Bonang inharmonicity is key element in creating sléndro system
    presets.push_back({ "World", "Balinese Bonang", {
        {"strikePosition", 0.52f}, {"malletHardness", 0.58f}, {"damping", 0.62f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.82f},
        {"bloomSpeed", 0.28f}, {"bloomAmount", 0.15f}, {"shimmer", 0.12f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.12f},
        {"octaveBlendSub", 0.15f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.6f},
        {"strikeTime", 0.18f}, {"brilliance", 0.55f}, {"bodyTime", 0.42f}, {"humSustain", 0.35f},
        {"attackLevel", 0.65f}, {"reverbMix", 0.28f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.5f}, {"velocityCurve", 0.0f}
    }, {} });

    // Trinidad Steel Pan - Hammered steel with harmonic nonlinearity
    // Research: Harmonics generated by nonlinear distortion in curved note area
    presets.push_back({ "World", "Trinidad Tenor Pan", {
        {"strikePosition", 0.62f}, {"malletHardness", 0.68f}, {"damping", 0.68f},
        {"overtoneBrightness", 0.72f}, {"acousticBrightness", 0.7f}, {"material", 0.5f}, {"inharmonicity", 0.42f},
        {"bloomSpeed", 0.22f}, {"bloomAmount", 0.18f}, {"shimmer", 0.2f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.15f},
        {"octaveBlendSub", 0.1f}, {"octaveBlendOct", 0.12f}, {"stereoSpread", 0.7f},
        {"strikeTime", 0.15f}, {"brilliance", 0.68f}, {"bodyTime", 0.45f}, {"humSustain", 0.38f},
        {"attackLevel", 0.7f}, {"reverbMix", 0.25f}, {"outputGain", 0.67f},
        {"nonlinearEffects", 0.15f}, {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.5f}
    }, {} });

    // African Balafon - Wooden bars over gourd resonators
    // Research: Buzzing membranes add texture, warm woody tone
    presets.push_back({ "World", "West African Balafon", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.35f}, {"damping", 0.52f},
        {"overtoneBrightness", 0.45f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.35f},
        {"bloomSpeed", 0.2f}, {"bloomAmount", 0.08f}, {"shimmer", 0.1f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.18f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.55f},
        {"strikeTime", 0.12f}, {"brilliance", 0.35f}, {"bodyTime", 0.38f}, {"humSustain", 0.42f},
        {"attackLevel", 0.5f}, {"reverbMix", 0.2f}, {"outputGain", 0.67f},
        {"nonlinearEffects", 0.08f}, {"strikeNoiseChar", 0.5f}, {"velocityCurve", 0.5f}
    }, {} });

    // Chinese Temple Block - Hollow wooden bell, dry percussive
    // Research: Very short sustain, emphasis on attack transient
    presets.push_back({ "World", "Temple Woodblock", {
        {"strikePosition", 0.65f}, {"malletHardness", 0.72f}, {"damping", 0.35f},
        {"overtoneBrightness", 0.58f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.55f},
        {"bloomSpeed", 0.1f}, {"bloomAmount", 0.0f}, {"shimmer", 0.05f},
        {"unisonCount", 0.0f}, {"unisonDetune", 0.0f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.4f},
        {"strikeTime", 0.08f}, {"brilliance", 0.5f}, {"bodyTime", 0.2f}, {"humSustain", 0.1f},
        {"attackLevel", 0.85f}, {"reverbMix", 0.15f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.5f}, {"velocityCurve", 0.0f}
    }, {} });

    // ========== AMBIENT (5 presets) ==========
    // Textural, atmospheric, and evolving bell sounds

    // Evolving pad with slow spectral bloom
    presets.push_back({ "Ambient", "Spectral Bloom", {
        {"strikePosition", 0.42f}, {"malletHardness", 0.32f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.7f}, {"material", 0.75f}, {"inharmonicity", 0.38f},
        {"bloomSpeed", 0.9f}, {"bloomAmount", 0.7f}, {"shimmer", 0.4f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.28f},
        {"octaveBlendSub", 0.35f}, {"octaveBlendOct", 0.25f}, {"stereoSpread", 0.95f},
        {"strikeTime", 0.55f}, {"brilliance", 0.45f}, {"bodyTime", 0.95f}, {"humSustain", 0.85f},
        {"attackLevel", 0.25f}, {"reverbMix", 0.65f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Crystalline texture with shimmering harmonics
    presets.push_back({ "Ambient", "Ice Crystals", {
        {"strikePosition", 0.58f}, {"malletHardness", 0.65f}, {"damping", 0.88f},
        {"overtoneBrightness", 0.85f}, {"acousticBrightness", 0.7f}, {"material", 0.75f}, {"inharmonicity", 0.28f},
        {"bloomSpeed", 0.45f}, {"bloomAmount", 0.25f}, {"shimmer", 0.55f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.22f},
        {"octaveBlendSub", 0.15f}, {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.88f},
        {"strikeTime", 0.18f}, {"brilliance", 0.88f}, {"bodyTime", 0.72f}, {"humSustain", 0.5f},
        {"attackLevel", 0.5f}, {"reverbMix", 0.55f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Deep droning texture with slow modulation
    presets.push_back({ "Ambient", "Subterranean Drone", {
        {"strikePosition", 0.2f}, {"malletHardness", 0.25f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.25f}, {"acousticBrightness", 0.7f}, {"material", 1.0f}, {"inharmonicity", 0.62f},
        {"bloomSpeed", 0.95f}, {"bloomAmount", 0.6f}, {"shimmer", 0.35f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.35f},
        {"octaveBlendSub", 0.65f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 1.0f},
        {"strikeTime", 0.7f}, {"brilliance", 0.2f}, {"bodyTime", 1.0f}, {"humSustain", 0.95f},
        {"attackLevel", 0.2f}, {"reverbMix", 0.7f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Delicate chime texture with space
    presets.push_back({ "Ambient", "Wind Chimes", {
        {"strikePosition", 0.55f}, {"malletHardness", 0.52f}, {"damping", 0.75f},
        {"overtoneBrightness", 0.7f}, {"acousticBrightness", 0.7f}, {"material", 0.75f}, {"inharmonicity", 0.35f},
        {"bloomSpeed", 0.3f}, {"bloomAmount", 0.15f}, {"shimmer", 0.28f},
        {"unisonCount", 0.33f}, {"unisonDetune", 0.12f},
        {"octaveBlendSub", 0.0f}, {"octaveBlendOct", 0.18f}, {"stereoSpread", 0.8f},
        {"strikeTime", 0.12f}, {"brilliance", 0.72f}, {"bodyTime", 0.55f}, {"humSustain", 0.4f},
        {"attackLevel", 0.45f}, {"reverbMix", 0.5f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Filtered, underwater-like bell texture
    presets.push_back({ "Ambient", "Sunken Cathedral", {
        {"strikePosition", 0.32f}, {"malletHardness", 0.28f}, {"damping", 0.95f},
        {"overtoneBrightness", 0.3f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.55f},
        {"bloomSpeed", 0.75f}, {"bloomAmount", 0.45f}, {"shimmer", 0.38f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.3f},
        {"octaveBlendSub", 0.5f}, {"octaveBlendOct", 0.1f}, {"stereoSpread", 0.92f},
        {"strikeTime", 0.5f}, {"brilliance", 0.25f}, {"bodyTime", 0.88f}, {"humSustain", 0.8f},
        {"attackLevel", 0.3f}, {"reverbMix", 0.72f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // ========== CINEMATIC (5 presets) ==========
    // Film score, trailer, and dramatic bell sounds

    // Massive impact bell for trailer moments
    presets.push_back({ "Cinematic", "Trailer Impact", {
        {"strikePosition", 0.3f}, {"malletHardness", 0.78f}, {"damping", 0.98f},
        {"overtoneBrightness", 0.55f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.58f},
        {"bloomSpeed", 0.6f}, {"bloomAmount", 0.35f}, {"shimmer", 0.22f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.3f},
        {"octaveBlendSub", 0.6f}, {"octaveBlendOct", 0.25f}, {"stereoSpread", 1.0f},
        {"strikeTime", 0.4f}, {"brilliance", 0.5f}, {"bodyTime", 0.9f}, {"humSustain", 0.75f},
        {"attackLevel", 0.75f}, {"reverbMix", 0.55f}, {"outputGain", 0.67f},
        {"pitchEnvelope", 0.08f}, {"pitchEnvTime", 0.45f},
        {"strikeNoiseChar", 0.5f}, {"velocityCurve", 0.5f}
    }, {} });

    // Tense, dissonant bell for suspense
    presets.push_back({ "Cinematic", "Dread Toll", {
        {"strikePosition", 0.72f}, {"malletHardness", 0.88f}, {"damping", 0.55f},
        {"overtoneBrightness", 0.82f}, {"acousticBrightness", 0.7f}, {"material", 1.0f}, {"inharmonicity", 0.78f},
        {"bloomSpeed", 0.35f}, {"bloomAmount", 0.2f}, {"shimmer", 0.15f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.4f},
        {"octaveBlendSub", 0.2f}, {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.75f},
        {"strikeTime", 0.15f}, {"brilliance", 0.78f}, {"bodyTime", 0.4f}, {"humSustain", 0.25f},
        {"attackLevel", 0.8f}, {"reverbMix", 0.35f}, {"outputGain", 0.67f},
        {"nonlinearEffects", 0.25f}, {"pitchEnvelope", 0.15f}, {"pitchEnvTime", 0.25f},
        {"strikeNoiseChar", 1.0f}, {"velocityCurve", 0.5f}
    }, {} });

    // Ethereal, angelic bell for emotional moments
    presets.push_back({ "Cinematic", "Ascension", {
        {"strikePosition", 0.45f}, {"malletHardness", 0.4f}, {"damping", 0.92f},
        {"overtoneBrightness", 0.68f}, {"acousticBrightness", 0.7f}, {"material", 0.75f}, {"inharmonicity", 0.3f},
        {"bloomSpeed", 0.7f}, {"bloomAmount", 0.45f}, {"shimmer", 0.4f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.2f},
        {"octaveBlendSub", 0.25f}, {"octaveBlendOct", 0.4f}, {"stereoSpread", 0.95f},
        {"strikeTime", 0.35f}, {"brilliance", 0.6f}, {"bodyTime", 0.85f}, {"humSustain", 0.7f},
        {"attackLevel", 0.35f}, {"reverbMix", 0.68f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Dark, ominous bell for horror/thriller
    presets.push_back({ "Cinematic", "Harbinger", {
        {"strikePosition", 0.18f}, {"malletHardness", 0.55f}, {"damping", 1.0f},
        {"overtoneBrightness", 0.22f}, {"acousticBrightness", 0.7f}, {"material", 1.0f}, {"inharmonicity", 0.7f},
        {"bloomSpeed", 0.85f}, {"bloomAmount", 0.5f}, {"shimmer", 0.25f},
        {"unisonCount", 1.0f}, {"unisonDetune", 0.38f},
        {"octaveBlendSub", 0.7f}, {"octaveBlendOct", 0.0f}, {"stereoSpread", 0.9f},
        {"strikeTime", 0.6f}, {"brilliance", 0.18f}, {"bodyTime", 0.98f}, {"humSustain", 0.92f},
        {"attackLevel", 0.4f}, {"reverbMix", 0.62f}, {"outputGain", 0.67f},
        {"nonlinearEffects", 0.12f}, {"pitchEnvelope", 0.05f}, {"pitchEnvTime", 0.7f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.0f}
    }, {} });

    // Bright, triumphant bell for victory moments
    presets.push_back({ "Cinematic", "Victory Peal", {
        {"strikePosition", 0.4f}, {"malletHardness", 0.7f}, {"damping", 0.85f},
        {"overtoneBrightness", 0.75f}, {"acousticBrightness", 0.7f}, {"material", 0.0f}, {"inharmonicity", 0.45f},
        {"bloomSpeed", 0.4f}, {"bloomAmount", 0.2f}, {"shimmer", 0.3f},
        {"unisonCount", 0.67f}, {"unisonDetune", 0.18f},
        {"octaveBlendSub", 0.3f}, {"octaveBlendOct", 0.35f}, {"stereoSpread", 0.88f},
        {"strikeTime", 0.25f}, {"brilliance", 0.72f}, {"bodyTime", 0.7f}, {"humSustain", 0.55f},
        {"attackLevel", 0.65f}, {"reverbMix", 0.48f}, {"outputGain", 0.67f},
        {"strikeNoiseChar", 0.0f}, {"velocityCurve", 0.5f}
    }, {} });

    presetManager.initializeFactoryPresets(presets);
}

//==============================================================================
// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBellsAudioProcessor();
}
