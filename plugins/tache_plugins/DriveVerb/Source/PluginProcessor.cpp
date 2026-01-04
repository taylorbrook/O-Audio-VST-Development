#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DriveVerbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // SIZE - Room dimensions control (0-100%, default 40%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "size", 1 },
        "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        40.0f,
        "%"
    ));

    // DECAY - Reverb tail length (0.5-10s, default 2s, logarithmic skew 0.3)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "decay", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.01f, 0.3f),
        2.0f,
        "s"
    ));

    // DRY/WET - Mix control (0-100%, default 30%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dryWet", 1 },
        "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"
    ));

    // DRIVE - Tape saturation amount (0-24dB, default 6dB, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f, 1.0f),
        6.0f,
        "dB"
    ));

    // FILTER - DJ-style filter (-100 to +100%, default 0%, linear)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "filter", 1 },
        "Filter",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"
    ));

    // FILTER POSITION - Pre/Post toggle (0.0=PRE, 1.0=POST, default 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "filterPosition", 1 },
        "Filter Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
        1.0f
    ));

    return layout;
}

DriveVerbAudioProcessor::DriveVerbAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

DriveVerbAudioProcessor::~DriveVerbAudioProcessor()
{
}

void DriveVerbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec for all components
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare reverb
    reverb.prepare(spec);

    // Prepare dry/wet mixer
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced); // Equal-power mixing

    // Prepare drive waveshaper with tanh transfer function (Stage 4.2)
    driveShaper.prepare(spec);
    driveShaper.functionToUse = [](float sample) { return std::tanh(sample); };

    // Prepare DJ-style filter (Stage 4.3)
    filterProcessor.prepare(spec);
}

void DriveVerbAudioProcessor::releaseResources()
{
    reverb.reset();
    dryWetMixer.reset();
    driveShaper.reset();
    filterProcessor.reset();
}

void DriveVerbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Get current parameter values (atomic reads, real-time safe)
    auto* sizeParam = parameters.getRawParameterValue("size");
    auto* decayParam = parameters.getRawParameterValue("decay");
    auto* dryWetParam = parameters.getRawParameterValue("dryWet");
    auto* driveParam = parameters.getRawParameterValue("drive");
    auto* filterParam = parameters.getRawParameterValue("filter");
    auto* filterPositionParam = parameters.getRawParameterValue("filterPosition");

    float sizeValue = sizeParam->load();      // 0-100%
    float decayValue = decayParam->load();    // 0.5-10s
    float dryWetValue = dryWetParam->load();  // 0-100%
    float driveValue = driveParam->load();    // 0-24dB
    float filterValue = filterParam->load();  // -100% to +100%
    bool isPostMode = filterPositionParam->load() > 0.5f;  // false=PRE, true=POST

    // Update reverb parameters
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = sizeValue / 100.0f;  // Normalize to 0-1
    reverbParams.damping = 0.5f;                  // Fixed for now (could add parameter later)
    reverbParams.wetLevel = 1.0f;                 // Full wet (dry/wet mixer handles blend)
    reverbParams.dryLevel = 0.0f;                 // No dry in reverb (dry/wet mixer handles it)
    reverbParams.width = 1.0f;                    // Full stereo width
    reverbParams.freezeMode = 0.0f;               // No freeze

    // Map decay time to reverb parameters (decay affects both room size and damping)
    // Longer decay = larger room + less damping
    reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, reverbParams.roomSize + (decayValue / 20.0f));
    reverbParams.damping = juce::jlimit(0.0f, 1.0f, 1.0f - (decayValue / 10.0f));

    reverb.setParameters(reverbParams);

    // Update dry/wet mix (normalize 0-100% to 0-1)
    dryWetMixer.setWetMixProportion(dryWetValue / 100.0f);

    // Create audio block for DSP processing
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Push dry signal into mixer
    dryWetMixer.pushDrySamples(block);

    // Process reverb
    reverb.process(context);

    // Stage 4.4: PRE/POST routing - apply drive and filter in different orders
    // PRE mode (filterPosition=0.0): Filter → Drive
    // POST mode (filterPosition=1.0): Drive → Filter

    if (isPostMode)
    {
        // POST MODE: Drive → Filter (drive affects harmonics, then filter shapes them)
        applyDrive(block, context, driveValue);
        applyFilter(block, context, filterValue);
    }
    else
    {
        // PRE MODE: Filter → Drive (filter shapes frequency content, then drive adds harmonics)
        applyFilter(block, context, filterValue);
        applyDrive(block, context, driveValue);
    }

    // Mix dry and wet signals
    dryWetMixer.mixWetSamples(block);
}

