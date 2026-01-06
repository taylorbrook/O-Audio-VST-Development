#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout Creation
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OuariconTremoloAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // SPEED_PARAM - Tremolo Speed (0.1 - 20.0 Hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SPEED_PARAM", 1 },
        "Speed",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f),
        4.5f,
        "Hz"
    ));

    // DEPTH_PARAM - Tremolo Depth (0 - 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DEPTH_PARAM", 1 },
        "Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f,
        "%"
    ));

    // WAVEFORM_PARAM - Waveform Type (Choice: 0-5)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "WAVEFORM_PARAM", 1 },
        "Waveform",
        juce::StringArray { "Sine", "Triangle", "Phasor", "Noise", "Square", "Pulse" },
        0  // Default: Sine
    ));

    // SMOOTHING_PARAM - Waveform Smoothing (0 - 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "SMOOTHING_PARAM", 1 },
        "Smoothing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        30.0f,
        "%"
    ));

    // PAN_SYNC_PARAM - Pan Sync Enable (Boolean)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "PAN_SYNC_PARAM", 1 },
        "Pan Sync",
        false  // Default: OFF
    ));

    // TEMPO_SYNC_PARAM - Tempo Sync Enable (Boolean)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "TEMPO_SYNC_PARAM", 1 },
        "Tempo Sync",
        false  // Default: OFF
    ));

    return layout;
}

//==============================================================================
// Constructor & Destructor
//==============================================================================
OuariconTremoloAudioProcessor::OuariconTremoloAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

OuariconTremoloAudioProcessor::~OuariconTremoloAudioProcessor()
{
}

//==============================================================================
// Audio Processing
//==============================================================================
void OuariconTremoloAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    // Store sample rate for phase increment calculations
    currentSampleRate = sampleRate;

    // Initialize LFO phase
    lfoPhase = 0.0f;

    // Calculate initial phase increment
    auto* speedParam = parameters.getRawParameterValue("SPEED_PARAM");
    float speedHz = speedParam->load();
    lfoPhaseIncrement = speedHz / static_cast<float>(currentSampleRate);

    // Reset smoothing filter state
    smoothedLFO_L = 0.0f;
    smoothedLFO_R = 0.0f;
}

void OuariconTremoloAudioProcessor::releaseResources()
{
    // No buffers to release for this simple effect
}

void OuariconTremoloAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic, real-time safe)
    auto* speedParam = parameters.getRawParameterValue("SPEED_PARAM");
    auto* depthParam = parameters.getRawParameterValue("DEPTH_PARAM");
    auto* waveformParam = parameters.getRawParameterValue("WAVEFORM_PARAM");
    auto* smoothingParam = parameters.getRawParameterValue("SMOOTHING_PARAM");
    auto* panSyncParam = parameters.getRawParameterValue("PAN_SYNC_PARAM");
    auto* tempoSyncParam = parameters.getRawParameterValue("TEMPO_SYNC_PARAM");

    float speedHz = speedParam->load();
    float depth = depthParam->load() / 100.0f;  // Convert 0-100% to 0-1
    int waveformType = static_cast<int>(waveformParam->load());
    float smoothing = smoothingParam->load();
    bool panSyncEnabled = panSyncParam->load() > 0.5f;
    bool tempoSyncEnabled = tempoSyncParam->load() > 0.5f;

    // Handle tempo sync
    if (tempoSyncEnabled)
    {
        // Query host for BPM
        if (auto* playHead = getPlayHead())
        {
            if (auto positionInfo = playHead->getPosition())
            {
                if (positionInfo->getBpm().hasValue())
                {
                    double bpm = *positionInfo->getBpm();
                    double beatsPerSecond = bpm / 60.0;

                    // Musical divisions table: beat multipliers
                    // Straight divisions
                    struct MusicalDivision {
                        const char* name;
                        float beatMultiplier;
                    };

                    const MusicalDivision divisions[] = {
                        // Straight divisions
                        { "1/1",   4.0f },      // Whole note
                        { "1/2",   2.0f },      // Half note
                        { "1/4",   1.0f },      // Quarter note
                        { "1/8",   0.5f },      // Eighth note
                        { "1/16",  0.25f },     // Sixteenth note
                        { "1/32",  0.125f },    // Thirty-second note

                        // Triplet divisions (3 notes in space of 2)
                        { "1/2T",  1.333333f }, // Half note triplet
                        { "1/4T",  0.666667f }, // Quarter triplet
                        { "1/8T",  0.333333f }, // Eighth triplet
                        { "1/16T", 0.166667f }, // Sixteenth triplet
                        { "1/32T", 0.083333f }, // Thirty-second triplet

                        // Quintuplet divisions (5 notes in space of 4)
                        { "1/2Q",  1.6f },      // Half note quintuplet
                        { "1/4Q",  0.8f },      // Quarter quintuplet
                        { "1/8Q",  0.4f },      // Eighth quintuplet
                        { "1/16Q", 0.2f },      // Sixteenth quintuplet
                        { "1/32Q", 0.1f }       // Thirty-second quintuplet
                    };

                    const int numDivisions = sizeof(divisions) / sizeof(divisions[0]);

                    // Find closest division based on current speed
                    float closestDivision = 1.0f;
                    float minDiff = 1000.0f;

                    for (int i = 0; i < numDivisions; ++i)
                    {
                        float divFreq = static_cast<float>(beatsPerSecond / divisions[i].beatMultiplier);
                        float diff = std::abs(speedHz - divFreq);
                        if (diff < minDiff)
                        {
                            minDiff = diff;
                            closestDivision = divisions[i].beatMultiplier;
                        }
                    }

                    // Calculate Hz from BPM and division
                    speedHz = static_cast<float>(beatsPerSecond / closestDivision);
                }
            }
        }
    }

    // Update phase increment
    lfoPhaseIncrement = speedHz / static_cast<float>(currentSampleRate);

    // Calculate smoothing filter coefficient
    // 0% smoothing: coefficient ≈ 1.0 (no filtering)
    // 100% smoothing: coefficient ≈ 0.01 (heavy filtering)
    float smoothingCoeff = 1.0f - (smoothing / 100.0f) * 0.99f;

    // Get number of channels and samples
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Process audio
    if (panSyncEnabled && numChannels == 2)
    {
        // Stereo tremolo with 180° phase offset
        auto* leftData = buffer.getWritePointer(0);
        auto* rightData = buffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Calculate left and right phases (R = L + 0.5 for 180° offset)
            float leftPhase = lfoPhase;
            float rightPhase = lfoPhase + 0.5f;
            if (rightPhase >= 1.0f) rightPhase -= 1.0f;

            // Generate raw waveform values
            float rawLFO_L = generateWaveform(leftPhase, waveformType);
            float rawLFO_R = generateWaveform(rightPhase, waveformType);

            // Apply smoothing filter
            float smoothedLFO_L_val = applySmoothingFilter(rawLFO_L, smoothedLFO_L, smoothingCoeff);
            float smoothedLFO_R_val = applySmoothingFilter(rawLFO_R, smoothedLFO_R, smoothingCoeff);

            // Convert LFO output (-1 to +1) to 0 to 1
            float lfoValue_L = (smoothedLFO_L_val + 1.0f) / 2.0f;
            float lfoValue_R = (smoothedLFO_R_val + 1.0f) / 2.0f;

            // Calculate gain multipliers with depth scaling
            float gainMultiplier_L = 1.0f - (lfoValue_L * depth);
            float gainMultiplier_R = 1.0f - (lfoValue_R * depth);

            // Apply gain modulation
            leftData[sample] *= gainMultiplier_L;
            rightData[sample] *= gainMultiplier_R;

            // Update LFO phase
            lfoPhase += lfoPhaseIncrement;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        }
    }
    else
    {
        // Mono tremolo (both channels modulated identically)
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Generate raw waveform value
            float rawLFO = generateWaveform(lfoPhase, waveformType);

            // Apply smoothing filter
            float smoothedLFO_val = applySmoothingFilter(rawLFO, smoothedLFO_L, smoothingCoeff);

            // Convert LFO output (-1 to +1) to 0 to 1
            float lfoValue = (smoothedLFO_val + 1.0f) / 2.0f;

            // Calculate gain multiplier with depth scaling
            float gainMultiplier = 1.0f - (lfoValue * depth);

            // Apply gain modulation to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer(channel);
                channelData[sample] *= gainMultiplier;
            }

            // Update LFO phase
            lfoPhase += lfoPhaseIncrement;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        }
    }
}

//==============================================================================
// Editor Creation
//==============================================================================
juce::AudioProcessorEditor* OuariconTremoloAudioProcessor::createEditor()
{
    return new OuariconTremoloAudioProcessorEditor(*this);
}

//==============================================================================
// State Management (Save/Load)
//==============================================================================
void OuariconTremoloAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OuariconTremoloAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// DSP Helper Methods
//==============================================================================
float OuariconTremoloAudioProcessor::generateWaveform(float phase, int waveformType)
{
    switch (waveformType)
    {
        case 0: // Sine
            return std::sin(phase * 2.0f * juce::MathConstants<float>::pi);

        case 1: // Triangle
        {
            // Piecewise linear: 0→1→0→-1→0
            if (phase < 0.25f)
                return phase * 4.0f;
            else if (phase < 0.75f)
                return 2.0f - (phase * 4.0f);
            else
                return -4.0f + (phase * 4.0f);
        }

        case 2: // Phasor (Sawtooth)
            return phase * 2.0f - 1.0f;

        case 3: // Noise
            // Sample and hold random value at each cycle start
            // For continuous noise, generate new random value
            return random.nextFloat() * 2.0f - 1.0f;

        case 4: // Square
            return phase < 0.5f ? 1.0f : -1.0f;

        case 5: // Pulse (20% duty cycle)
            return phase < 0.2f ? 1.0f : -1.0f;

        default:
            return 0.0f;
    }
}

float OuariconTremoloAudioProcessor::applySmoothingFilter(float rawLFO, float& prevSmoothed, float coefficient)
{
    // One-pole lowpass IIR filter
    // coefficient = 1.0 means no smoothing (instant response)
    // coefficient = 0.01 means heavy smoothing (slow response)
    float smoothed = prevSmoothed + (rawLFO - prevSmoothed) * coefficient;
    prevSmoothed = smoothed;
    return smoothed;
}

//==============================================================================
// Factory Function
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OuariconTremoloAudioProcessor();
}
