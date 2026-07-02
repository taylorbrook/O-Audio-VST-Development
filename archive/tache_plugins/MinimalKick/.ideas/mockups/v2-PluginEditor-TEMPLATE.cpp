//==============================================================================
// MinimalKick - PluginEditor.cpp (TEMPLATE for Stage 5)
// Generated from UI mockup v2
//==============================================================================

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MinimalKickAudioProcessorEditor::MinimalKickAudioProcessorEditor (MinimalKickAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set editor size (730×280px from mockup v2)
    setSize (730, 280);

    // ========================================================================
    // WEBVIEW OPTIONS (JUCE 8 Pattern #13)
    // ========================================================================

    juce::WebBrowserComponent::Options options;

    // Enable backend communication
    options = options.withBackend (
        juce::WebBrowserComponent::Options::Backend::webview2
    );

    // Enable native integrations
    options = options.withNativeIntegrationEnabled();

    // Set user script (WebView console → C++ console logging)
    options = options.withUserScript (R"(
        window.console = new Proxy(console, {
            get(target, prop) {
                if (['log', 'debug', 'info', 'warn', 'error'].includes(prop)) {
                    return new Proxy(target[prop], {
                        apply(fn, thisArg, args) {
                            window.__JUCE__.backend.emitEvent(
                                'jsLog',
                                args.map(x => typeof x === 'object' ? JSON.stringify(x) : String(x)).join(' ')
                            );
                            return Reflect.apply(fn, thisArg, args);
                        }
                    });
                }
                return Reflect.get(target, prop);
            }
        });
    )");

    // Resource provider for UI files
    options = options.withResourceProvider (
        [](const juce::String& url)
        {
            // Map URL to binary resource
            if (url == "https://juce.backend/index.html")
            {
                auto data = BinaryData::indexhtml;
                auto size = BinaryData::indexhtmlSize;
                return juce::WebBrowserComponent::Resource {
                    data, static_cast<size_t>(size), "text/html"
                };
            }
            else if (url == "https://juce.backend/js/juce/index.js")
            {
                auto data = BinaryData::indexjs;
                auto size = BinaryData::indexjsSize;
                return juce::WebBrowserComponent::Resource {
                    data, static_cast<size_t>(size), "application/javascript"
                };
            }

            // Return empty resource for unknown URLs
            return juce::WebBrowserComponent::Resource { "", 0, "" };
        },
        false  // Do not use fallback provider
    );

    // Initialize WebView with options
    webView = juce::WebBrowserComponent (options);

    // ========================================================================
    // PARAMETER ATTACHMENTS (JUCE 8 Pattern #16)
    // ========================================================================

    // Create attachments (connects APVTS ↔ relay ↔ WebView)
    sweepAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "sweep", sweepRelay
    );
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "time", timeRelay
    );
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "attack", attackRelay
    );
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "decay", decayRelay
    );
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "drive", driveRelay
    );

    // Integrate relays with WebView (JUCE 8 Pattern #15)
    webView.addWebSliderListener(&sweepRelay);
    webView.addWebSliderListener(&timeRelay);
    webView.addWebSliderListener(&attackRelay);
    webView.addWebSliderListener(&decayRelay);
    webView.addWebSliderListener(&driveRelay);

    // Backend event listener (JavaScript console.log → C++ console)
    webView.addEventListener("jsLog", [](const auto& event)
    {
        auto logMessage = event["message"].toString();
        DBG("[WebView] " << logMessage);
    });

    // Add WebView to editor
    addAndMakeVisible (webView);

    // Navigate to UI
    webView.goToURL ("https://juce.backend/index.html");
}

MinimalKickAudioProcessorEditor::~MinimalKickAudioProcessorEditor()
{
}

//==============================================================================
void MinimalKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    // WebView fills entire editor, no background paint needed
    g.fillAll (juce::Colours::black);
}

void MinimalKickAudioProcessorEditor::resized()
{
    // WebView fills entire editor bounds
    webView.setBounds (getLocalBounds());
}
