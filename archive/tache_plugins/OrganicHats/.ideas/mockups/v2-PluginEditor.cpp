#include "PluginProcessor.h"
#include "PluginEditor.h"

OrganicHatsAudioProcessorEditor::OrganicHatsAudioProcessorEditor(OrganicHatsAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set editor size to match mockup dimensions
    setSize(600, 590);

    // Create WebView with options
    juce::WebBrowserComponent::Options options;
    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(webView.get());

    // Resource provider for UI files
    webView->setResourceProvider(
        [](const juce::String& url) -> juce::WebBrowserComponent::Resource
        {
            if (url == "/")
            {
                auto* data = BinaryData::indexhtml;
                auto dataSize = BinaryData::indexhtmlSize;
                return { data, dataSize, "text/html" };
            }
            return {};
        },
        juce::URL("http://localhost/")
    );

    // Create parameter attachments with relays
    closedToneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "CLOSED_TONE", closedToneRelay
    );
    closedDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "CLOSED_DECAY", closedDecayRelay
    );
    closedNoiseColorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "CLOSED_NOISE_COLOR", closedNoiseColorRelay
    );
    openToneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "OPEN_TONE", openToneRelay
    );
    openReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "OPEN_RELEASE", openReleaseRelay
    );
    openNoiseColorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "OPEN_NOISE_COLOR", openNoiseColorRelay
    );
}

OrganicHatsAudioProcessorEditor::~OrganicHatsAudioProcessorEditor()
{
}

void OrganicHatsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void OrganicHatsAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}
