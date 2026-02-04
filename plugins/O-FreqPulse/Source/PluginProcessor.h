/*
  ==============================================================================

    O-FreqPulse - Audio Processor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class OFreqPulseAudioProcessor : public juce::AudioProcessor
{
public:
    OFreqPulseAudioProcessor();
    ~OFreqPulseAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-FreqPulse"; }

    // Thread-safe access for GUI timer
    int getCurrentStep() const { return currentStepAtomic.load(); }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }

private:
    // Per-band parameter cache structure
    struct BandParams
    {
        std::atomic<float>* enable = nullptr;
        std::atomic<float>* low = nullptr;
        std::atomic<float>* high = nullptr;
        std::atomic<float>* depth = nullptr;
        std::atomic<float>* eucOn = nullptr;
        std::atomic<float>* eucSteps = nullptr;
        std::atomic<float>* eucPulses = nullptr;
        std::atomic<float>* eucOffset = nullptr;
        std::array<std::atomic<float>*, 32> stepStates;  // 32 step parameters per band
    };

    // Cached parameter pointers (real-time safe access)
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* stepsParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* swingParam = nullptr;
    std::atomic<float>* smoothingParam = nullptr;

    std::array<BandParams, 4> bandParams;  // 4 bands

    // FFT Configuration Constants
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 2048;  // 2^11
    static constexpr int hopSize = 512;   // 75% overlap (fftSize / 4)
    static constexpr int numBins = 1025;  // fftSize / 2 + 1
    static constexpr float windowCorrection = 2.0f / 3.0f;  // COLA correction for Hann + 75% overlap

    // FFT Objects
    juce::dsp::FFT fft { fftOrder };
    std::vector<float> hannWindow;  // Pre-computed Hann window

    // STFT Buffers (stereo)
    std::array<std::vector<float>, 2> inputFifo;
    std::array<std::vector<float>, 2> outputFifo;
    std::array<std::vector<float>, 2> fftData;

    // STFT Tracking
    int inputWritePos = 0;
    int outputReadPos = 0;
    int hopCounter = 0;

    // Dry/Wet Mixer
    juce::dsp::DryWetMixer<float> dryWetMixer { 10 };  // 10ms max latency for mixer

    // Band Processing
    std::array<juce::SmoothedValue<float>, 4> bandGainSmooth;
    std::array<int, numBins> bandForBin;  // Maps bin index to band index (-1 = passthrough)

    // Euclidean Patterns
    std::array<std::array<bool, 32>, 4> euclideanPatterns;

    // Step Sequencer State
    int currentStep = 0;
    std::atomic<int> currentStepAtomic { 0 };  // Thread-safe for GUI access
    double lastPpqPosition = -1.0;

    // Free-running step counter (for standalone/no-host mode)
    double sampleAccumulator = 0.0;

    // Cached parameter values (for change detection)
    float lastBandFreqs[4][2] = { {0, 0}, {0, 0}, {0, 0}, {0, 0} };  // [band][low/high]
    int lastEuclideanParams[4][3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };  // [band][steps/pulses/offset]

    // APVTS (declared after cached pointers)
    juce::AudioProcessorValueTreeState parameters;

    // DSP state
    double currentSampleRate = 44100.0;

    // Helper Methods
    void recalculateBinMapping();
    std::array<bool, 32> generateEuclidean(int steps, int pulses, int offset);
    void updateEuclideanPatterns();
    int calculateCurrentStep(double ppq, int numSteps, int rateIndex, float swing);
    float getTargetGainForBand(int bandIndex, int currentStep);
    void processFrame(int channel);

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OFreqPulseAudioProcessor)
};
