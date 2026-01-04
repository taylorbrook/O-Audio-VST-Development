#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class DriveVerbAudioProcessor : public juce::AudioProcessor
{
public:
    DriveVerbAudioProcessor();
    ~DriveVerbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DriveVerb"; }
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

    // Public APVTS for editor access (Pattern #11)
    juce::AudioProcessorValueTreeState parameters;

    // VU meter support
    float getDriveOutputLevel() const { return driveOutputLevelDB.load(); }

private:

    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (Stage 4.1: Core reverb + dry/wet mixing)
    juce::dsp::Reverb reverb;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Stage 4.2: Drive saturation
    juce::dsp::WaveShaper<float> driveShaper;

    // Stage 4.3: DJ-style filter (low-pass/high-pass with center bypass)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> filterProcessor;
    bool previousWasLowPass = false;  // Track filter type transitions

    // Stage 4.4: Helper methods for PRE/POST routing
    void applyDrive(juce::dsp::AudioBlock<float>& block, juce::dsp::ProcessContextReplacing<float>& context, float driveValue);
    void applyFilter(juce::dsp::AudioBlock<float>& block, juce::dsp::ProcessContextReplacing<float>& context, float filterValue);

    // VU meter - drive output level
    std::atomic<float> driveOutputLevelDB { -60.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriveVerbAudioProcessor)
};
