/*
  ==============================================================================

    O-Bowed - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout (MUST be defined before constructor)
juce::AudioProcessorValueTreeState::ParameterLayout OBowedAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ========== Bow Controls (4) ==========

    // BOW_SPEED - Velocity of bow across string
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowSpeed", 1 },
        "Bow Speed",
        juce::NormalisableRange<float>(0.02f, 2.0f, 0.01f, 0.5f),
        0.2f,
        "m/s"
    ));

    // BOW_PRESSURE - Normal force of bow on string
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowPressure", 1 },
        "Bow Pressure",
        juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.5f),
        0.5f,
        "N"
    ));

    // BOW_POSITION - Contact point (sul ponticello to sul tasto)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowPosition", 1 },
        "Bow Position",
        juce::NormalisableRange<float>(0.02f, 0.30f, 0.01f),
        0.12f
    ));

    // ROSIN - Friction curve shape (smooth to aggressive)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "rosin", 1 },
        "Rosin",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // BOW_NOISE - Bow friction noise amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowNoise", 1 },
        "Bow Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // ========== Body Controls (3) ==========

    // BODY_MATERIAL - Body morph: membrane <-> wood <-> metal <-> glass
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyMaterial", 1 },
        "Material",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.4f
    ));

    // BODY_SIZE - Body resonant frequency scaling (violin <-> bass)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodySize", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // BRIGHTNESS - Bridge filter cutoff
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "brightness", 1 },
        "Brightness",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
        8000.0f,
        "Hz"
    ));

    // ========== String Configuration (7) ==========

    // STRING_COUNT - Number of active bowed strings
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "stringCount", 1 },
        "String Count",
        1,
        4,
        1
    ));

    // STRING_TUNING_1 - Pitch offset for string 1
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTuning1", 1 },
        "String 1 Tuning",
        juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
        0.0f,
        "cents"
    ));

    // STRING_TUNING_2 - Pitch offset for string 2
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTuning2", 1 },
        "String 2 Tuning",
        juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
        0.0f,
        "cents"
    ));

    // STRING_TUNING_3 - Pitch offset for string 3
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTuning3", 1 },
        "String 3 Tuning",
        juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
        0.0f,
        "cents"
    ));

    // STRING_TUNING_4 - Pitch offset for string 4
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringTuning4", 1 },
        "String 4 Tuning",
        juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
        0.0f,
        "cents"
    ));

    // SYMPATHETIC_AMOUNT - Coupling to passive sympathetic strings
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticAmount", 1 },
        "Sympathetic Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // SYMPATHETIC_COUNT - Number of passive waveguide strings
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "sympatheticCount", 1 },
        "Sympathetic Strings",
        0,
        12,
        0
    ));

    // ========== Advanced Physics (4) ==========

    // SYMPATHETIC_DECAY - How long sympathetic strings ring (loss coefficient)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sympatheticDecay", 1 },
        "Sympathetic Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // BODY_AMOUNT - Dry/wet blend of body resonator
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bodyAmount", 1 },
        "Body Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.6f
    ));

    // STRING_GAUGE - String wave impedance (thin/bright to thick/dark)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "stringGauge", 1 },
        "String Gauge",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f, 0.5f),
        0.5f
    ));

    // BOW_HAIR_STIFFNESS - Bristle stiffness for Enhanced/Quality friction tiers
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowHairStiffness", 1 },
        "Bow Hair Stiffness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    // ========== Output (2) ==========

    // WIDTH - Stereo spread of multi-string output
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "width", 1 },
        "Stereo Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f
    ));

    // OUTPUT_LEVEL - Master output gain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputLevel", 1 },
        "Output Level",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f),
        0.0f,
        "dB"
    ));

    // ========== Impossible Physics (3) ==========

    // INFINITE_SUSTAIN - Reduces damping toward zero
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "infiniteSustain", 1 },
        "Infinite Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // REVERSED_FRICTION - Inverts friction curve
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "reversedFriction", 1 },
        "Reversed Friction",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // SUB_HARMONICS - Sub-octave content via nonlinear feedback
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "subHarmonics", 1 },
        "Sub-Harmonics",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));

    // FRICTION_TIER - Friction model complexity selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "frictionTier", 1 },
        "Friction Tier",
        juce::StringArray { "Core", "Enhanced", "Quality" },
        0  // Default: Core
    ));

    // ========== Tuning (2) ==========

    // REFERENCE_PITCH - A4 reference frequency
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "referencePitch", 1 },
        "Reference Pitch",
        juce::NormalisableRange<float>(220.0f, 880.0f, 0.1f),
        440.0f,
        "Hz"
    ));

    // TUNING_SYSTEM - Tuning mode selection
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "tuningSystem", 1 },
        "Tuning System",
        juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" },
        2  // Default: 12-TET (index 2)
    ));

    return layout;
}

//==============================================================================
OBowedAudioProcessor::OBowedAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
    , presetManager(parameters, "O-Bowed")
{
    // Add 8 polyphonic voices with tuning engine + voice index
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new BowedStringVoice (&parameters);
        voice->setVoiceIndex (i);
        voice->setTuningEngine (&tuningEngine);
        synthesiser.addVoice (voice);
    }

    // Enable legacy mode for non-MPE controllers (standard keyboards, +/- 2 semitone bend)
    synthesiser.enableLegacyMode (2);

    initializeFactoryPresets();
}

OBowedAudioProcessor::~OBowedAudioProcessor()
{
}

void OBowedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BowedStringVoice*> (synthesiser.getVoice (i)))
            voice->prepareToPlay (sampleRate, samplesPerBlock);
    }

    // Report oversampling latency to host (all voices share identical config)
    if (auto* voice = dynamic_cast<BowedStringVoice*> (synthesiser.getVoice (0)))
        setLatencySamples (static_cast<int> (std::ceil (voice->getOversamplingLatency())));

    bodyResonator.prepare (sampleRate, samplesPerBlock);
    stereoWidthProcessor.prepare (sampleRate, samplesPerBlock);
    droneEngine.prepare (sampleRate, samplesPerBlock);
    sympatheticEngine.prepare (sampleRate, samplesPerBlock);
}

void OBowedAudioProcessor::releaseResources()
{
    // Cleanup will be added in Stage 2
}

void OBowedAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear all output channels
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // === 1. Read all params from APVTS ===
    int stringCount       = static_cast<int> (parameters.getRawParameterValue ("stringCount")->load());
    float tuning1         = parameters.getRawParameterValue ("stringTuning1")->load();
    float tuning2         = parameters.getRawParameterValue ("stringTuning2")->load();
    float tuning3         = parameters.getRawParameterValue ("stringTuning3")->load();
    float tuning4         = parameters.getRawParameterValue ("stringTuning4")->load();
    float sympatheticAmt  = parameters.getRawParameterValue ("sympatheticAmount")->load();
    int sympatheticCount  = static_cast<int> (parameters.getRawParameterValue ("sympatheticCount")->load());
    float material        = parameters.getRawParameterValue ("bodyMaterial")->load();
    float bodySize        = parameters.getRawParameterValue ("bodySize")->load();
    float width           = parameters.getRawParameterValue ("width")->load();
    float bowSpeed        = parameters.getRawParameterValue ("bowSpeed")->load();
    float bowPressure     = parameters.getRawParameterValue ("bowPressure")->load();
    float bowPos          = parameters.getRawParameterValue ("bowPosition")->load();
    float rosin           = parameters.getRawParameterValue ("rosin")->load();
    float brightness      = parameters.getRawParameterValue ("brightness")->load();
    float infSustain      = parameters.getRawParameterValue ("infiniteSustain")->load();
    float refPitch        = parameters.getRawParameterValue ("referencePitch")->load();
    float sympDecay       = parameters.getRawParameterValue ("sympatheticDecay")->load();
    float bodyAmount      = parameters.getRawParameterValue ("bodyAmount")->load();
    float stringGauge     = parameters.getRawParameterValue ("stringGauge")->load();
    float bowHairStiff    = parameters.getRawParameterValue ("bowHairStiffness")->load();

    // === 1b. Wire tuning engine ===
    tuningEngine.setMasterTune (static_cast<double> (refPitch));
    int tuningSystemIdx = static_cast<int> (parameters.getRawParameterValue ("tuningSystem")->load());
    switch (tuningSystemIdx)
    {
        case 0:  tuningEngine.setMode (TuningEngine::Mode::Scala); break;
        case 1:  tuningEngine.setMode (TuningEngine::Mode::MTSESP); break;
        default: tuningEngine.setMode (TuningEngine::Mode::TwelveTET); break;
    }

    // === 2. Dynamic voice cap ===
    int maxPolyphony = 8;
    switch (stringCount) {
        case 1: maxPolyphony = 8; break;
        case 2: maxPolyphony = 6; break;
        case 3: maxPolyphony = 5; break;
        case 4: maxPolyphony = 4; break;
    }

    int activeVoices = 0;
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
        if (synthesiser.getVoice (i)->isActive())
            activeVoices++;

    if (activeVoices > maxPolyphony)
    {
        for (int i = 0; i < synthesiser.getNumVoices() && activeVoices > maxPolyphony; ++i)
        {
            if (synthesiser.getVoice (i)->isActive())
            {
                synthesiser.getVoice (i)->noteStopped (false);
                --activeVoices;
            }
        }
    }

    // === 3. Set voice panning (center — drones handle stereo spread) ===
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BowedStringVoice*> (synthesiser.getVoice (i)))
            voice->setPan (0.707f, 0.707f);
    }

    // === 4. Render polyphonic voices ===
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // === 5. Update drone engine parameters ===
    droneEngine.setReferencePitch (refPitch);
    droneEngine.setTuning (0, tuning1);
    droneEngine.setTuning (1, tuning2);
    droneEngine.setTuning (2, tuning3);
    droneEngine.setTuning (3, tuning4);
    droneEngine.setBowSpeed (bowSpeed);
    droneEngine.setBowPressure (bowPressure);
    droneEngine.setBowPosition (bowPos);
    droneEngine.setRosin (rosin);
    droneEngine.setBrightness (brightness);
    droneEngine.setInfiniteSustain (infSustain);
    droneEngine.setStringCount (stringCount);

    // === 6. Update body resonator ===
    bodyResonator.setMaterial (material);
    bodyResonator.setSize (bodySize);
    bodyResonator.setBodyAmount (bodyAmount);

    // === 7. Update sympathetic engine ===
    sympatheticEngine.setCount (sympatheticCount);
    sympatheticEngine.setAmount (sympatheticAmt);
    sympatheticEngine.setDecay (sympDecay);

    // === 7b. Wire string gauge to drone friction models ===
    droneEngine.setStringGauge (stringGauge);

    // Collect fundamentals from active voices for sympathetic tuning
    float fundamentals[16];
    int numFundamentals = 0;
    for (int i = 0; i < synthesiser.getNumVoices() && numFundamentals < 12; ++i)
    {
        if (auto* voice = dynamic_cast<BowedStringVoice*> (synthesiser.getVoice (i)))
        {
            if (voice->isActive())
                fundamentals[numFundamentals++] = voice->getCurrentFrequency();
        }
    }
    // Add drone fundamentals
    float tunings[4] = { tuning1, tuning2, tuning3, tuning4 };
    for (int i = 0; i < stringCount && numFundamentals < 16; ++i)
        fundamentals[numFundamentals++] = refPitch * std::pow (2.0f, tunings[i] / 1200.0f);

    sympatheticEngine.updateTunings (fundamentals, numFundamentals);

    // === 8. Per-sample processing: drones + body stereo + sympathetics ===
    auto* leftData  = buffer.getWritePointer (0);
    auto* rightData = buffer.getWritePointer (1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Add drone output to stereo buffer
        float droneL = 0.0f, droneR = 0.0f;
        droneEngine.processSample (droneL, droneR);
        leftData[i]  += droneL;
        rightData[i] += droneR;

        // Capture pre-body bridge sum for sympathetic excitation
        float preBodyMono = (leftData[i] + rightData[i]) * 0.5f;

        // Body resonator (stereo)
        bodyResonator.processStereo (leftData[i], rightData[i]);

        // Post-body mono for sympathetic excitation
        float postBodyMono = (leftData[i] + rightData[i]) * 0.5f;

        // Sympathetic excitation: 50/50 pre/post body
        float excitation = (preBodyMono + postBodyMono) * 0.5f;
        auto symp = sympatheticEngine.processSample (excitation);
        leftData[i]  += symp.left;
        rightData[i] += symp.right;

        // DC blocker: y[n] = x[n] - x[n-1] + R * y[n-1], R = 0.9995
        constexpr float R = 0.9995f;
        float xL = leftData[i];
        float xR = rightData[i];
        leftData[i]  = xL - dcBlockX[0] + R * dcBlockY[0];
        rightData[i] = xR - dcBlockX[1] + R * dcBlockY[1];
        dcBlockX[0] = xL;  dcBlockY[0] = leftData[i];
        dcBlockX[1] = xR;  dcBlockY[1] = rightData[i];
    }

    // === 9. Stereo width ===
    stereoWidthProcessor.processBlock (buffer, width);

    // === 10. Master output gain ===
    float outputLevel = parameters.getRawParameterValue ("outputLevel")->load();
    buffer.applyGain (juce::Decibels::decibelsToGain (outputLevel));

    // === 11. Safety limiter ===
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch),
                                           buffer.getReadPointer (ch),
                                           -2.0f, 2.0f, buffer.getNumSamples());
}

juce::AudioProcessorEditor* OBowedAudioProcessor::createEditor()
{
    return new OBowedAudioProcessorEditor(*this);
}

void OBowedAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = presetManager.getStateAsXml();
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void OBowedAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        presetManager.setStateFromXml(xmlState.get());
}

void OBowedAudioProcessor::initializeFactoryPresets()
{
    auto factoryDir = presetManager.getFactoryPresetsDirectory();
    if (factoryDir.isDirectory() &&
        factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
        return;

    // All values normalized 0.0-1.0
    // bowSpeed: sqrt((v-0.02)/1.98)  |  bowPressure: sqrt((v-0.01)/4.99)
    // bowPosition: (v-0.02)/0.28  |  brightness: pow((v-20)/19980, 4.0)
    // stringCount: (v-1)/3.0  |  sympatheticCount: v/12.0
    // outputLevel: (v+60)/72  |  referencePitch: (v-220)/660
    // frictionTier: index/2.0  |  tuningSystem: index/2.0
    // Linear 0-1 params: rosin, bowNoise, bodyMaterial, bodySize, width(/2), sympatheticAmount,
    //                     infiniteSustain, reversedFriction, subHarmonics
    // stringTuning: (v+2400)/4800

    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // ===== Realistic Instruments (7) =====
        {
            "Violin",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.357f},
             {"rosin", 0.5f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.3f}, {"brightness", 0.876f},
             {"stringCount", 1.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Cello",
            {{"bowSpeed", 0.27832f}, {"bowPressure", 0.34996f}, {"bowPosition", 0.393f},
             {"rosin", 0.55f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.7f}, {"brightness", 0.8f},
             {"stringCount", 1.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Viola",
            {{"bowSpeed", 0.28925f}, {"bowPressure", 0.33098f}, {"bowPosition", 0.375f},
             {"rosin", 0.5f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.45f}, {"brightness", 0.84f},
             {"stringCount", 1.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Double Bass",
            {{"bowSpeed", 0.26591f}, {"bowPressure", 0.37606f}, {"bowPosition", 0.429f},
             {"rosin", 0.55f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.9f}, {"brightness", 0.7f},
             {"stringCount", 1.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Erhu",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.357f},
             {"rosin", 0.65f}, {"bowNoise", 0.05f},
             {"bodyMaterial", 0.15f}, {"bodySize", 0.3f}, {"brightness", 0.876f},
             {"stringCount", 0.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Sarangi",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.33098f}, {"bowPosition", 0.357f},
             {"rosin", 0.6f}, {"bowNoise", 0.1f},
             {"bodyMaterial", 0.15f}, {"bodySize", 0.45f}, {"brightness", 0.84f},
             {"stringCount", 0.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.4f}, {"sympatheticCount", 0.417f},
             {"width", 0.55f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Nyckelharpa",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.357f},
             {"rosin", 0.5f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.4f}, {"brightness", 0.876f},
             {"stringCount", 1.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.5f}, {"sympatheticCount", 0.833f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        // ===== Sound Design (4) =====
        {
            "Glass Bow",
            {{"bowSpeed", 0.27832f}, {"bowPressure", 0.27832f}, {"bowPosition", 0.321f},
             {"rosin", 0.35f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.9f}, {"bodySize", 0.3f}, {"brightness", 0.95f},
             {"stringCount", 0.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.8f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Metal Drone",
            {{"bowSpeed", 0.31623f}, {"bowPressure", 0.37606f}, {"bowPosition", 0.357f},
             {"rosin", 0.6f}, {"bowNoise", 0.15f},
             {"bodyMaterial", 0.7f}, {"bodySize", 0.7f}, {"brightness", 0.8f},
             {"stringCount", 0.333f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.65f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.5f}, {"reversedFriction", 0.4f}, {"subHarmonics", 0.6f},
             {"frictionTier", 0.5f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Impossible Strings",
            {{"bowSpeed", 0.34996f}, {"bowPressure", 0.34996f}, {"bowPosition", 0.286f},
             {"rosin", 0.7f}, {"bowNoise", 0.2f},
             {"bodyMaterial", 0.5f}, {"bodySize", 0.5f}, {"brightness", 0.876f},
             {"stringCount", 1.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.3f}, {"sympatheticCount", 0.5f},
             {"width", 0.7f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.7f}, {"reversedFriction", 0.6f}, {"subHarmonics", 0.8f},
             {"frictionTier", 1.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Breath of Strings",
            {{"bowSpeed", 0.25149f}, {"bowPressure", 0.23403f}, {"bowPosition", 0.393f},
             {"rosin", 0.3f}, {"bowNoise", 0.7f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.5f}, {"brightness", 0.876f},
             {"stringCount", 0.0f},
             {"stringTuning1", 0.5f}, {"stringTuning2", 0.5f}, {"stringTuning3", 0.5f}, {"stringTuning4", 0.5f},
             {"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.3f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"frictionTier", 0.0f}, {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(factoryPresets);
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OBowedAudioProcessor();
}
