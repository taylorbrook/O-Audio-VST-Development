#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LushVerbAudioProcessorEditor::LushVerbAudioProcessorEditor(LushVerbAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // ⚠️ CRITICAL: Initialization order must match member declaration order
    // Order: relays → webView → attachments

    // Step 1: Create relays (BEFORE WebView)
    sizeRelay = std::make_unique<juce::WebSliderRelay>("SIZE");
    dampingRelay = std::make_unique<juce::WebSliderRelay>("DAMPING");
    shimmerRelay = std::make_unique<juce::WebSliderRelay>("SHIMMER");
    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");

    // Step 2: Create WebView with resource provider and relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWantsKeyboardFocus(false)
            .withResourceProvider(
                [](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
                {
                    // Map URL to embedded resources
                    if (url == "/" || url == "/index.html")
                    {
                        return juce::WebBrowserComponent::Resource{
                            BinaryData::index_html,
                            BinaryData::index_htmlSize,
                            "text/html"
                        };
                    }

                    if (url == "/js/juce/index.js")
                    {
                        return juce::WebBrowserComponent::Resource{
                            BinaryData::juce_index_js,
                            BinaryData::juce_index_jsSize,
                            "text/javascript"
                        };
                    }

                    return std::nullopt;
                },
                juce::WebBrowserComponent::Options::Backend::webview2)
            .withOptionsFrom(*sizeRelay)
            .withOptionsFrom(*dampingRelay)
            .withOptionsFrom(*shimmerRelay)
            .withOptionsFrom(*mixRelay));

    addAndMakeVisible(*webView);

    // Step 3: Create attachments (AFTER WebView)
    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("SIZE"),
        *sizeRelay,
        nullptr  // No undo manager
    );

    dampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("DAMPING"),
        *dampingRelay,
        nullptr
    );

    shimmerAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("SHIMMER"),
        *shimmerRelay,
        nullptr
    );

    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("MIX"),
        *mixRelay,
        nullptr
    );

    // Set window size (from mockup v3)
    setSize(500, 300);
    setResizable(false, false);

    // Navigate to index
    webView->goToURL("https://juce.backend/");
}

LushVerbAudioProcessorEditor::~LushVerbAudioProcessorEditor()
{
    // Destruction happens in reverse order of member declaration (automatic)
}

//==============================================================================
void LushVerbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire component, no custom painting needed
    g.fillAll(juce::Colours::black);
}

void LushVerbAudioProcessorEditor::resized()
{
    // WebView fills entire editor bounds
    if (webView)
        webView->setBounds(getLocalBounds());
}
