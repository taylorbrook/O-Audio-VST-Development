#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "DrumRouletteVoice.h"

class DrumRouletteAudioProcessor : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    DrumRouletteAudioProcessor();
    ~DrumRouletteAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DrumRoulette"; }
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

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void loadSampleForSlot(int slotIndex, const juce::File& file);
    void setFolderPathForSlot(int slotIndex, const juce::String& path);
    juce::String getFolderPathForSlot(int slotIndex) const;

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static BusesProperties createBusesLayout();

    // AudioProcessorValueTreeState::Listener implementation
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Folder randomization helpers (Phase 4.4)
    void randomizeSample(int slotIndex);
    void randomizeAllUnlockedSlots();

    // DSP Components (declare BEFORE parameters for initialization order)
    juce::Synthesiser synthesiser;
    juce::AudioFormatManager formatManager;
    std::array<DrumRouletteVoice*, 8> voices;

    // Phase 4.4: Folder paths (not in APVTS - persisted via ValueTree)
    juce::String folderPaths[8];

    // Phase 4.4: Parameter pointers for button/toggle states
    std::atomic<float>* lockParams[8] = {};
    std::atomic<float>* soloParams[8] = {};
    std::atomic<float>* muteParams[8] = {};
    std::atomic<float>* randomizeParams[8] = {};
    std::atomic<float>* randomizeAllParam = nullptr;

    // Phase 4.4: Solo/mute state tracking
    bool anySoloActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumRouletteAudioProcessor)
};
