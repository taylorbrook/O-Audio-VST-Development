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

    // ========== String Configuration (2) ==========

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

    // BOW_HAIR_STIFFNESS - Core↔bristle friction blend (0 = pure Core / shipped timbre,
    // 1 = full elasto-plastic bristle). Default 0.0 keeps new instances and factory
    // presets sounding like the pre-v1.4.0 plugin now that WR-02 made this knob audible.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowHairStiffness", 1 },
        "Bow Hair Stiffness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
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

    // ========== Humanize (8) ==========
    // Four bow parameters each get a (range, rate) pair. Range 0 = off.
    // Rate maps internally to 0.15 - 8 Hz drift. All default off to preserve
    // existing preset behaviour.
    auto addHumanizePair = [&layout] (const juce::String& id,
                                      const juce::String& label,
                                      float defaultRate)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "Range", 1 },
            label + " Humanize",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
            0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "Rate", 1 },
            label + " Humanize Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
            defaultRate));
    };

    addHumanizePair ("humanizeSpeed",    "Speed",    0.25f); // slow-ish drift
    addHumanizePair ("humanizePressure", "Pressure", 0.30f);
    addHumanizePair ("humanizePosition", "Position", 0.20f);
    addHumanizePair ("humanizeRosin",    "Rosin",    0.35f);

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
        voice->setHumanizeEngine (&humanizeEngine);
        voice->setPendingTuningSource (&vst3Extensions.getPendingTable()); // Phase 24: NE
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

    // IN-09: clear master-path state on (re-)prepare so a sample-rate change doesn't
    // leak a startup transient. dcBlock persisted across prepares (releaseResources is
    // a no-op); the body/sympathetic engines also reset inside their prepare() below.
    dcBlockX[0] = dcBlockX[1] = 0.0f;
    dcBlockY[0] = dcBlockY[1] = 0.0f;
    bodyResonator.reset();
    sympatheticEngine.reset();

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
    sympatheticEngine.prepare (sampleRate, samplesPerBlock);
    humanizeEngine.prepare (sampleRate, samplesPerBlock);

    // WR-06: resolve APVTS atomic pointers once so processBlock reads are lock-free
    // and allocation-free (no per-callback string-keyed map lookups).
    pSympatheticAmount = parameters.getRawParameterValue ("sympatheticAmount");
    pSympatheticCount  = parameters.getRawParameterValue ("sympatheticCount");
    pBodyMaterial      = parameters.getRawParameterValue ("bodyMaterial");
    pBodySize          = parameters.getRawParameterValue ("bodySize");
    pWidth             = parameters.getRawParameterValue ("width");
    pReferencePitch    = parameters.getRawParameterValue ("referencePitch");
    pSympatheticDecay  = parameters.getRawParameterValue ("sympatheticDecay");
    pBodyAmount        = parameters.getRawParameterValue ("bodyAmount");
    pTuningSystem      = parameters.getRawParameterValue ("tuningSystem");
    pOutputLevel       = parameters.getRawParameterValue ("outputLevel");
    pHumanizeRange[0]  = parameters.getRawParameterValue ("humanizeSpeedRange");
    pHumanizeRange[1]  = parameters.getRawParameterValue ("humanizePressureRange");
    pHumanizeRange[2]  = parameters.getRawParameterValue ("humanizePositionRange");
    pHumanizeRange[3]  = parameters.getRawParameterValue ("humanizeRosinRange");
    pHumanizeRate[0]   = parameters.getRawParameterValue ("humanizeSpeedRate");
    pHumanizeRate[1]   = parameters.getRawParameterValue ("humanizePressureRate");
    pHumanizeRate[2]   = parameters.getRawParameterValue ("humanizePositionRate");
    pHumanizeRate[3]   = parameters.getRawParameterValue ("humanizeRosinRate");
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

    // VST3 Note Expression: drain the JUCE wrapper's raw-event queue and
    // correlate tuning deltas to their NoteOn's MIDI pitch (Phase 24).
    vst3Extensions.drainAndUpdate();

    // === 1. Read processor-level params via cached atomic pointers (WR-06) ===
    // The per-voice params (bowSpeed / bowPressure / bowPosition / rosin / brightness /
    // infiniteSustain / stringGauge / bowHairStiffness) are read inside the voice's
    // updateParametersFromAPVTS — the identical reads that used to sit here were dead
    // (declared, never used) and are removed. WR-06.
    float sympatheticAmt  = pSympatheticAmount->load();
    int sympatheticCount  = static_cast<int> (pSympatheticCount->load());
    float material        = pBodyMaterial->load();
    float bodySize        = pBodySize->load();
    float width           = pWidth->load();
    float refPitch        = pReferencePitch->load();
    float sympDecay       = pSympatheticDecay->load();
    float bodyAmount      = pBodyAmount->load();

    // === 1a. Advance humanize random walk (shared across all voices) ===
    const std::array<float, 4> humanRanges {
        pHumanizeRange[0]->load(), pHumanizeRange[1]->load(),
        pHumanizeRange[2]->load(), pHumanizeRange[3]->load()
    };
    const std::array<float, 4> humanRates {
        pHumanizeRate[0]->load(), pHumanizeRate[1]->load(),
        pHumanizeRate[2]->load(), pHumanizeRate[3]->load()
    };
    humanizeEngine.update (humanRanges, humanRates, buffer.getNumSamples());

    // === 1b. Wire tuning engine ===
    tuningEngine.setMasterTune (static_cast<double> (refPitch));
    int tuningSystemIdx = static_cast<int> (pTuningSystem->load());
    switch (tuningSystemIdx)
    {
        case 0:  tuningEngine.setMode (TuningEngine::Mode::Scala); break;
        case 1:  tuningEngine.setMode (TuningEngine::Mode::MTSESP); break;
        default: tuningEngine.setMode (TuningEngine::Mode::TwelveTET); break;
    }

    // === 2. Set voice panning ===
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<BowedStringVoice*> (synthesiser.getVoice (i)))
            voice->setPan (0.707f, 0.707f);
    }

    // === 4. Render polyphonic voices ===
    synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // === 4. Update body resonator ===
    bodyResonator.setMaterial (material);
    bodyResonator.setSize (bodySize);
    bodyResonator.setBodyAmount (bodyAmount);

    // === 5. Update sympathetic engine ===
    sympatheticEngine.setCount (sympatheticCount);
    sympatheticEngine.setAmount (sympatheticAmt);
    sympatheticEngine.setDecay (sympDecay);

    // Collect fundamentals from active voices for sympathetic tuning
    float fundamentals[12];
    int numFundamentals = 0;
    for (int i = 0; i < synthesiser.getNumVoices() && numFundamentals < 12; ++i)
    {
        if (auto* voice = dynamic_cast<BowedStringVoice*> (synthesiser.getVoice (i)))
        {
            if (voice->isActive())
                fundamentals[numFundamentals++] = voice->getCurrentFrequency();
        }
    }
    sympatheticEngine.updateTunings (fundamentals, numFundamentals);

    // === 6. Per-sample processing: body stereo + sympathetics ===
    auto* leftData  = buffer.getWritePointer (0);
    auto* rightData = buffer.getWritePointer (1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
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

    // === 7. Stereo width ===
    stereoWidthProcessor.processBlock (buffer, width);

    // === 8. Master output gain ===
    float outputLevel = pOutputLevel->load();
    buffer.applyGain (juce::Decibels::decibelsToGain (outputLevel));

    // === 9. Safety limiter ===
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch),
                                           buffer.getReadPointer (ch),
                                           -2.0f, 2.0f, buffer.getNumSamples());
}

