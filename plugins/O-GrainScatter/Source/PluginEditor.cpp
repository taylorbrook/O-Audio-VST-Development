#include "PluginEditor.h"
#include "BinaryData.h"

GrainScatterEditor::GrainScatterEditor(GrainScatterProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Create relays FIRST
    grainSizeRelay      = std::make_unique<juce::WebSliderRelay>("grain_size");
    densityRelay        = std::make_unique<juce::WebSliderRelay>("density");
    pitchRandomRelay    = std::make_unique<juce::WebSliderRelay>("pitch_random");
    panRandomRelay      = std::make_unique<juce::WebSliderRelay>("pan_random");
    scaleRelay          = std::make_unique<juce::WebComboBoxRelay>("scale");
    rootNoteRelay       = std::make_unique<juce::WebComboBoxRelay>("root_note");
    reverseRelay        = std::make_unique<juce::WebSliderRelay>("reverse");
    feedbackRelay       = std::make_unique<juce::WebSliderRelay>("feedback");
    dryWetRelay         = std::make_unique<juce::WebSliderRelay>("dry_wet");
    syncModeRelay       = std::make_unique<juce::WebComboBoxRelay>("sync_mode");
    probabilityRelay    = std::make_unique<juce::WebSliderRelay>("probability");
    repeatsRelay        = std::make_unique<juce::WebSliderRelay>("repeats");
    spreadRelay         = std::make_unique<juce::WebSliderRelay>("spread");
    pitchModeRelay      = std::make_unique<juce::WebComboBoxRelay>("pitch_mode");
    freezeRelay         = std::make_unique<juce::WebToggleButtonRelay>("freeze");
    stutterGateRelay    = std::make_unique<juce::WebToggleButtonRelay>("stutter_gate");
    euclideanPulsesRelay = std::make_unique<juce::WebSliderRelay>("euclidean_pulses");
    euclideanStepsRelay  = std::make_unique<juce::WebSliderRelay>("euclidean_steps");
    // Spatial
    spatialModeRelay     = std::make_unique<juce::WebComboBoxRelay>("spatial_mode");
    azimuthRelay         = std::make_unique<juce::WebSliderRelay>("azimuth");
    elevationRelay       = std::make_unique<juce::WebSliderRelay>("elevation");
    azSpreadRelay        = std::make_unique<juce::WebSliderRelay>("az_spread");
    elSpreadRelay        = std::make_unique<juce::WebSliderRelay>("el_spread");
    distanceRelay        = std::make_unique<juce::WebSliderRelay>("distance");
    spatialWidthRelay    = std::make_unique<juce::WebSliderRelay>("spatial_width");
    trajectoryRelay      = std::make_unique<juce::WebComboBoxRelay>("trajectory");

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OGrainScatter_WebView")))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*grainSizeRelay)
            .withOptionsFrom(*densityRelay)
            .withOptionsFrom(*pitchRandomRelay)
            .withOptionsFrom(*panRandomRelay)
            .withOptionsFrom(*scaleRelay)
            .withOptionsFrom(*rootNoteRelay)
            .withOptionsFrom(*reverseRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*dryWetRelay)
            .withOptionsFrom(*syncModeRelay)
            .withOptionsFrom(*probabilityRelay)
            .withOptionsFrom(*repeatsRelay)
            .withOptionsFrom(*spreadRelay)
            .withOptionsFrom(*pitchModeRelay)
            .withOptionsFrom(*freezeRelay)
            .withOptionsFrom(*stutterGateRelay)
            .withOptionsFrom(*euclideanPulsesRelay)
            .withOptionsFrom(*euclideanStepsRelay)
            .withOptionsFrom(*spatialModeRelay)
            .withOptionsFrom(*azimuthRelay)
            .withOptionsFrom(*elevationRelay)
            .withOptionsFrom(*azSpreadRelay)
            .withOptionsFrom(*elSpreadRelay)
            .withOptionsFrom(*distanceRelay)
            .withOptionsFrom(*spatialWidthRelay)
            .withOptionsFrom(*trajectoryRelay)
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments LAST
    grainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("grain_size"), *grainSizeRelay, nullptr);
    densityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("density"), *densityRelay, nullptr);
    pitchRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("pitch_random"), *pitchRandomRelay, nullptr);
    panRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("pan_random"), *panRandomRelay, nullptr);
    scaleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("scale"), *scaleRelay, nullptr);
    rootNoteAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("root_note"), *rootNoteRelay, nullptr);
    reverseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("reverse"), *reverseRelay, nullptr);
    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("feedback"), *feedbackRelay, nullptr);
    dryWetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("dry_wet"), *dryWetRelay, nullptr);
    syncModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("sync_mode"), *syncModeRelay, nullptr);
    probabilityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("probability"), *probabilityRelay, nullptr);
    repeatsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("repeats"), *repeatsRelay, nullptr);
    spreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("spread"), *spreadRelay, nullptr);
    pitchModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("pitch_mode"), *pitchModeRelay, nullptr);
    freezeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("freeze"), *freezeRelay, nullptr);
    stutterGateAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("stutter_gate"), *stutterGateRelay, nullptr);
    euclideanPulsesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("euclidean_pulses"), *euclideanPulsesRelay, nullptr);
    euclideanStepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("euclidean_steps"), *euclideanStepsRelay, nullptr);
    // Spatial
    spatialModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("spatial_mode"), *spatialModeRelay, nullptr);
    azimuthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("azimuth"), *azimuthRelay, nullptr);
    elevationAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("elevation"), *elevationRelay, nullptr);
    azSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("az_spread"), *azSpreadRelay, nullptr);
    elSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("el_spread"), *elSpreadRelay, nullptr);
    distanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("distance"), *distanceRelay, nullptr);
    spatialWidthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("spatial_width"), *spatialWidthRelay, nullptr);
    trajectoryAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("trajectory"), *trajectoryRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    startTimerHz(30);
    setSize(900, 850);
}

