/*
  ==============================================================================

    O-Gain - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OGainAudioProcessor : public juce::AudioProcessor
{
public:
    OGainAudioProcessor();
    ~OGainAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Gain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // =========================================================================
    // Metering atomics (audio thread writes, UI thread reads)
    // =========================================================================

    // Input metering (post-utilities, pre-gain)
    std::atomic<float> inputPeakL  { 0.0f };
    std::atomic<float> inputPeakR  { 0.0f };
    std::atomic<float> inputRmsL   { 0.0f };
    std::atomic<float> inputRmsR   { 0.0f };

    // Output metering (post-gain)
    std::atomic<float> outputPeakL { 0.0f };
    std::atomic<float> outputPeakR { 0.0f };
    std::atomic<float> outputRmsL  { 0.0f };
    std::atomic<float> outputRmsR  { 0.0f };

    // VU metering (ballistics-filtered, post-utilities pre-gain)
    std::atomic<float> vuLevelL    { 0.0f };
    std::atomic<float> vuLevelR    { 0.0f };

    // =========================================================================
    // Learn mode state (UI thread writes flags, audio thread reads/accumulates)
    // =========================================================================

    std::atomic<bool>  learnActive        { false };
    std::atomic<int>   learnState         { 0 };      // 0=idle, 1=learning, 2=complete

    // LUFS metering (updated during Learn)
    std::atomic<float> momentaryLUFS      { -100.0f };
    std::atomic<float> shortTermLUFS      { -100.0f };
    std::atomic<float> integratedLUFS     { -100.0f };
    std::atomic<float> truePeakDBTP       { -100.0f };

    // Learn results
    std::atomic<float> learnElapsedSeconds { 0.0f };
    std::atomic<int>   learnConfidence     { 0 };     // 0=none, 1=low, 2=medium, 3=high

    juce::AudioProcessorValueTreeState parameters;

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // DSP Components
    // =========================================================================

    // Gain stage with built-in smoothing
    juce::dsp::Gain<float> gainStage;

    // VU meter ballistics (300ms attack/release, RMS mode)
    juce::dsp::BallisticsFilter<float> vuBallisticsL;
    juce::dsp::BallisticsFilter<float> vuBallisticsR;

    // =========================================================================
    // K-Weighting Filters (double precision, per channel)
    // =========================================================================

    // BS.1770 Stage 1: Pre-filter (high-shelf ~+4dB above 2kHz)
    juce::dsp::IIR::Filter<double> kWeightPreFilterL;
    juce::dsp::IIR::Filter<double> kWeightPreFilterR;

    // BS.1770 Stage 2: RLB high-pass (~100Hz rolloff)
    juce::dsp::IIR::Filter<double> kWeightRlbFilterL;
    juce::dsp::IIR::Filter<double> kWeightRlbFilterR;

    // =========================================================================
    // LUFS Gating Block Accumulator
    // =========================================================================

    int lufsBlockSize       = 0;     // 400ms in samples
    int lufsHopSize         = 0;     // 100ms in samples
    int lufsHopCounter      = 0;     // counts samples within current hop

    // Per-channel squared accumulation for current 400ms window
    // Using circular buffer approach: accumulate power in overlapping windows
    // We keep 4 sub-accumulators (each 100ms), rotate them to form 400ms blocks
    static constexpr int kNumSubBlocks = 4;
    double subBlockPowerL[kNumSubBlocks] = {};
    double subBlockPowerR[kNumSubBlocks] = {};
    int    subBlockSampleCount[kNumSubBlocks] = {};
    int    currentSubBlock  = 0;

    // All gating block loudness values (for integrated LUFS dual-gate)
    std::vector<double> gatingBlockPowers;  // mean power per 400ms block (pre-allocated)
    int gatingBlockCount    = 0;

    // Short-term buffer: last 30 blocks (3s at 100ms hop)
    static constexpr int kShortTermBlocks = 30;
    double shortTermPowers[kShortTermBlocks] = {};
    int    shortTermWritePos = 0;
    int    shortTermBlockCount = 0;

    // =========================================================================
    // True Peak Detection (digital peak for MVP)
    // =========================================================================

    double truePeakMax      = 0.0;   // running maximum absolute sample value

    // =========================================================================
    // RMS Accumulation (for learn mode RMS measurement)
    // =========================================================================

    double rmsAccumL        = 0.0;
    double rmsAccumR        = 0.0;
    long long rmsSampleCount = 0;

    // =========================================================================
    // Learn Mode Internal State
    // =========================================================================

    bool   prevLearnActive  = false;  // for edge detection
    int    learnMeasurementModeAtStart = 0;  // snapshot of measurement_mode at learn start
    double learnSampleCount = 0.0;    // total samples accumulated during learn

    // Peak meter decay state
    float  inputPeakDecayL  = 0.0f;
    float  inputPeakDecayR  = 0.0f;
    float  outputPeakDecayL = 0.0f;
    float  outputPeakDecayR = 0.0f;

    // =========================================================================
    // Cached sample rate
    // =========================================================================

    double currentSampleRate = 44100.0;

    // =========================================================================
    // Helper Methods
    // =========================================================================

    // Initialize K-weight filter coefficients for a given sample rate
    void setupKWeightFilters(double sampleRate);

    // Reset all learn accumulators
    void resetLearnAccumulators();

    // Calculate integrated LUFS from accumulated gating blocks (with dual-gate)
    double calculateIntegratedLUFS() const;

    // Finalize learn: compute gain, write to APVTS
    void finalizeLearn();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OGainAudioProcessor)
};