bool OBowedAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo-only output (matches BusesProperties); no input bus for synth.
    // Required by AU validation (auval mono Render Test segfaulted without this
    // explicit gate — same pattern as O-Reed::isBusesLayoutSupported, Rule-3
    // inline fix during Phase 24 plan 24-06).
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    return true;
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
    // sympatheticCount: v/12.0  |  outputLevel: (v+60)/72  |  referencePitch: (v-220)/660
    // tuningSystem: index/2.0
    // Linear 0-1 params: rosin, bowNoise, bodyMaterial, bodySize, width(/2), sympatheticAmount,
    //                     infiniteSustain, reversedFriction, subHarmonics

    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        // ===== Realistic Instruments (7) =====
        {
            "Violin",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.357f},
             {"rosin", 0.5f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.3f}, {"brightness", 0.876f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Cello",
            {{"bowSpeed", 0.27832f}, {"bowPressure", 0.34996f}, {"bowPosition", 0.393f},
             {"rosin", 0.55f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.7f}, {"brightness", 0.8f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Viola",
            {{"bowSpeed", 0.28925f}, {"bowPressure", 0.33098f}, {"bowPosition", 0.375f},
             {"rosin", 0.5f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.45f}, {"brightness", 0.84f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Double Bass",
            {{"bowSpeed", 0.26591f}, {"bowPressure", 0.37606f}, {"bowPosition", 0.429f},
             {"rosin", 0.55f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.9f}, {"brightness", 0.7f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Erhu",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.357f},
             {"rosin", 0.65f}, {"bowNoise", 0.05f},
             {"bodyMaterial", 0.15f}, {"bodySize", 0.3f}, {"brightness", 0.876f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Sarangi",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.33098f}, {"bowPosition", 0.357f},
             {"rosin", 0.6f}, {"bowNoise", 0.1f},
             {"bodyMaterial", 0.15f}, {"bodySize", 0.45f}, {"brightness", 0.84f},
{"sympatheticAmount", 0.4f}, {"sympatheticCount", 0.417f},
             {"width", 0.55f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Nyckelharpa",
            {{"bowSpeed", 0.30151f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.357f},
             {"rosin", 0.5f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.4f}, {"brightness", 0.876f},
{"sympatheticAmount", 0.5f}, {"sympatheticCount", 0.833f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.0f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        // ===== Sound Design (3) =====
        {
            "Glass Bow",
            {{"bowSpeed", 0.27832f}, {"bowPressure", 0.27832f}, {"bowPosition", 0.321f},
             {"rosin", 0.35f}, {"bowNoise", 0.0f},
             {"bodyMaterial", 0.9f}, {"bodySize", 0.3f}, {"brightness", 0.95f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.45f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Impossible Strings",
            {{"bowSpeed", 0.34996f}, {"bowPressure", 0.31336f}, {"bowPosition", 0.286f},
             {"rosin", 0.6f}, {"bowNoise", 0.2f},
             {"bodyMaterial", 0.5f}, {"bodySize", 0.5f}, {"brightness", 0.876f},
             {"sympatheticAmount", 0.3f}, {"sympatheticCount", 0.5f},
             {"width", 0.7f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.4f}, {"reversedFriction", 0.3f}, {"subHarmonics", 0.35f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
            juce::var()
        },
        {
            "Breath of Strings",
            {{"bowSpeed", 0.25149f}, {"bowPressure", 0.23403f}, {"bowPosition", 0.393f},
             {"rosin", 0.3f}, {"bowNoise", 0.7f},
             {"bodyMaterial", 0.4f}, {"bodySize", 0.5f}, {"brightness", 0.876f},
{"sympatheticAmount", 0.0f}, {"sympatheticCount", 0.0f},
             {"width", 0.5f}, {"outputLevel", 0.833f},
             {"infiniteSustain", 0.15f}, {"reversedFriction", 0.0f}, {"subHarmonics", 0.0f},
             {"referencePitch", 0.333f}, {"tuningSystem", 1.0f}},
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
