/*
  ==============================================================================

    O-Reed - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (MUST be defined before constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OReedAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Primary Controls (5) ==========

    // breathPressure - Mouth pressure (p_mouth) -- main dynamics
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "breathPressure", 1 },
        "Breath Pressure",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // embouchure - Lip force -- brightness, pitch bending
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "embouchure", 1 },
        "Embouchure",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.4f
    ));

    // reedHardness - Reed stiffness (k_r) -- attack character, brightness
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reedHardness", 1 },
        "Reed Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // boreCharacter - Bore taper -- 0=cylindrical to 1=full cone
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "boreCharacter", 1 },
        "Bore Character",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // instrumentPreset - Macro crossfading full parameter sets between presets
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "instrumentPreset", 1 },
        "Instrument Preset",
        juce::StringArray {
            "Bb Clarinet", "Bass Clarinet",
            "Alto Saxophone", "Tenor Saxophone", "Soprano Saxophone", "Baritone Saxophone",
            "Oboe", "English Horn", "Bassoon",
            "Duduk", "Shehnai", "Suona", "Hichiriki", "Zurna", "Piri",
            "Arghul", "Launeddas", "Mijwiz",
            "Glass Reed", "Metal Wind", "Impossible Bore"
        },
        0
    ));

    // ========== Secondary Controls (5) ==========

    // reedOpening - Rest opening (H) -- ease of onset, dynamic range
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reedOpening", 1 },
        "Reed Opening",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.4f
    ));

    // bellSize - Bell flare -- projection, high-frequency radiation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bellSize", 1 },
        "Bell Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // airNoise - Breath noise mix -- breathiness
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "airNoise", 1 },
        "Air Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.15f
    ));

    // doubleReed - Psi confinement -- single to double reed character
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "doubleReed", 1 },
        "Double Reed",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // boreDiameter - Viscothermal loss and impedance
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "boreDiameter", 1 },
        "Bore Diameter",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // ========== Advanced / Sound Design (7) ==========

    // reedMass - mu_r -- reed resonance effects, transient character
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reedMass", 1 },
        "Reed Mass",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // reedDamping - g_r -- reed ring vs. muted bore-driven tone
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reedDamping", 1 },
        "Reed Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // mouthpieceVol - Helmholtz resonance -- pitch correction
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mouthpieceVol", 1 },
        "Mouthpiece Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // toneHoleCutoff - Spectral envelope from open tone holes (skew 0.3f for log-like)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "toneHoleCutoff", 1 },
        "Tone Hole Cutoff",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.3f),
        8000.0f,
        "Hz"
    ));

    // registerHole - Overblowing control
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "registerHole", 1 },
        "Register Hole",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // boreLength - Effective tube length -- register density
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "boreLength", 1 },
        "Bore Length",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // boreProfile - Simple (single taper) vs Multi-segment
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "boreProfile", 1 },
        "Bore Profile",
        juce::StringArray { "Simple", "Multi-segment" },
        0
    ));

    // ========== Expressive Controls (7) ==========

    // vibratoDepth - LFO modulation depth
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoDepth", 1 },
        "Vibrato Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // vibratoRate - LFO frequency
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "vibratoRate", 1 },
        "Vibrato Rate",
        juce::NormalisableRange<float>(1.0f, 10.0f, 0.1f),
        5.0f,
        "Hz"
    ));

    // vibratoSource - Lip / Breath / Throat modulation target
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "vibratoSource", 1 },
        "Vibrato Source",
        juce::StringArray { "Lip", "Breath", "Throat" },
        0
    ));

    // growlAmount - Vocal fold coupling strength
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "growlAmount", 1 },
        "Growl Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // flutterTongue - ~25 Hz pressure modulation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "flutterTongue", 1 },
        "Flutter Tongue",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // subtone - Minimum reed opening + high lip damping
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "subtone", 1 },
        "Subtone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // attackChiff - Pressure overshoot at note onset
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attackChiff", 1 },
        "Attack Chiff",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // ========== Impossible Physics (5) ==========

    // infiniteSustain - Reduces bore losses toward zero
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "infiniteSustain", 1 },
        "Infinite Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // reverseBore - Negative taper (hichiriki-like, extended)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reverseBore", 1 },
        "Reverse Bore",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // dualBore - Second parallel waveguide (arghul/launeddas drone)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "dualBore", 1 },
        "Dual Bore",
        false
    ));

    // dronePitch - Pitch offset for second bore (cents for fine control)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dronePitch", 2 },
        "Drone Pitch",
        juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
        0.0f,
        "cents"
    ));

    // feedbackPath - Cross-modulation between reed and bore
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedbackPath", 1 },
        "Feedback Path",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // ========== Tuning (2) ==========

    // referencePitch - A4 reference frequency
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "referencePitch", 1 },
        "Reference Pitch",
        juce::NormalisableRange<float>(220.0f, 880.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    // tuningSystem - Scala/TUN / MTS-ESP / 12-TET
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuningSystem", 1 },
        "Tuning System",
        juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" },
        2  // Default: 12-TET (index 2)
    ));

    // ========== Voice Configuration (3) ==========

    // polyMode - Monophonic (default) / Polyphonic
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "polyMode", 1 },
        "Polyphony Mode",
        juce::StringArray { "Monophonic", "Polyphonic" },
        0
    ));

    // maxVoices - Maximum polyphony (poly mode only)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "maxVoices", 1 },
        "Max Voices",
        1,
        16,
        8
    ));

    // oversampling - 2x (default) / 4x (mono quality)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "oversampling", 1 },
        "Oversampling",
        juce::StringArray { "2x", "4x" },
        0
    ));

    // ========== Output (1) ==========

    // outputGain - Master output level
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    return layout;
}

//==============================================================================
OReedAudioProcessor::OReedAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Reed")
{
    // Create 16 ReedWindVoice instances
    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new ReedWindVoice(i);
        voice->setAPVTS(&parameters);
        voice->setTuningEngine(&tuningEngine);
        voice->setPendingTuningSource(&vst3Extensions.getPendingTable()); // Phase 24: NE
        synthesiser.addVoice(voice);
    }

    // Enable legacy mode for standard MIDI (channel range 1-16, pitchbend +/-2 semitones)
    synthesiser.enableLegacyMode(2, juce::Range<int>(1, 17));

    initializeFactoryPresets();
}

OReedAudioProcessor::~OReedAudioProcessor()
{
}

//==============================================================================
void OReedAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    // Prepare all voice DSP components with sample rate and block size
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* reedVoice = dynamic_cast<ReedWindVoice*>(synthesiser.getVoice(i)))
            reedVoice->prepare(sampleRate, samplesPerBlock);
    }

    // Report oversampling latency to host (based on default 2x)
    if (auto* voice = dynamic_cast<ReedWindVoice*>(synthesiser.getVoice(0)))
        setLatencySamples(static_cast<int>(std::ceil(voice->getOversamplingLatency())));
}

void OReedAudioProcessor::releaseResources()
{
}

void OReedAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch (Phase 24).
    vst3Extensions.drainAndUpdate();

    // Wire tuning engine parameters
    float refPitch = parameters.getRawParameterValue("referencePitch")->load();
    tuningEngine.setMasterTune(static_cast<double>(refPitch));
    int tuningSystemIdx = static_cast<int>(parameters.getRawParameterValue("tuningSystem")->load());
    switch (tuningSystemIdx)
    {
        case 0:  tuningEngine.setMode(TuningEngine::Mode::Scala); break;
        case 1:  tuningEngine.setMode(TuningEngine::Mode::MTSESP); break;
        default: tuningEngine.setMode(TuningEngine::Mode::TwelveTET); break;
    }

    synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

//==============================================================================
bool OReedAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Output must be stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // No input bus for synth
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
}

//==============================================================================
juce::AudioProcessorEditor* OReedAudioProcessor::createEditor()
{
    return new OReedAudioProcessorEditor(*this);
}

//==============================================================================
void OReedAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OReedAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        presetManager.setStateFromXml(xmlState.get());

        // Migrate dronePitch v1 (semitones, range -24..24) -> v2 (cents, range -2400..2400)
        auto state = parameters.state;
        auto dronePitchParam = state.getChildWithProperty("id", "dronePitch");
        if (dronePitchParam.isValid())
        {
            float val = dronePitchParam.getProperty("value", 0.0f);
            if (std::abs(val) <= 24.5f && std::abs(val) > 0.01f)
                dronePitchParam.setProperty("value", val * 100.0f, nullptr);
        }
    }
}

//==============================================================================
void OReedAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    if (factoryDir.isDirectory() &&
        factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
        return;

    auto normalize = [this](const juce::String& paramId, float rawValue) -> float {
        if (auto* param = parameters.getParameter(paramId))
            return param->convertTo0to1(rawValue);
        return rawValue;
    };

    std::vector<OuariconPresetManager::FactoryPresetDef> presets = {
        // =====================================================================
        // WESTERN REED INSTRUMENTS (9)
        // =====================================================================
        {
            "Bb Clarinet",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.4f}, {"reedHardness", 0.5f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 0)},
                {"reedOpening", 0.4f}, {"bellSize", 0.35f}, {"airNoise", 0.1f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.45f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.25f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Bass Clarinet",
            {
                {"breathPressure", 0.55f}, {"embouchure", 0.35f}, {"reedHardness", 0.4f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 1)},
                {"reedOpening", 0.45f}, {"bellSize", 0.5f}, {"airNoise", 0.12f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.6f},
                {"reedMass", 0.4f}, {"reedDamping", 0.45f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 1200.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.7f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 4.5f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.15f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Alto Saxophone",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.45f}, {"reedHardness", 0.5f},
                {"boreCharacter", 0.8f}, {"instrumentPreset", normalize("instrumentPreset", 2)},
                {"reedOpening", 0.4f}, {"bellSize", 0.6f}, {"airNoise", 0.15f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.55f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2500.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.15f}, {"vibratoRate", normalize("vibratoRate", 5.5f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.35f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Tenor Saxophone",
            {
                {"breathPressure", 0.55f}, {"embouchure", 0.4f}, {"reedHardness", 0.45f},
                {"boreCharacter", 0.75f}, {"instrumentPreset", normalize("instrumentPreset", 3)},
                {"reedOpening", 0.45f}, {"bellSize", 0.65f}, {"airNoise", 0.12f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.6f},
                {"reedMass", 0.35f}, {"reedDamping", 0.45f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.55f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.2f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.1f}, {"attackChiff", 0.3f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Soprano Saxophone",
            {
                {"breathPressure", 0.45f}, {"embouchure", 0.5f}, {"reedHardness", 0.55f},
                {"boreCharacter", 0.85f}, {"instrumentPreset", normalize("instrumentPreset", 4)},
                {"reedOpening", 0.35f}, {"bellSize", 0.4f}, {"airNoise", 0.15f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.25f}, {"reedDamping", 0.55f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 3500.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.4f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.1f}, {"vibratoRate", normalize("vibratoRate", 5.5f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.35f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Baritone Saxophone",
            {
                {"breathPressure", 0.6f}, {"embouchure", 0.35f}, {"reedHardness", 0.4f},
                {"boreCharacter", 0.7f}, {"instrumentPreset", normalize("instrumentPreset", 5)},
                {"reedOpening", 0.5f}, {"bellSize", 0.75f}, {"airNoise", 0.1f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.7f},
                {"reedMass", 0.4f}, {"reedDamping", 0.4f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 1500.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.65f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.15f}, {"vibratoRate", normalize("vibratoRate", 4.5f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.1f}, {"attackChiff", 0.25f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Oboe",
            {
                {"breathPressure", 0.45f}, {"embouchure", 0.5f}, {"reedHardness", 0.6f},
                {"boreCharacter", 0.8f}, {"instrumentPreset", normalize("instrumentPreset", 6)},
                {"reedOpening", 0.3f}, {"bellSize", 0.3f}, {"airNoise", 0.08f},
                {"doubleReed", 0.4f}, {"boreDiameter", 0.35f},
                {"reedMass", 0.2f}, {"reedDamping", 0.6f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 3000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.45f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.1f}, {"vibratoRate", normalize("vibratoRate", 5.5f)},
                {"vibratoSource", normalize("vibratoSource", 1)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.3f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "English Horn",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.45f}, {"reedHardness", 0.5f},
                {"boreCharacter", 0.75f}, {"instrumentPreset", normalize("instrumentPreset", 7)},
                {"reedOpening", 0.35f}, {"bellSize", 0.25f}, {"airNoise", 0.06f},
                {"doubleReed", 0.35f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.25f}, {"reedDamping", 0.55f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2200.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.55f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.08f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 1)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Bassoon",
            {
                {"breathPressure", 0.55f}, {"embouchure", 0.4f}, {"reedHardness", 0.45f},
                {"boreCharacter", 0.7f}, {"instrumentPreset", normalize("instrumentPreset", 8)},
                {"reedOpening", 0.4f}, {"bellSize", 0.4f}, {"airNoise", 0.1f},
                {"doubleReed", 0.3f}, {"boreDiameter", 0.5f},
                {"reedMass", 0.35f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 1800.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.65f},
                {"boreProfile", normalize("boreProfile", 1)},
                {"vibratoDepth", 0.05f}, {"vibratoRate", normalize("vibratoRate", 4.5f)},
                {"vibratoSource", normalize("vibratoSource", 1)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.25f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },

        // =====================================================================
        // NON-WESTERN REED INSTRUMENTS (9)
        // =====================================================================
        {
            "Duduk",
            {
                {"breathPressure", 0.45f}, {"embouchure", 0.35f}, {"reedHardness", 0.35f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 9)},
                {"reedOpening", 0.5f}, {"bellSize", 0.15f}, {"airNoise", 0.05f},
                {"doubleReed", 0.25f}, {"boreDiameter", 0.55f},
                {"reedMass", 0.4f}, {"reedDamping", 0.4f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 1500.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.15f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 1)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.1f}, {"attackChiff", 0.15f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Shehnai",
            {
                {"breathPressure", 0.65f}, {"embouchure", 0.55f}, {"reedHardness", 0.6f},
                {"boreCharacter", 0.85f}, {"instrumentPreset", normalize("instrumentPreset", 10)},
                {"reedOpening", 0.35f}, {"bellSize", 0.7f}, {"airNoise", 0.15f},
                {"doubleReed", 0.7f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.2f}, {"reedDamping", 0.6f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 4000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.4f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.2f}, {"vibratoRate", normalize("vibratoRate", 6.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.4f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", -3.0f)}
            },
            juce::var()
        },
        {
            "Suona",
            {
                {"breathPressure", 0.7f}, {"embouchure", 0.6f}, {"reedHardness", 0.65f},
                {"boreCharacter", 0.9f}, {"instrumentPreset", normalize("instrumentPreset", 11)},
                {"reedOpening", 0.3f}, {"bellSize", 0.8f}, {"airNoise", 0.18f},
                {"doubleReed", 0.5f}, {"boreDiameter", 0.45f},
                {"reedMass", 0.2f}, {"reedDamping", 0.65f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 5000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.35f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.25f}, {"vibratoRate", normalize("vibratoRate", 6.5f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.5f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", -6.0f)}
            },
            juce::var()
        },
        {
            "Hichiriki",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.5f}, {"reedHardness", 0.55f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 12)},
                {"reedOpening", 0.4f}, {"bellSize", 0.2f}, {"airNoise", 0.1f},
                {"doubleReed", 0.45f}, {"boreDiameter", 0.45f},
                {"reedMass", 0.25f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2500.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.4f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.3f}, {"vibratoRate", normalize("vibratoRate", 5.5f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.3f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.4f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Zurna",
            {
                {"breathPressure", 0.7f}, {"embouchure", 0.55f}, {"reedHardness", 0.6f},
                {"boreCharacter", 0.85f}, {"instrumentPreset", normalize("instrumentPreset", 13)},
                {"reedOpening", 0.3f}, {"bellSize", 0.75f}, {"airNoise", 0.2f},
                {"doubleReed", 0.6f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.2f}, {"reedDamping", 0.65f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 4500.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.35f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.2f}, {"vibratoRate", normalize("vibratoRate", 6.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.05f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.45f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", -3.0f)}
            },
            juce::var()
        },
        {
            "Piri",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.45f}, {"reedHardness", 0.5f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 14)},
                {"reedOpening", 0.4f}, {"bellSize", 0.1f}, {"airNoise", 0.1f},
                {"doubleReed", 0.35f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.45f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.25f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.25f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Arghul",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.4f}, {"reedHardness", 0.45f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 15)},
                {"reedOpening", 0.45f}, {"bellSize", 0.2f}, {"airNoise", 0.1f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.45f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 1800.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.05f}, {"vibratoRate", normalize("vibratoRate", 4.5f)},
                {"vibratoSource", normalize("vibratoSource", 1)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 1.0f},
                {"dronePitch", normalize("dronePitch", -1200.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Launeddas",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.4f}, {"reedHardness", 0.45f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 16)},
                {"reedOpening", 0.4f}, {"bellSize", 0.15f}, {"airNoise", 0.08f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 1.0f},
                {"dronePitch", normalize("dronePitch", -50.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Mijwiz",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.4f}, {"reedHardness", 0.5f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 17)},
                {"reedOpening", 0.4f}, {"bellSize", 0.15f}, {"airNoise", 0.1f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.4f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 2000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.45f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 1.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },

        // =====================================================================
        // SOUND DESIGN (6)
        // =====================================================================
        {
            "Glass Reed",
            {
                {"breathPressure", 0.4f}, {"embouchure", 0.5f}, {"reedHardness", 0.9f},
                {"boreCharacter", 0.3f}, {"instrumentPreset", normalize("instrumentPreset", 18)},
                {"reedOpening", 0.25f}, {"bellSize", 0.4f}, {"airNoise", 0.05f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.35f},
                {"reedMass", 0.15f}, {"reedDamping", 0.2f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 5000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.4f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.1f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 1)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Metal Wind",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.45f}, {"reedHardness", 0.7f},
                {"boreCharacter", 0.5f}, {"instrumentPreset", normalize("instrumentPreset", 19)},
                {"reedOpening", 0.35f}, {"bellSize", 0.6f}, {"airNoise", 0.1f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.5f},
                {"reedMass", 0.2f}, {"reedDamping", 0.3f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 4000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.8f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.1f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 1)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", -3.0f)}
            },
            juce::var()
        },
        {
            "Impossible Bore",
            {
                {"breathPressure", 0.5f}, {"embouchure", 0.4f}, {"reedHardness", 0.5f},
                {"boreCharacter", 0.6f}, {"instrumentPreset", normalize("instrumentPreset", 20)},
                {"reedOpening", 0.4f}, {"bellSize", 0.5f}, {"airNoise", 0.12f},
                {"doubleReed", 0.2f}, {"boreDiameter", 0.5f},
                {"reedMass", 0.3f}, {"reedDamping", 0.5f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 3000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.5f},
                {"boreProfile", normalize("boreProfile", 1)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 5.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.3f},
                {"infiniteSustain", 0.3f}, {"reverseBore", 0.6f}, {"dualBore", 1.0f},
                {"dronePitch", normalize("dronePitch", 700.0f)}, {"feedbackPath", 0.3f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", -3.0f)}
            },
            juce::var()
        },
        {
            "Breath Drone",
            {
                {"breathPressure", 0.6f}, {"embouchure", 0.3f}, {"reedHardness", 0.3f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 0)},
                {"reedOpening", 0.6f}, {"bellSize", 0.2f}, {"airNoise", 0.6f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.6f},
                {"reedMass", 0.5f}, {"reedDamping", 0.3f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 800.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.6f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 3.0f)},
                {"vibratoSource", normalize("vibratoSource", 1)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.3f}, {"attackChiff", 0.1f},
                {"infiniteSustain", 0.5f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 4)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        },
        {
            "Giant Clarinet",
            {
                {"breathPressure", 0.6f}, {"embouchure", 0.35f}, {"reedHardness", 0.4f},
                {"boreCharacter", 0.0f}, {"instrumentPreset", normalize("instrumentPreset", 0)},
                {"reedOpening", 0.5f}, {"bellSize", 0.6f}, {"airNoise", 0.1f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.8f},
                {"reedMass", 0.5f}, {"reedDamping", 0.4f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 600.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.9f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 4.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.1f}, {"attackChiff", 0.2f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 0)},
                {"maxVoices", normalize("maxVoices", 4)},
                {"oversampling", normalize("oversampling", 0)},
                {"outputGain", normalize("outputGain", 3.0f)}
            },
            juce::var()
        },
        {
            "Micro Reed",
            {
                {"breathPressure", 0.35f}, {"embouchure", 0.6f}, {"reedHardness", 0.7f},
                {"boreCharacter", 0.3f}, {"instrumentPreset", normalize("instrumentPreset", 0)},
                {"reedOpening", 0.2f}, {"bellSize", 0.15f}, {"airNoise", 0.12f},
                {"doubleReed", 0.0f}, {"boreDiameter", 0.2f},
                {"reedMass", 0.1f}, {"reedDamping", 0.7f}, {"mouthpieceVol", 0.0f},
                {"toneHoleCutoff", normalize("toneHoleCutoff", 6000.0f)},
                {"registerHole", 0.0f}, {"boreLength", 0.15f},
                {"boreProfile", normalize("boreProfile", 0)},
                {"vibratoDepth", 0.0f}, {"vibratoRate", normalize("vibratoRate", 7.0f)},
                {"vibratoSource", normalize("vibratoSource", 0)},
                {"growlAmount", 0.0f}, {"flutterTongue", 0.0f}, {"subtone", 0.0f}, {"attackChiff", 0.4f},
                {"infiniteSustain", 0.0f}, {"reverseBore", 0.0f}, {"dualBore", 0.0f},
                {"dronePitch", normalize("dronePitch", 0.0f)}, {"feedbackPath", 0.0f},
                {"referencePitch", normalize("referencePitch", 440.0f)},
                {"tuningSystem", normalize("tuningSystem", 2)},
                {"polyMode", normalize("polyMode", 1)},
                {"maxVoices", normalize("maxVoices", 8)},
                {"oversampling", normalize("oversampling", 1)},
                {"outputGain", normalize("outputGain", 0.0f)}
            },
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(presets);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OReedAudioProcessor();
}