GrainScatterEditor::~GrainScatterEditor()
{
    stopTimer();
}

void GrainScatterEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
}

void GrainScatterEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void GrainScatterEditor::timerCallback()
{
    // Grain visualization data
    const auto& snap = audioProcessor.getVizSnapshot();
    juce::String grainJson = "{\"g\":[";
    bool first = true;
    for (int i = 0; i < 64; ++i)
    {
        const auto& v = snap.voices[static_cast<size_t> (i)];
        if (!v.active) continue;
        if (!first) grainJson += ",";
        first = false;
        grainJson += "{\"p\":" + juce::String (v.positionNorm, 3)
                   + ",\"s\":" + juce::String (v.pitchSemitones, 2)
                   + ",\"pn\":" + juce::String (v.pan, 2)
                   + ",\"e\":" + juce::String (v.envelope, 3)
                   + ",\"r\":" + juce::String (v.reverse ? 1 : 0)
                   + ",\"f\":" + juce::String (v.frozen ? 1 : 0)
                   + ",\"az\":" + juce::String (v.azimuth, 3)
                   + ",\"el\":" + juce::String (v.elevation, 3)
                   + ",\"d\":" + juce::String (v.distance, 2) + "}";
    }
    grainJson += "],\"ac\":" + juce::String (snap.activeCount) + "}";
    webView->emitEventIfBrowserIsVisible ("grainUpdate", grainJson);

    // Euclidean visualization data
    const auto& pattern = audioProcessor.getEuclideanPattern();
    int steps = audioProcessor.getEuclideanSteps();
    int step = audioProcessor.getEuclideanStep();
    juce::String eucJson = "{\"p\":[";
    for (int i = 0; i < steps; ++i)
    {
        if (i > 0) eucJson += ",";
        eucJson += pattern[static_cast<size_t> (i)] ? "1" : "0";
    }
    eucJson += "],\"s\":" + juce::String (step) + ",\"n\":" + juce::String (steps) + "}";
    webView->emitEventIfBrowserIsVisible ("euclideanUpdate", eucJson);
}

std::optional<juce::WebBrowserComponent::Resource>
GrainScatterEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")};
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/app.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::app_js, BinaryData::app_jsSize),
            juce::String("application/javascript")};
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
