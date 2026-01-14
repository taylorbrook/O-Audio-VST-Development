/*
  ==============================================================================

    Analog EQ Unit - Ouaricon Module System
    4-band analog-style parametric EQ with saturation

    Developer: Taylor Brook / Ouaricon Audio
    Module Version: 1.0.0

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * AnalogEQUnit - Embeddable 4-band analog EQ processor
 *
 * A self-contained EQ module that can be added to any plugin's effects chain.
 * Features:
 *   - LF Shelf (30-500 Hz)
 *   - LMF Bell with variable Q (100-2000 Hz)
 *   - HMF Bell with variable Q (500-8000 Hz)
 *   - HF Shelf (2000-20000 Hz)
 *   - Analog saturation circuit
 *   - Output gain trim
 *
 * Usage:
 *   1. Call createParameterLayout() and add to your APVTS
 *   2. Construct AnalogEQUnit with APVTS reference
 *   3. Call prepare() in prepareToPlay()
 *   4. Call process() in processBlock()
 */
class AnalogEQUnit
{
public:
    /**
     * Creates the parameter layout for this EQ unit.
     * Call this when building your plugin's APVTS parameter layout.
     *
     * @param prefix Optional prefix for parameter IDs (e.g., "fx_eq_" -> "fx_eq_lf_freq")
     * @return Vector of parameter unique_ptrs to add to your layout
     */
    static std::vector<std::unique_ptr<juce::RangedAudioParameter>> createParameterLayout(
        const juce::String& prefix = "eq_")
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // LF Band (Low Frequency Shelf)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "lf_freq", 1 },
            "LF Frequency",
            juce::NormalisableRange<float>(30.0f, 500.0f, 0.1f, 0.3f),
            100.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "lf_gain", 1 },
            "LF Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "lf_on", 1 },
            "LF On", true));

        // LMF Band (Low-Mid Frequency Bell)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "lmf_freq", 1 },
            "LMF Frequency",
            juce::NormalisableRange<float>(100.0f, 2000.0f, 0.1f, 0.3f),
            500.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "lmf_gain", 1 },
            "LMF Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { prefix + "lmf_q", 1 },
            "LMF Q",
            juce::StringArray { "WIDE", "MED", "TIGHT" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "lmf_on", 1 },
            "LMF On", true));

        // HMF Band (High-Mid Frequency Bell)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "hmf_freq", 1 },
            "HMF Frequency",
            juce::NormalisableRange<float>(500.0f, 8000.0f, 0.1f, 0.3f),
            2000.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "hmf_gain", 1 },
            "HMF Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { prefix + "hmf_q", 1 },
            "HMF Q",
            juce::StringArray { "WIDE", "MED", "TIGHT" }, 1));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "hmf_on", 1 },
            "HMF On", true));

        // HF Band (High Frequency Shelf)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "hf_freq", 1 },
            "HF Frequency",
            juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.1f, 0.3f),
            8000.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "hf_gain", 1 },
            "HF Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "hf_on", 1 },
            "HF On", true));

        // Global Controls
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "output_gain", 1 },
            "EQ Output Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "analog", 1 },
            "Analog", true));

        return params;
    }

    /**
     * Construct an AnalogEQUnit.
     *
     * @param apvts Reference to the plugin's AudioProcessorValueTreeState
     * @param prefix The same prefix used when creating parameters
     */
    AnalogEQUnit(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix = "eq_")
        : parameters(apvts)
        , paramPrefix(prefix)
    {
    }

    /**
     * Prepare the EQ for playback. Call this from prepareToPlay().
     */
    void prepare(double sampleRate, int samplesPerBlock, int numChannels)
    {
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
        spec.numChannels = static_cast<juce::uint32>(numChannels);

        // Prepare all DSP components
        lfFilter.prepare(spec);
        lmfFilter.prepare(spec);
        hmfFilter.prepare(spec);
        hfFilter.prepare(spec);
        saturation.prepare(spec);
        outputGain.prepare(spec);

        // Reset to initial state
        reset();

        // Initialize saturation: gentle warmth, gain-neutral
        saturation.functionToUse = [](float x) { return std::tanh(x * 0.5f) * 2.0f; };

        // Initialize filter coefficients
        updateAllCoefficients();
    }

    /**
     * Reset the EQ state (clear filter memory).
     */
    void reset()
    {
        lfFilter.reset();
        lmfFilter.reset();
        hmfFilter.reset();
        hfFilter.reset();
        saturation.reset();
        outputGain.reset();

        // Reset change detection
        previousLfFreq = previousLfGain = 0.0f;
        previousLmfFreq = previousLmfGain = 0.0f;
        previousLmfQ = -1;
        previousHmfFreq = previousHmfGain = 0.0f;
        previousHmfQ = -1;
        previousHfFreq = previousHfGain = 0.0f;
    }

    /**
     * Process audio through the EQ. Call this from processBlock().
     *
     * @param buffer The audio buffer to process in-place
     */
    void process(juce::AudioBuffer<float>& buffer)
    {
        juce::ScopedNoDenormals noDenormals;

        // Read parameters (atomic, real-time safe)
        float lfFreq = parameters.getRawParameterValue(paramPrefix + "lf_freq")->load();
        float lfGain = parameters.getRawParameterValue(paramPrefix + "lf_gain")->load();
        bool lfOn = parameters.getRawParameterValue(paramPrefix + "lf_on")->load() > 0.5f;

        float lmfFreq = parameters.getRawParameterValue(paramPrefix + "lmf_freq")->load();
        float lmfGain = parameters.getRawParameterValue(paramPrefix + "lmf_gain")->load();
        int lmfQChoice = static_cast<int>(parameters.getRawParameterValue(paramPrefix + "lmf_q")->load());
        bool lmfOn = parameters.getRawParameterValue(paramPrefix + "lmf_on")->load() > 0.5f;

        float hmfFreq = parameters.getRawParameterValue(paramPrefix + "hmf_freq")->load();
        float hmfGain = parameters.getRawParameterValue(paramPrefix + "hmf_gain")->load();
        int hmfQChoice = static_cast<int>(parameters.getRawParameterValue(paramPrefix + "hmf_q")->load());
        bool hmfOn = parameters.getRawParameterValue(paramPrefix + "hmf_on")->load() > 0.5f;

        float hfFreq = parameters.getRawParameterValue(paramPrefix + "hf_freq")->load();
        float hfGain = parameters.getRawParameterValue(paramPrefix + "hf_gain")->load();
        bool hfOn = parameters.getRawParameterValue(paramPrefix + "hf_on")->load() > 0.5f;

        float outputGainDB = parameters.getRawParameterValue(paramPrefix + "output_gain")->load();
        bool analogOn = parameters.getRawParameterValue(paramPrefix + "analog")->load() > 0.5f;

        // Update filter coefficients if parameters changed
        if (lfFreq != previousLfFreq || lfGain != previousLfGain)
        {
            float gainFactor = std::pow(10.0f, lfGain / 20.0f);
            *lfFilter.state = *IIRCoefficients::makeLowShelf(spec.sampleRate, lfFreq, 0.707f, gainFactor);
            previousLfFreq = lfFreq;
            previousLfGain = lfGain;
        }

        if (lmfFreq != previousLmfFreq || lmfGain != previousLmfGain || lmfQChoice != previousLmfQ)
        {
            float gainFactor = std::pow(10.0f, lmfGain / 20.0f);
            float qValue = getQValueFromChoice(lmfQChoice);
            *lmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, lmfFreq, qValue, gainFactor);
            previousLmfFreq = lmfFreq;
            previousLmfGain = lmfGain;
            previousLmfQ = lmfQChoice;
        }

        if (hmfFreq != previousHmfFreq || hmfGain != previousHmfGain || hmfQChoice != previousHmfQ)
        {
            float gainFactor = std::pow(10.0f, hmfGain / 20.0f);
            float qValue = getQValueFromChoice(hmfQChoice);
            *hmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, hmfFreq, qValue, gainFactor);
            previousHmfFreq = hmfFreq;
            previousHmfGain = hmfGain;
            previousHmfQ = hmfQChoice;
        }

        if (hfFreq != previousHfFreq || hfGain != previousHfGain)
        {
            float gainFactor = std::pow(10.0f, hfGain / 20.0f);
            *hfFilter.state = *IIRCoefficients::makeHighShelf(spec.sampleRate, hfFreq, 0.707f, gainFactor);
            previousHfFreq = hfFreq;
            previousHfGain = hfGain;
        }

        // Update output gain
        outputGain.setGainDecibels(outputGainDB);

        // Process audio through DSP chain
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        // Sequential processing: LF → LMF → HMF → HF → Saturation → Output Gain
        if (lfOn)
            lfFilter.process(context);

        if (lmfOn)
            lmfFilter.process(context);

        if (hmfOn)
            hmfFilter.process(context);

        if (hfOn)
            hfFilter.process(context);

        if (analogOn)
            saturation.process(context);

        outputGain.process(context);

        // Update output level for metering (optional)
        float peakLevel = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float channelPeak = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
            peakLevel = std::max(peakLevel, channelPeak);
        }
        float levelDB = peakLevel > 0.00001f
            ? juce::Decibels::gainToDecibels(peakLevel)
            : -100.0f;
        outputLevelDB.store(levelDB, std::memory_order_relaxed);
    }

    /**
     * Get the current output level in dB. Thread-safe for UI polling.
     */
    float getOutputLevelDB() const
    {
        return outputLevelDB.load(std::memory_order_relaxed);
    }

    /**
     * Get the parameter prefix used by this instance.
     */
    const juce::String& getParameterPrefix() const { return paramPrefix; }

    /**
     * Get parameter names with prefix applied.
     */
    static std::vector<juce::String> getParameterIDs(const juce::String& prefix = "eq_")
    {
        return {
            prefix + "lf_freq", prefix + "lf_gain", prefix + "lf_on",
            prefix + "lmf_freq", prefix + "lmf_gain", prefix + "lmf_q", prefix + "lmf_on",
            prefix + "hmf_freq", prefix + "hmf_gain", prefix + "hmf_q", prefix + "hmf_on",
            prefix + "hf_freq", prefix + "hf_gain", prefix + "hf_on",
            prefix + "output_gain", prefix + "analog"
        };
    }

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::String paramPrefix;

    // DSP Components
    juce::dsp::ProcessSpec spec;

    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefficients = juce::dsp::IIR::Coefficients<float>;

    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> lfFilter;
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> lmfFilter;
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> hmfFilter;
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefficients> hfFilter;

    juce::dsp::WaveShaper<float> saturation;
    juce::dsp::Gain<float> outputGain;

    // Output level for metering
    std::atomic<float> outputLevelDB { -100.0f };

    // Previous values for change detection
    float previousLfFreq = 0.0f, previousLfGain = 0.0f;
    float previousLmfFreq = 0.0f, previousLmfGain = 0.0f;
    int previousLmfQ = -1;
    float previousHmfFreq = 0.0f, previousHmfGain = 0.0f;
    int previousHmfQ = -1;
    float previousHfFreq = 0.0f, previousHfGain = 0.0f;

    float getQValueFromChoice(int choiceIndex) const
    {
        switch (choiceIndex)
        {
            case 0: return 0.5f;  // WIDE
            case 1: return 1.0f;  // MED
            case 2: return 2.0f;  // TIGHT
            default: return 1.0f;
        }
    }

    void updateAllCoefficients()
    {
        float lfFreq = parameters.getRawParameterValue(paramPrefix + "lf_freq")->load();
        float lfGain = parameters.getRawParameterValue(paramPrefix + "lf_gain")->load();
        float lmfFreq = parameters.getRawParameterValue(paramPrefix + "lmf_freq")->load();
        float lmfGain = parameters.getRawParameterValue(paramPrefix + "lmf_gain")->load();
        int lmfQChoice = static_cast<int>(parameters.getRawParameterValue(paramPrefix + "lmf_q")->load());
        float hmfFreq = parameters.getRawParameterValue(paramPrefix + "hmf_freq")->load();
        float hmfGain = parameters.getRawParameterValue(paramPrefix + "hmf_gain")->load();
        int hmfQChoice = static_cast<int>(parameters.getRawParameterValue(paramPrefix + "hmf_q")->load());
        float hfFreq = parameters.getRawParameterValue(paramPrefix + "hf_freq")->load();
        float hfGain = parameters.getRawParameterValue(paramPrefix + "hf_gain")->load();

        float lfGainFactor = std::pow(10.0f, lfGain / 20.0f);
        *lfFilter.state = *IIRCoefficients::makeLowShelf(spec.sampleRate, lfFreq, 0.707f, lfGainFactor);

        float lmfGainFactor = std::pow(10.0f, lmfGain / 20.0f);
        float lmfQ = getQValueFromChoice(lmfQChoice);
        *lmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, lmfFreq, lmfQ, lmfGainFactor);

        float hmfGainFactor = std::pow(10.0f, hmfGain / 20.0f);
        float hmfQ = getQValueFromChoice(hmfQChoice);
        *hmfFilter.state = *IIRCoefficients::makePeakFilter(spec.sampleRate, hmfFreq, hmfQ, hmfGainFactor);

        float hfGainFactor = std::pow(10.0f, hfGain / 20.0f);
        *hfFilter.state = *IIRCoefficients::makeHighShelf(spec.sampleRate, hfFreq, 0.707f, hfGainFactor);

        previousLfFreq = lfFreq; previousLfGain = lfGain;
        previousLmfFreq = lmfFreq; previousLmfGain = lmfGain; previousLmfQ = lmfQChoice;
        previousHmfFreq = hmfFreq; previousHmfGain = hmfGain; previousHmfQ = hmfQChoice;
        previousHfFreq = hfFreq; previousHfGain = hfGain;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogEQUnit)
};
