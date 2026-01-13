/*
  ==============================================================================

    VUMeterBridge.h
    Ouaricon Module System - VU Metering

    Thread-safe VU metering bridge for JUCE WebView plugins.
    Provides atomic level storage for audio thread and timer polling for UI.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

/**
 * Thread-safe VU metering bridge.
 *
 * The audio thread writes peak levels via storeLevels().
 * The UI thread reads levels via getLevels() and sends to WebView.
 *
 * Usage in Processor:
 *   VUMeterBridge meters;
 *
 *   void processBlock(...) {
 *       // Calculate peak levels
 *       float inputPeak = buffer.getMagnitude(0, numSamples);
 *       float outputPeak = buffer.getMagnitude(0, numSamples);
 *       meters.storeLevels(inputPeak, outputPeak);
 *   }
 *
 * Usage in Editor (timerCallback):
 *   auto [input, output] = processorRef.meters.getLevelsDB();
 *   juce::String script = juce::String::formatted(
 *       "updateMeters(%f, %f);", input, output);
 *   webView->evaluateJavascript(script, nullptr);
 */
class VUMeterBridge
{
public:
    //==========================================================================
    // Configuration
    //==========================================================================

    /** dB floor for meter display (default -60 dB). */
    float minDB = -60.0f;

    /** dB ceiling for meter display (default +3 dB). */
    float maxDB = 3.0f;

    //==========================================================================
    // Construction
    //==========================================================================

    VUMeterBridge() = default;
    ~VUMeterBridge() = default;

    //==========================================================================
    // Audio Thread Interface (lock-free)
    //==========================================================================

    /**
     * Store peak levels from audio thread.
     * Call this at the end of processBlock().
     *
     * @param inputGain   Input peak level (linear, 0-1+)
     * @param outputGain  Output peak level (linear, 0-1+)
     */
    void storeLevels(float inputGain, float outputGain)
    {
        inputLevelDB.store(juce::Decibels::gainToDecibels(inputGain, minDB));
        outputLevelDB.store(juce::Decibels::gainToDecibels(outputGain, minDB));
    }

    /**
     * Store single level (for plugins with only output metering).
     *
     * @param gain  Peak level (linear, 0-1+)
     */
    void storeLevel(float gain)
    {
        outputLevelDB.store(juce::Decibels::gainToDecibels(gain, minDB));
    }

    /**
     * Store additional metering values (gain reduction, envelope, etc.).
     * These are stored in the auxiliary slots.
     *
     * @param values  Vector of dB values
     */
    void storeAuxLevels(const std::vector<float>& values)
    {
        for (size_t i = 0; i < values.size() && i < auxLevels.size(); ++i)
        {
            auxLevels[i].store(values[i]);
        }
    }

    //==========================================================================
    // UI Thread Interface
    //==========================================================================

    /** Get input level in dB (thread-safe read). */
    float getInputLevelDB() const { return inputLevelDB.load(); }

    /** Get output level in dB (thread-safe read). */
    float getOutputLevelDB() const { return outputLevelDB.load(); }

    /** Get both levels as a pair. */
    std::pair<float, float> getLevelsDB() const
    {
        return { inputLevelDB.load(), outputLevelDB.load() };
    }

    /** Get auxiliary level by index. */
    float getAuxLevelDB(size_t index) const
    {
        if (index < auxLevels.size())
            return auxLevels[index].load();
        return minDB;
    }

    //==========================================================================
    // JavaScript Integration Helpers
    //==========================================================================

    /**
     * Generate JavaScript call for updateMeters(input, output).
     * Use with webView->evaluateJavascript().
     */
    juce::String generateMeterScript() const
    {
        return juce::String::formatted(
            "if (typeof updateMeters === 'function') { updateMeters(%f, %f); }",
            getInputLevelDB(), getOutputLevelDB());
    }

    /**
     * Generate JavaScript call with additional values.
     * Generates: updateMeters(input, output, aux0, aux1, ...)
     */
    juce::String generateMeterScriptWithAux(size_t numAux) const
    {
        juce::String script = "if (typeof updateMeters === 'function') { updateMeters(";
        script += juce::String(getInputLevelDB()) + ", ";
        script += juce::String(getOutputLevelDB());

        for (size_t i = 0; i < numAux && i < auxLevels.size(); ++i)
        {
            script += ", " + juce::String(auxLevels[i].load());
        }

        script += "); }";
        return script;
    }

private:
    // Primary meter levels (dB)
    std::atomic<float> inputLevelDB { -60.0f };
    std::atomic<float> outputLevelDB { -60.0f };

    // Auxiliary levels for gain reduction, envelope, etc. (dB)
    std::array<std::atomic<float>, 4> auxLevels = {
        std::atomic<float>{ 0.0f },
        std::atomic<float>{ 0.0f },
        std::atomic<float>{ 0.0f },
        std::atomic<float>{ 0.0f }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUMeterBridge)
};


//==============================================================================
// Helper: Peak Detector for processBlock
//==============================================================================

/**
 * Simple peak detector for use in processBlock.
 * Calculates peak magnitude across all channels.
 */
inline float getPeakMagnitude(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        peak = std::max(peak, buffer.getMagnitude(channel, startSample, numSamples));
    }
    return peak;
}

/**
 * Get peak magnitude for a range of channels.
 */
inline float getPeakMagnitude(const juce::AudioBuffer<float>& buffer,
                               int startChannel, int numChannels,
                               int startSample, int numSamples)
{
    float peak = 0.0f;
    int endChannel = std::min(startChannel + numChannels, buffer.getNumChannels());
    for (int channel = startChannel; channel < endChannel; ++channel)
    {
        peak = std::max(peak, buffer.getMagnitude(channel, startSample, numSamples));
    }
    return peak;
}
