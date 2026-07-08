/*
  ==============================================================================

    OuariconTremolo - Audio Processor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Constants
//==============================================================================
static constexpr float kTransitionWidth = 0.02f;  // 2% of cycle for smooth waveform transitions

struct MusicalDivision {
    const char* name;
    float beatMultiplier;
};

static constexpr MusicalDivision kMusicalDivisions[] = {
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

static constexpr int kNumMusicalDivisions = static_cast<int>(std::size(kMusicalDivisions));

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
    , presetManager(parameters, "O-Tremolo")
{
    // Initialize factory presets
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        {
            "Default",
            {{"SPEED_PARAM", 0.221f}, {"DEPTH_PARAM", 0.75f}, {"WAVEFORM_PARAM", 0.0f},
             {"SMOOTHING_PARAM", 0.30f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Slow Pulse",
            {{"SPEED_PARAM", 0.05f}, {"DEPTH_PARAM", 0.85f}, {"WAVEFORM_PARAM", 0.0f},
             {"SMOOTHING_PARAM", 0.50f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Fast Chop",
            {{"SPEED_PARAM", 0.75f}, {"DEPTH_PARAM", 1.0f}, {"WAVEFORM_PARAM", 0.8f},
             {"SMOOTHING_PARAM", 0.10f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Auto-Pan",
            {{"SPEED_PARAM", 0.30f}, {"DEPTH_PARAM", 0.80f}, {"WAVEFORM_PARAM", 0.0f},
             {"SMOOTHING_PARAM", 0.40f}, {"PAN_SYNC_PARAM", 1.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Subtle",
            {{"SPEED_PARAM", 0.20f}, {"DEPTH_PARAM", 0.35f}, {"WAVEFORM_PARAM", 0.0f},
             {"SMOOTHING_PARAM", 0.60f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Helicopter",
            {{"SPEED_PARAM", 0.598f}, {"DEPTH_PARAM", 0.90f}, {"WAVEFORM_PARAM", 0.2f},
             {"SMOOTHING_PARAM", 0.05f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Vintage Amp",
            {{"SPEED_PARAM", 0.271f}, {"DEPTH_PARAM", 0.55f}, {"WAVEFORM_PARAM", 0.0f},
             {"SMOOTHING_PARAM", 0.70f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Synced Sidechain",
            {{"SPEED_PARAM", 0.397f}, {"DEPTH_PARAM", 1.0f}, {"WAVEFORM_PARAM", 1.0f},
             {"SMOOTHING_PARAM", 0.15f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 1.0f}},
            juce::var()
        },
        {
            "Wide Stereo",
            {{"SPEED_PARAM", 0.095f}, {"DEPTH_PARAM", 0.60f}, {"WAVEFORM_PARAM", 0.2f},
             {"SMOOTHING_PARAM", 0.45f}, {"PAN_SYNC_PARAM", 1.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        },
        {
            "Glitch",
            {{"SPEED_PARAM", 0.899f}, {"DEPTH_PARAM", 1.0f}, {"WAVEFORM_PARAM", 0.6f},
             {"SMOOTHING_PARAM", 0.0f}, {"PAN_SYNC_PARAM", 0.0f}, {"TEMPO_SYNC_PARAM", 0.0f}},
            juce::var()
        }
    };

    presetManager.initializeFactoryPresets(factoryPresets);

    // Cache parameter pointers (stable for processor lifetime)
    speedParam = parameters.getRawParameterValue("SPEED_PARAM");
    depthParam = parameters.getRawParameterValue("DEPTH_PARAM");
    waveformParam = parameters.getRawParameterValue("WAVEFORM_PARAM");
    smoothingParam = parameters.getRawParameterValue("SMOOTHING_PARAM");
    panSyncParam = parameters.getRawParameterValue("PAN_SYNC_PARAM");
    tempoSyncParam = parameters.getRawParameterValue("TEMPO_SYNC_PARAM");
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
    float speedHz = speedParam->load();
    lfoPhaseIncrement = speedHz / static_cast<float>(currentSampleRate);

    // Reset smoothing filter state
    smoothedLFO_L = 0.0f;
    smoothedLFO_R = 0.0f;

    // Prime depth smoothing to the current value (no fade-in on the first block)
    depthSmoothed.reset(sampleRate, 0.02);  // 20 ms ramp
    depthSmoothed.setCurrentAndTargetValue(depthParam->load() / 100.0f);
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

    // Read parameters (atomic, real-time safe — pointers cached in constructor)
    float speedHz = speedParam->load();
    float depth = depthParam->load() / 100.0f;  // Convert 0-100% to 0-1
    int waveformType = static_cast<int>(waveformParam->load());
    float smoothing = smoothingParam->load();
    bool panSyncEnabled = panSyncParam->load() > 0.5f;
    bool tempoSyncEnabled = tempoSyncParam->load() > 0.5f;

    // Query host for BPM once per block and cache it for the UI (IN-04), regardless of
    // whether tempo sync is engaged — so the WebView readout is already correct the moment
    // the user toggles sync on.
    double bpm = 0.0;
    if (auto* playHead = getPlayHead())
        if (auto positionInfo = playHead->getPosition())
            if (positionInfo->getBpm().hasValue())
                bpm = *positionInfo->getBpm();

    if (bpm > 0.0)
        hostBpm.store(bpm);

    // Handle tempo sync. When sync is engaged but the host reports no BPM (e.g. Standalone),
    // fall back to 120 BPM — the same default the WebView readout uses (index.html
    // rebuildSyncSteps) — so the displayed division and the actual rate agree (v1.5.1 A).
    if (tempoSyncEnabled)
    {
        double effectiveBpm = (bpm > 0.0) ? bpm : 120.0;
        double beatsPerSecond = effectiveBpm / 60.0;

        // Find closest musical division based on current speed
        float closestDivision = 1.0f;
        float minDiff = 1000.0f;

        for (int i = 0; i < kNumMusicalDivisions; ++i)
        {
            float divFreq = static_cast<float>(beatsPerSecond / kMusicalDivisions[i].beatMultiplier);
            float diff = std::abs(speedHz - divFreq);
            if (diff < minDiff)
            {
                minDiff = diff;
                closestDivision = kMusicalDivisions[i].beatMultiplier;
            }
        }

        // Calculate Hz from BPM and division
        speedHz = static_cast<float>(beatsPerSecond / closestDivision);
    }

    // Update phase increment
    lfoPhaseIncrement = speedHz / static_cast<float>(currentSampleRate);

    // Ramp depth toward its new target; consumed per-sample below (WR-01)
    depthSmoothed.setTargetValue(depth);

    // Calculate smoothing filter coefficient
    // 0% smoothing: coefficient ≈ 1.0 (no filtering)
    // 100% smoothing: coefficient ≈ 0.01 (heavy filtering)
    float smoothingCoeff = 1.0f - (smoothing / 100.0f) * 0.99f;

    // Get number of channels and samples
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // FIX: Handle mono input → stereo output (center the mono signal)
    // Check if we have stereo output but mono input
    // This happens when a mono track is processed through the plugin
    const int totalInputChannels = getTotalNumInputChannels();
    const int totalOutputChannels = getTotalNumOutputChannels();

    if (totalInputChannels == 1 && totalOutputChannels == 2 && numChannels == 2)
    {
        // Duplicate mono input (channel 0) to channel 1 for centered output
        buffer.copyFrom(1, 0, buffer.getReadPointer(0), numSamples);
    }

    // Process audio
    if (panSyncEnabled && numChannels == 2)
    {
        // Stereo tremolo with 180° phase offset
        auto* leftData = buffer.getWritePointer(0);
        auto* rightData = buffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Per-sample smoothed depth (WR-01) — one step per sample keeps L/R in sync
            const float depthS = depthSmoothed.getNextValue();

            // Calculate left and right phases (R = L + 0.5 for 180° offset)
            float leftPhase = lfoPhase;
            float rightPhase = lfoPhase + 0.5f;
            if (rightPhase >= 1.0f) rightPhase -= 1.0f;

            // Generate raw waveform values
            // Pass lfoPhase for noise waveform to track sampling consistently
            float rawLFO_L = generateWaveform(leftPhase, waveformType, lfoPhase);
            float rawLFO_R = generateWaveform(rightPhase, waveformType, lfoPhase);

            // Apply smoothing filter
            float smoothedLFO_L_val = applySmoothingFilter(rawLFO_L, smoothedLFO_L, smoothingCoeff);
            float smoothedLFO_R_val = applySmoothingFilter(rawLFO_R, smoothedLFO_R, smoothingCoeff);

            // Convert LFO output (-1 to +1) to 0 to 1
            float lfoValue_L = (smoothedLFO_L_val + 1.0f) / 2.0f;
            float lfoValue_R = (smoothedLFO_R_val + 1.0f) / 2.0f;

            // Calculate gain multipliers with depth scaling
            float gainMultiplier_L = 1.0f - (lfoValue_L * depthS);
            float gainMultiplier_R = 1.0f - (lfoValue_R * depthS);

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
        // Hoist channel pointers outside sample loop
        float* channelPtrs[2] = { nullptr, nullptr };
        for (int channel = 0; channel < numChannels && channel < 2; ++channel)
            channelPtrs[channel] = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Per-sample smoothed depth (WR-01)
            const float depthS = depthSmoothed.getNextValue();

            // Generate raw waveform value
            float rawLFO = generateWaveform(lfoPhase, waveformType, lfoPhase);

            // Apply smoothing filter
            float smoothedLFO_val = applySmoothingFilter(rawLFO, smoothedLFO_L, smoothingCoeff);

            // Convert LFO output (-1 to +1) to 0 to 1
            float lfoValue = (smoothedLFO_val + 1.0f) / 2.0f;

            // Calculate gain multiplier with depth scaling
            float gainMultiplier = 1.0f - (lfoValue * depthS);

            // Apply gain modulation to all channels
            for (int channel = 0; channel < numChannels && channel < 2; ++channel)
                channelPtrs[channel][sample] *= gainMultiplier;

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
// State Management (Save/Load via PresetManager)
//==============================================================================
void OuariconTremoloAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = presetManager.getStateAsXml())
        copyXmlToBinary(*xml, destData);
}

void OuariconTremoloAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        presetManager.setStateFromXml(xml.get());
}

//==============================================================================
// DSP Helper Methods
//==============================================================================
float OuariconTremoloAudioProcessor::smoothTransition(float t)
{
    // Cubic polynomial smoothstep: 3t^2 - 2t^3
    // Provides smooth S-curve transition from 0 to 1
    return t * t * (3.0f - 2.0f * t);
}

float OuariconTremoloAudioProcessor::generateWaveform(float phase, int waveformType, float mainLfoPhase)
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

        case 2: // Phasor (Sawtooth) with smooth reset
        {
            // Normal ramp: phase * 2 - 1 maps [0,1] to [-1,+1]
            float rampValue = phase * 2.0f - 1.0f;

            // Last 2% of cycle: smooth down preparing for wrap to -1
            if (phase > 1.0f - kTransitionWidth)
            {
                float t = (phase - (1.0f - kTransitionWidth)) / kTransitionWidth;
                float smoothed = smoothTransition(t);
                float endValue = (1.0f - kTransitionWidth) * 2.0f - 1.0f;
                return endValue + smoothed * (-1.0f - endValue);
            }
            // First 2% of cycle: smooth up from -1
            else if (phase < kTransitionWidth)
            {
                float t = phase / kTransitionWidth;
                float smoothed = smoothTransition(t);
                float startRamp = kTransitionWidth * 2.0f - 1.0f;
                return -1.0f + smoothed * (startRamp - (-1.0f));
            }
            else
            {
                return rampValue;
            }
        }

        case 3: // Random (Sample-and-hold, 4 samples per cycle) with smooth transitions
        {
            // Use mainLfoPhase to determine quarter (ignores Pan Sync phase offset)
            // This ensures consistent sampling regardless of stereo phase offset
            int currentQuarter = static_cast<int>(mainLfoPhase * 4.0f);

            // Generate new random value when entering a new quarter
            if (currentQuarter != noiseLastQuarter)
            {
                noisePrevHeldValue = noiseHeldValue;  // Save previous value for smooth transition
                noiseHeldValue = random.nextFloat() * 2.0f - 1.0f;
                noiseLastQuarter = currentQuarter;
            }

            // Apply smooth transition at quarter boundaries
            float quarterPhase = mainLfoPhase * 4.0f - currentQuarter;  // Phase within current quarter (0.0-1.0)

            // First 2% of each quarter: smooth from previous to current value
            if (quarterPhase < kTransitionWidth)
            {
                float t = quarterPhase / kTransitionWidth;
                float smoothed = smoothTransition(t);
                return noisePrevHeldValue + smoothed * (noiseHeldValue - noisePrevHeldValue);
            }

            return noiseHeldValue;
        }

        case 4: // Square with smooth transitions
        {
            if (phase < 0.5f)
            {
                if (phase < kTransitionWidth)
                {
                    float t = phase / kTransitionWidth;
                    return -1.0f + smoothTransition(t) * 2.0f;
                }
                return 1.0f;
            }
            else
            {
                if (phase < 0.5f + kTransitionWidth)
                {
                    float t = (phase - 0.5f) / kTransitionWidth;
                    return 1.0f - smoothTransition(t) * 2.0f;
                }
                return -1.0f;
            }
        }

        case 5: // Pulse (20% duty cycle) with smooth transitions
        {
            const float dutyCycle = 0.2f;

            if (phase < dutyCycle)
            {
                if (phase < kTransitionWidth)
                {
                    float t = phase / kTransitionWidth;
                    return -1.0f + smoothTransition(t) * 2.0f;
                }
                return 1.0f;
            }
            else
            {
                if (phase < dutyCycle + kTransitionWidth)
                {
                    float t = (phase - dutyCycle) / kTransitionWidth;
                    return 1.0f - smoothTransition(t) * 2.0f;
                }
                return -1.0f;
            }
        }

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
