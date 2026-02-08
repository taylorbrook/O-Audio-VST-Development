/*
  ==============================================================================

    O-Chorus - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation) - Placeholder WebView UI

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
OChorusAudioProcessorEditor::OChorusAudioProcessorEditor(OChorusAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    //==========================================================================
    // CRITICAL: Initialization Order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    //==========================================================================

    // 1️⃣ Create relays FIRST (no dependencies)
    rateRelay = std::make_unique<juce::WebSliderRelay>("rate");
    depthRelay = std::make_unique<juce::WebSliderRelay>("depth");
    voicesRelay = std::make_unique<juce::WebSliderRelay>("voices");
    widthRelay = std::make_unique<juce::WebSliderRelay>("width");
    toneRelay = std::make_unique<juce::WebSliderRelay>("tone");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");

    // 2️⃣ Create WebView with relay options (depends on relays)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*rateRelay)
            .withOptionsFrom(*depthRelay)
            .withOptionsFrom(*voicesRelay)
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*toneRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*driveRelay)
    );

    addAndMakeVisible(*webView);

#if OUARICON_LICENSING_ENABLED
    // Licensing: activation overlay (visible until licensed)
    auto& license = audioProcessor.getLicenseManager();
    licenseOverlay = std::make_unique<OuariconLicenseOverlay>(license);
    addAndMakeVisible(licenseOverlay.get());

    license.addListener(this);

    if (! license.isLicensed())
        webView->setVisible(false);
    else
        licenseOverlay->setVisible(false);
#endif

    // 3️⃣ Create attachments LAST (depend on relays and webView)
    rateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("rate"), *rateRelay, nullptr);
    depthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("depth"), *depthRelay, nullptr);
    voicesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("voices"), *voicesRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("width"), *widthRelay, nullptr);
    toneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("tone"), *toneRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("mix"), *mixRelay, nullptr);
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("drive"), *driveRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    setSize(700, 250);
}

OChorusAudioProcessorEditor::~OChorusAudioProcessorEditor()
{
#if OUARICON_LICENSING_ENABLED
    audioProcessor.getLicenseManager().removeListener(this);
#endif

    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OChorusAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OChorusAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    webView->setBounds(getLocalBounds());

#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OChorusAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    //==========================================================================
    // CRITICAL: Explicit URL Mapping
    // BinaryData flattens paths (index.js → index_js)
    // HTML requests use original paths (./js/juce/index.js)
    // Must map manually with correct MIME types
    //==========================================================================

    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/img/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

#if OUARICON_LICENSING_ENABLED
//==============================================================================
void OChorusAudioProcessorEditor::licenseStatusChanged(
    OuariconLicense&, OuariconLicense::Status newStatus)
{
    juce::MessageManager::callAsync([this, newStatus]()
    {
        bool licensed = (newStatus == OuariconLicense::Status::Licensed);
        webView->setVisible(licensed);

        if (licenseOverlay)
            licenseOverlay->setVisible(! licensed);
    });
}
#endif
