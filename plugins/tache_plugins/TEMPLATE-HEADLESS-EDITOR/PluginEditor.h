#pragma once
#include "PluginProcessor.h"

class [PluginName]AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit [PluginName]AudioProcessorEditor([PluginName]AudioProcessor& p)
        : AudioProcessorEditor(&p), processorRef(p)
    {
        // Fixed size for headless editor
        setSize(500, 200);
    }

    ~[PluginName]AudioProcessorEditor() override = default;

    void paint(juce::Graphics& g) override
    {
        // Dark background
        g.fillAll(juce::Colours::darkgrey);

        // Plugin name (large)
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
        auto nameArea = getLocalBounds().removeFromTop(100);
        g.drawFittedText("[PluginName]", nameArea, juce::Justification::centred, 1);

        // Instruction text (small)
        g.setFont(juce::FontOptions(16.0f));
        auto instructionArea = getLocalBounds().reduced(20);
        g.drawFittedText("Use your DAW's generic plugin controls to adjust parameters",
                         instructionArea, juce::Justification::centred, 2);
    }

    void resized() override
    {
        // No components to layout
    }

private:
    [PluginName]AudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR([PluginName]AudioProcessorEditor)
};
