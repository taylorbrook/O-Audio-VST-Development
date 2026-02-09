#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OuariconAnalogEQAudioProcessorEditor::OuariconAnalogEQAudioProcessorEditor(OuariconAnalogEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // ⚠️ CRITICAL: Initialization order must match member declaration order
    // Order: relays → webView → attachments

    // Step 1: Create relays (BEFORE WebView)
    // LF Band
    lfFreqRelay = std::make_unique<juce::WebSliderRelay>("lf_freq");
    lfGainRelay = std::make_unique<juce::WebSliderRelay>("lf_gain");
    lfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("lf_on");

    // LMF Band
    lmfFreqRelay = std::make_unique<juce::WebSliderRelay>("lmf_freq");
    lmfGainRelay = std::make_unique<juce::WebSliderRelay>("lmf_gain");
    lmfQRelay = std::make_unique<juce::WebSliderRelay>("lmf_q");
    lmfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("lmf_on");

    // HMF Band
    hmfFreqRelay = std::make_unique<juce::WebSliderRelay>("hmf_freq");
    hmfGainRelay = std::make_unique<juce::WebSliderRelay>("hmf_gain");
    hmfQRelay = std::make_unique<juce::WebSliderRelay>("hmf_q");
    hmfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("hmf_on");

    // HF Band
    hfFreqRelay = std::make_unique<juce::WebSliderRelay>("hf_freq");
    hfGainRelay = std::make_unique<juce::WebSliderRelay>("hf_gain");
    hfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("hf_on");

    // Global
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("output_gain");
    analogRelay = std::make_unique<juce::WebToggleButtonRelay>("analog");

    // Step 2: Create WebView with resource provider and relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWantsKeyboardFocus(false)
            .withResourceProvider(
                [](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
                {
                    // Helper lambda to convert binary data to vector
                    auto makeVector = [](const char* data, int size) {
                        return std::vector<std::byte>(
                            reinterpret_cast<const std::byte*>(data),
                            reinterpret_cast<const std::byte*>(data) + size
                        );
                    };

                    // Explicit URL mapping (REQUIRED - see JUCE 8 critical patterns)
                    if (url == "/" || url == "/index.html")
                    {
                        return juce::WebBrowserComponent::Resource{
                            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
                            juce::String("text/html")
                        };
                    }

                    if (url == "/js/juce/index.js")
                    {
                        return juce::WebBrowserComponent::Resource{
                            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
                            juce::String("application/javascript")
                        };
                    }

                    if (url == "/images/paper1.jpg")
                    {
                        return juce::WebBrowserComponent::Resource{
                            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
                            juce::String("image/jpeg")
                        };
                    }

                    if (url == "/images/flower_ferdinandibauer00baue_0021.png")
                    {
                        return juce::WebBrowserComponent::Resource{
                            makeVector(BinaryData::flower_ferdinandibauer00baue_0021_png,
                                      BinaryData::flower_ferdinandibauer00baue_0021_pngSize),
                            juce::String("image/png")
                        };
                    }

                    return std::nullopt;  // 404
                },
                juce::WebBrowserComponent::Options::Backend::webview2)
            // Register all relays
            .withOptionsFrom(*lfFreqRelay)
            .withOptionsFrom(*lfGainRelay)
            .withOptionsFrom(*lfOnRelay)
            .withOptionsFrom(*lmfFreqRelay)
            .withOptionsFrom(*lmfGainRelay)
            .withOptionsFrom(*lmfQRelay)
            .withOptionsFrom(*lmfOnRelay)
            .withOptionsFrom(*hmfFreqRelay)
            .withOptionsFrom(*hmfGainRelay)
            .withOptionsFrom(*hmfQRelay)
            .withOptionsFrom(*hmfOnRelay)
            .withOptionsFrom(*hfFreqRelay)
            .withOptionsFrom(*hfGainRelay)
            .withOptionsFrom(*hfOnRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*analogRelay));

    addAndMakeVisible(*webView);

    // Step 3: Create attachments (AFTER WebView)
    // LF Band
    lfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lf_freq"),
        *lfFreqRelay,
        nullptr  // No undo manager
    );
    lfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lf_gain"),
        *lfGainRelay,
        nullptr
    );
    lfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("lf_on"),
        *lfOnRelay
    );

    // LMF Band
    lmfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lmf_freq"),
        *lmfFreqRelay,
        nullptr
    );
    lmfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lmf_gain"),
        *lmfGainRelay,
        nullptr
    );
    lmfQAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lmf_q"),
        *lmfQRelay,
        nullptr
    );
    lmfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("lmf_on"),
        *lmfOnRelay
    );

    // HMF Band
    hmfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("hmf_freq"),
        *hmfFreqRelay,
        nullptr
    );
    hmfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("hmf_gain"),
        *hmfGainRelay,
        nullptr
    );
    hmfQAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("hmf_q"),
        *hmfQRelay,
        nullptr
    );
    hmfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("hmf_on"),
        *hmfOnRelay
    );

    // HF Band
    hfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("hf_freq"),
        *hfFreqRelay,
        nullptr
    );
    hfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("hf_gain"),
        *hfGainRelay,
        nullptr
    );
    hfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("hf_on"),
        *hfOnRelay
    );

    // Global
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("output_gain"),
        *outputGainRelay,
        nullptr
    );
    analogAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("analog"),
        *analogRelay
    );

    // Set window size (from mockup v3)
    setSize(920, 220);
    setResizable(false, false);

    // Navigate to index
    webView->goToURL("https://juce.backend/");
}

OuariconAnalogEQAudioProcessorEditor::~OuariconAnalogEQAudioProcessorEditor()
{
    // Destruction happens in reverse order of member declaration (automatic)
}

//==============================================================================
void OuariconAnalogEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire component, no custom painting needed
    g.fillAll(juce::Colours::black);
}

void OuariconAnalogEQAudioProcessorEditor::resized()
{
    // WebView fills entire editor bounds
    if (webView)
        webView->setBounds(getLocalBounds());
}
