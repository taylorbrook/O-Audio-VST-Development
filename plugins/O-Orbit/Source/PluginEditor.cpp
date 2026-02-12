#include "PluginEditor.h"
#include "BinaryData.h"

OOrbitEditor::OOrbitEditor (OOrbitProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // 1. Create all 17 relays FIRST (constructor body, NOT initializer list)
    // CRITICAL: Parameter IDs MUST match APVTS exactly
    speedRelay = std::make_unique<juce::WebSliderRelay> ("speed");
    widthRelay = std::make_unique<juce::WebSliderRelay> ("width");
    depthRelay = std::make_unique<juce::WebSliderRelay> ("depth");
    tiltRelay = std::make_unique<juce::WebSliderRelay> ("tilt");
    phaseRelay = std::make_unique<juce::WebSliderRelay> ("phase");
    elevationRangeRelay = std::make_unique<juce::WebSliderRelay> ("elevation_range");
    distanceRelay = std::make_unique<juce::WebSliderRelay> ("distance");
    airAbsorptionRelay = std::make_unique<juce::WebSliderRelay> ("air_absorption");
    centerDivergeRelay = std::make_unique<juce::WebSliderRelay> ("center_diverge");
    lrOffsetRelay = std::make_unique<juce::WebSliderRelay> ("lr_offset");
    mixRelay = std::make_unique<juce::WebSliderRelay> ("mix");

    pathRelay = std::make_unique<juce::WebComboBoxRelay> ("path");
    tempoSyncRelay = std::make_unique<juce::WebComboBoxRelay> ("tempo_sync");
    speakerLayoutRelay = std::make_unique<juce::WebComboBoxRelay> ("speaker_layout");
    attenuationCurveRelay = std::make_unique<juce::WebComboBoxRelay> ("attenuation_curve");
    sourceModeRelay = std::make_unique<juce::WebComboBoxRelay> ("source_mode");

    elevationEnableRelay = std::make_unique<juce::WebToggleButtonRelay> ("elevation_enable");

    // 2. Build WebView options with all relays registered
    auto options = juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
        .withKeepPageLoadedWhenBrowserIsHidden()  // Prevents about:blank in FL Studio
        .withOptionsFrom (*speedRelay)
        .withOptionsFrom (*widthRelay)
        .withOptionsFrom (*depthRelay)
        .withOptionsFrom (*tiltRelay)
        .withOptionsFrom (*phaseRelay)
        .withOptionsFrom (*elevationRangeRelay)
        .withOptionsFrom (*distanceRelay)
        .withOptionsFrom (*airAbsorptionRelay)
        .withOptionsFrom (*centerDivergeRelay)
        .withOptionsFrom (*lrOffsetRelay)
        .withOptionsFrom (*mixRelay)
        .withOptionsFrom (*pathRelay)
        .withOptionsFrom (*tempoSyncRelay)
        .withOptionsFrom (*speakerLayoutRelay)
        .withOptionsFrom (*attenuationCurveRelay)
        .withOptionsFrom (*sourceModeRelay)
        .withOptionsFrom (*elevationEnableRelay)
        .withNativeFunction ("getSpeakerLayout", [this] (auto, auto complete) {
            auto& layout = processorRef.getCurrentLayout();
            juce::Array<juce::var> speakers;
            for (const auto& spk : layout.speakers)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("azimuth", spk.azimuth);
                obj->setProperty ("elevation", spk.elevation);
                obj->setProperty ("distance", spk.distance);
                obj->setProperty ("label", spk.label);
                obj->setProperty ("isLFE", spk.isLFE);
                speakers.add (juce::var (obj));
            }
            complete (juce::var (speakers));
        })
        .withNativeFunction ("addSpeaker", [this] (const auto& args, auto complete) {
            if (args.size() >= 4)
            {
                processorRef.addSpeakerToLayout (
                    (float)(double) args[0], (float)(double) args[1],
                    (float)(double) args[2], args[3].toString());
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("removeSpeaker", [this] (const auto& args, auto complete) {
            if (args.size() >= 1)
            {
                processorRef.removeSpeakerFromLayout ((int) args[0]);
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("moveSpeaker", [this] (const auto& args, auto complete) {
            if (args.size() >= 3)
            {
                processorRef.moveSpeakerInLayout (
                    (int) args[0], (float)(double) args[1], (float)(double) args[2]);
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("setCustomLayout", [this] (const auto& args, auto complete) {
            if (args.size() >= 1 && args[0].isArray())
            {
                SpeakerLayout layout;
                layout.name = "Custom";
                auto* arr = args[0].getArray();
                for (const auto& item : *arr)
                {
                    if (auto* obj = item.getDynamicObject())
                    {
                        Speaker spk;
                        spk.azimuth   = (float)(double) obj->getProperty ("azimuth");
                        spk.elevation = (float)(double) obj->getProperty ("elevation");
                        spk.distance  = (float)(double) obj->getProperty ("distance");
                        spk.label     = obj->getProperty ("label").toString();
                        spk.isLFE     = (bool) obj->getProperty ("isLFE");
                        if (spk.elevation != 0.0f) layout.is3D = true;
                        layout.speakers.push_back (spk);
                    }
                }
                processorRef.setCustomSpeakerLayout (layout);
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("getDownmixStatus", [this] (auto, auto complete) {
            auto& dmx = processorRef.getDownmixEngine();
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("active", dmx.isActive());
            obj->setProperty ("sourceChannels", dmx.getSourceChannels());
            obj->setProperty ("targetChannels", dmx.getTargetChannels());
            complete (juce::var (obj));
        })
        .withNativeFunction ("exportLayout", [this] (auto, auto complete) {
            fileChooser = std::make_shared<juce::FileChooser> (
                "Export Speaker Layout", juce::File{}, "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete] (const juce::FileChooser& fc) {
                    auto result = fc.getResult();
                    if (result == juce::File{})
                    {
                        complete (juce::var (false));
                        return;
                    }

                    auto& layout = processorRef.getCurrentLayout();
                    juce::DynamicObject::Ptr root = new juce::DynamicObject();
                    root->setProperty ("name", layout.name);
                    root->setProperty ("is3D", layout.is3D);

                    juce::Array<juce::var> spkArr;
                    for (const auto& spk : layout.speakers)
                    {
                        auto* s = new juce::DynamicObject();
                        s->setProperty ("azimuth", spk.azimuth);
                        s->setProperty ("elevation", spk.elevation);
                        s->setProperty ("distance", spk.distance);
                        s->setProperty ("label", spk.label);
                        s->setProperty ("isLFE", spk.isLFE);
                        spkArr.add (juce::var (s));
                    }
                    root->setProperty ("speakers", juce::var (spkArr));

                    result.replaceWithText (juce::JSON::toString (juce::var (root.get())));
                    complete (juce::var (true));
                });
        })
        .withNativeFunction ("importLayout", [this] (auto, auto complete) {
            fileChooser = std::make_shared<juce::FileChooser> (
                "Import Speaker Layout", juce::File{}, "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete] (const juce::FileChooser& fc) {
                    auto result = fc.getResult();
                    if (result == juce::File{} || ! result.existsAsFile())
                    {
                        complete (juce::var (false));
                        return;
                    }

                    auto json = juce::JSON::parse (result.loadFileAsString());
                    if (auto* root = json.getDynamicObject())
                    {
                        SpeakerLayout layout;
                        layout.name = root->getProperty ("name").toString();
                        layout.is3D = (bool) root->getProperty ("is3D");

                        if (auto* arr = root->getProperty ("speakers").getArray())
                        {
                            for (const auto& item : *arr)
                            {
                                if (auto* obj = item.getDynamicObject())
                                {
                                    Speaker spk;
                                    spk.azimuth   = (float)(double) obj->getProperty ("azimuth");
                                    spk.elevation = (float)(double) obj->getProperty ("elevation");
                                    spk.distance  = (float)(double) obj->getProperty ("distance");
                                    spk.label     = obj->getProperty ("label").toString();
                                    spk.isLFE     = (bool) obj->getProperty ("isLFE");
                                    layout.speakers.push_back (spk);
                                }
                            }
                        }

                        if (layout.speakers.size() >= 2)
                        {
                            processorRef.setCustomSpeakerLayout (layout);

                            // Return the layout back to JS for updating the editor view
                            juce::Array<juce::var> spkResult;
                            for (const auto& spk : layout.speakers)
                            {
                                auto* s = new juce::DynamicObject();
                                s->setProperty ("azimuth", spk.azimuth);
                                s->setProperty ("elevation", spk.elevation);
                                s->setProperty ("distance", spk.distance);
                                s->setProperty ("label", spk.label);
                                s->setProperty ("isLFE", spk.isLFE);
                                spkResult.add (juce::var (s));
                            }
                            complete (juce::var (spkResult));
                            return;
                        }
                    }
                    complete (juce::var (false));
                });
        });

    // Windows-specific: set user data folder for plugin host compatibility
   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OOrbit_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    // Register resource provider (guarded for cross-platform compatibility)
   #if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    options = options.withResourceProvider (
        [this] (const auto& url) { return getResource (url); });
   #endif

    // Create WebView with options
    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. Create all 17 parameter attachments LAST
    // CRITICAL: 3 parameters required in JUCE 8 (parameter, relay, undoManager)
    speedAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("speed"), *speedRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("width"), *widthRelay, nullptr);
    depthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("depth"), *depthRelay, nullptr);
    tiltAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("tilt"), *tiltRelay, nullptr);
    phaseAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("phase"), *phaseRelay, nullptr);
    elevationRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("elevation_range"), *elevationRangeRelay, nullptr);
    distanceAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("distance"), *distanceRelay, nullptr);
    airAbsorptionAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("air_absorption"), *airAbsorptionRelay, nullptr);
    centerDivergeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("center_diverge"), *centerDivergeRelay, nullptr);
    lrOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("lr_offset"), *lrOffsetRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("mix"), *mixRelay, nullptr);

    pathAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("path"), *pathRelay, nullptr);
    tempoSyncAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("tempo_sync"), *tempoSyncRelay, nullptr);
    speakerLayoutAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("speaker_layout"), *speakerLayoutRelay, nullptr);
    attenuationCurveAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("attenuation_curve"), *attenuationCurveRelay, nullptr);
    sourceModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("source_mode"), *sourceModeRelay, nullptr);

    elevationEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *processorRef.parameters.getParameter ("elevation_enable"), *elevationEnableRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible (*webView);

    // Set editor size (800x600 per Phase 3.1 spec)
    setSize (800, 600);

    // Start 30Hz timer for motion state push to JS visualizer
    startTimerHz (30);
}

