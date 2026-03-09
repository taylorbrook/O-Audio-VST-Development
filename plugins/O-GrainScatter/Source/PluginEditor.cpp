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
    grainShapeRelay      = std::make_unique<juce::WebComboBoxRelay>("grain_shape");
    sizeRandomRelay      = std::make_unique<juce::WebSliderRelay>("size_random");
    ampRandomRelay       = std::make_unique<juce::WebSliderRelay>("amp_random");
    euclideanPulsesRelay   = std::make_unique<juce::WebSliderRelay>("euclidean_pulses");
    euclideanStepsRelay    = std::make_unique<juce::WebSliderRelay>("euclidean_steps");
    euclideanRotationRelay = std::make_unique<juce::WebSliderRelay>("euclidean_rotation");
    euclideanSwingRelay    = std::make_unique<juce::WebSliderRelay>("euclidean_swing");
    // Spatial
    spatialModeRelay     = std::make_unique<juce::WebComboBoxRelay>("spatial_mode");
    azimuthRelay         = std::make_unique<juce::WebSliderRelay>("azimuth");
    elevationRelay       = std::make_unique<juce::WebSliderRelay>("elevation");
    azSpreadRelay        = std::make_unique<juce::WebSliderRelay>("az_spread");
    elSpreadRelay        = std::make_unique<juce::WebSliderRelay>("el_spread");
    distanceRelay        = std::make_unique<juce::WebSliderRelay>("distance");
    spatialWidthRelay    = std::make_unique<juce::WebSliderRelay>("spatial_width");
    trajectoryRelay      = std::make_unique<juce::WebComboBoxRelay>("trajectory");
    trajSpeedRelay       = std::make_unique<juce::WebSliderRelay>("traj_speed");
    distLpfRelay         = std::make_unique<juce::WebSliderRelay>("dist_lpf");
    dopplerRelay         = std::make_unique<juce::WebSliderRelay>("doppler");
    spatialSmoothRelay   = std::make_unique<juce::WebSliderRelay>("spatial_smooth");

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
            .withOptionsFrom(*grainShapeRelay)
            .withOptionsFrom(*sizeRandomRelay)
            .withOptionsFrom(*ampRandomRelay)
            .withOptionsFrom(*euclideanPulsesRelay)
            .withOptionsFrom(*euclideanStepsRelay)
            .withOptionsFrom(*euclideanRotationRelay)
            .withOptionsFrom(*euclideanSwingRelay)
            .withOptionsFrom(*spatialModeRelay)
            .withOptionsFrom(*azimuthRelay)
            .withOptionsFrom(*elevationRelay)
            .withOptionsFrom(*azSpreadRelay)
            .withOptionsFrom(*elSpreadRelay)
            .withOptionsFrom(*distanceRelay)
            .withOptionsFrom(*spatialWidthRelay)
            .withOptionsFrom(*trajectoryRelay)
            .withOptionsFrom(*trajSpeedRelay)
            .withOptionsFrom(*distLpfRelay)
            .withOptionsFrom(*dopplerRelay)
            .withOptionsFrom(*spatialSmoothRelay)
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
    grainShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("grain_shape"), *grainShapeRelay, nullptr);
    sizeRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("size_random"), *sizeRandomRelay, nullptr);
    ampRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("amp_random"), *ampRandomRelay, nullptr);
    euclideanPulsesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("euclidean_pulses"), *euclideanPulsesRelay, nullptr);
    euclideanStepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("euclidean_steps"), *euclideanStepsRelay, nullptr);
    euclideanRotationAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("euclidean_rotation"), *euclideanRotationRelay, nullptr);
    euclideanSwingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("euclidean_swing"), *euclideanSwingRelay, nullptr);
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
    trajSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("traj_speed"), *trajSpeedRelay, nullptr);
    distLpfAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("dist_lpf"), *distLpfRelay, nullptr);
    dopplerAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("doppler"), *dopplerRelay, nullptr);
    spatialSmoothAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("spatial_smooth"), *spatialSmoothRelay, nullptr);

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
    // Read snapshot once (triple-buffered, thread-safe)
    const auto& snap = audioProcessor.getVizSnapshot();

    // Grain visualization data
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

    // Euclidean visualization data (read from snapshot, not cross-thread reference)
    int steps = snap.euclideanSteps;
    int step = snap.euclideanStep;
    juce::String eucJson = "{\"p\":[";
    for (int i = 0; i < steps; ++i)
    {
        if (i > 0) eucJson += ",";
        eucJson += snap.euclideanPattern[static_cast<size_t> (i)] ? "1" : "0";
    }
    int rotation = snap.euclideanRotation;
    eucJson += "],\"s\":" + juce::String (step) + ",\"n\":" + juce::String (steps)
             + ",\"r\":" + juce::String (rotation) + "}";
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
