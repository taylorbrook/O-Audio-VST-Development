/*
  ==============================================================================

    O-Texture - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <onnxruntime_cxx_api.h>

#include "DSP/OverlapAddProcessor.h"
#include "DSP/PerlinNoise1D.h"
#include "DSP/TiltFilter.h"

class TextureProcessor : public juce::AudioProcessor
{
public:
    TextureProcessor();
    ~TextureProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "O-Texture"; }
    bool acceptsMidi() const override { return true; }
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

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // Parameters (cached atomic pointers)
    // =========================================================================
    std::atomic<float>* sourceParam       = nullptr;
    std::atomic<float>* modeParam         = nullptr;
    std::atomic<float>* xParam            = nullptr;
    std::atomic<float>* yParam            = nullptr;
    std::atomic<float>* characterAParam   = nullptr;
    std::atomic<float>* characterBParam   = nullptr;
    std::atomic<float>* evolveParam       = nullptr;
    std::atomic<float>* freezeParam       = nullptr;
    std::atomic<float>* brightnessParam   = nullptr;
    std::atomic<float>* mixParam          = nullptr;

    // =========================================================================
    // Dimension map (loaded from BinaryData)
    // =========================================================================
    struct DimMap
    {
        int latentDim = 32;
        int xDim = 0;
        int yDim = 1;
        int charADim = 2;
        int charBDim = 3;
        std::vector<int> evolveDims;
        std::vector<int> inactiveDims;
    };

    DimMap dimMap;
    void loadDimMap();

    // =========================================================================
    // ONNX Runtime decoder inference (direct, synchronous)
    // =========================================================================
    static constexpr int kLatentDim = 32;
    static constexpr int kBlockSize = 4096;
    static constexpr int kHopSize   = 2048;

    Ort::Env ortEnv { ORT_LOGGING_LEVEL_WARNING, "OTexture" };
    std::unique_ptr<Ort::Session> decoderSession;

    void initDecoderSession();
    bool runDecoder(const float* latent, float* outputAudio);

    // =========================================================================
    // Latent vector state
    // =========================================================================
    std::array<float, 32> latentL{};
    std::array<float, 32> latentR{};

    // Full decoded block buffers (4096 samples each)
    std::vector<float> decodedBufferL;
    std::vector<float> decodedBufferR;
    std::vector<float> decoderOutputBuffer; // Pre-allocated for runDecoder()
    bool decoderReady = false;

    void constructLatentVectors(float x, float y, float charA, float charB,
                                float evolveRate, bool freeze);

    // =========================================================================
    // Evolve noise modulation
    // =========================================================================
    PerlinNoise1D<28> evolveNoise;

    // =========================================================================
    // Overlap-add processor
    // =========================================================================
    OverlapAddProcessor olaProcessor;
    bool needsNewBlock = true;

    // =========================================================================
    // Tilt filter (brightness)
    // =========================================================================
    TiltFilter tiltFilter;

    // =========================================================================
    // Stereo decorrelation offset
    // =========================================================================
    static constexpr float kStereoOffset = 0.1f;

    // =========================================================================
    // Random number generator for inactive dims
    // =========================================================================
    juce::Random inactiveRng;

    // =========================================================================
    // State
    // =========================================================================
    double currentSampleRate = 48000.0;
    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureProcessor)
};
