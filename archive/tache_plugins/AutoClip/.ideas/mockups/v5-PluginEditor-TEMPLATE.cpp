#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
AutoClipAudioProcessorEditor::AutoClipAudioProcessorEditor (juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), apvts (vts),
      // CRITICAL: Initialization order MUST match member declaration order
      // Order: relays → webView → attachments
      clipThresholdRelay (apvts, "clipThreshold", [this](float value) {
          if (webView != nullptr)
              webView->emitEventIfBrowserIsVisible("parameterValueChange",
                  "{\"parameterID\":\"clipThreshold\",\"value\":" + juce::String(value) + "}");
      }),
      soloClippedRelay (apvts, "soloClipped", [this](float value) {
          if (webView != nullptr)
              webView->emitEventIfBrowserIsVisible("parameterValueChange",
                  "{\"parameterID\":\"soloClipped\",\"value\":" + juce::String(value) + "}");
      })
{
    // Set editor dimensions (300x500 from mockup)
    setSize (300, 500);

    // Create WebView options
    juce::WebBrowserComponent::Options options;

    // Set up resource provider for UI files
    options = options.withResourceProvider(
        [](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
        {
            // Serve the main HTML file
            if (url == "https://juce.backend/index.html" || url == "https://juce.backend/")
            {
                auto data = juce::MemoryBlock(BinaryData::index_html, BinaryData::index_htmlSize);
                return juce::WebBrowserComponent::Resource { std::move(data), "text/html" };
            }

            // Serve the JUCE frontend library
            if (url == "https://juce.backend/js/juce/index.js")
            {
                auto data = juce::MemoryBlock(BinaryData::index_js, BinaryData::index_jsSize);
                return juce::WebBrowserComponent::Resource { std::move(data), "application/javascript" };
            }

            return std::nullopt;
        },
        // Use JUCE backend URL scheme
        juce::URL { "https://juce.backend/" }
    );

    // Set up backend for parameter communication
    options = options.withBackend(
        [this](const juce::String& event, const juce::var& detail) -> juce::var
        {
            if (event == "sliderValueChanged")
            {
                auto parameterId = detail["parameterId"].toString();
                auto value = (float) detail["value"];

                if (parameterId == "clipThreshold")
                {
                    if (auto* param = apvts.getParameter("clipThreshold"))
                        param->setValueNotifyingHost(value);
                }

                return juce::var();
            }

            if (event == "toggleButtonValueChanged")
            {
                auto parameterId = detail["parameterId"].toString();
                auto value = (bool) detail["value"];

                if (parameterId == "soloClipped")
                {
                    if (auto* param = apvts.getParameter("soloClipped"))
                        param->setValueNotifyingHost(value ? 1.0f : 0.0f);
                }

                return juce::var();
            }

            if (event == "getSliderState")
            {
                auto parameterId = detail.toString();

                if (parameterId == "clipThreshold")
                {
                    if (auto* param = apvts.getParameter("clipThreshold"))
                    {
                        return juce::JSON::parse(
                            "{\"value\":" + juce::String(param->getValue()) + "}"
                        );
                    }
                }

                return juce::var();
            }

            if (event == "getToggleButtonState")
            {
                auto parameterId = detail.toString();

                if (parameterId == "soloClipped")
                {
                    if (auto* param = apvts.getParameter("soloClipped"))
                        return param->getValue() > 0.5f;
                }

                return false;
            }

            return juce::var();
        }
    );

    // Create WebView with options
    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(webView.get());

    // Navigate to the main HTML file
    webView->goToURL("https://juce.backend/index.html");

    // Create parameter attachments (MUST be created AFTER relays and webView)
    clipThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        clipThresholdRelay
    );
    soloClippedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        soloClippedRelay
    );
}

AutoClipAudioProcessorEditor::~AutoClipAudioProcessorEditor()
{
}

//==============================================================================
void AutoClipAudioProcessorEditor::paint (juce::Graphics& g)
{
    // WebView fills the entire editor, no background painting needed
}

void AutoClipAudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}
