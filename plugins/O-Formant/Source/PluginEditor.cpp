#include "PluginEditor.h"
#include "BinaryData.h"

OFormantEditor::OFormantEditor (OFormantAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // 1. Create relays (names MUST match JS relay names)
    vowelXRelay        = std::make_unique<juce::WebSliderRelay> ("vowelXSlider");
    vowelYRelay        = std::make_unique<juce::WebSliderRelay> ("vowelYSlider");
    vowelFocusRelay    = std::make_unique<juce::WebSliderRelay> ("vowelFocusSlider");
    glottalRdRelay     = std::make_unique<juce::WebSliderRelay> ("glottalRdSlider");
    breathinessRelay   = std::make_unique<juce::WebSliderRelay> ("breathinessSlider");
    vibratoRateRelay   = std::make_unique<juce::WebSliderRelay> ("vibratoRateSlider");
    vibratoDepthRelay  = std::make_unique<juce::WebSliderRelay> ("vibratoDepthSlider");
    vibratoDelayRelay  = std::make_unique<juce::WebSliderRelay> ("vibratoDelaySlider");
    jitterRelay        = std::make_unique<juce::WebSliderRelay> ("jitterSlider");
    shimmerRelay       = std::make_unique<juce::WebSliderRelay> ("shimmerSlider");
    rdModDepthRelay    = std::make_unique<juce::WebSliderRelay> ("rdModDepthSlider");
    spectralTiltRelay  = std::make_unique<juce::WebSliderRelay> ("spectralTiltSlider");
    consonantLevelRelay = std::make_unique<juce::WebSliderRelay> ("consonantLevelSlider");
    consonantToneRelay = std::make_unique<juce::WebSliderRelay> ("consonantToneSlider");
    sibilanceRelay     = std::make_unique<juce::WebSliderRelay> ("sibilanceSlider");
    autoConsonantRelay = std::make_unique<juce::WebToggleButtonRelay> ("autoConsonantToggle");
    attackRelay        = std::make_unique<juce::WebSliderRelay> ("attackSlider");
    decayRelay         = std::make_unique<juce::WebSliderRelay> ("decaySlider");
    sustainRelay       = std::make_unique<juce::WebSliderRelay> ("sustainSlider");
    releaseRelay       = std::make_unique<juce::WebSliderRelay> ("releaseSlider");
    formantTopologyRelay = std::make_unique<juce::WebComboBoxRelay> ("formantTopologyComboBox");
    formantShiftRelay  = std::make_unique<juce::WebSliderRelay> ("formantShiftSlider");
    formantSpreadRelay = std::make_unique<juce::WebSliderRelay> ("formantSpreadSlider");
    pitchGlideRelay    = std::make_unique<juce::WebSliderRelay> ("pitchGlideSlider");
    outputGainRelay    = std::make_unique<juce::WebSliderRelay> ("outputGainSlider");
    stereoWidthRelay   = std::make_unique<juce::WebSliderRelay> ("stereoWidthSlider");

    // 2. Build WebView options and create component
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile ("OFormant_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
#endif
            .withOptionsFrom (*vowelXRelay)
            .withOptionsFrom (*vowelYRelay)
            .withOptionsFrom (*vowelFocusRelay)
            .withOptionsFrom (*glottalRdRelay)
            .withOptionsFrom (*breathinessRelay)
            .withOptionsFrom (*vibratoRateRelay)
            .withOptionsFrom (*vibratoDepthRelay)
            .withOptionsFrom (*vibratoDelayRelay)
            .withOptionsFrom (*jitterRelay)
            .withOptionsFrom (*shimmerRelay)
            .withOptionsFrom (*rdModDepthRelay)
            .withOptionsFrom (*spectralTiltRelay)
            .withOptionsFrom (*consonantLevelRelay)
            .withOptionsFrom (*consonantToneRelay)
            .withOptionsFrom (*sibilanceRelay)
            .withOptionsFrom (*autoConsonantRelay)
            .withOptionsFrom (*attackRelay)
            .withOptionsFrom (*decayRelay)
            .withOptionsFrom (*sustainRelay)
            .withOptionsFrom (*releaseRelay)
            .withOptionsFrom (*formantTopologyRelay)
            .withOptionsFrom (*formantShiftRelay)
            .withOptionsFrom (*formantSpreadRelay)
            .withOptionsFrom (*pitchGlideRelay)
            .withOptionsFrom (*outputGainRelay)
            .withOptionsFrom (*stereoWidthRelay)

            // ── Preset Native Functions ──

            .withNativeFunction ("getPresetList", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto presets = pm.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : presets)
                    arr.add (name);
                complete (juce::var (arr));
            })

            .withNativeFunction ("getPresetListWithCategories", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto categorized = pm.getPresetListWithCategories();
                auto* obj = new juce::DynamicObject();
                for (const auto& [category, presets] : categorized)
                {
                    juce::Array<juce::var> arr;
                    for (const auto& name : presets)
                        arr.add (name);
                    obj->setProperty (category, juce::var (arr));
                }
                complete (juce::var (obj));
            })

            .withNativeFunction ("getCurrentPreset", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                complete (juce::var (pm.getCurrentPresetName()));
            })

            .withNativeFunction ("loadPreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPreset (args[0].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("loadPresetFromCategory", [this] (const auto& args, auto complete) {
                if (args.size() < 2 || ! args[0].isString() || ! args[1].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPresetFromCategory (args[0].toString(), args[1].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("savePreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.savePreset (args[0].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("selectNextPreset", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getNextPreset();
                complete (juce::var (name));
            })

            .withNativeFunction ("selectPreviousPreset", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getPreviousPreset();
                complete (juce::var (name));
            })

            .withNativeFunction ("deletePreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.deletePreset (args[0].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("isFactoryPreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool factory = pm.isFactoryPreset (args[0].toString());
                complete (juce::var (factory));
            })
    );

    addAndMakeVisible (*webView);

    // 3. Create attachments (link APVTS parameter -> relay)
    auto& apvts = processorRef.getAPVTS();

    vowelXAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vowelX"), *vowelXRelay);
    vowelYAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vowelY"), *vowelYRelay);
    vowelFocusAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vowelFocus"), *vowelFocusRelay);
    glottalRdAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("glottalRd"), *glottalRdRelay);
    breathinessAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("breathiness"), *breathinessRelay);
    vibratoRateAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vibratoRate"), *vibratoRateRelay);
    vibratoDepthAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vibratoDepth"), *vibratoDepthRelay);
    vibratoDelayAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vibratoDelay"), *vibratoDelayRelay);
    jitterAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("jitter"), *jitterRelay);
    shimmerAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("shimmer"), *shimmerRelay);
    rdModDepthAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("rdModDepth"), *rdModDepthRelay);
    spectralTiltAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("spectralTilt"), *spectralTiltRelay);
    consonantLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("consonantLevel"), *consonantLevelRelay);
    consonantToneAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("consonantTone"), *consonantToneRelay);
    sibilanceAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("sibilance"), *sibilanceRelay);
    autoConsonantAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("autoConsonant"), *autoConsonantRelay);
    attackAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("attack"), *attackRelay);
    decayAttachment         = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("decay"), *decayRelay);
    sustainAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("sustain"), *sustainRelay);
    releaseAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("release"), *releaseRelay);
    formantTopologyAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (*apvts.getParameter ("formantTopology"), *formantTopologyRelay);
    formantShiftAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("formantShift"), *formantShiftRelay);
    formantSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("formantSpread"), *formantSpreadRelay);
    pitchGlideAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("pitchGlide"), *pitchGlideRelay);
    outputGainAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("outputGain"), *outputGainRelay);
    stereoWidthAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("stereoWidth"), *stereoWidthRelay);

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    setSize (800, 600);
}

