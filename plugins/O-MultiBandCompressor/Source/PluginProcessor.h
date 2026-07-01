/*
  ==============================================================================

    O-MultiBandCompressor - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include "DSP/MultiBandProcessor.h"

// FFT Configuration
static constexpr int FFT_ORDER = 11;                     // 2^11 = 2048 samples
static constexpr int FFT_SIZE = 1 << FFT_ORDER;          // 2048
static constexpr int SPECTRUM_BINS = 64;                 // Bins sent to UI (downsampled)

class OMultiBandCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    OMultiBandCompressorAudioProcessor();
    ~OMultiBandCompressorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-MultiBandCompressor"; }
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

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Access to gain reduction meters for UI (all 4 bands)
    float getLowBandGainReduction() const { return lowBandGainReduction.load(std::memory_order_relaxed); }
    float getLoMidBandGainReduction() const { return loMidBandGainReduction.load(std::memory_order_relaxed); }
    float getHiMidBandGainReduction() const { return hiMidBandGainReduction.load(std::memory_order_relaxed); }
    float getHighBandGainReduction() const { return highBandGainReduction.load(std::memory_order_relaxed); }

    // Access to input/output level meters for UI (Phase 5.3)
    float getInputLevelL() const { return inputLevelL.load(std::memory_order_relaxed); }
    float getInputLevelR() const { return inputLevelR.load(std::memory_order_relaxed); }
    float getOutputLevelL() const { return outputLevelL.load(std::memory_order_relaxed); }
    float getOutputLevelR() const { return outputLevelR.load(std::memory_order_relaxed); }

    // FFT Spectrum data access (v1.2.0)
    void getSpectrumData(std::array<float, SPECTRUM_BINS>& dest) const;
    bool hasNewSpectrumData() const { return spectrumDataReady.load(std::memory_order_acquire); }
    void clearSpectrumDataFlag() { spectrumDataReady.store(false, std::memory_order_release); }

private:
    // DSP Components (BEFORE parameters for initialization order)
    juce::dsp::ProcessSpec spec;

    // Phase 4.2: Multiband processor (4-band crossover + compressors)
    MultiBandProcessor multibandProcessor;

    // Gain stages
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;

    // Phase 4.3: Dry/wet mixer for parallel compression
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Gain reduction metering (atomic for thread-safe UI access)
    std::atomic<float> lowBandGainReduction { 0.0f };
    std::atomic<float> loMidBandGainReduction { 0.0f };
    std::atomic<float> hiMidBandGainReduction { 0.0f };
    std::atomic<float> highBandGainReduction { 0.0f };

    // Input/output level metering (Phase 5.3)
    std::atomic<float> inputLevelL { 0.0f };
    std::atomic<float> inputLevelR { 0.0f };
    std::atomic<float> outputLevelL { 0.0f };
    std::atomic<float> outputLevelR { 0.0f };

    // FFT Spectrum analyzer (v1.2.0)
    juce::dsp::FFT fft { FFT_ORDER };
    juce::dsp::WindowingFunction<float> fftWindow { FFT_SIZE, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, FFT_SIZE * 2> fftWorkBuffer {};    // FFT work buffer
    std::array<float, FFT_SIZE> fftInputFifo {};         // Sample accumulation
    int fftFifoWriteIndex = 0;
    std::atomic<bool> spectrumDataReady { false };

    // WR-01: lock-free triple-buffer hand-off for the spectrum snapshot. The audio thread
    // never blocks — it fills its private write slot and atomically swaps it into the
    // "ready" slot; the UI thread atomically claims the ready slot to read. Each of the
    // three slots is owned by exactly one role at a time, so writer and reader never touch
    // the same buffer.
    std::array<std::array<float, SPECTRUM_BINS>, 3> spectrumBuffers {};
    int spectrumWriteSlot = 0;                            // audio-thread private
    mutable int spectrumReadSlot = 1;                     // UI-thread private
    mutable std::atomic<int> spectrumReadySlot { 2 };     // published-slot hand-off

    // M/S scratch buffer, preallocated in prepareToPlay (CR-03) — avoids per-block AudioBuffer
    // allocation in Mid/Side modes.
    juce::AudioBuffer<float> msScratchBuffer;

    // APVTS comes AFTER DSP components
    juce::AudioProcessorValueTreeState parameters;

    // CR-02: raw parameter pointers resolved once in prepareToPlay so processBlock never
    // builds juce::String IDs or hashes the parameter map on the audio thread.
    enum BandParam
    {
        bpThreshold = 0, bpRatio, bpAttack, bpRelease, bpKnee, bpMakeup,
        bpPeakRms, bpBypass, bpSolo, bpScHPF, bpScLPF, bpScListen, numBandParams
    };
    std::atomic<float>* bandParamPtrs[4][numBandParams] {};
    std::atomic<float>* pInputGain = nullptr;
    std::atomic<float>* pOutputGain = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pAutoMakeup = nullptr;
    std::atomic<float>* pMsMode = nullptr;
    std::atomic<float>* pXover1 = nullptr;
    std::atomic<float>* pXover2 = nullptr;
    std::atomic<float>* pXover3 = nullptr;
    void cacheParameterPointers();

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMultiBandCompressorAudioProcessor)
};