void DriveVerbAudioProcessor::applyDrive(juce::dsp::AudioBlock<float>& block, juce::dsp::ProcessContextReplacing<float>& context, float driveValue)
{
    // Apply drive to wet signal (Stage 4.2)
    // Convert dB to linear gain: gain = 10^(dB/20)
    float driveGain = std::pow(10.0f, driveValue / 20.0f);

    // Apply gain before waveshaping (increases saturation with higher drive)
    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* channelData = block.getChannelPointer(channel);
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            channelData[sample] *= driveGain;
        }
    }

    // Apply tanh waveshaping (tape-like saturation)
    driveShaper.process(context);

    // Measure output level for VU meter (after waveshaping)
    float maxLevel = 0.0f;
    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
    {
        const float* channelData = block.getChannelPointer(channel);
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            maxLevel = std::max(maxLevel, std::abs(channelData[sample]));
        }
    }

    // Convert to dB and store atomically
    float levelDB = maxLevel > 0.0f
        ? juce::Decibels::gainToDecibels(maxLevel)
        : -60.0f;
    driveOutputLevelDB.store(levelDB);
}

void DriveVerbAudioProcessor::applyFilter(juce::dsp::AudioBlock<float>& block, juce::dsp::ProcessContextReplacing<float>& context, float filterValue)
{
    // Apply DJ-style filter (Stage 4.3)
    // Center bypass zone: ±0.5% = no filtering (prevents filter artifacts at bypass)
    if (std::abs(filterValue) > 0.5f)
    {
        float sampleRate = static_cast<float>(getSampleRate());
        bool isLowPass = (filterValue < 0.0f);

        // Reset filter state when switching between low-pass and high-pass
        // Prevents burst caused by residual energy in delay buffers
        if (isLowPass != previousWasLowPass)
        {
            filterProcessor.reset();
        }
        previousWasLowPass = isLowPass;

        if (isLowPass)
        {
            // Low-pass filter (negative values)
            // Exponential mapping: -100% = 200Hz (heavy bass), 0% = 20kHz (bypass)
            float normalizedValue = std::abs(filterValue) / 100.0f; // 0.0 to 1.0
            float cutoffHz = 20000.0f * std::pow(10.0f, -normalizedValue * std::log10(20000.0f / 200.0f));

            *filterProcessor.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, juce::jlimit(200.0f, 20000.0f, cutoffHz), 0.707f
            );
        }
        else
        {
            // High-pass filter (positive values)
            // Exponential mapping: 0% = 20Hz (bypass), +100% = 10kHz (heavy treble)
            float normalizedValue = filterValue / 100.0f; // 0.0 to 1.0
            float cutoffHz = 20.0f * std::pow(10.0f, normalizedValue * std::log10(10000.0f / 20.0f));

            *filterProcessor.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, juce::jlimit(20.0f, 10000.0f, cutoffHz), 0.707f
            );
        }

        // Process buffer through filter
        filterProcessor.process(context);
    }
    else
    {
        // Reset filter state when entering bypass zone
        // Prevents residual energy when re-entering filter range
        if (previousWasLowPass)
        {
            filterProcessor.reset();
            previousWasLowPass = false;
        }
    }
}

juce::AudioProcessorEditor* DriveVerbAudioProcessor::createEditor()
{
    return new DriveVerbAudioProcessorEditor(*this);
}

void DriveVerbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DriveVerbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriveVerbAudioProcessor();
}