OFormantEditor::~OFormantEditor()
{
    // Destroy in reverse: attachments first, then webView, then relays
    stereoWidthAttachment.reset();
    outputGainAttachment.reset();
    pitchGlideAttachment.reset();
    formantSpreadAttachment.reset();
    formantTopologyAttachment.reset();
    formantShiftAttachment.reset();
    releaseAttachment.reset();
    sustainAttachment.reset();
    decayAttachment.reset();
    attackAttachment.reset();
    autoConsonantAttachment.reset();
    sibilanceAttachment.reset();
    consonantToneAttachment.reset();
    consonantLevelAttachment.reset();
    vibratoDelayAttachment.reset();
    spectralTiltAttachment.reset();
    rdModDepthAttachment.reset();
    shimmerAttachment.reset();
    jitterAttachment.reset();
    vibratoDepthAttachment.reset();
    vibratoRateAttachment.reset();
    breathinessAttachment.reset();
    glottalRdAttachment.reset();
    vowelFocusAttachment.reset();
    vowelYAttachment.reset();
    vowelXAttachment.reset();

    webView.reset();

    stereoWidthRelay.reset();
    outputGainRelay.reset();
    pitchGlideRelay.reset();
    formantSpreadRelay.reset();
    formantTopologyRelay.reset();
    formantShiftRelay.reset();
    releaseRelay.reset();
    sustainRelay.reset();
    decayRelay.reset();
    attackRelay.reset();
    autoConsonantRelay.reset();
    sibilanceRelay.reset();
    consonantToneRelay.reset();
    consonantLevelRelay.reset();
    vibratoDelayRelay.reset();
    spectralTiltRelay.reset();
    rdModDepthRelay.reset();
    shimmerRelay.reset();
    jitterRelay.reset();
    vibratoDepthRelay.reset();
    vibratoRateRelay.reset();
    breathinessRelay.reset();
    glottalRdRelay.reset();
    vowelFocusRelay.reset();
    vowelYRelay.reset();
    vowelXRelay.reset();
}

void OFormantEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffF5E6D3));
}

void OFormantEditor::resized()
{
    webView->setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OFormantEditor::getResource (const juce::String& url)
{
    auto makeVector = [] (const char* data, int size) {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html") };
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/js/main.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::main_js, BinaryData::main_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/img/flora.png")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::flora_png, BinaryData::flora_pngSize),
            juce::String ("image/png") };
    }

    juce::Logger::writeToLog ("Resource not found: " + url);
    return std::nullopt;
}