OOrbitEditor::~OOrbitEditor()
{
    stopTimer();  // CRITICAL: stop timer before member destruction
}

void OOrbitEditor::paint (juce::Graphics& g)
{
    // WebView handles all painting
    g.fillAll (juce::Colours::black);
}

void OOrbitEditor::resized()
{
    // WebView fills entire editor bounds
    webView->setBounds (getLocalBounds());
}

void OOrbitEditor::timerCallback()
{
    // Push motion state to JS visualizer at 30Hz via custom event
    float azL  = processorRef.getUIAzimuthL();
    float elL  = processorRef.getUIElevationL();
    float azR  = processorRef.getUIAzimuthR();
    float elR  = processorRef.getUIElevationR();
    float dist = processorRef.getUIDistance();
    bool isLRSplit = processorRef.parameters.getRawParameterValue ("source_mode")->load() > 0.5f;

    juce::String json = juce::String::formatted (
        "{\"azL\":%f,\"elL\":%f,\"azR\":%f,\"elR\":%f,\"dist\":%f,\"split\":%s}",
        azL, elL, azR, elR, dist, isLRSplit ? "true" : "false");

    webView->emitEventIfBrowserIsVisible ("motionUpdate", json);
}

void OOrbitEditor::parentHierarchyChanged()
{
    // Lazy navigation pattern: navigate only when editor becomes visible
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OOrbitEditor::getResource (const juce::String& url)
{
    // Helper to convert BinaryData to vector<byte>
    auto makeVector = [] (const char* data, int size)
    {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

    // Normalize URL
    auto resource = url.replaceCharacter ('\\', '/');

    // Root "/" → index.html
    if (resource == "/" || resource.isEmpty())
        resource = "/index.html";

    // Remove leading slash for BinaryData lookup
    auto path = resource.substring (1);

    // Explicit URL-to-BinaryData mapping
    // BinaryData naming: slashes/dots become underscores
    if (path == "index.html")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html") };

    if (path == "css/styles.css")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::styles_css, BinaryData::styles_cssSize),
            juce::String ("text/css") };

    if (path == "js/app.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::app_js, BinaryData::app_jsSize),
            juce::String ("application/javascript") };

    if (path == "js/juce/index.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("application/javascript") };

    if (path == "js/juce/check_native_interop.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String ("application/javascript") };

    if (path == "img/shell.png")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String ("image/png") };

    // Resource not found
    juce::Logger::writeToLog ("O-Orbit: Resource not found: " + url);
    return std::nullopt;
}
